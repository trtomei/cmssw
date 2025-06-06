#include "SimTracker/TrackTriggerAssociation/interface/StubAssociation.h"

#include <map>
#include <vector>
#include <utility>
#include <numeric>

namespace tt {

  StubAssociation::StubAssociation(const Config& iConfig, const Setup* setup)
      : setup_(setup),
        minLayersGood_(iConfig.minLayersGood_),
        minLayersGoodPS_(iConfig.minLayersGoodPS_),
        maxLayersBad_(iConfig.maxLayersBad_),
        maxLayersBadPS_(iConfig.maxLayersBadPS_) {}

  // insert a TPPtr and its associated collection of TTstubRefs into the underlayering maps
  void StubAssociation::insert(const TPPtr& tpPtr, const std::vector<TTStubRef>& ttSTubRefs) {
    mapTPPtrsTTStubRefs_.insert({tpPtr, ttSTubRefs});
    for (const TTStubRef& ttSTubRef : ttSTubRefs)
      mapTTStubRefsTPPtrs_[ttSTubRef].push_back(tpPtr);
  }

  // returns collection of TPPtrs associated to given TTstubRef
  std::vector<TPPtr> StubAssociation::findTrackingParticlePtrs(const TTStubRef& ttStubRef) const {
    const auto it = mapTTStubRefsTPPtrs_.find(ttStubRef);
    const std::vector<TPPtr> res = it != mapTTStubRefsTPPtrs_.end() ? it->second : emptyTPPtrs_;
    return res;
  }

  // returns collection of TTStubRefs associated to given TPPtr
  std::vector<TTStubRef> StubAssociation::findTTStubRefs(const TPPtr& tpPtr) const {
    const auto it = mapTPPtrsTTStubRefs_.find(tpPtr);
    return it != mapTPPtrsTTStubRefs_.end() ? it->second : emptyTTStubRefs_;
  }

  // Get all TPs that are matched to these stubs in at least 'tpMinLayers' layers and 'tpMinLayersPS' ps layers
  std::vector<TPPtr> StubAssociation::associate(const std::vector<TTStubRef>& ttStubRefs) const {
    // count associated layer for each TP
    std::map<TPPtr, std::pair<std::set<int>, std::set<int>>> m;
    for (const TTStubRef& ttStubRef : ttStubRefs) {
      for (const TPPtr& tpPtr : findTrackingParticlePtrs(ttStubRef)) {
        const int layerId = setup_->layerId(ttStubRef);
        m[tpPtr].first.insert(layerId);
        if (setup_->psModule(ttStubRef))
          m[tpPtr].second.insert(layerId);
      }
    }
    // count matched TPs
    auto acc = [this](int sum, const std::pair<TPPtr, std::pair<std::set<int>, std::set<int>>>& p) {
      return sum + (static_cast<int>(p.second.first.size()) < minLayersGood_ ||
                            static_cast<int>(p.second.second.size()) < minLayersGoodPS_
                        ? 0
                        : 1);
    };
    const int nTPs = std::accumulate(m.begin(), m.end(), 0, acc);
    std::vector<TPPtr> tpPtrs;
    tpPtrs.reserve(nTPs);
    // fill and return matched TPs
    for (const auto& p : m)
      if (static_cast<int>(p.second.first.size()) >= minLayersGood_ &&
          static_cast<int>(p.second.second.size()) >= minLayersGoodPS_)
        tpPtrs.push_back(p.first);
    return tpPtrs;
  }

  // Get all TPs that are matched to these stubs in at least 'tpMinLayers' layers and 'tpMinLayersPS' ps layers with not more then 'tpMaxBadStubs2S' not associated 2S stubs and not more then 'tpMaxBadStubsPS' associated PS stubs
  std::vector<TPPtr> StubAssociation::associateFinal(const std::vector<TTStubRef>& ttStubRefs) const {
    // Get all TPs that are matched to these stubs in at least 'tpMinLayers' layers and 'tpMinLayersPS' ps layers
    std::vector<TPPtr> tpPtrs = associate(ttStubRefs);
    // remove TPs with more then 'tpMaxBadStubs2S' not associated 2S stubs and more then 'tpMaxBadStubsPS' not associated PS stubs
    auto check = [this, &ttStubRefs](const TPPtr& tpPtr) {
      int bad2S(0);
      int badPS(0);
      for (const TTStubRef& ttStubRef : ttStubRefs) {
        const std::vector<TPPtr>& tpPtrs = findTrackingParticlePtrs(ttStubRef);
        if (std::find(tpPtrs.begin(), tpPtrs.end(), tpPtr) == tpPtrs.end())
          setup_->psModule(ttStubRef) ? badPS++ : bad2S++;
      }
      return (badPS > maxLayersBadPS_ || badPS + bad2S > maxLayersBad_);
    };
    tpPtrs.erase(std::remove_if(tpPtrs.begin(), tpPtrs.end(), check), tpPtrs.end());
    return tpPtrs;
  }

}  // namespace tt
