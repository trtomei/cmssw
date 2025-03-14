/**
 *  See header file for a description of this class.
 *  
 *  All the code is under revision
 *
 *
 *  \author A. Vitelli - INFN Torino, V.Palichik
 *  \author ported by: R. Bellan - INFN Torino
 */

#include "RecoMuon/MuonSeedGenerator/plugins/MuonSeedGenerator.h"
#include "RecoMuon/MuonSeedGenerator/interface/MuonSeedFinder.h"
#include "RecoMuon/MuonSeedGenerator/interface/MuonSeedOrcaPatternRecognition.h"
#include "RecoMuon/MuonSeedGenerator/interface/MuonSeedFinder.h"
#include "RecoMuon/MuonSeedGenerator/interface/MuonSeedSimpleCleaner.h"

// Data Formats
#include "DataFormats/TrajectorySeed/interface/TrajectorySeed.h"
#include "DataFormats/TrajectorySeed/interface/TrajectorySeedCollection.h"

#include "DataFormats/Common/interface/Handle.h"

#include "RecoMuon/TransientTrackingRecHit/interface/MuonTransientTrackingRecHit.h"
#include "RecoMuon/Records/interface/MuonRecoGeometryRecord.h"

// Geometry
#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "TrackingTools/DetLayers/interface/DetLayer.h"

#include "RecoMuon/MeasurementDet/interface/MuonDetLayerMeasurements.h"
#include "RecoMuon/DetLayers/interface/MuonDetLayerGeometry.h"
#include "RecoMuon/Records/interface/MuonRecoGeometryRecord.h"

// Framework
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

// C++
#include <vector>

using namespace std;

typedef MuonTransientTrackingRecHit::MuonRecHitPointer MuonRecHitPointer;
typedef MuonTransientTrackingRecHit::ConstMuonRecHitPointer ConstMuonRecHitPointer;
typedef MuonTransientTrackingRecHit::MuonRecHitContainer MuonRecHitContainer;

// Constructor
MuonSeedGenerator::MuonSeedGenerator(const edm::ParameterSet& pset)
    : theSeedFinder(new MuonSeedFinder(pset)),
      theSeedCleaner(new MuonSeedSimpleCleaner()),
      theBeamSpotTag(pset.getParameter<edm::InputTag>("beamSpotTag")) {
  produces<TrajectorySeedCollection>();

  edm::ConsumesCollector iC = consumesCollector();
  thePatternRecognition = new MuonSeedOrcaPatternRecognition(pset, iC);

  beamspotToken = consumes<reco::BeamSpot>(theBeamSpotTag);
  magFieldToken = esConsumes<MagneticField, IdealMagneticFieldRecord>();
}

// Destructor
MuonSeedGenerator::~MuonSeedGenerator() {
  delete thePatternRecognition;
  delete theSeedFinder;
  delete theSeedCleaner;
}

// reconstruct muon's seeds
void MuonSeedGenerator::produce(edm::Event& event, const edm::EventSetup& eSetup) {
  // create the pointer to the Seed container
  auto output = std::make_unique<TrajectorySeedCollection>();

  edm::ESHandle<MagneticField> field = eSetup.getHandle(magFieldToken);
  theSeedFinder->setBField(&*field);

  reco::BeamSpot beamSpot;
  edm::Handle<reco::BeamSpot> beamSpotHandle;
  event.getByToken(beamspotToken, beamSpotHandle);
  if (beamSpotHandle.isValid()) {
    beamSpot = *beamSpotHandle;

  } else {
    edm::LogInfo("MuonSeedGenerator") << "No beam spot available from EventSetup \n";
  }

  // make it a vector so we can subtract it from position vectors
  GlobalVector gv(beamSpot.x0(), beamSpot.y0(), beamSpot.z0());
  theSeedFinder->setBeamSpot(gv);

  std::vector<MuonRecHitContainer> patterns;
  thePatternRecognition->produce(event, eSetup, patterns);

  for (std::vector<MuonRecHitContainer>::const_iterator seedSegments = patterns.begin(); seedSegments != patterns.end();
       ++seedSegments) {
    theSeedFinder->seeds(*seedSegments, *output);
  }

  theSeedCleaner->clean(*output);

  event.put(std::move(output));
}

void MuonSeedGenerator::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setAllowAnything();
  desc.add<edm::InputTag>("beamSpotTag", edm::InputTag("offlineBeamSpot"));
  desc.add<bool>("scaleDT", true);
  desc.add<edm::InputTag>("CSCRecSegmentLabel", edm::InputTag("cscSegments"));
  desc.add<edm::InputTag>("DTRecSegmentLabel", edm::InputTag("dt4DSegments"));
  desc.add<edm::InputTag>("ME0RecSegmentLabel", edm::InputTag("me0Segments"));
  desc.add<bool>("EnableDTMeasurement", true);
  desc.add<bool>("EnableCSCMeasurement", true);
  desc.add<bool>("EnableME0Measurement", false);
  desc.add<std::vector<double>>("crackEtas", {0.2, 1.6, 1.7});
  desc.add<double>("crackWindow", 0.04);
  desc.add<double>("deltaPhiSearchWindow", 0.25);
  desc.add<double>("deltaEtaSearchWindow", 0.2);
  desc.add<double>("deltaEtaCrackSearchWindow", 0.25);
  descriptions.add("muonSeedGenerator", desc);
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(MuonSeedGenerator);
