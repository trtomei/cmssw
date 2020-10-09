#ifndef L1PFEnergySumFilter_h
#define L1PFEnergySumFilter_h

/** \class L1PFEnergySumFilter
 *
 *
 *  This class is an HLTFilter (-> EDFilter) implementing a very basic
 *  HLT trigger acting on PFEnergySum candidates
 *  
 *
 *
 *  \author Simone Gennai
 *  \author Thiago Tomei
 */

#include "HLTrigger/HLTcore/interface/HLTFilter.h"
#include "DataFormats/METReco/interface/MET.h"
#include "DataFormats/METReco/interface/METFwd.h"
#include "CondFormats/HLTObjects/interface/L1TObjScalingConstants.h"
#include "CondFormats/DataRecord/interface/L1TObjScalingRcd.h"

//
// class declaration
//

class L1PFEnergySumFilter : public HLTFilter {
public:
  explicit L1PFEnergySumFilter(const edm::ParameterSet&);
  ~L1PFEnergySumFilter() override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  bool hltFilter(edm::Event&,
                 const edm::EventSetup&,
                 trigger::TriggerFilterObjectWithRefs& filterproduct) const override;

private:
  edm::InputTag l1PfSumTag_;                          //input tag for L1 PF Energy Sum product
  edm::ESInputTag l1PfSumScalingTag_;                 //input tag for L1 PF Energy Sum scaling
  edm::EDGetTokenT<reco::METCollection> pfSumToken_;  // token identifying product containing L1 PFEnergySums
  edm::ESGetToken<L1TObjScalingConstants, L1TObjScalingRcd> scalingToken_;  // token identifying the L1T scaling constants

  std::string typeOfSum_;
  trigger::TriggerObjectType typeOfSumEnum_;
  double min_Pt_;  // min pt cut

  double PFEnergySumOfflineEt(double Et, const L1TObjScalingConstants& scalingConstants) const;
};

#endif  //L1PFEnergySumFilter_h
