#include "FWCore/Framework/interface/ProductProvenanceRetriever.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "FWCore/Utilities/interface/EDMException.h"

#include <cassert>
#include <limits>

/*
  ProductProvenanceRetriever
*/

namespace edm {
  void ProvenanceReaderBase::unsafe_fillProvenance(unsigned int transitionIndex) const {}

  ProductProvenanceRetriever::ProductProvenanceRetriever(unsigned int iTransitionIndex)
      : ProductProvenanceLookup(), nextRetriever_(), provenanceReader_(), transitionIndex_(iTransitionIndex) {}

  ProductProvenanceRetriever::ProductProvenanceRetriever(unsigned int iTransitionIndex,
                                                         edm::ProductRegistry const& iReg)
      : ProductProvenanceLookup(iReg), nextRetriever_(), provenanceReader_(), transitionIndex_(iTransitionIndex) {}

  ProductProvenanceRetriever::ProductProvenanceRetriever(unsigned int iTransitionIndex,
                                                         std::unique_ptr<ProvenanceReaderBase> reader)
      : ProductProvenanceLookup(),
        nextRetriever_(),
        provenanceReader_(reader.release()),
        transitionIndex_(iTransitionIndex) {
    assert(provenanceReader_);
  }

  std::unique_ptr<const std::set<ProductProvenance>> ProductProvenanceRetriever::readProvenance() const {
    std::unique_ptr<const std::set<ProductProvenance>> temp;
    if (provenanceReader_) {
      temp = std::make_unique<std::set<ProductProvenance> const>(provenanceReader_->readProvenance(transitionIndex_));
    }
    return temp;
  }

  void ProductProvenanceRetriever::readProvenanceAsync(
      WaitingTaskHolder task, ModuleCallingContext const* moduleCallingContext) const noexcept {
    if (provenanceReader_ and nullptr == readEntryInfoSet_.load()) {
      provenanceReader_->readProvenanceAsync(task, moduleCallingContext, transitionIndex_, readEntryInfoSet_);
    }
    if (nextRetriever_) {
      nextRetriever_->readProvenanceAsync(task, moduleCallingContext);
    }
  }

  void ProductProvenanceRetriever::deepCopy(ProductProvenanceRetriever const& iFrom) {
    if (iFrom.readEntryInfoSet_) {
      if (readEntryInfoSet_) {
        delete readEntryInfoSet_.exchange(nullptr);
      }
      readEntryInfoSet_ = new std::set<ProductProvenance>(*iFrom.readEntryInfoSet_);
    } else {
      if (readEntryInfoSet_) {
        delete readEntryInfoSet_.load();
        readEntryInfoSet_ = nullptr;
      }
    }
    assert(iFrom.entryInfoSet_.empty());
    provenanceReader_ = iFrom.provenanceReader_;

    if (iFrom.nextRetriever_) {
      if (not nextRetriever_) {
        nextRetriever_ = std::make_shared<ProductProvenanceRetriever>(transitionIndex_);
      }
      nextRetriever_->deepCopy(*(iFrom.nextRetriever_));
    }
  }

  void ProductProvenanceRetriever::reset() {
    delete readEntryInfoSet_.load();
    readEntryInfoSet_ = nullptr;
    for (auto& e : entryInfoSet_) {
      e.resetParentage();
    }
    if (nextRetriever_) {
      nextRetriever_->reset();
    }
    parentProcessRetriever_ = nullptr;
  }

  void ProductProvenanceRetriever::mergeProvenanceRetrievers(std::shared_ptr<ProductProvenanceRetriever> other) {
    nextRetriever_ = other;
  }

  void ProductProvenanceRetriever::mergeParentProcessRetriever(ProductProvenanceRetriever const& provRetriever) {
    parentProcessRetriever_ = &provRetriever;
  }

  void ProductProvenanceRetriever::unsafe_fillProvenance() {
    provenanceReader_->unsafe_fillProvenance(transitionIndex_);
  }

  ProvenanceReaderBase::~ProvenanceReaderBase() {}
}  // namespace edm
