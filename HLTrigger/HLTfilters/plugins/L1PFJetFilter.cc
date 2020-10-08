/** \class L1PFJetFilter
 *
 * See header file for documentation
 *
 *
 *  \author Martin Grunewald
 *
 */

#include "L1PFJetFilter.h"
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

L1PFJetFilter::L1PFJetFilter(const edm::ParameterSet& iConfig)
    : HLTFilter(iConfig),
      l1PFJetTag_(iConfig.getParameter<edm::InputTag>("inputTag")),
      l1PFJetScalingTag_(iConfig.getParameter<edm::ESInputTag>("esScalingTag")),
      pfJetToken_(consumes<l1t::PFJetCollection>(l1PFJetTag_)),
      scalingToken_(esConsumes<L1TObjScalingConstants, L1TObjScalingRcd>(l1PFJetScalingTag_)) {
  min_Pt_ = iConfig.getParameter<double>("MinPt");
  min_N_ = iConfig.getParameter<int>("MinN");
  min_Eta_ = iConfig.getParameter<double>("MinEta");
  max_Eta_ = iConfig.getParameter<double>("MaxEta");
}

L1PFJetFilter::~L1PFJetFilter() = default;

//
// member functions
//

void L1PFJetFilter::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  makeHLTFilterDescription(desc);
  desc.add<double>("MinPt", -1.0);
  desc.add<double>("MinEta", -5.0);
  desc.add<double>("MaxEta", 5.0);
  desc.add<int>("MinN", 1);
  desc.add<edm::InputTag>("inputTag", edm::InputTag("ak4PFL1PuppiCorrected"));
  desc.add<edm::ESInputTag>("esScalingTag", edm::ESInputTag("L1TScalingESSource", "L1PFJetScaling"));
  descriptions.add("L1PFJetFilter", desc);
}

// ------------ method called to produce the data  ------------
bool L1PFJetFilter::hltFilter(edm::Event& iEvent,
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
    filterproduct.addCollectionTag(l1PFJetTag_);
  }

  // Specific filter code

  // get hold of products from Event
  Handle<l1t::PFJetCollection> pfJets;
  iEvent.getByToken(pfJetToken_, pfJets);

  // get scaling constants
  // do we *need* to get these every event? can't we cache them somewhere?
  edm::ESHandle<L1TObjScalingConstants> scalingConstants_ = iSetup.getHandle(scalingToken_);

  // std::cout << "Je suis PFjet" << std::endl;
  // std::cout << scalingConstants_->m_constants.at(0).m_constant << std::endl;
  // std::cout << scalingConstants_->m_constants.at(0).m_linear << std::endl;
  // std::cout << scalingConstants_->m_constants.at(0).m_quadratic << std::endl;
  // std::cout << scalingConstants_->m_constants.at(1).m_constant << std::endl;
  // std::cout << scalingConstants_->m_constants.at(1).m_linear << std::endl;
  // std::cout << scalingConstants_->m_constants.at(1).m_quadratic << std::endl;
  // std::cout << scalingConstants_->m_constants.at(2).m_constant << std::endl;
  // std::cout << scalingConstants_->m_constants.at(2).m_linear << std::endl;
  // std::cout << scalingConstants_->m_constants.at(2).m_quadratic << std::endl;

  int npfjet(0);
  auto apfjets(pfJets->begin());
  auto opfjets(pfJets->end());
  l1t::PFJetCollection::const_iterator ipfJet;
  for (ipfJet = apfjets; ipfJet != opfjets; ipfJet++) {
    double offlinePt = this->PFJetOfflineEt(ipfJet->pt(), ipfJet->eta(), *scalingConstants_);
    if (offlinePt >= min_Pt_ && ipfJet->eta() <= max_Eta_ && ipfJet->eta() >= min_Eta_) {
      npfjet++;
      l1t::PFJetRef ref(l1t::PFJetRef(pfJets, distance(apfjets, ipfJet)));
      filterproduct.addObject(trigger::TriggerObjectType::TriggerL1tkMu, ref);
    }
  }

  // error case
  // filterproduct.addObject(0,Ref<vector<int> >());

  // final filter decision:
  const bool accept(npfjet >= min_N_);

  // return with final filter decision
  return accept;
}

double L1PFJetFilter::PFJetOfflineEt(double Et, double Eta, const L1TObjScalingConstants& scalingConstants) const {
  if (std::abs(Eta) < 1.5)
    return (scalingConstants.m_constants.at(0).m_constant + Et * scalingConstants.m_constants.at(0).m_linear +
            Et * Et * scalingConstants.m_constants.at(0).m_quadratic);
  else if (std::abs(Eta) < 2.4)
    return (scalingConstants.m_constants.at(1).m_constant + Et * scalingConstants.m_constants.at(1).m_linear +
            Et * Et * scalingConstants.m_constants.at(1).m_quadratic);
  else
    return (scalingConstants.m_constants.at(2).m_constant + Et * scalingConstants.m_constants.at(2).m_linear +
            Et * Et * scalingConstants.m_constants.at(2).m_quadratic);
}
