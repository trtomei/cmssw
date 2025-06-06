#ifndef FWCore_Sources_DaqProvenanceHelper_h
#define FWCore_Sources_DaqProvenanceHelper_h

#include <map>
#include <string>
#include <vector>
#include "oneapi/tbb/concurrent_unordered_map.h"

#include "DataFormats/Provenance/interface/ProductDescription.h"
#include "DataFormats/Provenance/interface/ProductDependenciesFwd.h"
#include "DataFormats/Provenance/interface/ParentageID.h"
#include "DataFormats/Provenance/interface/ProcessConfiguration.h"
#include "DataFormats/Provenance/interface/ProcessHistoryID.h"
#include "DataFormats/Provenance/interface/ProductProvenance.h"
#include "DataFormats/Provenance/interface/BranchID.h"
#include "DataFormats/Provenance/interface/BranchIDList.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

namespace edm {
  class ProcessHistoryRegistry;

  namespace dqh {
    struct parentage_hash {
      std::size_t operator()(edm::ParentageID const& iKey) const { return iKey.smallHash(); }
    };
  }  // namespace dqh

  struct DaqProvenanceHelper {
    typedef std::map<ProcessHistoryID, ProcessHistoryID> ProcessHistoryIDMap;
    typedef oneapi::tbb::concurrent_unordered_map<ParentageID, ParentageID, dqh::parentage_hash> ParentageIDMap;
    explicit DaqProvenanceHelper(TypeID const& rawDataType);
    explicit DaqProvenanceHelper(TypeID const& rawDataType,
                                 std::string const& collectionName,
                                 std::string const& friendlyName,
                                 std::string const& sourceLabel);
    ProcessHistoryID daqInit(ProductRegistry& productRegistry, ProcessHistoryRegistry& processHistoryRegistry) const;
    void saveInfo(ProductDescription const& oldBD, ProductDescription const& newBD) {
      oldProcessName_ = oldBD.processName();
      oldBranchID_ = oldBD.branchID();
      newBranchID_ = newBD.branchID();
    }
    bool matchProcesses(ProcessConfiguration const& pc, ProcessHistory const& ph) const;
    void fixMetaData(ProcessConfigurationVector& pcv, std::vector<ProcessHistory>& phv);
    void fixMetaData(std::vector<BranchID>& branchIDs) const;
    void fixMetaData(BranchIDLists&) const;
    void fixMetaData(ProductDependencies& productDependencies) const;
    ProcessHistoryID const& mapProcessHistoryID(ProcessHistoryID const& phid);
    ParentageID const& mapParentageID(ParentageID const& phid) const;
    BranchID const& mapBranchID(BranchID const& branchID) const;

    ProductDescription const& productDescription() const { return constProductDescription_; }
    ProcessHistoryID const* oldProcessHistoryID() const { return oldProcessHistoryID_; }
    ProductProvenance const& dummyProvenance() const { return dummyProvenance_; }

    void setOldParentageIDToNew(ParentageID const& iOld, ParentageID const& iNew);

  private:
    ProductDescription const constProductDescription_;
    ProductProvenance dummyProvenance_;
    ParameterSet processParameterSet_;

    std::string oldProcessName_;
    BranchID oldBranchID_;
    BranchID newBranchID_;
    ProcessHistoryID const* oldProcessHistoryID_;
    ProcessHistoryIDMap phidMap_;
    ParentageIDMap parentageIDMap_;
  };
}  // namespace edm
#endif
