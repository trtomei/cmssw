#ifndef L1PFJetFilter_h
#define L1PFJetFilter_h

/** \class L1PFJetFilter
 *
 *
 *  This class is an HLTFilter (-> EDFilter) implementing a very basic
 *  HLT trigger acting on l1t::PFJet candidates
 *  
 *
 *
 *  \author Simone Gennai
 *  \author Thiago Tomei
 */

#include "HLTrigger/HLTcore/interface/HLTFilter.h"
#include "DataFormats/L1TParticleFlow/interface/PFJet.h"
#include "CondFormats/HLTObjects/interface/L1TObjScalingConstants.h"
#include "CondFormats/DataRecord/interface/L1TObjScalingRcd.h"

//
// class declaration
//

class L1PFJetFilter : public HLTFilter {
public:
  explicit L1PFJetFilter(const edm::ParameterSet&);
  ~L1PFJetFilter() override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  bool hltFilter(edm::Event&,
                 const edm::EventSetup&,
                 trigger::TriggerFilterObjectWithRefs& filterproduct) const override;

private:
  edm::InputTag l1PFJetTag_;
  edm::ESInputTag l1PFJetScalingTag_;
  edm::EDGetTokenT<l1t::PFJetCollection> pfJetToken_;
  edm::ESGetToken<L1TObjScalingConstants, L1TObjScalingRcd> scalingToken_;  // token identifying the L1T scaling constants

  double min_Pt_;   // min pt cut
  int min_N_;       // min number of candidates above pT cut
  double min_Eta_;  //min eta cut
  double max_Eta_;  //max eta cut

  double PFJetOfflineEt(double Et, double Eta, const L1TObjScalingConstants& scalingConstants) const;
};

#endif  //L1PFJetFilter_h
