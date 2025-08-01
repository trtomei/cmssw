#ifndef RecoTracker_LSTCore_interface_TrackCandidatesHostCollection_h
#define RecoTracker_LSTCore_interface_TrackCandidatesHostCollection_h

#include "RecoTracker/LSTCore/interface/TrackCandidatesSoA.h"
#include "DataFormats/Portable/interface/PortableHostCollection.h"

namespace lst {
  using TrackCandidatesBaseHostCollection = PortableHostCollection<TrackCandidatesBaseSoA>;
  using TrackCandidatesExtendedHostCollection = PortableHostCollection<TrackCandidatesExtendedSoA>;
}  // namespace lst
#endif
