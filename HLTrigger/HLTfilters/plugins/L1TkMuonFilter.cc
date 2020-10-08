/** \class L1TkMuonFilter
 *
 * See header file for documentation
 *
 *
 *  \author Martin Grunewald
 *
 */

#include "L1TkMuonFilter.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"

#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/Common/interface/Ref.h"
#include "DataFormats/HLTReco/interface/TriggerFilterObjectWithRefs.h"
#include "DataFormats/HLTReco/interface/TriggerTypeDefs.h"

#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/EventSetupRecord.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

//
// constructors and destructor
//

L1TkMuonFilter::L1TkMuonFilter(const edm::ParameterSet& iConfig)
    : HLTFilter(iConfig),
      l1TkMuonTag_(iConfig.getParameter<edm::InputTag>("inputTag")),
      l1TkMuonScalingTag_(iConfig.getParameter<edm::ESInputTag>("esScalingTag")),
      tkMuonToken_(consumes<TkMuonCollection>(l1TkMuonTag_)),
      scalingToken_(esConsumes<L1TObjScalingConstants, L1TObjScalingRcd>(l1TkMuonScalingTag_)) {
  min_Pt_ = iConfig.getParameter<double>("MinPt");
  min_N_ = iConfig.getParameter<int>("MinN");
  min_Eta_ = iConfig.getParameter<double>("MinEta");
  max_Eta_ = iConfig.getParameter<double>("MaxEta");
}

L1TkMuonFilter::~L1TkMuonFilter() = default;

//
// member functions
//

void L1TkMuonFilter::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  makeHLTFilterDescription(desc);
  desc.add<double>("MinPt", -1.0);
  desc.add<double>("MinEta", -5.0);
  desc.add<double>("MaxEta", 5.0);
  desc.add<int>("MinN", 1);
  desc.add<edm::InputTag>("inputTag", edm::InputTag("L1TkMuons"));
  desc.add<edm::ESInputTag>("esScalingTag", edm::ESInputTag("L1TScalingESSource", "L1TkMuonScaling"));
  descriptions.add("L1TkMuonFilter", desc);
}

// ------------ method called to produce the data  ------------
bool L1TkMuonFilter::hltFilter(edm::Event& iEvent,
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
    filterproduct.addCollectionTag(l1TkMuonTag_);
  }

  // Specific filter code

  // get hold of products from Event
  Handle<l1t::TkMuonCollection> tkMuons;
  iEvent.getByToken(tkMuonToken_, tkMuons);

  // get scaling constants
  // do we *need* to get these every event? can't we cache them somewhere?
  edm::ESHandle<L1TObjScalingConstants> scalingConstants_ = iSetup.getHandle(scalingToken_);

  // std::cout << "Je suis muon" << std::endl;
  // std::cout << scalingConstants_->m_constants.at(0).m_constant << std::endl;
  // std::cout << scalingConstants_->m_constants.at(0).m_linear << std::endl;
  // std::cout << scalingConstants_->m_constants.at(0).m_quadratic << std::endl;
  // std::cout << scalingConstants_->m_constants.at(1).m_constant << std::endl;
  // std::cout << scalingConstants_->m_constants.at(1).m_linear << std::endl;
  // std::cout << scalingConstants_->m_constants.at(1).m_quadratic << std::endl;
  // std::cout << scalingConstants_->m_constants.at(2).m_constant << std::endl;
  // std::cout << scalingConstants_->m_constants.at(2).m_linear << std::endl;
  // std::cout << scalingConstants_->m_constants.at(2).m_quadratic << std::endl;

  // trkMuon
  int ntrkmuon(0);
  auto atrkmuons(tkMuons->begin());
  auto otrkmuons(tkMuons->end());
  TkMuonCollection::const_iterator itkMuon;
  for (itkMuon = atrkmuons; itkMuon != otrkmuons; itkMuon++) {
    double offlinePt = this->TkMuonOfflineEt(itkMuon->pt(), itkMuon->eta(), *scalingConstants_);
    if (offlinePt >= min_Pt_ && itkMuon->eta() <= max_Eta_ && itkMuon->eta() >= min_Eta_) {
      ntrkmuon++;
      l1t::TkMuonRef ref(l1t::TkMuonRef(tkMuons, distance(atrkmuons, itkMuon)));
      filterproduct.addObject(trigger::TriggerObjectType::TriggerL1tkMu, ref);
    }
  }

  // error case
  // filterproduct.addObject(0,Ref<vector<int> >());

  // final filter decision:
  const bool accept(ntrkmuon >= min_N_);

  // return with final filter decision
  return accept;
}

double L1TkMuonFilter::TkMuonOfflineEt(double Et, double Eta, const L1TObjScalingConstants& scalingConstants) const {
  if (std::abs(Eta) < 0.9)
    return (scalingConstants.m_constants.at(0).m_constant + Et * scalingConstants.m_constants.at(0).m_linear +
            Et * Et * scalingConstants.m_constants.at(0).m_quadratic);
  else if (std::abs(Eta) < 1.2)
    return (scalingConstants.m_constants.at(1).m_constant + Et * scalingConstants.m_constants.at(1).m_linear +
            Et * Et * scalingConstants.m_constants.at(1).m_quadratic);
  else
    return (scalingConstants.m_constants.at(2).m_constant + Et * scalingConstants.m_constants.at(2).m_linear +
            Et * Et * scalingConstants.m_constants.at(2).m_quadratic);
}
