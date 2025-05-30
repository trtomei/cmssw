/**----------------------------------------------------------------------
  ----------------------------------------------------------------------*/
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wc++20-extensions"
#endif

#include "FWCore/Framework/interface/Principal.h"

#include "DataFormats/Provenance/interface/ProcessConfiguration.h"
#include "DataFormats/Provenance/interface/ProductResolverIndexHelper.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "DataFormats/Common/interface/FunctorHandleExceptionFactory.h"
#include "FWCore/Framework/interface/DelayedReader.h"
#include "FWCore/Framework/interface/HistoryAppender.h"
#include "FWCore/Framework/src/ProductDeletedException.h"
#include "FWCore/Framework/interface/ProductPutterBase.h"
#include "FWCore/Framework/interface/EDConsumerBase.h"
#include "DroppedDataProductResolver.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/Utilities/interface/ProductResolverIndex.h"
#include "FWCore/Utilities/interface/TypeID.h"
#include "FWCore/Utilities/interface/WrappedClassName.h"
#include "FWCore/Utilities/interface/Likely.h"
#include "FWCore/Utilities/interface/thread_safety_macros.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <typeinfo>
#include <atomic>
namespace edm {

  static ProcessHistory const s_emptyProcessHistory;

  static std::string appendCurrentProcessIfAlias(std::string const& processFromInputTag,
                                                 std::string const& currentProcess) {
    if (processFromInputTag == InputTag::kCurrentProcess) {
      std::string returnValue = processFromInputTag;
      returnValue += " (";
      returnValue += currentProcess;
      returnValue += ")";
      return returnValue;
    }
    return processFromInputTag;
  }

  static void throwProductNotFoundException(char const* where, errors::ErrorCodes error, BranchID const& bid) {
    throw Exception(error, "InvalidID") << "Principal::" << where << ": no product with given branch id: " << bid
                                        << "\n";
  }

  static std::shared_ptr<cms::Exception> makeNotFoundException(char const* where,
                                                               KindOfType kindOfType,
                                                               TypeID const& productType,
                                                               std::string const& label,
                                                               std::string const& instance,
                                                               std::string const& process) {
    std::shared_ptr<cms::Exception> exception = std::make_shared<Exception>(errors::ProductNotFound);
    if (kindOfType == PRODUCT_TYPE) {
      *exception << "Principal::" << where
                 << ": Found zero products matching all criteria\nLooking for type: " << productType << "\n"
                 << "Looking for module label: " << label << "\n"
                 << "Looking for productInstanceName: " << instance << "\n"
                 << (process.empty() ? "" : "Looking for process: ") << process << "\n";
    } else {
      *exception << "Principal::" << where
                 << ": Found zero products matching all criteria\nLooking for a container with elements of type: "
                 << productType << "\n"
                 << "Looking for module label: " << label << "\n"
                 << "Looking for productInstanceName: " << instance << "\n"
                 << (process.empty() ? "" : "Looking for process: ") << process << "\n";
    }
    return exception;
  }

  static void throwAmbiguousException(const char* where,
                                      TypeID const& productType,
                                      std::string const& label,
                                      std::string const& instance,
                                      std::string const& process) {
    cms::Exception exception("AmbiguousProduct");
    exception << "Principal::" << where
              << ": More than 1 product matches all criteria\nLooking for type: " << productType << "\n"
              << "Looking for module label: " << label << "\n"
              << "Looking for productInstanceName: " << instance << "\n"
              << (process.empty() ? "" : "Looking for process: ") << process << "\n"
              << "This can only occur with get function calls using a Handle<View> argument.\n"
              << "Try a get not using a View or change the instance name of one of the products";
    throw exception;
  }

  namespace {
    void failedToRegisterConsumes(KindOfType kindOfType,
                                  TypeID const& productType,
                                  std::string const& moduleLabel,
                                  std::string const& productInstanceName,
                                  std::string const& processName) {
      cms::Exception exception("GetByLabelWithoutRegistration");
      exception << "::getByLabel without corresponding call to consumes or mayConsumes for this module.\n"
                << (kindOfType == PRODUCT_TYPE ? "  type: " : " type: edm::View<") << productType
                << (kindOfType == PRODUCT_TYPE ? "\n  module label: " : ">\n  module label: ") << moduleLabel
                << "\n  product instance name: '" << productInstanceName << "'\n  process name: '" << processName
                << "'\n";
      throw exception;
    }
  }  // namespace

  // Value 0 means unset and is used in Principal constructor. First call to fillPrincipal() will get value 1.
  CMS_THREAD_SAFE static std::array<std::atomic<Principal::CacheIdentifier_t>, edm::NumBranchTypes> s_nextIdentifiers{
      {1, 1, 1, 1}};
  static inline Principal::CacheIdentifier_t nextIdentifier(edm::BranchType bt) {
    return s_nextIdentifiers[bt].fetch_add(1, std::memory_order_acq_rel);
  }

  Principal::Principal(std::shared_ptr<ProductRegistry const> reg,
                       std::vector<std::shared_ptr<ProductResolverBase>>&& resolvers,
                       ProcessConfiguration const& pc,
                       BranchType bt,
                       HistoryAppender* historyAppender)
      : EDProductGetter(),
        processHistoryPtr_(),
        processHistoryID_(),
        processHistoryIDBeforeConfig_(),
        processConfiguration_(&pc),
        productResolvers_(resolvers.begin(), resolvers.end()),
        preg_(reg),
        productLookup_(reg->productLookup(bt)),
        lookupProcessOrder_(productLookup_->lookupProcessNames().size(), 0),
        reader_(),
        branchType_(bt),
        historyAppender_(historyAppender),
        cacheIdentifier_(0) {}
  Principal::~Principal() {}

  // Number of products in the Principal.
  // For products in an input file and not yet read in due to delayed read,
  // this routine assumes a real product is there.
  size_t Principal::size() const {
    size_t size = 0U;
    for (auto const& prod : *this) {
      if (prod->singleProduct() &&  // Not a NoProcessProductResolver
          !prod->productUnavailable() && !prod->unscheduledWasNotRun() && !prod->productDescription().dropped()) {
        ++size;
      }
    }
    return size;
  }

  void Principal::possiblyUpdateAfterAddition(std::shared_ptr<ProductRegistry const> iProd) {
    if (iProd.get() != preg_.get() || iProd->cacheIdentifier() != preg_->cacheIdentifier()) {
      preg_ = iProd;
      adjustIndexesAfterProductRegistryAddition();
    }
  }

  void Principal::addDroppedProduct(ProductDescription const& bd) {
    addProductOrThrow(std::make_unique<DroppedDataProductResolver>(std::make_shared<ProductDescription const>(bd)));
  }

  // "Zero" the principal so it can be reused for another Event.
  void Principal::clearPrincipal() {
    //We do not clear the product history information
    // because it rarely changes and recalculating takes
    // time.
    reader_ = nullptr;
    for (auto& prod : *this) {
      prod->resetProductData();
    }
  }

  void Principal::deleteProduct(BranchID const& id) const {
    auto phb = getExistingProduct(id);
    assert(nullptr != phb);
    phb->unsafe_deleteProduct();
  }

  void Principal::setupUnscheduled(UnscheduledConfigurator const& iConfigure) {
    applyToResolvers([&iConfigure](ProductResolverBase* iResolver) { iResolver->setupUnscheduled(iConfigure); });
  }

  void Principal::fillPrincipal(DelayedReader* reader) {
    //increment identifier here since clearPrincipal isn't called for Run/Lumi
    cacheIdentifier_ = nextIdentifier(branchType_);
    if (reader) {
      reader_ = reader;
    }
  }

  // Set the principal for the Event, Lumi, or Run.
  void Principal::fillPrincipal(ProcessHistoryID const& hist,
                                ProcessHistory const* processHistory,
                                DelayedReader* reader) {
    fillPrincipal(reader);

    if (historyAppender_ && productRegistry().anyProductProduced()) {
      if ((not processHistoryPtr_) || (processHistoryIDBeforeConfig_ != hist)) {
        processHistoryPtr_ = historyAppender_->appendToProcessHistory(hist, processHistory, *processConfiguration_);
        processHistoryID_ = processHistoryPtr_->id();
        processHistoryIDBeforeConfig_ = hist;
      }
    } else {
      std::shared_ptr<ProcessHistory const> inputProcessHistory;
      if ((not processHistoryPtr_) || (processHistoryIDBeforeConfig_ != hist)) {
        if (hist.isValid()) {
          //does not own the pointer
          auto noDel = [](void const*) {};
          inputProcessHistory = std::shared_ptr<ProcessHistory const>(processHistory, noDel);
          if (inputProcessHistory.get() == nullptr) {
            throw Exception(errors::LogicError) << "Principal::fillPrincipal\n"
                                                << "Input ProcessHistory not found in registry\n"
                                                << "Contact a Framework developer\n";
          }
        } else {
          //Since this is static we don't want it deleted
          inputProcessHistory = std::shared_ptr<ProcessHistory const>(&s_emptyProcessHistory, [](void const*) {});
          //no need to do any ordering since it is empty
          orderProcessHistoryID_ = hist;
        }
        processHistoryID_ = hist;
        processHistoryPtr_ = inputProcessHistory;
        processHistoryIDBeforeConfig_ = hist;
      }
    }

    if (orderProcessHistoryID_ != processHistoryID_) {
      std::vector<std::string> const& lookupProcessNames = productLookup_->lookupProcessNames();
      lookupProcessOrder_.assign(lookupProcessNames.size(), 0);
      unsigned int k = 0;

      // We loop over processes in reverse order of the ProcessHistory.
      // If any entries in the product lookup tables are associated with
      // the process we add it to the vector of processes in the order
      // the lookup should be performed. There is one exception though,
      // We start with the current process even if it is not in the ProcessHistory.
      // The current process might be needed but not be in the process
      // history if all the products produced in the current process are
      // transient.
      {
        auto nameIterCurrentProcess =
            std::find(lookupProcessNames.begin(), lookupProcessNames.end(), processConfiguration_->processName());
        if (nameIterCurrentProcess != lookupProcessNames.end()) {
          lookupProcessOrder_.at(k) = nameIterCurrentProcess - lookupProcessNames.begin();
          ++k;
        }
      }

      // We just looked for the current process so skip it if
      // it is in the ProcessHistory.
      auto iter = processHistoryPtr_->rbegin();
      if (iter->processName() == processConfiguration_->processName()) {
        ++iter;
      }

      for (auto iEnd = processHistoryPtr_->rend(); iter != iEnd; ++iter) {
        auto nameIter = std::find(lookupProcessNames.begin(), lookupProcessNames.end(), iter->processName());
        if (nameIter == lookupProcessNames.end()) {
          continue;
        }
        lookupProcessOrder_.at(k) = nameIter - lookupProcessNames.begin();
        ++k;
      }
      orderProcessHistoryID_ = processHistoryID_;
    }
  }

  // Set the principal for the ProcessBlock
  void Principal::fillPrincipal(std::string const& processNameOfBlock, DelayedReader* reader) {
    fillPrincipal(reader);

    std::vector<std::string> const& lookupProcessNames = productLookup_->lookupProcessNames();
    lookupProcessOrder_.assign(lookupProcessNames.size(), 0);
    if (!lookupProcessOrder_.empty()) {
      auto iter = std::find(lookupProcessNames.begin(), lookupProcessNames.end(), processNameOfBlock);
      if (iter != lookupProcessNames.end()) {
        lookupProcessOrder_[0] = iter - lookupProcessNames.begin();
      }
    }
  }

  ProductResolverBase* Principal::getExistingProduct(BranchID const& branchID) {
    return const_cast<ProductResolverBase*>(const_cast<const Principal*>(this)->getExistingProduct(branchID));
  }

  ProductResolverBase const* Principal::getExistingProduct(BranchID const& branchID) const {
    ProductResolverIndex index = preg_->indexFrom(branchID);
    assert(index != ProductResolverIndexInvalid);
    return productResolvers_.at(index).get();
  }

  ProductResolverBase const* Principal::getExistingProduct(ProductResolverBase const& productResolver) const {
    auto phb = getExistingProduct(productResolver.productDescription().branchID());
    if (nullptr != phb && BranchKey(productResolver.productDescription()) != BranchKey(phb->productDescription())) {
      ProductDescription const& newProduct = phb->productDescription();
      ProductDescription const& existing = productResolver.productDescription();
      if (newProduct.branchName() != existing.branchName() && newProduct.branchID() == existing.branchID()) {
        throw cms::Exception("HashCollision")
            << "Principal::getExistingProduct\n"
            << " Branch " << newProduct.branchName() << " has same branch ID as branch " << existing.branchName()
            << "\n"
            << "Workaround: change process name or product instance name of " << newProduct.branchName() << "\n";
      } else {
        assert(nullptr == phb ||
               BranchKey(productResolver.productDescription()) == BranchKey(phb->productDescription()));
      }
    }
    return phb;
  }

  std::vector<ProductDescription const*> Principal::productDescriptions() const {
    std::vector<ProductDescription const*> retValue;
    for (auto const& p : productRegistry().productList()) {
      if (p.second.branchType() == branchType()) {
        retValue.push_back(&p.second);
      }
    }
    return retValue;
  }

  void Principal::addProduct_(std::unique_ptr<ProductResolverBase> productResolver) {
    ProductDescription const& bd = productResolver->productDescription();
    assert(!bd.className().empty());
    assert(!bd.friendlyClassName().empty());
    assert(!bd.moduleLabel().empty());
    assert(!bd.processName().empty());
    SharedProductPtr phb(productResolver.release());

    ProductResolverIndex index = preg_->indexFrom(bd.branchID());
    assert(index != ProductResolverIndexInvalid);
    productResolvers_[index] = phb;
  }

  void Principal::addProductOrThrow(std::unique_ptr<ProductResolverBase> productResolver) {
    ProductResolverBase const* phb = getExistingProduct(*productResolver);
    if (phb != nullptr) {
      ProductDescription const& bd = productResolver->productDescription();
      throw Exception(errors::InsertFailure, "AlreadyPresent")
          << "addProductOrThrow: Problem found while adding product, "
          << "product already exists for (" << bd.friendlyClassName() << "," << bd.moduleLabel() << ","
          << bd.productInstanceName() << "," << bd.processName() << ")\n";
    }
    addProduct_(std::move(productResolver));
  }

  Principal::ConstProductResolverPtr Principal::getProductResolver(BranchID const& bid) const {
    ProductResolverIndex index = preg_->indexFrom(bid);
    if (index == ProductResolverIndexInvalid) {
      return ConstProductResolverPtr();
    }
    return getProductResolverByIndex(index);
  }

  Principal::ConstProductResolverPtr Principal::getProductResolverByIndex(
      ProductResolverIndex const& index) const noexcept {
    assert(index < productResolvers_.size());
    ConstProductResolverPtr const phb = productResolvers_[index].get();
    return phb;
  }

  unsigned int Principal::processBlockIndex(std::string const&) const {
    throw Exception(errors::LogicError) << "Principal::processBlockIndex not implemented for this type of Principal";
  }

  BasicHandle Principal::getByLabel(KindOfType kindOfType,
                                    TypeID const& typeID,
                                    InputTag const& inputTag,
                                    EDConsumerBase const* consumer,
                                    SharedResourcesAcquirer* sra,
                                    ModuleCallingContext const* mcc) const {
    // Not implemented for ProcessBlocks, it might work though, not tested
    // The other getByLabel function is used for ProcessBlocks by TestProcessor
    assert(branchType_ != InProcess);

    ProductData const* result = findProductByLabel(kindOfType, typeID, inputTag, consumer, sra, mcc);
    if (result == nullptr) {
      return BasicHandle(makeHandleExceptionFactory([=, this]() -> std::shared_ptr<cms::Exception> {
        return makeNotFoundException(
            "getByLabel",
            kindOfType,
            typeID,
            inputTag.label(),
            inputTag.instance(),
            appendCurrentProcessIfAlias(inputTag.process(), processConfiguration_->processName()));
      }));
    }
    return BasicHandle(result->wrapper(), &(result->provenance()));
  }

  BasicHandle Principal::getByLabel(KindOfType kindOfType,
                                    TypeID const& typeID,
                                    std::string const& label,
                                    std::string const& instance,
                                    std::string const& process,
                                    EDConsumerBase const* consumer,
                                    SharedResourcesAcquirer* sra,
                                    ModuleCallingContext const* mcc) const {
    ProductData const* result = findProductByLabel(kindOfType, typeID, label, instance, process, consumer, sra, mcc);
    if (result == nullptr) {
      return BasicHandle(makeHandleExceptionFactory([=]() -> std::shared_ptr<cms::Exception> {
        return makeNotFoundException("getByLabel", kindOfType, typeID, label, instance, process);
      }));
    }
    return BasicHandle(result->wrapper(), &(result->provenance()));
  }

  BasicHandle Principal::getByToken(KindOfType,
                                    TypeID const&,
                                    ProductResolverIndex index,
                                    bool skipCurrentProcess,
                                    bool& ambiguous,
                                    SharedResourcesAcquirer* sra,
                                    ModuleCallingContext const* mcc) const {
    assert(index != ProductResolverIndexInvalid);
    auto& productResolver = productResolvers_[index];
    assert(nullptr != productResolver.get());
    auto resolution = productResolver->resolveProduct(*this, skipCurrentProcess, sra, mcc);
    if (resolution.isAmbiguous()) {
      ambiguous = true;
      //The caller is looking explicitly for this case
      // and uses the extra data at the caller to setup the exception
      return BasicHandle::makeInvalid();
    }
    auto productData = resolution.data();
    if (productData == nullptr) {
      //The caller is looking explicitly for this case
      // and uses the extra data at the caller to setup the exception
      return BasicHandle::makeInvalid();
    }
    return BasicHandle(productData->wrapper(), &(productData->provenance()));
  }

  void Principal::prefetchAsync(WaitingTaskHolder task,
                                ProductResolverIndex index,
                                bool skipCurrentProcess,
                                ServiceToken const& token,
                                ModuleCallingContext const* mcc) const {
    auto const& productResolver = productResolvers_.at(index);
    assert(nullptr != productResolver.get());
    productResolver->prefetchAsync(task, *this, skipCurrentProcess, token, nullptr, mcc);
  }

  ProductData const* Principal::findProductByLabel(KindOfType kindOfType,
                                                   TypeID const& typeID,
                                                   InputTag const& inputTag,
                                                   EDConsumerBase const* consumer,
                                                   SharedResourcesAcquirer* sra,
                                                   ModuleCallingContext const* mcc) const {
    bool skipCurrentProcess = inputTag.willSkipCurrentProcess();

    ProductResolverIndex index = inputTag.indexFor(typeID, branchType(), &productRegistry());

    if (index == ProductResolverIndexInvalid) {
      char const* processName = inputTag.process().c_str();
      if (skipCurrentProcess) {
        processName = "\0";
      } else if (inputTag.process() == InputTag::kCurrentProcess) {
        processName = processConfiguration_->processName().c_str();
      }

      index =
          productLookup().index(kindOfType, typeID, inputTag.label().c_str(), inputTag.instance().c_str(), processName);

      if (index == ProductResolverIndexAmbiguous) {
        throwAmbiguousException("findProductByLabel",
                                typeID,
                                inputTag.label(),
                                inputTag.instance(),
                                appendCurrentProcessIfAlias(inputTag.process(), processConfiguration_->processName()));
      } else if (index == ProductResolverIndexInvalid) {
        // can occur because of missing consumes if nothing else in the process consumes the product
        for (auto const& item : preg_->productList()) {
          auto const& bd = item.second;
          if (bd.present() and bd.unwrappedTypeID() == typeID and bd.moduleLabel() == inputTag.label() and
              bd.productInstanceName() == inputTag.instance()) {
            bool const inCurrentProcess = bd.processName() == processConfiguration_->processName();
            if (inputTag.process().empty() or bd.processName() == inputTag.process() or
                (skipCurrentProcess and not inCurrentProcess) or
                (inputTag.process() == InputTag::kCurrentProcess and inCurrentProcess)) {
              failedToRegisterConsumes(
                  kindOfType,
                  typeID,
                  inputTag.label(),
                  inputTag.instance(),
                  appendCurrentProcessIfAlias(inputTag.process(), processConfiguration_->processName()));
            }
          }
        }
        return nullptr;
      }
      inputTag.tryToCacheIndex(index, typeID, branchType(), &productRegistry());
    }
    if (UNLIKELY(consumer and (not consumer->registeredToConsume(index, skipCurrentProcess, branchType())))) {
      failedToRegisterConsumes(kindOfType,
                               typeID,
                               inputTag.label(),
                               inputTag.instance(),
                               appendCurrentProcessIfAlias(inputTag.process(), processConfiguration_->processName()));
    }

    auto const& productResolver = productResolvers_[index];

    auto resolution = productResolver->resolveProduct(*this, skipCurrentProcess, sra, mcc);
    if (resolution.isAmbiguous()) {
      throwAmbiguousException("findProductByLabel",
                              typeID,
                              inputTag.label(),
                              inputTag.instance(),
                              appendCurrentProcessIfAlias(inputTag.process(), processConfiguration_->processName()));
    }
    return resolution.data();
  }

  ProductData const* Principal::findProductByLabel(KindOfType kindOfType,
                                                   TypeID const& typeID,
                                                   std::string const& label,
                                                   std::string const& instance,
                                                   std::string const& process,
                                                   EDConsumerBase const* consumer,
                                                   SharedResourcesAcquirer* sra,
                                                   ModuleCallingContext const* mcc) const {
    ProductResolverIndex index =
        productLookup().index(kindOfType, typeID, label.c_str(), instance.c_str(), process.c_str());

    if (index == ProductResolverIndexAmbiguous) {
      throwAmbiguousException("findProductByLabel", typeID, label, instance, process);
    } else if (index == ProductResolverIndexInvalid) {
      // can occur because of missing consumes if nothing else in the process consumes the product
      for (auto const& item : preg_->productList()) {
        auto const& bd = item.second;
        if (bd.present() and bd.unwrappedTypeID() == typeID and bd.moduleLabel() == label and
            bd.productInstanceName() == instance) {
          if (process.empty() or bd.processName() == process) {
            failedToRegisterConsumes(kindOfType, typeID, label, instance, process);
          }
        }
      }
      return nullptr;
    }

    if (UNLIKELY(consumer and (not consumer->registeredToConsume(index, false, branchType())))) {
      failedToRegisterConsumes(kindOfType, typeID, label, instance, process);
    }

    auto const& productResolver = productResolvers_[index];

    auto resolution = productResolver->resolveProduct(*this, false, sra, mcc);
    if (resolution.isAmbiguous()) {
      throwAmbiguousException("findProductByLabel", typeID, label, instance, process);
    }
    return resolution.data();
  }

  ProductData const* Principal::findProductByTag(TypeID const& typeID,
                                                 InputTag const& tag,
                                                 ModuleCallingContext const* mcc) const {
    // Not implemented for ProcessBlocks
    assert(branchType_ != InProcess);

    ProductData const* productData = findProductByLabel(PRODUCT_TYPE, typeID, tag, nullptr, nullptr, mcc);
    return productData;
  }

  Provenance const& Principal::getProvenance(BranchID const& bid) const {
    ConstProductResolverPtr const phb = getProductResolver(bid);
    if (phb == nullptr) {
      throwProductNotFoundException("getProvenance", errors::ProductNotFound, bid);
    }

    if (phb->unscheduledWasNotRun()) {
      throw edm::Exception(errors::UnimplementedFeature)
          << "Requesting provenance from unrun EDProducer. The requested branch ID was: " << bid;
    }
    return *phb->provenance();
  }

  StableProvenance const& Principal::getStableProvenance(BranchID const& bid) const {
    ConstProductResolverPtr const phb = getProductResolver(bid);
    if (phb == nullptr) {
      throwProductNotFoundException("getStableProvenance", errors::ProductNotFound, bid);
    }
    //NOTE: in all implementations, this never returns a nullptr
    return *phb->stableProvenance();
  }

  // This one is mostly for test printout purposes
  // No attempt to trigger on demand execution
  // Skips provenance when the EDProduct is not there
  void Principal::getAllProvenance(std::vector<Provenance const*>& provenances) const {
    provenances.clear();
    for (auto const& productResolver : *this) {
      if (productResolver->singleProduct() && productResolver->provenanceAvailable() &&
          !productResolver->productDescription().isAnyAlias()) {
        // We do not attempt to get the event/lumi/run status from the provenance,
        // because the per event provenance may have been dropped.
        if (productResolver->provenance()->productDescription().present()) {
          provenances.push_back(productResolver->provenance());
        }
      }
    }
  }

  // This one is also mostly for test printout purposes
  // No attempt to trigger on demand execution
  // Skips provenance for dropped branches.
  void Principal::getAllStableProvenance(std::vector<StableProvenance const*>& provenances) const {
    provenances.clear();
    for (auto const& productResolver : *this) {
      if (productResolver->singleProduct() && !productResolver->productDescription().isAnyAlias()) {
        if (productResolver->stableProvenance()->productDescription().present()) {
          provenances.push_back(productResolver->stableProvenance());
        }
      }
    }
  }

  void Principal::recombine(Principal& other, std::vector<BranchID> const& bids) {
    for (auto& prod : bids) {
      ProductResolverIndex index = preg_->indexFrom(prod);
      assert(index != ProductResolverIndexInvalid);
      ProductResolverIndex indexO = other.preg_->indexFrom(prod);
      assert(indexO != ProductResolverIndexInvalid);
      get_underlying_safe(productResolvers_[index]).swap(get_underlying_safe(other.productResolvers_[indexO]));
    }
    reader_->mergeReaders(other.reader());
  }

  WrapperBase const* Principal::getIt(ProductID const&) const {
    assert(false);
    return nullptr;
  }

  std::optional<std::tuple<WrapperBase const*, unsigned int>> Principal::getThinnedProduct(ProductID const&,
                                                                                           unsigned int) const {
    assert(false);
    return std::nullopt;
  }

  void Principal::getThinnedProducts(ProductID const&,
                                     std::vector<WrapperBase const*>&,
                                     std::vector<unsigned int>&) const {
    assert(false);
  }

  OptionalThinnedKey Principal::getThinnedKeyFrom(ProductID const&, unsigned int, ProductID const&) const {
    assert(false);
    return std::monostate{};
  }

  void Principal::put_(std::unique_ptr<WrapperBase> prod, ProductResolverBase const* phb) const {
    dynamic_cast<ProductPutterBase const*>(phb)->putProduct(std::move(prod));
  }

  void Principal::put_(ProductDescription const& bd, std::unique_ptr<WrapperBase> edp) const {
    if (edp.get() == nullptr) {
      throw edm::Exception(edm::errors::InsertFailure, "Null Pointer")
          << "put: Cannot put because unique_ptr to product is null."
          << "\n";
    }
    auto phb = getExistingProduct(bd.branchID());
    assert(phb);
    // ProductResolver assumes ownership
    put_(std::move(edp), phb);
  }

  void Principal::adjustIndexesAfterProductRegistryAddition() {
    if (preg_->getNextIndexValue(branchType_) != productResolvers_.size()) {
      bool changed = false;
      productResolvers_.resize(preg_->getNextIndexValue(branchType_));
      for (auto const& prod : preg_->productList()) {
        ProductDescription const& bd = prod.second;
        if (bd.branchType() == branchType_) {
          ProductResolverIndex index = preg_->indexFrom(bd.branchID());
          assert(index != ProductResolverIndexInvalid);
          if (!productResolvers_[index]) {
            // no product holder.  Must add one. The new entry must be an input product holder.
            assert(!bd.produced());
            assert(bd.dropped());
            //adding the resolver allows access to the provenance for the data product
            addDroppedProduct(bd);
            changed = true;
          }
        }
      }
      if (changed) {
        changedIndexes_();
      }
    }
    assert(preg_->getNextIndexValue(branchType_) == productResolvers_.size());
  }

  void Principal::readAllFromSourceAndMergeImmediately(MergeableRunProductMetadata const* mergeableRunProductMetadata) {
    if (not reader()) {
      return;
    }

    for (auto& prod : *this) {
      prod->retrieveAndMerge(*this, mergeableRunProductMetadata);
    }
  }
}  // namespace edm
