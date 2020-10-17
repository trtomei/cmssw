#ifndef L1TkEleFilter_h
#define L1TkEleFilter_h

/** \class L1TkEleFilter
 *
 *
 *  This class is an HLTFilter (-> EDFilter) implementing a very basic
 *  HLT trigger acting on TkEle candidates
 *  This has support for *two* collections, since electrons can come
 *  either from crystal calo or HGCAL
 *
 *  \author Simone gennai
 *  \author Thiago Tomei
 */

#include "HLTrigger/HLTcore/interface/HLTFilter.h"
#include "DataFormats/L1TCorrelator/interface/TkElectron.h"
#include "DataFormats/L1TCorrelator/interface/TkElectronFwd.h"
#include "CondFormats/HLTObjects/interface/L1TObjScalingConstants.h"
#include "CondFormats/DataRecord/interface/L1TObjScalingRcd.h"

//
// class declaration
//

class L1TkEleFilter : public HLTFilter {
public:
  explicit L1TkEleFilter(const edm::ParameterSet&);
  ~L1TkEleFilter() override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  bool hltFilter(edm::Event&,
                 const edm::EventSetup&,
                 trigger::TriggerFilterObjectWithRefs& filterproduct) const override;

private:
  edm::InputTag l1TkEleTag1_;          //input tag for L1 Tk Ele product
  edm::InputTag l1TkEleTag2_;          //input tag for L1 Tk Ele product
  edm::ESInputTag l1TkEleScalingTag_;  //input tag for L1 Tk Ele scaling

  typedef std::vector<l1t::TkElectron> TkEleCollection;
  edm::EDGetTokenT<TkEleCollection> tkEleToken1_;  // token identifying product containing L1 TkEles
  edm::EDGetTokenT<TkEleCollection> tkEleToken2_;  // token identifying product containing L1 TkEles
  edm::ESGetToken<L1TObjScalingConstants, L1TObjScalingRcd> scalingToken_;  // token identifying the L1T scaling constants

  double min_Pt_;                            // min pt cut
  int min_N_;                                // min number of candidates above pT cut
  double min_Eta_;                           //min eta cut
  double max_Eta_;                           //max eta cut
  std::vector<double> etaBinsForIsolation_;  // abs. eta bin edges for isolation. First edge at 0.0 must be explicit!
  std::vector<double> trkIsolation_;         // values for track isolation in the bins defined above
  int quality1_;                             // quality for electrons of 1st collection
  int quality2_;                             // quality for electrons of 2nd collection
  int qual1IsMask_;                          // is qual for electrons of 1st collection a mask?
  int qual2IsMask_;                          // is qual for electrons of 2nd collection a mask?
  bool applyQual1_;                          // should we apply quality 1?
  bool applyQual2_;                          // should we apply quality 2?

  double TkEleOfflineEt(double Et, double Eta, const L1TObjScalingConstants& scalingConstants) const;
};

#endif  //L1TkEleFilter_h
