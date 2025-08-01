#ifndef FWCore_Framework_StreamSchedule_h
#define FWCore_Framework_StreamSchedule_h

/*
  Author: Jim Kowalkowski  28-01-06

  A class for creating a schedule based on paths in the configuration file.
  The schedule is maintained as a sequence of paths.
  After construction, events can be fed to the object and passed through
  all the modules in the schedule.  All accounting about processing
  of events by modules and paths is contained here or in object held
  by containment.

  The trigger results producer and product are generated and managed here.
  This class also manages endpaths and calls to endjob and beginjob.
  Endpaths are just treated as a simple list of modules that need to
  do processing of the event and do not participate in trigger path
  activities.

  This class requires the high-level process pset.  It uses @process_name.
  If the high-level pset contains an "options" pset, then the
  following optional parameter can be present:
  bool wantSummary = true/false   # default false

  wantSummary indicates whether or not the pass/fail/error stats
  for modules and paths should be printed at the end-of-job.

  A TriggerResults object will always be inserted into the event
  for any schedule.  The producer of the TriggerResults EDProduct
  is always the first module in the endpath.  The TriggerResultInserter
  is given a fixed label of "TriggerResults".

  Processing of an event happens by pushing the event through the Paths.
  The scheduler performs the reset() on each of the workers independent
  of the Path objects.

  ------------------------

  About Paths:
  Paths fit into two categories:
  1) trigger paths that contribute directly to saved trigger bits
  2) end paths
  The StreamSchedule holds these paths in two data structures:
  1) main path list
  2) end path list

  Trigger path processing always precedes endpath processing.
  The order of the paths from the input configuration is
  preserved in the main paths list.


*/

#include "DataFormats/Common/interface/HLTGlobalStatus.h"
#include "DataFormats/Provenance/interface/ModuleDescription.h"
#include "FWCore/Framework/interface/ExceptionActions.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/ExceptionHelpers.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/OccurrenceTraits.h"
#include "FWCore/Framework/interface/WorkerManager.h"
#include "FWCore/Framework/interface/Path.h"
#include "FWCore/Framework/interface/TransitionInfoTypes.h"
#include "FWCore/Framework/interface/ModuleInPath.h"
#include "FWCore/Framework/interface/maker/Worker.h"
#include "FWCore/Framework/interface/EarlyDeleteHelper.h"
#include "FWCore/MessageLogger/interface/ExceptionMessages.h"
#include "FWCore/MessageLogger/interface/JobReport.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/ServiceRegistry/interface/ServiceRegistry.h"
#include "FWCore/ServiceRegistry/interface/ServiceRegistryfwd.h"
#include "FWCore/ServiceRegistry/interface/ServiceToken.h"
#include "FWCore/ServiceRegistry/interface/StreamContext.h"
#include "FWCore/Concurrency/interface/FunctorTask.h"
#include "FWCore/Concurrency/interface/WaitingTaskHolder.h"
#include "FWCore/Utilities/interface/Algorithms.h"
#include "FWCore/Utilities/interface/BranchType.h"
#include "FWCore/Utilities/interface/ConvertException.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "FWCore/Utilities/interface/get_underlying_safe.h"
#include "FWCore/Utilities/interface/propagate_const.h"
#include "FWCore/Utilities/interface/thread_safety_macros.h"

#include "oneapi/tbb/task_arena.h"

#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <atomic>
#include <unordered_set>
#include <utility>

namespace edm {

  class BranchIDListHelper;
  class ExceptionCollector;
  class ExceptionToActionTable;
  class OutputModuleCommunicator;
  class WorkerInPath;
  class ModuleRegistry;
  class TriggerResultInserter;
  class PathStatusInserter;
  class EndPathStatusInserter;

  namespace maker {
    class ModuleHolder;
  }

  class StreamSchedule {
  public:
    using vstring = std::vector<std::string>;
    using TrigPaths = std::vector<Path>;
    using TrigResPtr = std::shared_ptr<HLTGlobalStatus>;
    using TrigResConstPtr = std::shared_ptr<HLTGlobalStatus const>;
    using AllWorkers = std::vector<Worker*>;

    using Workers = std::vector<Worker*>;

    using PathWorkers = std::vector<WorkerInPath>;

    struct PathInfo {
      PathInfo(std::string name, std::vector<edm::ModuleInPath> modules, std::shared_ptr<PathStatusInserter> inserter)
          : name_(std::move(name)), modules_(std::move(modules)), inserter_(std::move(inserter)) {}
      std::string name_;
      std::vector<edm::ModuleInPath> modules_;
      std::shared_ptr<PathStatusInserter> inserter_;
    };
    struct EndPathInfo {
      EndPathInfo(std::string name,
                  std::vector<edm::ModuleInPath> modules,
                  std::shared_ptr<EndPathStatusInserter> inserter)
          : name_(std::move(name)), modules_(std::move(modules)), inserter_(std::move(inserter)) {}
      std::string name_;
      std::vector<edm::ModuleInPath> modules_;
      std::shared_ptr<EndPathStatusInserter> inserter_;
    };

    StreamSchedule(std::vector<PathInfo> const& paths,
                   std::vector<EndPathInfo> const& endPaths,
                   std::vector<ModuleDescription const*> const& unscheduledModules,
                   std::shared_ptr<TriggerResultInserter> inserter,
                   std::shared_ptr<ModuleRegistry>,
                   ExceptionToActionTable const& actions,
                   std::shared_ptr<ActivityRegistry> areg,
                   StreamID streamID,
                   ProcessContext const* processContext);

    StreamSchedule(StreamSchedule const&) = delete;

    void processOneEventAsync(
        WaitingTaskHolder iTask,
        EventTransitionInfo&,
        ServiceToken const& token,
        std::vector<edm::propagate_const<std::shared_ptr<PathStatusInserter>>>& pathStatusInserters);

    template <typename T>
    void processOneStreamAsync(WaitingTaskHolder iTask,
                               typename T::TransitionInfoType& transitionInfo,
                               ServiceToken const& token,
                               bool cleaningUpAfterException = false);

    void beginStream(ModuleRegistry& iModuleRegistry);
    void endStream(ModuleRegistry& iModuleRegistry, ExceptionCollector& collector, std::mutex& collectorMutex) noexcept;

    StreamID streamID() const { return streamID_; }

    /// Return a vector allowing const access to all the
    /// ModuleDescriptions for this StreamSchedule.

    /// *** N.B. *** Ownership of the ModuleDescriptions is *not*
    /// *** passed to the caller. Do not call delete on these
    /// *** pointers!
    std::vector<ModuleDescription const*> getAllModuleDescriptions() const;

    ///adds to oLabelsToFill the labels for all paths in the process
    void availablePaths(std::vector<std::string>& oLabelsToFill) const;

    ///adds to oLabelsToFill in execution order the labels of all modules in path iPathLabel
    void modulesInPath(std::string const& iPathLabel, std::vector<std::string>& oLabelsToFill) const;

    void moduleDescriptionsInPath(std::string const& iPathLabel,
                                  std::vector<ModuleDescription const*>& descriptions,
                                  unsigned int hint) const;

    void moduleDescriptionsInEndPath(std::string const& iEndPathLabel,
                                     std::vector<ModuleDescription const*>& descriptions,
                                     unsigned int hint) const;

    /// Return the number of events this StreamSchedule has tried to process
    /// (inclues both successes and failures, including failures due
    /// to exceptions during processing).
    int totalEvents() const { return total_events_; }

    /// Return the number of events which have been passed by one or
    /// more trigger paths.
    int totalEventsPassed() const { return total_passed_; }

    /// Return the number of events that have not passed any trigger.
    /// (N.B. totalEventsFailed() + totalEventsPassed() == totalEvents()
    int totalEventsFailed() const { return totalEvents() - totalEventsPassed(); }

    /// Return the trigger report information on paths,
    /// modules-in-path, modules-in-endpath, and modules.
    void getTriggerReport(TriggerReport& rep) const;

    ///  Clear all the counters in the trigger report.
    void clearCounters();

    /// clone the type of module with label iLabel but configure with iPSet.
    void replaceModule(maker::ModuleHolder* iMod, std::string const& iLabel);

    /// Delete the module with label iLabel
    void deleteModule(std::string const& iLabel);

    void initializeEarlyDelete(ModuleRegistry& modReg,
                               std::vector<std::string> const& branchesToDeleteEarly,
                               std::multimap<std::string, std::string> const& referencesToBranches,
                               std::vector<std::string> const& modulesToSkip,
                               edm::ProductRegistry const& preg);

    /// returns the collection of pointers to workers
    AllWorkers const& allWorkersRuns() const { return workerManagerRuns_.allWorkers(); }
    AllWorkers const& allWorkersLumisAndEvents() const { return workerManagerLumisAndEvents_.allWorkers(); }

    AllWorkers const& unscheduledWorkersLumisAndEvents() const {
      return workerManagerLumisAndEvents_.unscheduledWorkers();
    }
    unsigned int numberOfUnscheduledModules() const { return number_of_unscheduled_modules_; }

    StreamContext const& context() const { return streamContext_; }

  private:
    /// returns the action table
    ExceptionToActionTable const& actionTable() const { return workerManagerLumisAndEvents_.actionTable(); }

    void resetAll();

    void finishedPaths(std::atomic<std::exception_ptr*>&, WaitingTaskHolder, EventTransitionInfo&);

    std::exception_ptr finishProcessOneEvent(std::exception_ptr);

    void reportSkipped(EventPrincipal const& ep) const;

    PathWorkers fillWorkers(std::vector<ModuleInPath> const&);
    void fillTrigPath(PathInfo const&, int bitpos, TrigResPtr);
    void fillEndPath(EndPathInfo const&, int bitpos);

    void resetEarlyDelete();

    TrigResConstPtr results() const { return get_underlying_safe(results_); }
    TrigResPtr& results() { return get_underlying_safe(results_); }

    template <typename T>
    void preScheduleSignal(StreamContext const*) const;

    template <typename T>
    void postScheduleSignal(StreamContext const*, std::exception_ptr&) const noexcept;

    void handleException(StreamContext const&, bool cleaningUpAfterException, std::exception_ptr&) const noexcept;

    std::vector<unsigned int> moduleBeginStreamFailed_;
    WorkerManager workerManagerRuns_;
    WorkerManager workerManagerLumisAndEvents_;
    std::shared_ptr<ActivityRegistry> actReg_;  // We do not use propagate_const because the registry itself is mutable.

    edm::propagate_const<TrigResPtr> results_;

    edm::propagate_const<Worker*> results_inserter_;
    std::vector<edm::propagate_const<Worker*>> pathStatusInserterWorkers_;
    std::vector<edm::propagate_const<Worker*>> endPathStatusInserterWorkers_;

    TrigPaths trig_paths_;
    TrigPaths end_paths_;
    std::vector<int> empty_trig_paths_;
    std::vector<int> empty_end_paths_;

    //For each branch that has been marked for early deletion
    // keep track of how many modules are left that read this data but have
    // not yet been run in this event
    std::vector<BranchToCount> earlyDeleteBranchToCount_;
    //NOTE the following is effectively internal data for each EarlyDeleteHelper
    // but putting it into one vector makes for better allocation as well as
    // faster iteration when used to reset the earlyDeleteBranchToCount_
    // Each EarlyDeleteHelper hold a begin and end range into this vector. The values
    // of this vector correspond to indexes into earlyDeleteBranchToCount_ so
    // tell which EarlyDeleteHelper is associated with which BranchIDs.
    std::vector<unsigned int> earlyDeleteHelperToBranchIndicies_;
    //There is one EarlyDeleteHelper per Module which are reading data that
    // has been marked for early deletion
    std::vector<EarlyDeleteHelper> earlyDeleteHelpers_;

    int total_events_;
    int total_passed_;
    unsigned int number_of_unscheduled_modules_;

    StreamID streamID_;
    StreamContext streamContext_;
  };

  void inline StreamSchedule::reportSkipped(EventPrincipal const& ep) const {
    Service<JobReport> reportSvc;
    reportSvc->reportSkippedEvent(ep.id().run(), ep.id().event());
  }

  template <typename T>
  void StreamSchedule::processOneStreamAsync(WaitingTaskHolder iHolder,
                                             typename T::TransitionInfoType& transitionInfo,
                                             ServiceToken const& token,
                                             bool cleaningUpAfterException) {
    auto group = iHolder.group();
    auto const& principal = transitionInfo.principal();
    T::setStreamContext(streamContext_, principal);

    ServiceWeakToken weakToken = token;
    auto doneTask = make_waiting_task([this, iHolder = std::move(iHolder), cleaningUpAfterException, weakToken](
                                          std::exception_ptr const* iPtr) mutable {
      std::exception_ptr excpt;
      {
        ServiceRegistry::Operate op(weakToken.lock());

        if (iPtr) {
          excpt = *iPtr;
          handleException(streamContext_, cleaningUpAfterException, excpt);
        }
        postScheduleSignal<T>(&streamContext_, excpt);
      }  // release service token before calling doneWaiting
      iHolder.doneWaiting(excpt);
    });

    auto task =
        make_functor_task([this, h = WaitingTaskHolder(*group, doneTask), info = transitionInfo, weakToken]() mutable {
          auto token = weakToken.lock();
          ServiceRegistry::Operate op(token);
          // Caught exception is propagated via WaitingTaskHolder
          WorkerManager* workerManager = &workerManagerRuns_;
          if (T::branchType_ == InLumi) {
            workerManager = &workerManagerLumisAndEvents_;
          }
          CMS_SA_ALLOW try {
            preScheduleSignal<T>(&streamContext_);
            workerManager->resetAll();
          } catch (...) {
            // Just remember the exception at this point,
            // let the destructor of h call doneWaiting() so the
            // ServiceRegistry::Operator object is destroyed first
            h.presetTaskAsFailed(std::current_exception());
            return;
          }

          workerManager->processOneOccurrenceAsync<T>(h, info, token, streamID_, &streamContext_, &streamContext_);
        });

    if (streamID_.value() == 0) {
      //Enqueueing will start another thread if there is only
      // one thread in the job. Having stream == 0 use spawn
      // avoids starting up another thread when there is only one stream.
      group->run([task]() {
        TaskSentry s{task};
        task->execute();
      });
    } else {
      oneapi::tbb::task_arena arena{oneapi::tbb::task_arena::attach()};
      arena.enqueue([task]() {
        TaskSentry s{task};
        task->execute();
      });
    }
  }

  template <typename T>
  void StreamSchedule::preScheduleSignal(StreamContext const* streamContext) const {
    try {
      convertException::wrap([this, streamContext]() { T::preScheduleSignal(actReg_.get(), streamContext); });
    } catch (cms::Exception& ex) {
      std::ostringstream ost;
      ex.addContext("Handling pre signal, likely in a service function");
      exceptionContext(ost, *streamContext);
      ex.addContext(ost.str());
      throw;
    }
  }

  template <typename T>
  void StreamSchedule::postScheduleSignal(StreamContext const* streamContext,
                                          std::exception_ptr& excpt) const noexcept {
    try {
      convertException::wrap([this, streamContext]() { T::postScheduleSignal(actReg_.get(), streamContext); });
    } catch (cms::Exception& ex) {
      if (not excpt) {
        std::ostringstream ost;
        ex.addContext("Handling post signal, likely in a service function");
        exceptionContext(ost, *streamContext);
        ex.addContext(ost.str());
        excpt = std::current_exception();
      }
    }
  }
}  // namespace edm

#endif
