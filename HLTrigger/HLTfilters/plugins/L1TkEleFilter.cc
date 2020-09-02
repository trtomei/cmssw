/** \class L1TkEleFilter
 *
 * See header file for documentation
 *
 *
 *  \author Martin Grunewald
 *
 */

#include "L1TkEleFilter.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"

#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/Common/interface/Ref.h"
#include "DataFormats/HLTReco/interface/TriggerFilterObjectWithRefs.h"
#include "DataFormats/HLTReco/interface/TriggerTypeDefs.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

//
// constructors and destructor
//

L1TkEleFilter::L1TkEleFilter(const edm::ParameterSet& iConfig)
    : HLTFilter(iConfig),
      l1TkEleTag1_(iConfig.getParameter<edm::InputTag>("inputTag1")),
      l1TkEleTag2_(iConfig.getParameter<edm::InputTag>("inputTag2")),
      tkEleToken1_(consumes<TkEleCollection>(l1TkEleTag1_)),
      tkEleToken2_(consumes<TkEleCollection>(l1TkEleTag2_)){
        min_Pt_= iConfig.getParameter<double>("MinPt");
        min_N_ = iConfig.getParameter<int>("MinN");
        min_Eta_=iConfig.getParameter<double>("MinEta");
        max_Eta_=iConfig.getParameter<double>("MaxEta");
        
        } 

L1TkEleFilter::~L1TkEleFilter() = default;

//
// member functions
//

void L1TkEleFilter::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  makeHLTFilterDescription(desc);
  desc.add<double>("MinPt", -1.0);
  desc.add<double>("MinEta", -5.0);
  desc.add<double>("MaxEta", 5.0);
  desc.add<int>("MinN",1);
  desc.add<edm::InputTag>("inputTag1",edm::InputTag("L1TkElectrons1"));
  desc.add<edm::InputTag>("inputTag2",edm::InputTag("L1TkElectrons2"));
  descriptions.add("L1TkEleFilter", desc);
}

// ------------ method called to produce the data  ------------
bool L1TkEleFilter::hltFilter(edm::Event& iEvent,
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
    filterproduct.addCollectionTag(l1TkEleTag1_);
    filterproduct.addCollectionTag(l1TkEleTag2_);
  }

  // Specific filter code

  // get hold of products from Event

  Handle<l1t::TkElectronCollection> tkEles1;
  iEvent.getByToken(tkEleToken1_,tkEles1);


// trkEle
  int ntrkEle(0);
  auto atrkEles(tkEles1->begin());
  auto otrkEles(tkEles1->end());
  TkEleCollection::const_iterator itkEle;
  for (itkEle = atrkEles; itkEle != otrkEles; itkEle++) {
    if (itkEle->pt() >= min_Pt_ && itkEle->eta() <= max_Eta_ && itkEle->eta() >= min_Eta_) {
      ntrkEle++;
      l1t::TkElectronRef ref1(l1t::TkElectronRef(tkEles1, distance(atrkEles, itkEle)));
      filterproduct.addObject(-78, ref1);
    }
  }
  // final filter decision:
  const bool accept(ntrkEle >= min_N_);
  if (accept) {
    return accept;
  }
  Handle<l1t::TkElectronCollection> tkEles2;
  iEvent.getByToken(tkEleToken2_,tkEles2);

  atrkEles = tkEles2->begin();
  otrkEles = tkEles2->end();
  for (itkEle = atrkEles; itkEle != otrkEles; itkEle++) {
    if (itkEle->pt() >= min_Pt_ && itkEle->eta() <= max_Eta_ && itkEle->eta() >= min_Eta_) {
      ntrkEle++;
      l1t::TkElectronRef ref2(l1t::TkElectronRef(tkEles2, distance(atrkEles, itkEle)));
      filterproduct.addObject(-78, ref2);
    }
  }
   const bool accept2(ntrkEle >= min_N_);
  // return with final filter decision
  return accept2;
}
