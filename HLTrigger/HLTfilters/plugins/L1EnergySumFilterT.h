#ifndef HLTrigger_HLTfilters_L1EnergySumFilterT_h
#define HLTrigger_HLTfilters_L1EnergySumFilterT_h

#include <vector>

#include "CondFormats/HLTObjects/interface/L1TObjScalingConstants.h"
#include "CondFormats/DataRecord/interface/L1TObjScalingRcd.h"
#include "DataFormats/Common/interface/Ref.h"
#include "DataFormats/HLTReco/interface/TriggerFilterObjectWithRefs.h"
#include "DataFormats/HLTReco/interface/TriggerTypeDefs.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "HLTrigger/HLTcore/interface/HLTFilter.h"

template <typename T>
class L1EnergySumFilterT : public HLTFilter {
public:
  explicit L1EnergySumFilterT(const edm::ParameterSet&);
  ~L1EnergySumFilterT() override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  bool hltFilter(edm::Event&, edm::EventSetup const&, trigger::TriggerFilterObjectWithRefs& filterproduct) const override;

private:
  edm::InputTag const l1SumTag_;                     // input tag for L1 Energy Sum product
  edm::ESInputTag const l1SumScalingTag_;            // input tag for L1 Energy Sum scaling
  edm::EDGetTokenT<std::vector<T>> const sumToken_;  // token identifying product containing L1 EnergySums
  edm::ESGetToken<L1TObjScalingConstants, L1TObjScalingRcd> const scalingToken_;  // token identifying the L1T scaling constants

  trigger::TriggerObjectType const typeOfSumEnum_;
  double const minPt_;

  trigger::TriggerObjectType typeOfSumEnum(std::string const&) const;

  double EnergySumOfflineEt(double const Et, L1TObjScalingConstants const& scalingConstants) const;
};

template <typename T>
L1EnergySumFilterT<T>::L1EnergySumFilterT(const edm::ParameterSet& iConfig)
  : HLTFilter(iConfig),
    l1SumTag_(iConfig.getParameter<edm::InputTag>("inputTag")),
    l1SumScalingTag_(iConfig.getParameter<edm::ESInputTag>("esScalingTag")),
    sumToken_(consumes<std::vector<T>>(l1SumTag_)),
    scalingToken_(esConsumes<L1TObjScalingConstants, L1TObjScalingRcd>(l1SumScalingTag_)),
    typeOfSumEnum_(typeOfSumEnum(iConfig.getParameter<std::string>("TypeOfSum"))),
    minPt_(iConfig.getParameter<double>("MinPt")) {}

template <typename T>
L1EnergySumFilterT<T>::~L1EnergySumFilterT() = default;

template <typename T>
trigger::TriggerObjectType L1EnergySumFilterT<T>::typeOfSumEnum(std::string const& typeOfSum) const {
  trigger::TriggerObjectType sumEnum;

  if (typeOfSum == "MET")
    sumEnum = trigger::TriggerObjectType::TriggerL1PFMET;
  else if (typeOfSum == "ETT")
    sumEnum = trigger::TriggerObjectType::TriggerL1PFETT;
  else if (typeOfSum == "HT")
    sumEnum = trigger::TriggerObjectType::TriggerL1PFHT;
  else if (typeOfSum == "MHT")
    sumEnum = trigger::TriggerObjectType::TriggerL1PFMHT;
  else {
    throw cms::Exception("ConfigurationError") << "Wrong type of energy sum: \"" << typeOfSum << "\" (valid choices are: MET, ETT, HT, MHT)";
  }

  return sumEnum;
}

template <typename T>
void L1EnergySumFilterT<T>::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  makeHLTFilterDescription(desc);
  desc.add<edm::InputTag>("inputTag", edm::InputTag("L1PFEnergySums"));
  desc.add<edm::ESInputTag>("esScalingTag", edm::ESInputTag("L1TScalingESSource", "L1PFEnergySumScaling"));
  desc.add<std::string>("TypeOfSum", "HT");
  desc.add<double>("MinPt", -1.0);
  descriptions.add(defaultModuleLabel<L1EnergySumFilterT<T>>(), desc);
}

template <typename T>
bool L1EnergySumFilterT<T>::hltFilter(edm::Event& iEvent, edm::EventSetup const& iSetup, trigger::TriggerFilterObjectWithRefs& filterproduct) const {
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
  for (auto isum = sums->begin(); isum != sums->end(); isum++) {
    double offlinePt = 0.0;
    // pt or sumEt?
    if (typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFMET or typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFMHT)
      offlinePt = this->EnergySumOfflineEt(isum->pt(), *scalingConstants_);
    else if (typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFETT or typeOfSumEnum_ == trigger::TriggerObjectType::TriggerL1PFHT)
      offlinePt = this->EnergySumOfflineEt(isum->sumEt(), *scalingConstants_);
    else {
      throw cms::Exception("Input") << "Invalid trigger::TriggerObjectType enum: " << typeOfSumEnum_;
    }

    if (offlinePt >= minPt_) {
      nsum++;
      edm::Ref<std::vector<T>> ref(sums, distance(sums->begin(), isum));
      filterproduct.addObject(typeOfSumEnum_, ref);
    }
  }

  // return final filter decision
  return (nsum > 0);
}

template <typename T>
double L1EnergySumFilterT<T>::EnergySumOfflineEt(double const Et, L1TObjScalingConstants const& scalingConstants) const {
  return (scalingConstants.m_constants.at(0).m_constant + Et * scalingConstants.m_constants.at(0).m_linear + Et * Et * scalingConstants.m_constants.at(0).m_quadratic);
}

#endif // HLTrigger_HLTfilters_L1EnergySumFilterT_h
