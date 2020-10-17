#ifndef HLTrigger_HLTfilters_L1EnergySumFilterT_h
#define HLTrigger_HLTfilters_L1EnergySumFilterT_h

#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/EventSetupRecord.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "CondFormats/HLTObjects/interface/L1TObjScalingConstants.h"
#include "CondFormats/DataRecord/interface/L1TObjScalingRcd.h"

#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Common/interface/Ref.h"
#include "DataFormats/HLTReco/interface/TriggerFilterObjectWithRefs.h"
#include "DataFormats/HLTReco/interface/TriggerTypeDefs.h"

#include "HLTrigger/HLTcore/interface/HLTFilter.h"

template <typename T>
class L1EnergySumFilterT : public HLTFilter {
public:
  explicit L1EnergySumFilterT(const edm::ParameterSet&);
  ~L1EnergySumFilterT() override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  bool hltFilter(edm::Event&, const edm::EventSetup&, trigger::TriggerFilterObjectWithRefs& filterproduct) const override;

private:
  edm::InputTag const l1SumTag_;                     // input tag for L1 Energy Sum product
  edm::ESInputTag const l1SumScalingTag_;            // input tag for L1 Energy Sum scaling
  edm::EDGetTokenT<std::vector<T>> const sumToken_;  // token identifying product containing L1 EnergySums
  edm::ESGetToken<L1TObjScalingConstants, L1TObjScalingRcd> const scalingToken_;  // token identifying the L1T scaling constants

  double const min_Pt_;
  std::string const typeOfSum_;

  trigger::TriggerObjectType typeOfSumEnum_;

  double EnergySumOfflineEt(double const Et, L1TObjScalingConstants const& scalingConstants) const;
};

template <typename T>
L1EnergySumFilterT<T>::L1EnergySumFilterT(const edm::ParameterSet& iConfig)
    : HLTFilter(iConfig),
      l1SumTag_(iConfig.getParameter<edm::InputTag>("inputTag")),
      l1SumScalingTag_(iConfig.getParameter<edm::ESInputTag>("esScalingTag")),
      sumToken_(consumes<std::vector<T>>(l1SumTag_)),
      scalingToken_(esConsumes<L1TObjScalingConstants, L1TObjScalingRcd>(l1SumScalingTag_)),
      min_Pt_(iConfig.getParameter<double>("MinPt")),
      typeOfSum_(iConfig.getParameter<std::string>("TypeOfSum")) {

  if (typeOfSum_ == "MET")
    typeOfSumEnum_ = trigger::TriggerObjectType::TriggerL1PFMET;
  else if (typeOfSum_ == "ETT")
    typeOfSumEnum_ = trigger::TriggerObjectType::TriggerL1PFETT;
  else if (typeOfSum_ == "HT")
    typeOfSumEnum_ = trigger::TriggerObjectType::TriggerL1PFHT;
  else if (typeOfSum_ == "MHT")
    typeOfSumEnum_ = trigger::TriggerObjectType::TriggerL1PFMHT;
  else {
    throw cms::Exception("ConfigurationError") << "Wrong type of energy sum: " << typeOfSum_ << "\n"
      << "Valid choices are: MET, ETT, HT, MHT\n";
  }
}

template <typename T>
L1EnergySumFilterT<T>::~L1EnergySumFilterT() = default;

template <typename T>
void L1EnergySumFilterT<T>::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  makeHLTFilterDescription(desc);
  desc.add<double>("MinPt", -1.0);
  desc.add<edm::InputTag>("inputTag", edm::InputTag("L1PFEnergySums"));
  desc.add<edm::ESInputTag>("esScalingTag", edm::ESInputTag("L1TScalingESSource", "L1PFEnergySumScaling"));
  desc.add<std::string>("TypeOfSum", std::string("HT"));
  descriptions.add(defaultModuleLabel<L1EnergySumFilterT<T>>(), desc);
}

template <typename T>
bool L1EnergySumFilterT<T>::hltFilter(edm::Event& iEvent, const edm::EventSetup& iSetup, trigger::TriggerFilterObjectWithRefs& filterproduct) const {
  // All HLT filters must create and fill an HLT filter object,
  // recording any reconstructed physics objects satisfying (or not)
  // this HLT filter, and place it in the Event.

  // The filter object
  if (saveTags()) {
    filterproduct.addCollectionTag(l1SumTag_);
  }

  // Specific filter code

  // get hold of products from Event
  auto const sums = iEvent.getHandle(sumToken_);

  // get scaling constants
  // do we *need* to get these every event? can't we cache them somewhere?
  edm::ESHandle<L1TObjScalingConstants> scalingConstants_ = iSetup.getHandle(scalingToken_);

  // sum
  int nsum(0);
  auto isum = sums->begin();
  for (; isum != sums->end(); isum++) {
    double offlinePt = 0.0;
    // pt or sumEt?
    if (typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFMET or
        typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFMHT)
      offlinePt = this->EnergySumOfflineEt(isum->pt(), *scalingConstants_);
    else if (typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFETT or
             typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFHT)
      offlinePt = this->EnergySumOfflineEt(isum->sumEt(), *scalingConstants_);

    if (offlinePt >= min_Pt_) {
      nsum++;
      edm::Ref<std::vector<T>> ref(sums, distance(sums->begin(), isum));
      filterproduct.addObject(typeOfSumEnum_, ref);
    }
  }

  // error case
  // filterproduct.addObject(0,Ref<vector<int> >());

  // final filter decision:
  bool const accept(nsum > 0);

  // return final filter decision
  return accept;
}

template <typename T>
double L1EnergySumFilterT<T>::EnergySumOfflineEt(double Et, const L1TObjScalingConstants& scalingConstants) const {
  return (scalingConstants.m_constants.at(0).m_constant + Et * scalingConstants.m_constants.at(0).m_linear + Et * Et * scalingConstants.m_constants.at(0).m_quadratic);
}

#endif // HLTrigger_HLTfilters_L1EnergySumFilterT_h
