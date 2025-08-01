// -*- C++ -*-
//
// Package:     Framework
// Class  :     EventSetupRecordIOVQueue
//
//
// Author:      W. David Dagenhart
// Created:     22 Feb 2019

#include "FWCore/Framework/interface/EventSetupRecordIOVQueue.h"
#include "FWCore/Framework/interface/EventSetupRecordProvider.h"
#include "FWCore/Concurrency/interface/WaitingTask.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/Utilities/interface/thread_safety_macros.h"

#include <exception>

namespace edm {
  namespace eventsetup {

    EventSetupRecordIOVQueue::EventSetupRecordIOVQueue(unsigned int nConcurrentIOVs)
        : iovQueue_(nConcurrentIOVs),
          isAvailable_(nConcurrentIOVs),
          // start valid cacheIdentifiers at 1 and only increment them
          // so that the EventSetup system will never return the value 0
          // as a cacheIdentifier. Then clients which store a cache identifier
          // identifying the state of their own cache can store a 0 when their
          // cache is uninitialized.
          cacheIdentifier_(1) {
      for (auto& i : isAvailable_) {
        i.store(true);
      }
    }

    EventSetupRecordIOVQueue::~EventSetupRecordIOVQueue() { assert(endIOVCalled_); }

    void EventSetupRecordIOVQueue::endIOVAsync(edm::WaitingTaskHolder iEndTask) {
      endIOVTasks_.reset();
      if (endIOVTaskHolder_.hasTask()) {
        endIOVTasks_.add(std::move(iEndTask));
        { WaitingTaskHolder tmp{std::move(endIOVTaskHolder_)}; }
      }
      endIOVCalled_ = true;
    }

    void EventSetupRecordIOVQueue::setNewInterval() {
      bool newInterval = (recordProvider_->intervalStatus() == EventSetupRecordProvider::IntervalStatus::NewInterval);
      recordProvider_->setNewInterval(newInterval);
    }

    void EventSetupRecordIOVQueue::checkForNewIOVs(WaitingTaskHolder const& taskToStartAfterIOVInit,
                                                   WaitingTaskList& endIOVWaitingTasks,
                                                   bool newEventSetupImpl) {
      if (recordProvider_->newInterval()) {
        startNewIOVAsync(taskToStartAfterIOVInit, endIOVWaitingTasks);
        return;
      }

      if (recordProvider_->intervalStatus() != EventSetupRecordProvider::IntervalStatus::Invalid) {
        endIOVWaitingTasks.add(endIOVTaskHolder_);
      }

      recordProvider_->continueIOV(newEventSetupImpl);
    }

    void EventSetupRecordIOVQueue::startNewIOVAsync(WaitingTaskHolder const& taskToStartAfterIOVInit,
                                                    WaitingTaskList& endIOVWaitingTasks) {
      ++cacheIdentifier_;
      {
        // Let the old IOV end when all the lumis using it are done.
        // otherwise we'll deadlock when there is only one thread.
        WaitingTaskHolder tmp{std::move(endIOVTaskHolder_)};
      }
      auto taskHolder = std::make_shared<WaitingTaskHolder>(taskToStartAfterIOVInit);
      auto startIOVForRecord =
          [this, taskHolder, &endIOVWaitingTasks](edm::LimitedTaskQueue::Resumer iResumer) mutable {
            // Caught exception is propagated via WaitingTaskHolder
            CMS_SA_ALLOW try {
              unsigned int iovIndex = 0;
              auto nConcurrentIOVs = isAvailable_.size();
              for (; iovIndex < nConcurrentIOVs; ++iovIndex) {
                bool expected = true;
                if (isAvailable_[iovIndex].compare_exchange_strong(expected, false)) {
                  break;
                }
              }
              // Should never fail, just a sanity check
              if (iovIndex == nConcurrentIOVs) {
                throw edm::Exception(edm::errors::LogicError)
                    << "EventSetupRecordIOVQueue::startNewIOVAsync\n"
                    << "Couldn't find available IOV slot. This should never happen.\n"
                    << "Contact a Framework Developer\n";
              }
              recordProvider_->initializeForNewIOV(iovIndex, cacheIdentifier_);

              auto endIOVWaitingTask = make_waiting_task([this, iResumer, iovIndex](std::exception_ptr const*) mutable {
                recordProvider_->endIOV(iovIndex);
                isAvailable_[iovIndex].store(true);
                iResumer.resume();
                endIOVTasks_.doneWaiting(std::exception_ptr());
                // There is nothing in this task to catch an exception
                // because it is extremely unlikely to throw.
              });
              endIOVTaskHolder_ = WaitingTaskHolder{*taskHolder->group(), endIOVWaitingTask};
              endIOVWaitingTasks.add(endIOVTaskHolder_);
            } catch (...) {
              taskHolder->doneWaiting(std::current_exception());
              return;
            }
            taskHolder->doneWaiting(std::exception_ptr{});
          };
      iovQueue_.pushAndPause(*taskToStartAfterIOVInit.group(), std::move(startIOVForRecord));
    }

    void EventSetupRecordIOVQueue::addRecProvider(EventSetupRecordProvider* recProvider) {
      recordProvider_ = recProvider;
    }

  }  // namespace eventsetup
}  // namespace edm
