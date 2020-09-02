#ifndef L1TkEleFilter_h
#define L1TkEleFilter_h

/** \class L1TkEleFilter
 *
 *
 *  This class is an HLTFilter (-> EDFilter) implementing a very basic
 *  HLT trigger acting on TkEle candidates
 *  
 *
 *
 *  \author Simone gennai
 *
 */

#include "HLTrigger/HLTcore/interface/HLTFilter.h"
#include "DataFormats/L1TCorrelator/interface/TkElectron.h"
#include "DataFormats/L1TCorrelator/interface/TkElectronFwd.h"

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
  edm::InputTag l1TkEleTag1_; //input tag for L1 Tk Ele product
  edm::InputTag l1TkEleTag2_; //input tag for L1 Tk Ele product

  typedef std::vector<l1t::TkElectron> TkEleCollection;
  edm::EDGetTokenT<TkEleCollection> tkEleToken1_;  // token identifying product containing L1 TkEles
  edm::EDGetTokenT<TkEleCollection> tkEleToken2_;  // token identifying product containing L1 TkEles

  double min_Pt_;  // min pt cut
  int    min_N_;   // min number of candidates above pT cut
  double min_Eta_; //min eta cut
  double max_Eta_; //max eta cut
};

#endif  //L1TkEleFilter_h
