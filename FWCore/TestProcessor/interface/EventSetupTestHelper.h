// -*- C++ -*-
#ifndef FWCore_TestProcessor_EventSetupTestHelper_h
#define FWCore_TestProcessor_EventSetupTestHelper_h
//
// Package:     FWCore/TestProcessor
// Class  :     EventSetupTestHelper
//
/**\class edm::test::EventSetupTestHelper

 Description: [one line class summary]

 Usage:
    <usage>

*/
//
// Original Author:  root
//         Created:  Tue, 08 May 2018 18:33:09 GMT
//

// system include files
#include <vector>

// user include files
#include "FWCore/Framework/interface/ESProductResolverProvider.h"
#include "FWCore/Framework/interface/EventSetupRecordIntervalFinder.h"

#include "FWCore/TestProcessor/interface/ESProduceEntry.h"

// forward declarations
namespace edm {
  namespace test {

    class EventSetupTestHelper : public eventsetup::ESProductResolverProvider, public EventSetupRecordIntervalFinder {
    public:
      EventSetupTestHelper(std::vector<ESProduceEntry>);
      EventSetupTestHelper(const EventSetupTestHelper&) = delete;
      const EventSetupTestHelper& operator=(const EventSetupTestHelper&) = delete;

      std::shared_ptr<eventsetup::ESProductResolver> getResolver(unsigned int index);

      void resetAllResolvers();

      std::vector<eventsetup::ESModuleProducesInfo> producesInfo() const final;

    protected:
      void setIntervalFor(const eventsetup::EventSetupRecordKey&, const IOVSyncValue&, ValidityInterval&) final;

      KeyedResolversVector registerResolvers(const eventsetup::EventSetupRecordKey&, unsigned int iovIndex) final;

    private:
      // ---------- member data --------------------------------
      std::vector<ESProduceEntry> resolvers_;
    };
  }  // namespace test
}  // namespace edm
#endif
