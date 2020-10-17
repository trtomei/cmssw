#ifndef L1TkEmFilter_h
#define L1TkEmFilter_h

/** \class L1TkEmFilter
 *
 *
 *  This class is an HLTFilter (-> EDFilter) implementing a very basic
 *  HLT trigger acting on TkEm candidates
 *  This has support for *two* collections, since Photons can come
 *  either from crystal calo or HGCAL
 *
 *  \author Simone gennai
 *  \author Thiago Tomei
 */

#include "HLTrigger/HLTcore/interface/HLTFilter.h"
#include "DataFormats/L1TCorrelator/interface/TkEm.h"
#include "DataFormats/L1TCorrelator/interface/TkEmFwd.h"
#include "CondFormats/HLTObjects/interface/L1TObjScalingConstants.h"
#include "CondFormats/DataRecord/interface/L1TObjScalingRcd.h"

//
// class declaration
//

class L1TkEmFilter : public HLTFilter {
public:
  explicit L1TkEmFilter(const edm::ParameterSet&);
  ~L1TkEmFilter() override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  bool hltFilter(edm::Event&,
                 const edm::EventSetup&,
                 trigger::TriggerFilterObjectWithRefs& filterproduct) const override;

private:
  edm::InputTag l1TkEmTag1_;          //input tag for L1 Tk Em product
  edm::InputTag l1TkEmTag2_;          //input tag for L1 Tk Em product
  edm::ESInputTag l1TkEmScalingTag_;  //input tag for L1 Tk Em scaling

  typedef std::vector<l1t::TkEm> TkEmCollection;
  edm::EDGetTokenT<TkEmCollection> tkEmToken1_;  // token identifying product containing L1 TkEms
  edm::EDGetTokenT<TkEmCollection> tkEmToken2_;  // token identifying product containing L1 TkEms
  edm::ESGetToken<L1TObjScalingConstants, L1TObjScalingRcd> scalingToken_;  // token identifying the L1T scaling constants

  double min_Pt_;                            // min pt cut
  int min_N_;                                // min number of candidates above pT cut
  double min_Eta_;                           //min eta cut
  double max_Eta_;                           //max eta cut
  std::vector<double> etaBinsForIsolation_;  // abs. eta bin edges for isolation. First edge at 0.0 must be explicit!
  std::vector<double> trkIsolation_;         // values for track isolation in the bins defined above
  int quality1_;                             // quality for photons of 1st collection
  int quality2_;                             // quality for photons of 2nd collection
  int qual1IsMask_;                          // is qual for photons of 1st collection a mask?
  int qual2IsMask_;                          // is qual for photons of 2nd collection a mask?
  bool applyQual1_;                          // should we apply quality 1?
  bool applyQual2_;                          // should we apply quality 2?

  double TkEmOfflineEt(double Et, double Eta, const L1TObjScalingConstants& scalingConstants) const;
};

#endif  //L1TkEmFilter_h
