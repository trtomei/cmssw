#include "FastSimulation/TrackingRecHitProducer/interface/TrackingRecHitAlgorithm.h"

#include "FWCore/AbstractServices/interface/RandomNumberGenerator.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/TrackerRecHit2D/interface/FastSingleTrackerRecHit.h"

#include "SimDataFormats/TrackingHit/interface/PSimHit.h"

#include "FastSimulation/TrackingRecHitProducer/interface/TrackingRecHitProduct.h"

#include "FWCore/Utilities/interface/Exception.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

TrackingRecHitAlgorithm::TrackingRecHitAlgorithm(const std::string& name,
                                                 const edm::ParameterSet& config,
                                                 edm::ConsumesCollector& consumesCollector)
    : _name(name),
      _selectionString(config.getParameter<std::string>("select")),
      _trackerTopology(nullptr),
      _trackerGeometry(nullptr),
      _misalignedTrackerGeometry(nullptr),
      trackerTopologyESToken_(consumesCollector.esConsumes()),
      trackerGeometryESToken_(consumesCollector.esConsumes()),
      misalignedTrackerGeometryESToken_(consumesCollector.esConsumes(edm::ESInputTag("", "MisAligned"))),
      _randomEngine(nullptr) {}

const TrackerTopology& TrackingRecHitAlgorithm::getTrackerTopology() const {
  if (!_trackerTopology) {
    throw cms::Exception("TrackingRecHitAlgorithm ") << _name << ": TrackerTopology not defined";
  }
  return *_trackerTopology;
}

const TrackerGeometry& TrackingRecHitAlgorithm::getTrackerGeometry() const {
  if (!_trackerGeometry) {
    throw cms::Exception("TrackingRecHitAlgorithm ") << _name << ": TrackerGeometry not defined";
  }
  return *_trackerGeometry;
}

const TrackerGeometry& TrackingRecHitAlgorithm::getMisalignedGeometry() const {
  if (!_misalignedTrackerGeometry) {
    throw cms::Exception("TrackingRecHitAlgorithm ") << _name << ": MisalignedGeometry not defined";
  }
  return *_misalignedTrackerGeometry;
}

const RandomEngineAndDistribution& TrackingRecHitAlgorithm::getRandomEngine() const {
  if (!_randomEngine) {
    throw cms::Exception("TrackingRecHitAlgorithm ") << _name << ": RandomEngineAndDistribution not defined";
  }
  return *_randomEngine;
}

void TrackingRecHitAlgorithm::beginStream(const edm::StreamID& id) {
  _randomEngine = std::make_shared<RandomEngineAndDistribution>(id);
}

void TrackingRecHitAlgorithm::beginRun(edm::Run const& run,
                                       const edm::EventSetup& eventSetup,
                                       const SiPixelTemplateDBObject* pixelTemplateDBObjectPtr,
                                       const std::vector<SiPixelTemplateStore>& tempStoreRef) {
  // The default is to do nothing.
}

void TrackingRecHitAlgorithm::beginEvent(edm::Event& event, const edm::EventSetup& eventSetup) {
  _trackerTopology = &eventSetup.getData(trackerTopologyESToken_);
  _trackerGeometry = &eventSetup.getData(trackerGeometryESToken_);
  _misalignedTrackerGeometry = &eventSetup.getData(misalignedTrackerGeometryESToken_);
}

TrackingRecHitProductPtr TrackingRecHitAlgorithm::process(TrackingRecHitProductPtr product) const { return product; }

void TrackingRecHitAlgorithm::endEvent(edm::Event& event, const edm::EventSetup& eventSetup) {
  //set these to 0 -> ensures that beginEvent needs to be executed before accessing these pointers again
  _trackerGeometry = nullptr;
  _trackerTopology = nullptr;
  _misalignedTrackerGeometry = nullptr;
}

void TrackingRecHitAlgorithm::endStream() { _randomEngine.reset(); }

TrackingRecHitAlgorithm::~TrackingRecHitAlgorithm() {}
