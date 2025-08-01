#ifndef FWCore_Framework_stream_EDAnalyzerAdaptorBase_h
#define FWCore_Framework_stream_EDAnalyzerAdaptorBase_h
// -*- C++ -*-
//
// Package:     FWCore/Framework
// Class  :     EDAnalyzerAdaptorBase
//
/**\class edm::stream::EDAnalyzerAdaptorBase EDAnalyzerAdaptorBase.h "FWCore/Framework/interface/stream/EDAnalyzerAdaptorBase.h"

 Description: [one line class summary]

 Usage:
    <usage>

*/
//
// Original Author:  Chris Jones
//         Created:  Fri, 02 Aug 2013 18:09:15 GMT
//

// system include files
#include <array>
#include <map>
#include <string>
#include <vector>

// user include files
#include "FWCore/Utilities/interface/ProductResolverIndex.h"
#include "FWCore/Common/interface/FWCoreCommonFwd.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "DataFormats/Provenance/interface/ModuleDescription.h"
#include "FWCore/ParameterSet/interface/ParameterSetfwd.h"
#include "FWCore/ServiceRegistry/interface/ServiceRegistryfwd.h"
#include "FWCore/Concurrency/interface/WaitingTaskHolder.h"
#include "FWCore/Utilities/interface/BranchType.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "FWCore/Utilities/interface/RunIndex.h"
#include "FWCore/Utilities/interface/LuminosityBlockIndex.h"
#include "FWCore/Utilities/interface/ESIndices.h"
#include "FWCore/Utilities/interface/Transition.h"

// forward declarations

namespace edm {
  class ModuleCallingContext;
  class ProductResolverIndexHelper;
  class EDConsumerBase;
  class PreallocationConfiguration;
  class ProductResolverIndexAndSkipBit;
  class ActivityRegistry;
  class ThinnedAssociationsHelper;
  class SignallingProductRegistryFiller;
  struct ModuleConsumesMinimalESInfo;

  namespace maker {
    template <typename T>
    class ModuleHolderT;
  }

  namespace eventsetup {
    struct ComponentDescription;
    class ESRecordsToProductResolverIndices;
  }  // namespace eventsetup

  namespace stream {
    class EDAnalyzerBase;

    class EDAnalyzerAdaptorBase {
    public:
      template <typename T>
      friend class edm::WorkerT;
      template <typename T>
      friend class edm::maker::ModuleHolderT;

      EDAnalyzerAdaptorBase();
      EDAnalyzerAdaptorBase(const EDAnalyzerAdaptorBase&) = delete;                   // stop default
      const EDAnalyzerAdaptorBase& operator=(const EDAnalyzerAdaptorBase&) = delete;  // stop default
      virtual ~EDAnalyzerAdaptorBase();

      // ---------- const member functions ---------------------

      // ---------- static member functions --------------------

      // ---------- member functions ---------------------------
      const ModuleDescription& moduleDescription() const { return moduleDescription_; }

      virtual bool wantsProcessBlocks() const noexcept = 0;
      virtual bool wantsInputProcessBlocks() const noexcept = 0;
      virtual bool wantsGlobalRuns() const noexcept = 0;
      virtual bool wantsGlobalLuminosityBlocks() const noexcept = 0;
      virtual bool wantsStreamRuns() const noexcept = 0;
      virtual bool wantsStreamLuminosityBlocks() const noexcept = 0;

      void registerProductsAndCallbacks(EDAnalyzerAdaptorBase const*, SignallingProductRegistryFiller* reg);

    protected:
      template <typename T>
      void createStreamModules(T iFunc) {
        unsigned int iStreamModule = 0;
        for (auto& m : m_streamModules) {
          m = iFunc(iStreamModule);
          setModuleDescriptionPtr(m);
          ++iStreamModule;
        }
      }

      //Same interface as EDConsumerBase
      void itemsToGet(BranchType, std::vector<ProductResolverIndexAndSkipBit>&) const;
      void itemsMayGet(BranchType, std::vector<ProductResolverIndexAndSkipBit>&) const;
      std::vector<ProductResolverIndexAndSkipBit> const& itemsToGetFrom(BranchType) const;

      std::vector<ESResolverIndex> const& esGetTokenIndicesVector(edm::Transition iTrans) const;
      std::vector<ESRecordIndex> const& esGetTokenRecordIndicesVector(edm::Transition iTrans) const;

      void updateLookup(BranchType iBranchType, ProductResolverIndexHelper const&, bool iPrefetchMayGet);
      void updateLookup(eventsetup::ESRecordsToProductResolverIndices const&);
      void releaseMemoryPostLookupSignal();

      virtual void selectInputProcessBlocks(ProductRegistry const&, ProcessBlockHelperBase const&) = 0;

      const EDConsumerBase* consumer() const;

      void convertCurrentProcessAlias(std::string const& processName);

      std::vector<ModuleConsumesInfo> moduleConsumesInfos() const;
      std::vector<ModuleConsumesMinimalESInfo> moduleConsumesMinimalESInfos() const;
      void deleteModulesEarly();

    private:
      bool doEvent(EventTransitionInfo const&, ActivityRegistry*, ModuleCallingContext const*);
      void doPreallocate(PreallocationConfiguration const&);
      virtual void preallocRuns(unsigned int) {}
      virtual void preallocLumis(unsigned int) {}

      //For now this is a placeholder
      /*virtual*/ void preActionBeforeRunEventAsync(WaitingTaskHolder,
                                                    ModuleCallingContext const&,
                                                    Principal const&) const noexcept {}

      virtual void setupStreamModules() = 0;
      virtual void doBeginJob() = 0;
      virtual void doEndJob() = 0;

      void doBeginStream(StreamID id);
      void doEndStream(StreamID id);
      void doStreamBeginRun(StreamID, RunTransitionInfo const&, ModuleCallingContext const*);
      virtual void setupRun(EDAnalyzerBase*, RunIndex) = 0;
      void doStreamEndRun(StreamID, RunTransitionInfo const&, ModuleCallingContext const*);
      virtual void streamEndRunSummary(EDAnalyzerBase*, edm::Run const&, edm::EventSetup const&) = 0;

      void doStreamBeginLuminosityBlock(StreamID, LumiTransitionInfo const&, ModuleCallingContext const*);
      virtual void setupLuminosityBlock(EDAnalyzerBase*, LuminosityBlockIndex) = 0;
      void doStreamEndLuminosityBlock(StreamID, LumiTransitionInfo const&, ModuleCallingContext const*);
      virtual void streamEndLuminosityBlockSummary(EDAnalyzerBase*,
                                                   edm::LuminosityBlock const&,
                                                   edm::EventSetup const&) = 0;

      virtual void doBeginProcessBlock(ProcessBlockPrincipal const&, ModuleCallingContext const*) = 0;
      virtual void doAccessInputProcessBlock(ProcessBlockPrincipal const&, ModuleCallingContext const*) = 0;
      virtual void doEndProcessBlock(ProcessBlockPrincipal const&, ModuleCallingContext const*) = 0;
      virtual void doBeginRun(RunTransitionInfo const&, ModuleCallingContext const*) = 0;
      virtual void doEndRun(RunTransitionInfo const&, ModuleCallingContext const*) = 0;
      virtual void doBeginLuminosityBlock(LumiTransitionInfo const&, ModuleCallingContext const*) = 0;
      virtual void doEndLuminosityBlock(LumiTransitionInfo const&, ModuleCallingContext const*) = 0;

      virtual void doRespondToCloseOutputFile() = 0;

      bool hasAcquire() const noexcept { return false; }
      bool hasAccumulator() const noexcept { return false; }

      // ---------- member data --------------------------------
      void setModuleDescriptionPtr(EDAnalyzerBase* m);
      void setModuleDescription(ModuleDescription const& md) { moduleDescription_ = md; }
      ModuleDescription moduleDescription_;

      std::vector<EDAnalyzerBase*> m_streamModules;
    };
  }  // namespace stream
}  // namespace edm
#endif
