#include "FWCore/Framework/interface/maker/WorkerT.h"
#include "FWCore/Framework/interface/EventPrincipal.h"

#include "FWCore/Framework/interface/one/EDProducerBase.h"
#include "FWCore/Framework/interface/one/EDFilterBase.h"
#include "FWCore/Framework/interface/one/EDAnalyzerBase.h"
#include "FWCore/Framework/interface/one/OutputModuleBase.h"
#include "FWCore/Framework/interface/global/EDProducerBase.h"
#include "FWCore/Framework/interface/global/EDFilterBase.h"
#include "FWCore/Framework/interface/global/EDAnalyzerBase.h"
#include "FWCore/Framework/interface/global/OutputModuleBase.h"

#include "FWCore/Framework/interface/stream/EDProducerAdaptorBase.h"
#include "FWCore/Framework/interface/stream/EDFilterAdaptorBase.h"
#include "FWCore/Framework/interface/stream/EDAnalyzerAdaptorBase.h"

#include "FWCore/Framework/interface/limited/EDProducerBase.h"
#include "FWCore/Framework/interface/limited/EDFilterBase.h"
#include "FWCore/Framework/interface/limited/EDAnalyzerBase.h"
#include "FWCore/Framework/interface/limited/OutputModuleBase.h"

#include "FWCore/ServiceRegistry/interface/ModuleConsumesInfo.h"

#include <type_traits>

namespace edm {
  namespace workerimpl {
    template <typename T>
    struct has_stream_functions {
      static bool constexpr value = false;
    };

    template <>
    struct has_stream_functions<edm::global::EDProducerBase> {
      static bool constexpr value = true;
    };

    template <>
    struct has_stream_functions<edm::global::EDFilterBase> {
      static bool constexpr value = true;
    };

    template <>
    struct has_stream_functions<edm::global::EDAnalyzerBase> {
      static bool constexpr value = true;
    };

    template <>
    struct has_stream_functions<edm::limited::EDProducerBase> {
      static bool constexpr value = true;
    };

    template <>
    struct has_stream_functions<edm::limited::EDFilterBase> {
      static bool constexpr value = true;
    };

    template <>
    struct has_stream_functions<edm::limited::EDAnalyzerBase> {
      static bool constexpr value = true;
    };

    template <>
    struct has_stream_functions<edm::stream::EDProducerAdaptorBase> {
      static bool constexpr value = true;
    };

    template <>
    struct has_stream_functions<edm::stream::EDFilterAdaptorBase> {
      static bool constexpr value = true;
    };

    template <>
    struct has_stream_functions<edm::stream::EDAnalyzerAdaptorBase> {
      static bool constexpr value = true;
    };

    template <typename T>
    struct has_only_stream_transition_functions {
      static bool constexpr value = false;
    };

    template <>
    struct has_only_stream_transition_functions<edm::global::OutputModuleBase> {
      static bool constexpr value = true;
    };

    struct DoNothing {
      template <typename... T>
      inline void operator()(const T&...) {}
    };

    template <typename T>
    struct DoBeginStream {
      inline void operator()(WorkerT<T>* iWorker, StreamID id) { iWorker->callWorkerBeginStream(0, id); }
    };

    template <typename T>
    struct DoEndStream {
      inline void operator()(WorkerT<T>* iWorker, StreamID id) { iWorker->callWorkerEndStream(0, id); }
    };

    template <typename T, typename INFOTYPE>
    struct DoStreamBeginTrans {
      inline void operator()(WorkerT<T>* iWorker, StreamID id, INFOTYPE const& info, ModuleCallingContext const* mcc) {
        iWorker->callWorkerStreamBegin(0, id, info, mcc);
      }
    };

    template <typename T, typename INFOTYPE>
    struct DoStreamEndTrans {
      inline void operator()(WorkerT<T>* iWorker, StreamID id, INFOTYPE const& info, ModuleCallingContext const* mcc) {
        iWorker->callWorkerStreamEnd(0, id, info, mcc);
      }
    };
  }  // namespace workerimpl

  template <typename T>
  inline WorkerT<T>::WorkerT(std::shared_ptr<T> ed, ModuleDescription const& md, ExceptionToActionTable const* actions)
      : Worker(md, actions), module_(ed) {
    assert(module_ != nullptr);
  }

  template <typename T>
  WorkerT<T>::~WorkerT() {}

  template <typename T>
  bool WorkerT<T>::wantsProcessBlocks() const noexcept {
    return module_->wantsProcessBlocks();
  }

  template <typename T>
  bool WorkerT<T>::wantsInputProcessBlocks() const noexcept {
    return module_->wantsInputProcessBlocks();
  }

  template <typename T>
  bool WorkerT<T>::wantsGlobalRuns() const noexcept {
    return module_->wantsGlobalRuns();
  }

  template <typename T>
  bool WorkerT<T>::wantsGlobalLuminosityBlocks() const noexcept {
    return module_->wantsGlobalLuminosityBlocks();
  }

  template <typename T>
  bool WorkerT<T>::wantsStreamRuns() const noexcept {
    return module_->wantsStreamRuns();
  }

  template <typename T>
  bool WorkerT<T>::wantsStreamLuminosityBlocks() const noexcept {
    return module_->wantsStreamLuminosityBlocks();
  }

  template <typename T>
  SerialTaskQueue* WorkerT<T>::globalRunsQueue() {
    return nullptr;
  }
  template <typename T>
  SerialTaskQueue* WorkerT<T>::globalLuminosityBlocksQueue() {
    return nullptr;
  }

  //one
  template <>
  SerialTaskQueue* WorkerT<one::EDProducerBase>::globalRunsQueue() {
    return module_->globalRunsQueue();
  }
  template <>
  SerialTaskQueue* WorkerT<one::EDProducerBase>::globalLuminosityBlocksQueue() {
    return module_->globalLuminosityBlocksQueue();
  }
  template <>
  SerialTaskQueue* WorkerT<one::EDFilterBase>::globalRunsQueue() {
    return module_->globalRunsQueue();
  }
  template <>
  SerialTaskQueue* WorkerT<one::EDFilterBase>::globalLuminosityBlocksQueue() {
    return module_->globalLuminosityBlocksQueue();
  }
  template <>
  SerialTaskQueue* WorkerT<one::EDAnalyzerBase>::globalRunsQueue() {
    return module_->globalRunsQueue();
  }
  template <>
  SerialTaskQueue* WorkerT<one::EDAnalyzerBase>::globalLuminosityBlocksQueue() {
    return module_->globalLuminosityBlocksQueue();
  }
  template <>
  SerialTaskQueue* WorkerT<one::OutputModuleBase>::globalRunsQueue() {
    return module_->globalRunsQueue();
  }
  template <>
  SerialTaskQueue* WorkerT<one::OutputModuleBase>::globalLuminosityBlocksQueue() {
    return module_->globalLuminosityBlocksQueue();
  }

  template <typename T>
  inline bool WorkerT<T>::implDo(EventTransitionInfo const& info, ModuleCallingContext const* mcc) {
    EventPrincipal const& ep = info.principal();
    std::shared_ptr<Worker> sentry(this, [&ep](Worker* obj) { obj->postDoEvent(ep); });
    return module_->doEvent(info, activityRegistry(), mcc);
  }

  template <typename T>
  inline void WorkerT<T>::implDoAcquire(EventTransitionInfo const&, ModuleCallingContext const*, WaitingTaskHolder&&) {}

  template <>
  inline void WorkerT<global::EDProducerBase>::implDoAcquire(EventTransitionInfo const& info,
                                                             ModuleCallingContext const* mcc,
                                                             WaitingTaskHolder&& holder) {
    module_->doAcquire(info, activityRegistry(), mcc, std::move(holder));
  }

  template <>
  inline void WorkerT<global::EDFilterBase>::implDoAcquire(EventTransitionInfo const& info,
                                                           ModuleCallingContext const* mcc,
                                                           WaitingTaskHolder&& holder) {
    module_->doAcquire(info, activityRegistry(), mcc, std::move(holder));
  }

  template <>
  inline void WorkerT<global::OutputModuleBase>::implDoAcquire(EventTransitionInfo const& info,
                                                               ModuleCallingContext const* mcc,
                                                               WaitingTaskHolder&& holder) {
    module_->doAcquire(info, activityRegistry(), mcc, std::move(holder));
  }

  template <>
  inline void WorkerT<stream::EDProducerAdaptorBase>::implDoAcquire(EventTransitionInfo const& info,
                                                                    ModuleCallingContext const* mcc,
                                                                    WaitingTaskHolder&& holder) {
    module_->doAcquire(info, activityRegistry(), mcc, std::move(holder));
  }

  template <>
  inline void WorkerT<stream::EDFilterAdaptorBase>::implDoAcquire(EventTransitionInfo const& info,
                                                                  ModuleCallingContext const* mcc,
                                                                  WaitingTaskHolder&& holder) {
    module_->doAcquire(info, activityRegistry(), mcc, std::move(holder));
  }

  template <typename T>
  inline void WorkerT<T>::implDoTransformAsync(WaitingTaskHolder iTask,
                                               size_t iTransformIndex,
                                               EventPrincipal const& iEvent,
                                               ParentContext const& iParent,
                                               ServiceWeakToken const& weakToken) noexcept {
    CMS_SA_ALLOW try {
      ServiceRegistry::Operate guard(weakToken.lock());

      ModuleCallingContext mcc(
          &module_->moduleDescription(), iTransformIndex + 1, ModuleCallingContext::State::kRunning, iParent, nullptr);
      module_->doTransformAsync(iTask, iTransformIndex, iEvent, activityRegistry(), mcc, weakToken);
    } catch (...) {
      iTask.doneWaiting(std::current_exception());
      return;
    }
    iTask.doneWaiting(std::exception_ptr());
  }

  template <>
  inline void WorkerT<global::EDAnalyzerBase>::implDoTransformAsync(WaitingTaskHolder task,
                                                                    size_t iTransformIndex,
                                                                    EventPrincipal const& iEvent,
                                                                    ParentContext const& iParent,
                                                                    ServiceWeakToken const& weakToken) noexcept {}
  template <>
  inline void WorkerT<global::OutputModuleBase>::implDoTransformAsync(WaitingTaskHolder task,
                                                                      size_t iTransformIndex,
                                                                      EventPrincipal const& iEvent,
                                                                      ParentContext const& iParent,
                                                                      ServiceWeakToken const& weakToken) noexcept {}
  template <>
  inline void WorkerT<limited::EDAnalyzerBase>::implDoTransformAsync(WaitingTaskHolder task,
                                                                     size_t iTransformIndex,
                                                                     EventPrincipal const& iEvent,
                                                                     ParentContext const& iParent,
                                                                     ServiceWeakToken const& weakToken) noexcept {}
  template <>
  inline void WorkerT<limited::OutputModuleBase>::implDoTransformAsync(WaitingTaskHolder task,
                                                                       size_t iTransformIndex,
                                                                       EventPrincipal const& iEvent,
                                                                       ParentContext const& iParent,
                                                                       ServiceWeakToken const& weakToken) noexcept {}
  template <>
  inline void WorkerT<one::EDAnalyzerBase>::implDoTransformAsync(WaitingTaskHolder task,
                                                                 size_t iTransformIndex,
                                                                 EventPrincipal const& iEvent,
                                                                 ParentContext const& iParent,
                                                                 ServiceWeakToken const& weakToken) noexcept {}
  template <>
  inline void WorkerT<one::OutputModuleBase>::implDoTransformAsync(WaitingTaskHolder task,
                                                                   size_t iTransformIndex,
                                                                   EventPrincipal const& iEvent,
                                                                   ParentContext const& iParent,
                                                                   ServiceWeakToken const& weakToken) noexcept {}
  template <>
  inline void WorkerT<stream::EDAnalyzerAdaptorBase>::implDoTransformAsync(WaitingTaskHolder task,
                                                                           size_t iTransformIndex,
                                                                           EventPrincipal const& iEvent,
                                                                           ParentContext const& iParent,
                                                                           ServiceWeakToken const& weakToken) noexcept {
  }

  template <typename T>
  inline size_t WorkerT<T>::transformIndex(edm::ProductDescription const&) const noexcept {
    return -1;
  }
  template <>
  inline size_t WorkerT<global::EDFilterBase>::transformIndex(edm::ProductDescription const& iBranch) const noexcept {
    return module_->transformIndex_(iBranch);
  }
  template <>
  inline size_t WorkerT<global::EDProducerBase>::transformIndex(edm::ProductDescription const& iBranch) const noexcept {
    return module_->transformIndex_(iBranch);
  }
  template <>
  inline size_t WorkerT<stream::EDProducerAdaptorBase>::transformIndex(
      edm::ProductDescription const& iBranch) const noexcept {
    return module_->transformIndex_(iBranch);
  }
  template <>
  inline size_t WorkerT<limited::EDFilterBase>::transformIndex(edm::ProductDescription const& iBranch) const noexcept {
    return module_->transformIndex_(iBranch);
  }
  template <>
  inline size_t WorkerT<limited::EDProducerBase>::transformIndex(edm::ProductDescription const& iBranch) const noexcept {
    return module_->transformIndex_(iBranch);
  }
  template <>
  inline size_t WorkerT<one::EDFilterBase>::transformIndex(edm::ProductDescription const& iBranch) const noexcept {
    return module_->transformIndex_(iBranch);
  }
  template <>
  inline size_t WorkerT<one::EDProducerBase>::transformIndex(edm::ProductDescription const& iBranch) const noexcept {
    return module_->transformIndex_(iBranch);
  }

  template <typename T>
  inline ProductResolverIndex WorkerT<T>::itemToGetForTransform(size_t iTransformIndex) const noexcept {
    return -1;
  }
  template <>
  inline ProductResolverIndex WorkerT<global::EDFilterBase>::itemToGetForTransform(
      size_t iTransformIndex) const noexcept {
    return module_->transformPrefetch_(iTransformIndex);
  }
  template <>
  inline ProductResolverIndex WorkerT<global::EDProducerBase>::itemToGetForTransform(
      size_t iTransformIndex) const noexcept {
    return module_->transformPrefetch_(iTransformIndex);
  }
  template <>
  inline ProductResolverIndex WorkerT<stream::EDProducerAdaptorBase>::itemToGetForTransform(
      size_t iTransformIndex) const noexcept {
    return module_->transformPrefetch_(iTransformIndex);
  }
  template <>
  inline ProductResolverIndex WorkerT<limited::EDFilterBase>::itemToGetForTransform(
      size_t iTransformIndex) const noexcept {
    return module_->transformPrefetch_(iTransformIndex);
  }
  template <>
  inline ProductResolverIndex WorkerT<limited::EDProducerBase>::itemToGetForTransform(
      size_t iTransformIndex) const noexcept {
    return module_->transformPrefetch_(iTransformIndex);
  }
  template <>
  inline ProductResolverIndex WorkerT<one::EDFilterBase>::itemToGetForTransform(size_t iTransformIndex) const noexcept {
    return module_->transformPrefetch_(iTransformIndex);
  }
  template <>
  inline ProductResolverIndex WorkerT<one::EDProducerBase>::itemToGetForTransform(
      size_t iTransformIndex) const noexcept {
    return module_->transformPrefetch_(iTransformIndex);
  }

  template <typename T>
  inline bool WorkerT<T>::implNeedToRunSelection() const noexcept {
    return false;
  }

  template <typename T>
  inline bool WorkerT<T>::implDoPrePrefetchSelection(StreamID id,
                                                     EventPrincipal const& ep,
                                                     ModuleCallingContext const* mcc) {
    return true;
  }
  template <typename T>
  inline void WorkerT<T>::itemsToGetForSelection(std::vector<ProductResolverIndexAndSkipBit>&) const {}

  template <>
  inline bool WorkerT<edm::one::OutputModuleBase>::implNeedToRunSelection() const noexcept {
    return true;
  }

  template <>
  inline bool WorkerT<edm::one::OutputModuleBase>::implDoPrePrefetchSelection(StreamID id,
                                                                              EventPrincipal const& ep,
                                                                              ModuleCallingContext const* mcc) {
    return module_->prePrefetchSelection(id, ep, mcc);
  }
  template <>
  inline void WorkerT<edm::one::OutputModuleBase>::itemsToGetForSelection(
      std::vector<ProductResolverIndexAndSkipBit>& iItems) const {
    iItems = module_->productsUsedBySelection();
  }

  template <>
  inline bool WorkerT<edm::global::OutputModuleBase>::implNeedToRunSelection() const noexcept {
    return true;
  }
  template <>
  inline bool WorkerT<edm::global::OutputModuleBase>::implDoPrePrefetchSelection(StreamID id,
                                                                                 EventPrincipal const& ep,
                                                                                 ModuleCallingContext const* mcc) {
    return module_->prePrefetchSelection(id, ep, mcc);
  }
  template <>
  inline void WorkerT<edm::global::OutputModuleBase>::itemsToGetForSelection(
      std::vector<ProductResolverIndexAndSkipBit>& iItems) const {
    iItems = module_->productsUsedBySelection();
  }

  template <>
  inline bool WorkerT<edm::limited::OutputModuleBase>::implNeedToRunSelection() const noexcept {
    return true;
  }
  template <>
  inline bool WorkerT<edm::limited::OutputModuleBase>::implDoPrePrefetchSelection(StreamID id,
                                                                                  EventPrincipal const& ep,
                                                                                  ModuleCallingContext const* mcc) {
    return module_->prePrefetchSelection(id, ep, mcc);
  }
  template <>
  inline void WorkerT<edm::limited::OutputModuleBase>::itemsToGetForSelection(
      std::vector<ProductResolverIndexAndSkipBit>& iItems) const {
    iItems = module_->productsUsedBySelection();
  }

  template <typename T>
  bool WorkerT<T>::implDoBeginProcessBlock(ProcessBlockPrincipal const& pbp, ModuleCallingContext const* mcc) {
    module_->doBeginProcessBlock(pbp, mcc);
    return true;
  }

  template <typename T>
  bool WorkerT<T>::implDoAccessInputProcessBlock(ProcessBlockPrincipal const& pbp, ModuleCallingContext const* mcc) {
    module_->doAccessInputProcessBlock(pbp, mcc);
    return true;
  }

  template <typename T>
  bool WorkerT<T>::implDoEndProcessBlock(ProcessBlockPrincipal const& pbp, ModuleCallingContext const* mcc) {
    module_->doEndProcessBlock(pbp, mcc);
    return true;
  }

  template <typename T>
  inline bool WorkerT<T>::implDoBegin(RunTransitionInfo const& info, ModuleCallingContext const* mcc) {
    module_->doBeginRun(info, mcc);
    return true;
  }

  template <typename T>
  template <typename D>
  void WorkerT<T>::callWorkerStreamBegin(D,
                                         StreamID id,
                                         RunTransitionInfo const& info,
                                         ModuleCallingContext const* mcc) {
    module_->doStreamBeginRun(id, info, mcc);
  }

  template <typename T>
  template <typename D>
  void WorkerT<T>::callWorkerStreamEnd(D, StreamID id, RunTransitionInfo const& info, ModuleCallingContext const* mcc) {
    module_->doStreamEndRun(id, info, mcc);
  }

  template <typename T>
  inline bool WorkerT<T>::implDoStreamBegin(StreamID id,
                                            RunTransitionInfo const& info,
                                            ModuleCallingContext const* mcc) {
    std::conditional_t<workerimpl::has_stream_functions<T>::value,
                       workerimpl::DoStreamBeginTrans<T, RunTransitionInfo const>,
                       workerimpl::DoNothing>
        might_call;
    might_call(this, id, info, mcc);
    return true;
  }

  template <typename T>
  inline bool WorkerT<T>::implDoStreamEnd(StreamID id, RunTransitionInfo const& info, ModuleCallingContext const* mcc) {
    std::conditional_t<workerimpl::has_stream_functions<T>::value,
                       workerimpl::DoStreamEndTrans<T, RunTransitionInfo const>,
                       workerimpl::DoNothing>
        might_call;
    might_call(this, id, info, mcc);
    return true;
  }

  template <typename T>
  inline bool WorkerT<T>::implDoEnd(RunTransitionInfo const& info, ModuleCallingContext const* mcc) {
    module_->doEndRun(info, mcc);
    return true;
  }

  template <typename T>
  inline bool WorkerT<T>::implDoBegin(LumiTransitionInfo const& info, ModuleCallingContext const* mcc) {
    module_->doBeginLuminosityBlock(info, mcc);
    return true;
  }

  template <typename T>
  template <typename D>
  void WorkerT<T>::callWorkerStreamBegin(D,
                                         StreamID id,
                                         LumiTransitionInfo const& info,
                                         ModuleCallingContext const* mcc) {
    module_->doStreamBeginLuminosityBlock(id, info, mcc);
  }

  template <typename T>
  template <typename D>
  void WorkerT<T>::callWorkerStreamEnd(D, StreamID id, LumiTransitionInfo const& info, ModuleCallingContext const* mcc) {
    module_->doStreamEndLuminosityBlock(id, info, mcc);
  }

  template <typename T>
  inline bool WorkerT<T>::implDoStreamBegin(StreamID id,
                                            LumiTransitionInfo const& info,
                                            ModuleCallingContext const* mcc) {
    std::conditional_t<workerimpl::has_stream_functions<T>::value,
                       workerimpl::DoStreamBeginTrans<T, LumiTransitionInfo>,
                       workerimpl::DoNothing>
        might_call;
    might_call(this, id, info, mcc);
    return true;
  }

  template <typename T>
  inline bool WorkerT<T>::implDoStreamEnd(StreamID id,
                                          LumiTransitionInfo const& info,
                                          ModuleCallingContext const* mcc) {
    std::conditional_t<workerimpl::has_stream_functions<T>::value,
                       workerimpl::DoStreamEndTrans<T, LumiTransitionInfo>,
                       workerimpl::DoNothing>
        might_call;
    might_call(this, id, info, mcc);

    return true;
  }

  template <typename T>
  inline bool WorkerT<T>::implDoEnd(LumiTransitionInfo const& info, ModuleCallingContext const* mcc) {
    module_->doEndLuminosityBlock(info, mcc);
    return true;
  }

  template <typename T>
  template <typename D>
  void WorkerT<T>::callWorkerBeginStream(D, StreamID id) {
    module_->doBeginStream(id);
  }

  template <typename T>
  template <typename D>
  void WorkerT<T>::callWorkerEndStream(D, StreamID id) {
    module_->doEndStream(id);
  }

  template <typename T>
  inline Worker::TaskQueueAdaptor WorkerT<T>::serializeRunModule() {
    return Worker::TaskQueueAdaptor{};
  }
  template <>
  Worker::TaskQueueAdaptor WorkerT<one::EDAnalyzerBase>::serializeRunModule() {
    return &(module_->sharedResourcesAcquirer().serialQueueChain());
  }
  template <>
  Worker::TaskQueueAdaptor WorkerT<one::EDFilterBase>::serializeRunModule() {
    return &(module_->sharedResourcesAcquirer().serialQueueChain());
  }
  template <>
  Worker::TaskQueueAdaptor WorkerT<one::EDProducerBase>::serializeRunModule() {
    return &(module_->sharedResourcesAcquirer().serialQueueChain());
  }
  template <>
  Worker::TaskQueueAdaptor WorkerT<one::OutputModuleBase>::serializeRunModule() {
    return &(module_->sharedResourcesAcquirer().serialQueueChain());
  }
  template <>
  Worker::TaskQueueAdaptor WorkerT<limited::EDAnalyzerBase>::serializeRunModule() {
    return &(module_->queue());
  }
  template <>
  Worker::TaskQueueAdaptor WorkerT<limited::EDFilterBase>::serializeRunModule() {
    return &(module_->queue());
  }
  template <>
  Worker::TaskQueueAdaptor WorkerT<limited::EDProducerBase>::serializeRunModule() {
    return &(module_->queue());
  }
  template <>
  Worker::TaskQueueAdaptor WorkerT<limited::OutputModuleBase>::serializeRunModule() {
    return &(module_->queue());
  }

  template <>
  Worker::Types WorkerT<edm::one::EDProducerBase>::moduleType() const {
    return Worker::kProducer;
  }
  template <>
  Worker::Types WorkerT<edm::one::EDFilterBase>::moduleType() const {
    return Worker::kFilter;
  }
  template <>
  Worker::Types WorkerT<edm::one::EDAnalyzerBase>::moduleType() const {
    return Worker::kAnalyzer;
  }
  template <>
  Worker::Types WorkerT<edm::one::OutputModuleBase>::moduleType() const {
    return Worker::kOutputModule;
  }

  template <>
  Worker::Types WorkerT<edm::global::EDProducerBase>::moduleType() const {
    return Worker::kProducer;
  }
  template <>
  Worker::Types WorkerT<edm::global::EDFilterBase>::moduleType() const {
    return Worker::kFilter;
  }
  template <>
  Worker::Types WorkerT<edm::global::EDAnalyzerBase>::moduleType() const {
    return Worker::kAnalyzer;
  }
  template <>
  Worker::Types WorkerT<edm::global::OutputModuleBase>::moduleType() const {
    return Worker::kOutputModule;
  }

  template <>
  Worker::Types WorkerT<edm::limited::EDProducerBase>::moduleType() const {
    return Worker::kProducer;
  }
  template <>
  Worker::Types WorkerT<edm::limited::EDFilterBase>::moduleType() const {
    return Worker::kFilter;
  }
  template <>
  Worker::Types WorkerT<edm::limited::EDAnalyzerBase>::moduleType() const {
    return Worker::kAnalyzer;
  }
  template <>
  Worker::Types WorkerT<edm::limited::OutputModuleBase>::moduleType() const {
    return Worker::kOutputModule;
  }

  template <>
  Worker::Types WorkerT<edm::stream::EDProducerAdaptorBase>::moduleType() const {
    return Worker::kProducer;
  }
  template <>
  Worker::Types WorkerT<edm::stream::EDFilterAdaptorBase>::moduleType() const {
    return Worker::kFilter;
  }
  template <>
  Worker::Types WorkerT<edm::stream::EDAnalyzerAdaptorBase>::moduleType() const {
    return Worker::kAnalyzer;
  }

  template <>
  Worker::ConcurrencyTypes WorkerT<edm::one::EDProducerBase>::moduleConcurrencyType() const {
    return Worker::kOne;
  }
  template <>
  Worker::ConcurrencyTypes WorkerT<edm::one::EDFilterBase>::moduleConcurrencyType() const {
    return Worker::kOne;
  }
  template <>
  Worker::ConcurrencyTypes WorkerT<edm::one::EDAnalyzerBase>::moduleConcurrencyType() const {
    return Worker::kOne;
  }
  template <>
  Worker::ConcurrencyTypes WorkerT<edm::one::OutputModuleBase>::moduleConcurrencyType() const {
    return Worker::kOne;
  }

  template <>
  Worker::ConcurrencyTypes WorkerT<edm::global::EDProducerBase>::moduleConcurrencyType() const {
    return Worker::kGlobal;
  }
  template <>
  Worker::ConcurrencyTypes WorkerT<edm::global::EDFilterBase>::moduleConcurrencyType() const {
    return Worker::kGlobal;
  }
  template <>
  Worker::ConcurrencyTypes WorkerT<edm::global::EDAnalyzerBase>::moduleConcurrencyType() const {
    return Worker::kGlobal;
  }
  template <>
  Worker::ConcurrencyTypes WorkerT<edm::global::OutputModuleBase>::moduleConcurrencyType() const {
    return Worker::kGlobal;
  }

  template <>
  Worker::ConcurrencyTypes WorkerT<edm::limited::EDProducerBase>::moduleConcurrencyType() const {
    return Worker::kLimited;
  }
  template <>
  Worker::ConcurrencyTypes WorkerT<edm::limited::EDFilterBase>::moduleConcurrencyType() const {
    return Worker::kLimited;
  }
  template <>
  Worker::ConcurrencyTypes WorkerT<edm::limited::EDAnalyzerBase>::moduleConcurrencyType() const {
    return Worker::kLimited;
  }
  template <>
  Worker::ConcurrencyTypes WorkerT<edm::limited::OutputModuleBase>::moduleConcurrencyType() const {
    return Worker::kLimited;
  }

  template <>
  Worker::ConcurrencyTypes WorkerT<edm::stream::EDProducerAdaptorBase>::moduleConcurrencyType() const {
    return Worker::kStream;
  }
  template <>
  Worker::ConcurrencyTypes WorkerT<edm::stream::EDFilterAdaptorBase>::moduleConcurrencyType() const {
    return Worker::kStream;
  }
  template <>
  Worker::ConcurrencyTypes WorkerT<edm::stream::EDAnalyzerAdaptorBase>::moduleConcurrencyType() const {
    return Worker::kStream;
  }

  //Explicitly instantiate our needed templates to avoid having the compiler
  // instantiate them in all of our libraries
  template class WorkerT<one::EDProducerBase>;
  template class WorkerT<one::EDFilterBase>;
  template class WorkerT<one::EDAnalyzerBase>;
  template class WorkerT<one::OutputModuleBase>;
  template class WorkerT<global::EDProducerBase>;
  template class WorkerT<global::EDFilterBase>;
  template class WorkerT<global::EDAnalyzerBase>;
  template class WorkerT<global::OutputModuleBase>;
  template class WorkerT<stream::EDProducerAdaptorBase>;
  template class WorkerT<stream::EDFilterAdaptorBase>;
  template class WorkerT<stream::EDAnalyzerAdaptorBase>;
  template class WorkerT<limited::EDProducerBase>;
  template class WorkerT<limited::EDFilterBase>;
  template class WorkerT<limited::EDAnalyzerBase>;
  template class WorkerT<limited::OutputModuleBase>;
}  // namespace edm
