// S. Zenz, 12 February 2015
//
// Splits a track collection into two, based on whether they propagate to the HGCal or not
// Tracks with bad pt resolution (suspected fakes) are dropped and not in either collection

#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "RecoParticleFlow/PFClusterProducer/interface/InitialClusteringStepBase.h"
#include "DataFormats/ParticleFlowReco/interface/PFRecHitFraction.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "RecoParticleFlow/PFClusterProducer/interface/PFCPositionCalculatorBase.h"
#include "RecoParticleFlow/PFTracking/interface/PFTrackAlgoTools.h"
#include "DataFormats/ForwardDetId/interface/ForwardSubdetector.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/ParticleFlowReco/interface/PFRecTrackFwd.h"
#include "DataFormats/ParticleFlowReco/interface/PFRecTrack.h"
#include "DataFormats/TrackReco/interface/Track.h"

// for track propagation through HGC
// N.B. we are only propogating to first layer, so check these later
#include "DataFormats/TrajectorySeed/interface/PropagationDirection.h"
#include "TrackingTools/TrajectoryState/interface/TrajectoryStateTransform.h"
#include "TrackingTools/MaterialEffects/interface/PropagatorWithMaterial.h"
#include "DataFormats/GeometrySurface/interface/BoundDisk.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "MagneticField/Engine/interface/MagneticField.h"
#include "Geometry/HGCalGeometry/interface/HGCalGeometry.h"

//geometry records
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"
#include "Geometry/Records/interface/IdealGeometryRecord.h"

#include <memory>
#include <unordered_map>

class HGCalTrackCollectionProducer : public edm::stream::EDProducer<> {
public:
  HGCalTrackCollectionProducer(const edm::ParameterSet&);
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void produce(edm::Event&, const edm::EventSetup&) override;
  void beginLuminosityBlock(const edm::LuminosityBlock&, const edm::EventSetup&) override;

  edm::EDGetTokenT<edm::View<reco::PFRecTrack>> src_;

  // variables needed for copied goodPtResolution function
  // need to go back and figure out sensible values

  const reco::TrackBase::TrackQuality trackQuality_;
  const std::vector<double> DPtovPtCut_;
  const std::vector<unsigned> NHitCut_;
  const bool useIterTracking_;
  //  const bool _useFirstLayerOnly; // always true now

  // variables needed for copied extrapolation
  const edm::ESGetToken<MagneticField, IdealMagneticFieldRecord> magneticFieldToken_;
  const edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> tkerGeomToken_;
  const MagneticField* bField_;
  const TrackerGeometry* tkGeom_;
  std::array<std::string, 1> hgc_names_;  // 3 --> 1; extrapolate to hgcee only
  std::array<edm::ESGetToken<HGCalGeometry, IdealGeometryRecord>, 1>
      hgcGeometryTokens_;                              // 3 --> 1; extrapolate to hgcee only
  std::array<const HGCalGeometry*, 1> hgcGeometries_;  // 3 --> 1; extrapolate to hgcee only
  std::array<std::vector<ReferenceCountingPointer<BoundDisk>>, 1> plusSurface_,
      minusSurface_;  // 3 --> 1; extrapolate to hgcee only
  std::unique_ptr<PropagatorWithMaterial> mat_prop_;

  float diskOuterRadius_;
  float diskInnerRadius_;
};

void HGCalTrackCollectionProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("src", {"pfTrack"});
  desc.add<std::string>("trackQuality", "highPurity");
  desc.add<bool>("useIterativeTracking", true);
  desc.add<std::vector<double>>("DPtOverPtCuts_byTrackAlgo", {10.0, 10.0, 10.0, 10.0, 10.0, 5.0});
  desc.add<std::vector<uint32_t>>("NHitCuts_byTrackAlgo", {3, 3, 3, 3, 3, 3});
  edm::ParameterSetDescription pset;
  pset.add<std::string>("HGC_ECAL", "HGCalEESensitive");
  desc.add<edm::ParameterSetDescription>("hgcalGeometryNames", pset);
  descriptions.add("hgcalTrackCollection", desc);
}

HGCalTrackCollectionProducer::HGCalTrackCollectionProducer(const edm::ParameterSet& iConfig)
    : src_(consumes<edm::View<reco::PFRecTrack>>(iConfig.getParameter<edm::InputTag>("src"))),
      trackQuality_(reco::TrackBase::qualityByName(iConfig.getParameter<std::string>("trackQuality"))),
      DPtovPtCut_(iConfig.getParameter<std::vector<double>>("DPtOverPtCuts_byTrackAlgo")),
      NHitCut_(iConfig.getParameter<std::vector<unsigned>>("NHitCuts_byTrackAlgo")),
      useIterTracking_(iConfig.getParameter<bool>("useIterativeTracking")),
      magneticFieldToken_(esConsumes<edm::Transition::BeginLuminosityBlock>()),
      tkerGeomToken_(esConsumes<edm::Transition::BeginLuminosityBlock>()) {
  LogDebug("HGCalTrackCollectionProducer")
      << " HGCalTrackCollectionProducer::HGCalTrackCollectionProducer " << std::endl;

  const edm::ParameterSet& geoconf = iConfig.getParameterSet("hgcalGeometryNames");
  hgc_names_[0] = geoconf.getParameter<std::string>("HGC_ECAL");
  for (unsigned i = 0; i < hgcGeometryTokens_.size(); i++) {
    hgcGeometryTokens_[i] = esConsumes<edm::Transition::BeginLuminosityBlock>(edm::ESInputTag("", hgc_names_[i]));
  }

  produces<reco::PFRecTrackCollection>("TracksInHGCal");
  produces<reco::PFRecTrackCollection>("TracksNotInHGCal");
}

// From https://github.com/cms-sw/cmssw/blob/CMSSW_6_2_X_SLHC/RecoParticleFlow/PFClusterProducer/src/HGCClusterizer.cc#L441-L447 and beyond
// TODO: we only need the front of the calorimeter, so modify this
void HGCalTrackCollectionProducer::beginLuminosityBlock(const edm::LuminosityBlock& lumi, const edm::EventSetup& es) {
  constexpr float m_pion = 0.1396;
  // get dependencies for setting up propagator
  bField_ = &es.getData(magneticFieldToken_);
  tkGeom_ = &es.getData(tkerGeomToken_);
  // get HGC geometries (assume that layers are ordered in Z!)
  for (unsigned i = 0; i < hgcGeometries_.size(); ++i) {
    hgcGeometries_[i] = &es.getData(hgcGeometryTokens_[i]);
  }

  // make propagator
  mat_prop_ = std::make_unique<PropagatorWithMaterial>(alongMomentum, m_pion, bField_);
  // setup HGC layers for track propagation
  Surface::RotationType rot;  //unit rotation matrix
  for (unsigned i = 0; i < hgcGeometries_.size(); ++i) {
    minusSurface_[i].clear();
    plusSurface_[i].clear();
    const HGCalDDDConstants& dddCons = hgcGeometries_[i]->topology().dddConstants();
    std::map<float, float> zrhoCoord;
    std::map<float, float> innerRadiusCoord;
    auto theTrForms = dddCons.getTrForms();
    const auto& firstLayerIt = theTrForms.back();
    float Z(std::abs(firstLayerIt.h3v.z()));
    // use hardcoded radii for now (FIX ME)
    diskInnerRadius_ = 31.5;
    diskOuterRadius_ = 161.0f;
    LogDebug("HGCalTrackCollectionProducer") << "O HAI I'm making a bound disk with Outer R=" << diskOuterRadius_
                                             << " Inner R=" << diskInnerRadius_ << " and Z=" << Z << std::endl;
    minusSurface_[i].push_back(ReferenceCountingPointer<BoundDisk>(new BoundDisk(
        Surface::PositionType(0, 0, -Z), rot, new SimpleDiskBounds(diskInnerRadius_, diskOuterRadius_, -0.001, 0.001))));
    plusSurface_[i].push_back(ReferenceCountingPointer<BoundDisk>(new BoundDisk(
        Surface::PositionType(0, 0, +Z), rot, new SimpleDiskBounds(diskInnerRadius_, diskOuterRadius_, -0.001, 0.001))));
  }
}

void HGCalTrackCollectionProducer::produce(edm::Event& evt, const edm::EventSetup& iSetup) {
  edm::Handle<edm::View<reco::PFRecTrack>> trackHandle;
  evt.getByToken(src_, trackHandle);
  const auto& tracks = *trackHandle;

  auto outputInHGCal = std::make_unique<reco::PFRecTrackCollection>();
  auto outputNotInHGCal = std::make_unique<reco::PFRecTrackCollection>();

  for (unsigned int i = 0; i < tracks.size(); i++) {
    const auto track = tracks.ptrAt(i);
    bool isGood =
        PFTrackAlgoTools::goodPtResolution(track->trackRef(), DPtovPtCut_, NHitCut_, useIterTracking_, trackQuality_);
    LogDebug("HGCalTrackCollectionProducer") << "HGCalTrackCollectionProducer Track number " << i
                                             << " has a goodPtResolution result of " << isGood << std::endl;
    if (!isGood)
      continue;
    bool found = false;
    const TrajectoryStateOnSurface myTSOS =
        trajectoryStateTransform::outerStateOnSurface(*(track->trackRef()), *tkGeom_, bField_);
    auto detbegin = myTSOS.globalPosition().z() > 0 ? plusSurface_.begin() : minusSurface_.begin();
    auto detend = myTSOS.globalPosition().z() > 0 ? plusSurface_.end() : minusSurface_.end();
    for (auto det = detbegin; det != detend; ++det) {
      LogDebug("HGCalTrackCollectionProducer") << "at HGC detector: " << std::distance(detbegin, det) << std::endl;
      unsigned layer_count = 1;
      for (const auto& layer : *det) {
        LogDebug("HGCalTrackCollectionProducer") << "  at DET layer: " << layer_count++ << std::endl;
        TrajectoryStateOnSurface piStateAtSurface = mat_prop_->propagate(myTSOS, *layer);
        if (piStateAtSurface.isValid()) {
          LogDebug("HGCalTrackCollectionProducer") << "Extrapolation is valid!" << std::endl;
          GlobalPoint pt = piStateAtSurface.globalPosition();
          if (pt.perp() < diskOuterRadius_) {
            if (pt.perp() > diskInnerRadius_) {
              LogDebug("HGCalTrackCollectionProducer")
                  << "(x,y,z,r)=(" << pt.x() << ", " << pt.y() << ", " << pt.z() << ", "
                  << sqrt(pt.x() * pt.x() + pt.y() * pt.y()) << ")" << std::endl;
              if (std::abs(track->trackRef()->eta()) < 1.47)
                LogDebug("HGCalTrackCollectionProducer") << " ETA IN BARREL REGION: " << track->trackRef()->eta()
                                                         << " (PT: " << track->trackRef()->pt() << ")" << std::endl;
              found = true;
            } else {
              LogDebug("HGCalTrackCollectionProducer")
                  << " but r=" << pt.perp() << " < diskInnerRadius=" << diskInnerRadius_ << " so skipping "
                  << std::endl;
            }
          } else {
            LogDebug("HGCalTrackCollectionProducer")
                << " but r=" << pt.perp() << " > diskOuterRadius=" << diskOuterRadius_ << " so skipping " << std::endl;
          }
        } else {
          LogDebug("HGCalTrackCollectionProducer") << "Extrapolation is NOT valid!" << std::endl;
        }
      }
    }
    if (found) {
      LogDebug("HGCalTrackCollectionProducer") << " Track going to outputInHGCal pt eta " << track->trackRef()->pt()
                                               << " " << track->trackRef()->eta() << std::endl;
      outputInHGCal->push_back(*track);
    } else {
      outputNotInHGCal->push_back(*track);
    }
  }  // track loop

  evt.put(std::move(outputInHGCal), "TracksInHGCal");
  evt.put(std::move(outputNotInHGCal), "TracksNotInHGCal");
}

DEFINE_FWK_MODULE(HGCalTrackCollectionProducer);
