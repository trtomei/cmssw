#ifndef HLTrigger_HLTfilters_L1EnergySumFilterT_h
#define HLTrigger_HLTfilters_L1EnergySumFilterT_h

#include <vector>
#include <iterator>

#include "CondFormats/HLTObjects/interface/L1TObjScalingConstants.h"
#include "CondFormats/DataRecord/interface/L1TObjScalingRcd.h"
#include "DataFormats/Common/interface/Ref.h"
#include "DataFormats/HLTReco/interface/TriggerFilterObjectWithRefs.h"
#include "DataFormats/HLTReco/interface/TriggerTypeDefs.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "HLTrigger/HLTcore/interface/HLTFilter.h"
#include "HLTrigger/HLTcore/interface/defaultModuleLabel.h"

template <typename T>
class L1EnergySumFilterT : public HLTFilter {
public:
  explicit L1EnergySumFilterT(const edm::ParameterSet&);
  ~L1EnergySumFilterT() override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  bool hltFilter(edm::Event&, edm::EventSetup const&, trigger::TriggerFilterObjectWithRefs& filterproduct) const override;

private:
  edm::InputTag const l1tSumTag_;
  edm::EDGetTokenT<std::vector<T>> const l1tSumToken_;
  edm::ESInputTag const l1tSumScalingTag_;
  edm::ESGetToken<L1TObjScalingConstants, L1TObjScalingRcd> const l1tSumScalingToken_;

  trigger::TriggerObjectType const l1tSumType_;
  double const minPt_;

  trigger::TriggerObjectType typeOfL1TSum(std::string const&) const;

  double offlineEnergySum(double const Et, L1TObjScalingConstants const& scalingConstants) const;
};

template <typename T>
L1EnergySumFilterT<T>::L1EnergySumFilterT(const edm::ParameterSet& iConfig)
  : HLTFilter(iConfig),
    l1tSumTag_(iConfig.getParameter<edm::InputTag>("inputTag")),
    l1tSumToken_(consumes<std::vector<T>>(l1tSumTag_)),
    l1tSumScalingTag_(iConfig.getParameter<edm::ESInputTag>("esScalingTag")),
    l1tSumScalingToken_(esConsumes<L1TObjScalingConstants, L1TObjScalingRcd>(l1tSumScalingTag_)),
    l1tSumType_(typeOfL1TSum(iConfig.getParameter<std::string>("TypeOfSum"))),
    minPt_(iConfig.getParameter<double>("MinPt")) {}

template <typename T>
L1EnergySumFilterT<T>::~L1EnergySumFilterT() = default;

template <typename T>
trigger::TriggerObjectType L1EnergySumFilterT<T>::typeOfL1TSum(std::string const& typeOfSum) const {
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
  desc.add<double>("MinPt", -1.);
  descriptions.add(defaultModuleLabel<L1EnergySumFilterT<T>>(), desc);
}

template <typename T>
bool L1EnergySumFilterT<T>::hltFilter(edm::Event& iEvent, edm::EventSetup const& iSetup, trigger::TriggerFilterObjectWithRefs& filterproduct) const {
  // All HLT filters must create and fill an HLT filter object,
  // recording any reconstructed physics objects satisfying (or not)
  // this HLT filter, and place it in the Event.

  // The filter object
  if (saveTags()) {
    filterproduct.addCollectionTag(l1tSumTag_);
  }

  auto const& l1tSums = iEvent.getHandle(l1tSumToken_);
  auto const& l1tSumScalingConstants = iSetup.getData(l1tSumScalingToken_);

  int nSum(0);
  for (auto iSum = l1tSums->begin(); iSum != l1tSums->end(); ++iSum) {
    double offlinePt = 0.0;
    // pt or sumEt?
    if (l1tSumType_ == trigger::TriggerObjectType::TriggerL1PFMET or l1tSumType_ == trigger::TriggerObjectType::TriggerL1PFMHT) {
      offlinePt = offlineEnergySum(iSum->pt(), l1tSumScalingConstants);
    }
    else if (l1tSumType_ == trigger::TriggerObjectType::TriggerL1PFETT or l1tSumType_ == trigger::TriggerObjectType::TriggerL1PFHT) {
      offlinePt = offlineEnergySum(iSum->sumEt(), l1tSumScalingConstants);
    }
    else {
      throw cms::Exception("Input") << "Invalid trigger::TriggerObjectType enum: " << l1tSumType_;
    }

    if (offlinePt >= minPt_) {
      ++nSum;
      edm::Ref<std::vector<T>> ref(l1tSums, std::distance(l1tSums->begin(), iSum));
      filterproduct.addObject(l1tSumType_, ref);
    }
  }

  // return final filter decision
  return nSum > 0;
}

template <typename T>
double L1EnergySumFilterT<T>::offlineEnergySum(double const Et, L1TObjScalingConstants const& scalingConstants) const {
  return scalingConstants.m_constants.at(0).m_constant + Et * scalingConstants.m_constants.at(0).m_linear + Et * Et * scalingConstants.m_constants.at(0).m_quadratic;
}

#endif // HLTrigger_HLTfilters_L1EnergySumFilterT_h
