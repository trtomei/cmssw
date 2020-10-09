/** \class L1PFEnergySumFilter
 *
 * See header file for documentation
 *
 *
 *  \author Martin Grunewald
 *  \author Thiago Tomei
 */

#include "L1PFEnergySumFilter.h"
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

L1PFEnergySumFilter::L1PFEnergySumFilter(const edm::ParameterSet& iConfig)
    : HLTFilter(iConfig),
      l1PfSumTag_(iConfig.getParameter<edm::InputTag>("inputTag")),
      l1PfSumScalingTag_(iConfig.getParameter<edm::ESInputTag>("esScalingTag")),
      pfSumToken_(consumes<reco::METCollection>(l1PfSumTag_)),
      scalingToken_(esConsumes<L1TObjScalingConstants, L1TObjScalingRcd>(l1PfSumScalingTag_)) {
  min_Pt_ = iConfig.getParameter<double>("MinPt");
  typeOfSum_ = iConfig.getParameter<std::string>("TypeOfSum");

  if (typeOfSum_ == "MET")
    typeOfSumEnum_ = trigger::TriggerObjectType::TriggerL1PFMET;
  else if (typeOfSum_ == "ETT")
    typeOfSumEnum_ = trigger::TriggerObjectType::TriggerL1PFETT;
  else if (typeOfSum_ == "HT")
    typeOfSumEnum_ = trigger::TriggerObjectType::TriggerL1PFHT;
  else if (typeOfSum_ == "MHT")
    typeOfSumEnum_ = trigger::TriggerObjectType::TriggerL1PFMHT;
  else {
    cms::Exception e("ConfigurationError");
    e << "Wrong type of energy sum: " << typeOfSum_ << "\n"
      << "Valid choices are: MET, ETT, HT, MHT\n";
    throw e;
  }
}

L1PFEnergySumFilter::~L1PFEnergySumFilter() = default;

//
// member functions
//

void L1PFEnergySumFilter::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  makeHLTFilterDescription(desc);
  desc.add<double>("MinPt", -1.0);
  desc.add<edm::InputTag>("inputTag", edm::InputTag("L1PFEnergySums"));
  desc.add<edm::ESInputTag>("esScalingTag", edm::ESInputTag("L1TScalingESSource", "L1PFEnergySumScaling"));
  desc.add<std::string>("TypeOfSum",std::string("HT"));
  descriptions.add("L1PFEnergySumFilter", desc);
}

// ------------ method called to produce the data  ------------
bool L1PFEnergySumFilter::hltFilter(edm::Event& iEvent,
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
    filterproduct.addCollectionTag(l1PfSumTag_);
  }

  // Specific filter code

  // get hold of products from Event
  Handle<METCollection> pfSums;
  iEvent.getByToken(pfSumToken_, pfSums);

  // get scaling constants
  // do we *need* to get these every event? can't we cache them somewhere?
  edm::ESHandle<L1TObjScalingConstants> scalingConstants_ = iSetup.getHandle(scalingToken_);

  // pfSum
  int npfsum(0);
  auto apfsums(pfSums->begin());
  auto opfsums(pfSums->end());
  METCollection::const_iterator ipfSum;
  for (ipfSum = apfsums; ipfSum != opfsums; ipfSum++) {
    double offlinePt = 0.0;
    // pt or sumEt?
    if (typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFMET or
        typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFMHT)
      offlinePt = this->PFEnergySumOfflineEt(ipfSum->pt(), *scalingConstants_);
    else if (typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFETT or
             typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFHT)
      offlinePt = this->PFEnergySumOfflineEt(ipfSum->sumEt(), *scalingConstants_);

    if (offlinePt >= min_Pt_) {
      npfsum++;
      METRef ref(METRef(pfSums, distance(apfsums, ipfSum)));
      filterproduct.addObject(typeOfSumEnum_, ref);
    }
  }

  // error case
  // filterproduct.addObject(0,Ref<vector<int> >());

  // final filter decision:
  const bool accept(npfsum > 0);

  // return with final filter decision
  return accept;
}

double L1PFEnergySumFilter::PFEnergySumOfflineEt(double Et, const L1TObjScalingConstants& scalingConstants) const {
  return (scalingConstants.m_constants.at(0).m_constant + Et * scalingConstants.m_constants.at(0).m_linear +
          Et * Et * scalingConstants.m_constants.at(0).m_quadratic);
}
