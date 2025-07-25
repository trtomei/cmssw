// -*- C++ -*-
#ifndef FWCore_Framework_EventSetupRecordProvider_h
#define FWCore_Framework_EventSetupRecordProvider_h
//
// Package:     Framework
// Class  :     EventSetupRecordProvider
//
/**\class edm::eventsetup::EventSetupRecordProvider

 Description: Coordinates all EventSetupDataProviders with the same 'interval of validity'

 Usage:
    <usage>

*/
//
// Author:      Chris Jones
// Created:     Fri Mar 25 19:02:23 EST 2005
//

// user include files
#include "FWCore/Framework/interface/EventSetupRecordKey.h"
#include "FWCore/Framework/interface/EventSetupRecordImpl.h"
#include "FWCore/Framework/interface/ValidityInterval.h"
#include "FWCore/Utilities/interface/get_underlying_safe.h"
#include "FWCore/Utilities/interface/propagate_const.h"

// system include files
#include <map>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

// forward declarations
namespace edm {
  class ActivityRegistry;
  class EventSetupImpl;
  class EventSetupRecordIntervalFinder;

  namespace eventsetup {
    struct ComponentDescription;
    class DataKey;
    class ESProductResolverProvider;
    class ESRecordsToProductResolverIndices;
    class EventSetupProvider;
    class EventSetupRecordImpl;

    class EventSetupRecordProvider {
    public:
      enum class IntervalStatus { NotInitializedForSyncValue, Invalid, NewInterval, UpdateIntervalEnd, SameInterval };

      using DataToPreferredProviderMap = std::map<DataKey, ComponentDescription>;

      EventSetupRecordProvider(EventSetupRecordKey const& iKey,
                               ActivityRegistry const*,
                               unsigned int nConcurrentIOVs = 1);

      EventSetupRecordProvider(EventSetupRecordProvider const&) = delete;
      EventSetupRecordProvider const& operator=(EventSetupRecordProvider const&) = delete;

      // ---------- const member functions ---------------------

      unsigned int nConcurrentIOVs() const { return nConcurrentIOVs_; }

      ValidityInterval const& validityInterval() const { return validityInterval_; }
      EventSetupRecordKey const& key() const { return key_; }

      // Returns a reference for the first of the allowed concurrent IOVs.
      // There is always at least one IOV allowed. Intended for cases where
      // you only need one of them and don't care which one you get.
      EventSetupRecordImpl const& firstRecordImpl() const;

      ///Returns the list of Records the provided Record depends on (usually none)
      std::set<EventSetupRecordKey> dependentRecords() const;

      ///return information on which ESProductResolverProviders are supplying information
      std::set<ComponentDescription> resolverProviderDescriptions() const;

      /// The available DataKeys in the Record. The order can be used to request the data by index
      std::vector<DataKey> const& registeredDataKeys() const;

      std::vector<ComponentDescription const*> componentsForRegisteredDataKeys() const;
      std::vector<unsigned int> produceMethodIDsForRegisteredDataKeys() const;

      // ---------- member functions ---------------------------

      ///returns the first matching ESProductResolverProvider or a 'null' if not found
      std::shared_ptr<ESProductResolverProvider> resolverProvider(ComponentDescription const&);

      void add(std::shared_ptr<ESProductResolverProvider>);
      ///For now, only use one finder
      void addFinder(std::shared_ptr<EventSetupRecordIntervalFinder>);

      ///Intended for use only in unit tests
      void setValidityInterval_forTesting(ValidityInterval const&);

      /** This function is called when an IOV is started up by the asynchronous task assigned to start IOVs.
       *  This is the point where we know the value of iovIndex, so we know which EventSetupRecordImpl
       *  object will be used. We set a pointer to it. In the EventSetupRecordImpl we set the cacheIdentifier
       *  and validity interval. We also set the pointer in the EventSetupImpl to the EventSetupRecordImpl here.
       */
      void initializeForNewIOV(unsigned int iovIndex, unsigned long long cacheIdentifier);

      /** This function is called when we do not need to start a new IOV for a record and syncValue.
       *  If a new EventSetupImpl was created, then this will set its pointer to the EventSetupRecordImpl.
       *  This is also the place where the validity interval will be updated in the special case
       *  where the beginning of the interval did not change but the end changed so it is not treated
       *  as a new IOV.
       */
      void continueIOV(bool newEventSetupImpl);

      /** The asynchronous task called when an IOV ends calls this function. It clears the caches
       *  of the ESProductResolver's.
       */
      void endIOV(unsigned int iovIndex);

      /** Set a flag each time the EventSetupProvider starts initializing with a new sync value.
       *  setValidityIntervalFor can be called multiple times because of dependent records.
       *  Using this flag allows setValidityIntervalFor to avoid duplicating its work on the same
       *  syncValue.
       */
      void initializeForNewSyncValue();

      bool doWeNeedToWaitForIOVsToFinish(IOVSyncValue const&) const;

      /** Sets the validity interval for this sync value and returns true if we have a valid
       *  interval for sync value. Also sets the interval status.
       */
      bool setValidityIntervalFor(IOVSyncValue const&);

      bool newInterval() const { return newInterval_; }
      void setNewInterval(bool value) { newInterval_ = value; }

      ///If the provided Record depends on other Records, here are the dependent Providers
      void setDependentProviders(std::vector<std::shared_ptr<EventSetupRecordProvider>> const&);

      /**In the case of a conflict, sets what Provider to call.  This must be called after
         all providers have been added.  An empty map is acceptable. */
      void usePreferred(DataToPreferredProviderMap const&);

      ///This will clear the cache's of all the Resolvers so that next time they are called they will run
      void resetResolvers();

      std::shared_ptr<EventSetupRecordIntervalFinder const> finder() const { return get_underlying_safe(finder_); }
      std::shared_ptr<EventSetupRecordIntervalFinder>& finder() { return get_underlying_safe(finder_); }

      void getReferencedESProducers(
          std::map<EventSetupRecordKey, std::vector<ComponentDescription const*>>& referencedESProducers) const;

      void setEventSetupImpl(EventSetupImpl* value) { eventSetupImpl_ = value; }

      IntervalStatus intervalStatus() const { return intervalStatus_; }

      void fillAllESProductResolverProviders(std::vector<ESProductResolverProvider const*>&,
                                             std::unordered_set<unsigned int>& componentIDs) const;

      void updateLookup(ESRecordsToProductResolverIndices const&);

    protected:
      void addResolversToRecordHelper(edm::propagate_const<std::shared_ptr<ESProductResolverProvider>>& dpp,
                                      DataToPreferredProviderMap const& mp) {
        addResolversToRecord(get_underlying_safe(dpp), mp);
      }
      void addResolversToRecord(std::shared_ptr<ESProductResolverProvider>, DataToPreferredProviderMap const&);

      std::shared_ptr<EventSetupRecordIntervalFinder> swapFinder(std::shared_ptr<EventSetupRecordIntervalFinder> iNew) {
        std::swap(iNew, finder());
        return iNew;
      }

    private:
      // ---------- member data --------------------------------
      const EventSetupRecordKey key_;

      // This holds the interval most recently initialized with a call to
      // eventSetupForInstance. A particular EventSetupRecordImpl in flight
      // might contain an older interval.
      ValidityInterval validityInterval_;

      EventSetupImpl* eventSetupImpl_ = nullptr;

      std::vector<EventSetupRecordImpl> recordImpls_;
      EventSetupRecordImpl const* recordImpl_ = nullptr;

      edm::propagate_const<std::shared_ptr<EventSetupRecordIntervalFinder>> finder_;
      std::vector<edm::propagate_const<std::shared_ptr<ESProductResolverProvider>>> providers_;
      std::unique_ptr<std::vector<edm::propagate_const<std::shared_ptr<EventSetupRecordIntervalFinder>>>>
          multipleFinders_;

      const unsigned int nConcurrentIOVs_;
      IntervalStatus intervalStatus_ = IntervalStatus::NotInitializedForSyncValue;
      bool newInterval_ = false;
      bool hasNonconcurrentFinder_ = false;
    };
  }  // namespace eventsetup
}  // namespace edm
#endif
