#ifndef GenJetParticleSelector_h
#define GenJetParticleSelector_h
/* \class GenJetParticleSelector
 *
 * \author Luca Lista, INFN
 *
 * \version $Id: GenJetParticleSelector.h,v 1.1 2008/02/07 14:37:04 llista Exp $
 *
 */
#include "SimGeneral/HepPDTRecord/interface/PdtEntry.h"
#include "SimGeneral/HepPDTRecord/interface/ParticleDataTable.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include <set>

namespace edm {
  class ParameterSet;
  class EventSetup;
  class Event;
  class ConsumesCollector;
}  // namespace edm
namespace reco {
  class Candidate;
}

class GenJetParticleSelector {
public:
  GenJetParticleSelector(const edm::ParameterSet&, edm::ConsumesCollector& iC);
  bool operator()(const reco::Candidate&);
  void init(const edm::EventSetup&);

  static void fillPSetDescription(edm::ParameterSetDescription& desc);

private:
  typedef std::vector<PdtEntry> vpdt;
  bool stableOnly_;
  bool partons_;
  vpdt pdtList_;
  bool bInclude_;
  std::set<int> pIds_;
  const edm::ESGetToken<HepPDT::ParticleDataTable, edm::DefaultRecord> tableToken_;
};

#include "CommonTools/UtilAlgos/interface/EventSetupInitTrait.h"

namespace reco {
  namespace modules {
    struct GenJetParticleSelectorEventSetupInit {
      static void init(GenJetParticleSelector& selector, const edm::Event& evt, const edm::EventSetup& es) {
        selector.init(es);
      }

      static void fillPSetDescription(edm::ParameterSetDescription& desc) {
        GenJetParticleSelector::fillPSetDescription(desc);
      }
    };

    template <>
    struct EventSetupInit<GenJetParticleSelectorEventSetupInit> {
      typedef GenJetParticleSelectorEventSetupInit type;
    };
  }  // namespace modules
}  // namespace reco

#endif
