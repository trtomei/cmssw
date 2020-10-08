#ifndef L1TkMuonFilter_h
#define L1TkMuonFilter_h

/** \class L1TkMuonFilter
 *
 *
 *  This class is an HLTFilter (-> EDFilter) implementing a very basic
 *  HLT trigger acting on TkMuon candidates
 *  
 *
 *
 *  \author Simone Gennai
 *  \author Thiago Tomei
 */

#include "HLTrigger/HLTcore/interface/HLTFilter.h"
#include "DataFormats/L1TCorrelator/interface/TkMuon.h"
#include "DataFormats/L1TCorrelator/interface/TkMuonFwd.h"
#include "CondFormats/HLTObjects/interface/L1TObjScalingConstants.h"
#include "CondFormats/DataRecord/interface/L1TObjScalingRcd.h"

//
// class declaration
//

class L1TkMuonFilter : public HLTFilter {
public:
  explicit L1TkMuonFilter(const edm::ParameterSet&);
  ~L1TkMuonFilter() override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  bool hltFilter(edm::Event&,
                 const edm::EventSetup&,
                 trigger::TriggerFilterObjectWithRefs& filterproduct) const override;

private:
  edm::InputTag l1TkMuonTag_;           //input tag for L1 Tk Muon product
  edm::ESInputTag l1TkMuonScalingTag_;  //input tag for L1 Tk Muon scaling
  typedef std::vector<l1t::TkMuon> TkMuonCollection;
  edm::EDGetTokenT<TkMuonCollection> tkMuonToken_;  // token identifying product containing L1 TkMuons
  edm::ESGetToken<L1TObjScalingConstants, L1TObjScalingRcd> scalingToken_;  // token identifying the L1T scaling constants
  //edm::ESHandle<L1TObjScalingConstants> scalingConstants_;

  double min_Pt_;   // min pt cut
  int min_N_;       // min number of candidates above pT cut
  double min_Eta_;  //min eta cut
  double max_Eta_;  //max eta cut

  double TkMuonOfflineEt(double Et, double Eta, const L1TObjScalingConstants& scalingConstants) const;
};

#endif  //L1TkMuonFilter_h
