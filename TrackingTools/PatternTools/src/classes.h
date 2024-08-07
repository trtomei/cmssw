#include "DataFormats/Common/interface/Wrapper.h"
#include "DataFormats/Common/interface/AssociationMap.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "Rtypes.h"
#include "Math/Cartesian3D.h"
#include "Math/Polar3D.h"
#include "Math/CylindricalEta3D.h"
#include "DataFormats/TrackCandidate/interface/TrackCandidate.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "TrackingTools/PatternTools/interface/Trajectory.h"
#include "TrackingTools/PatternTools/interface/TrajAnnealing.h"
#include "TrackingTools/TrajectoryState/interface/FreeTrajectoryState.h"
#include "DataFormats/GeometrySurface/interface/Surface.h"
#include "DataFormats/CLHEP/interface/Migration.h"
#include "DataFormats/CLHEP/interface/AlgebraicObjects.h"

#include "DataFormats/GeometryVector/interface/LocalVector.h"
#include "TrackingTools/TrajectoryState/interface/TrajectoryStateOnSurface.h"
#include "DataFormats/Common/interface/AssociationMap.h"
#include "DataFormats/Common/interface/OneToOne.h"
#include "DataFormats/Common/interface/OwnVector.h"
#include "DataFormats/Common/interface/Wrapper.h"
#include "DataFormats/TrackingRecHit/interface/TrackingRecHitFwd.h"
#include "TrackingTools/PatternTools/interface/TrajTrackAssociation.h"
#include "TrackingTools/PatternTools/interface/TrackConstraintAssociation.h"
#include <vector>
