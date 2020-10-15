/** \class L1TkEmFilter
 *
 * See header file for documentation
 *
 *
 *  \author Martin Grunewald
 *
 */

#include "L1TkEmFilter.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"

#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/Common/interface/Ref.h"
#include "DataFormats/HLTReco/interface/TriggerFilterObjectWithRefs.h"
#include "DataFormats/HLTReco/interface/TriggerTypeDefs.h"

#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/EventSetupRecord.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

//
// constructors and destructor
//

L1TkEmFilter::L1TkEmFilter(const edm::ParameterSet& iConfig)
    : HLTFilter(iConfig),
      l1TkEmTag1_(iConfig.getParameter<edm::InputTag>("inputTag1")),
      l1TkEmTag2_(iConfig.getParameter<edm::InputTag>("inputTag2")),
      l1TkEmScalingTag_(iConfig.getParameter<edm::ESInputTag>("esScalingTag")),
      tkEmToken1_(consumes<TkEmCollection>(l1TkEmTag1_)),
      tkEmToken2_(consumes<TkEmCollection>(l1TkEmTag2_)),
      scalingToken_(esConsumes<L1TObjScalingConstants, L1TObjScalingRcd>(l1TkEmScalingTag_)) {
  min_Pt_ = iConfig.getParameter<double>("MinPt");
  min_N_ = iConfig.getParameter<int>("MinN");
  min_Eta_ = iConfig.getParameter<double>("MinEta");
  max_Eta_ = iConfig.getParameter<double>("MaxEta");
  etaBinsForIsolation_ = iConfig.getParameter<std::vector<double> >("EtaBinsForIsolation");
  trkIsolation_ = iConfig.getParameter<std::vector<double> >("TrkIsolation");
  quality1_ = iConfig.getParameter<int>("Quality1");
  quality2_ = iConfig.getParameter<int>("Quality2");
  qual1IsMask_ = iConfig.getParameter<bool>("Qual1IsMask");
  qual2IsMask_ = iConfig.getParameter<bool>("Qual2IsMask");
  applyQual1_ = iConfig.getParameter<bool>("ApplyQual1");
  applyQual2_ = iConfig.getParameter<bool>("ApplyQual2");

  if (etaBinsForIsolation_.size() != (trkIsolation_.size() + 1))
    throw cms::Exception("ConfigurationError")
        << "Vector of isolation values should have same size of vector of eta bins plus one.";
}

L1TkEmFilter::~L1TkEmFilter() = default;

//
// member functions
//

void L1TkEmFilter::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  makeHLTFilterDescription(desc);
  desc.add<double>("MinPt", -1.0);
  desc.add<double>("MinEta", -5.0);
  desc.add<double>("MaxEta", 5.0);
  desc.add<int>("MinN", 1);
  desc.add<edm::InputTag>("inputTag1", edm::InputTag("L1TkEms1"));
  desc.add<edm::InputTag>("inputTag2", edm::InputTag("L1TkEms2"));
  desc.add<edm::ESInputTag>("esScalingTag", edm::ESInputTag("L1TScalingESSource", "L1TkEmScaling"));
  desc.add<std::vector<double> >("EtaBinsForIsolation", {0.0, 1.479});
  desc.add<std::vector<double> >("TrkIsolation",
                                 {
                                     99999.9,
                                 });
  desc.add<int>("Quality1", 0);
  desc.add<int>("Quality2", 0);
  desc.add<bool>("Qual1IsMask", false);
  desc.add<bool>("Qual2IsMask", false);
  desc.add<bool>("ApplyQual1", false);
  desc.add<bool>("ApplyQual2", false);
  descriptions.add("L1TkEmFilter", desc);
}

// ------------ method called to produce the data  ------------
bool L1TkEmFilter::hltFilter(edm::Event& iEvent,
                             const edm::EventSetup& iSetup,
                             trigger::TriggerFilterObjectWithRefs& filterproduct) const {
  using namespace std;
  using namespace edm;
  using namespace reco;
  using namespace trigger;

  // All HLT filters must create and fill an HLT filter object,
  // recording any reconstructed physics objects satisfying (or not)
  // this HLT filter, and place it in the Event.

  // The filter object
  if (saveTags()) {
    filterproduct.addCollectionTag(l1TkEmTag1_);
    filterproduct.addCollectionTag(l1TkEmTag2_);
  }

  // Specific filter code

  // get hold of products from Event

  /// Barrel collection
  Handle<l1t::TkEmCollection> tkEms1;
  iEvent.getByToken(tkEmToken1_, tkEms1);

  /// Endcap collection
  Handle<l1t::TkEmCollection> tkEms2;
  iEvent.getByToken(tkEmToken2_, tkEms2);

  // get scaling constants
  // do we *need* to get these every event? can't we cache them somewhere?
  edm::ESHandle<L1TObjScalingConstants> scalingConstants_ = iSetup.getHandle(scalingToken_);

  int ntrkEm(0);
  // Loop over first collection
  auto atrkEms(tkEms1->begin());
  auto otrkEms(tkEms1->end());
  TkEmCollection::const_iterator itkEm;
  for (itkEm = atrkEms; itkEm != otrkEms; itkEm++) {
    double offlinePt = this->TkEmOfflineEt(itkEm->pt(), itkEm->eta(), *scalingConstants_);
    bool passQuality(false);
    bool passIsolation(false);

    if (applyQual1_) {
      if (qual1IsMask_)
        passQuality = (itkEm->EGRef()->hwQual() & quality1_);
      else
        passQuality = (itkEm->EGRef()->hwQual() == quality1_);
    } else
      passQuality = true;

    // There has to be a better way to do this.
    for (unsigned int etabin = 1; etabin != etaBinsForIsolation_.size(); ++etabin) {
      if (std::abs(itkEm->eta()) < etaBinsForIsolation_.at(etabin) and
          std::abs(itkEm->eta()) > etaBinsForIsolation_.at(etabin - 1) and
          itkEm->trkIsol() < trkIsolation_.at(etabin - 1))
        passIsolation = true;
    }

    if (offlinePt >= min_Pt_ && itkEm->eta() <= max_Eta_ && itkEm->eta() >= min_Eta_ && passQuality && passIsolation) {
      ntrkEm++;
      l1t::TkEmRef ref1(l1t::TkEmRef(tkEms1, distance(atrkEms, itkEm)));
      filterproduct.addObject(trigger::TriggerObjectType::TriggerL1TkEm, ref1);
    }
  }

  // Loop over second collection. Notice we don't reset ntrkEm
  atrkEms = tkEms2->begin();
  otrkEms = tkEms2->end();
  for (itkEm = atrkEms; itkEm != otrkEms; itkEm++) {
    double offlinePt = this->TkEmOfflineEt(itkEm->pt(), itkEm->eta(), *scalingConstants_);
    bool passQuality(false);
    bool passIsolation(false);

    if (applyQual2_) {
      if (qual2IsMask_)
        passQuality = (itkEm->EGRef()->hwQual() & quality2_);
      else
        passQuality = (itkEm->EGRef()->hwQual() == quality2_);
    } else
      passQuality = true;

    for (unsigned int etabin = 1; etabin != etaBinsForIsolation_.size(); ++etabin) {
      if (std::abs(itkEm->eta()) < etaBinsForIsolation_.at(etabin) and
          std::abs(itkEm->eta()) > etaBinsForIsolation_.at(etabin - 1) and
          itkEm->trkIsol() < trkIsolation_.at(etabin - 1))
        passIsolation = true;
    }

    if (offlinePt >= min_Pt_ && itkEm->eta() <= max_Eta_ && itkEm->eta() >= min_Eta_ && passQuality && passIsolation) {
      ntrkEm++;
      l1t::TkEmRef ref2(l1t::TkEmRef(tkEms2, distance(atrkEms, itkEm)));
      filterproduct.addObject(trigger::TriggerObjectType::TriggerL1TkEm, ref2);
    }
  }

  // return with final filter decision
  const bool accept(ntrkEm >= min_N_);
  return accept;
}

double L1TkEmFilter::TkEmOfflineEt(double Et, double Eta, const L1TObjScalingConstants& scalingConstants) const {
  if (std::abs(Eta) < 1.5)
    return (scalingConstants.m_constants.at(0).m_constant + Et * scalingConstants.m_constants.at(0).m_linear +
            Et * Et * scalingConstants.m_constants.at(0).m_quadratic);
  else
    return (scalingConstants.m_constants.at(1).m_constant + Et * scalingConstants.m_constants.at(1).m_linear +
            Et * Et * scalingConstants.m_constants.at(1).m_quadratic);
}
