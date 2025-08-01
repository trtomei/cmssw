#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "Geometry/Records/interface/IdealGeometryRecord.h"
#include "Geometry/HGCalGeometry/interface/HGCalGeometry.h"
#include "DataFormats/ForwardDetId/interface/HFNoseDetId.h"
#include "DataFormats/ForwardDetId/interface/HGCScintillatorDetId.h"
#include "DataFormats/ForwardDetId/interface/HGCSiliconDetId.h"
#include <iostream>
#include <sstream>

class HGCalGeometryDump : public edm::one::EDAnalyzer<> {
public:
  explicit HGCalGeometryDump(const edm::ParameterSet&);
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

  void beginJob() override {}
  void analyze(edm::Event const& iEvent, edm::EventSetup const&) override;
  void endJob() override {}

private:
  const std::vector<std::string> names_;
  const std::vector<int> types_;
  std::vector<edm::ESGetToken<HGCalGeometry, IdealGeometryRecord>> geomTokens_;
};

HGCalGeometryDump::HGCalGeometryDump(const edm::ParameterSet& iC)
    : names_(iC.getParameter<std::vector<std::string>>("detectorNames")),
      types_(iC.getParameter<std::vector<int>>("detectorTypes")) {
  for (unsigned int k = 0; k < names_.size(); ++k) {
    edm::LogVerbatim("HGCalGeomX") << "Study detector [" << k << "] " << names_[k] << " of type " << types_[k]
                                   << std::endl;
    geomTokens_.emplace_back(esConsumes<HGCalGeometry, IdealGeometryRecord>(edm::ESInputTag{"", names_[k]}));
  }
}

void HGCalGeometryDump::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  std::vector<std::string> names = {"HGCalEESensitive", "HGCalHESiliconSensitive", "HGCalHEScintillatorSensitive"};
  desc.add<std::vector<std::string>>("detectorNames", names);
  std::vector<int> types = {0, 0, 1};
  desc.add<std::vector<int>>("detectorTypes", types);
  descriptions.add("hgcalGeometryDump", desc);
}

void HGCalGeometryDump::analyze(const edm::Event& /*iEvent*/, const edm::EventSetup& iSetup) {
  for (unsigned int k = 0; k < names_.size(); ++k) {
    const auto& geomR = iSetup.getData(geomTokens_[k]);
    const HGCalGeometry* geom = &geomR;

    std::vector<double> thick = geom->topology().dddConstants().cellThickness();
    std::ostringstream st1;
    st1 << "Geometry has " << thick.size() << " wafers of thickness (micron)";
    for (unsigned int k = 0; k < thick.size(); ++k)
      st1 << " : " << thick[k];
    edm::LogVerbatim("HGCalGeomX") << st1.str() << std::endl;

    const std::vector<DetId>& ids = geom->getValidDetIds();
    edm::LogVerbatim("HGCalGeomX") << ids.size() << " valid Ids for detector " << names_[k];
    int nall(0);
    for (auto id : ids) {
      ++nall;
      auto cell = geom->getGeometry(id);
      if (types_[k] == 0) {
        HGCSiliconDetId hid(id);
        edm::LogVerbatim("HGCalGeomX") << "[" << nall << "] " << hid << " Reference " << std::setprecision(4)
                                       << cell->getPosition() << " Back " << cell->getBackPoint() << " [r,eta,phi] ("
                                       << cell->rhoPos() << ", " << cell->etaPos() << ":" << cell->etaSpan() << ", "
                                       << cell->phiPos() << ":" << cell->phiSpan() << ")";
      } else if (types_[k] == 1) {
        HGCScintillatorDetId hid(id);
        edm::LogVerbatim("HGCalGeomX") << "[" << nall << "] " << hid << " Reference " << std::setprecision(4)
                                       << cell->getPosition() << " Back " << cell->getBackPoint() << " [r,eta,phi] ("
                                       << cell->rhoPos() << ", " << cell->etaPos() << ":" << cell->etaSpan() << ", "
                                       << cell->phiPos() << ":" << cell->phiSpan() << ")";
      } else {
        HFNoseDetId hid(id);
        edm::LogVerbatim("HGCalGeomX") << "[" << nall << "] " << hid << " Reference " << std::setprecision(4)
                                       << cell->getPosition() << " Back " << cell->getBackPoint() << " [r,eta,phi] ("
                                       << cell->rhoPos() << ", " << cell->etaPos() << ":" << cell->etaSpan() << ", "
                                       << cell->phiPos() << ":" << cell->phiSpan() << ")";
      }
    }
    edm::LogVerbatim("HGCalGeomX") << "\n\nDumps " << nall << " cells of the detector\n";
  }
}

DEFINE_FWK_MODULE(HGCalGeometryDump);
