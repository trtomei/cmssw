#ifndef HLTfilters_HLTL1TScaling_h
#define HLTfilters_HLTL1TScaling_h

#include "CondFormats/HLTObjects/interface/L1TObjScalingConstants.h"

/// Small collection of functions to convert the L1T objects' values
/// into something we can cut with "offline" values.

namespace hltp2 {

  // L1TkMuon scaling
    double TkMuonOfflineEt(double Et, double Eta, const L1TObjScalingConstants& scalingConstants) {
    if (std::abs(Eta) < 0.9)
        return (scalingConstants.m_constants.at(0).m_constant + 
                Et * scalingConstants.m_constants.at(0).m_linear + 
                Et * Et * scalingConstants.m_constants.at(0).m_quadratic);
    else if (std::abs(Eta) < 1.2)
        return (scalingConstants.m_constants.at(1).m_constant +
                Et * scalingConstants.m_constants.at(1).m_linear    +
                Et * Et * scalingConstants.m_constants.at(1).m_quadratic);
    else
        return (scalingConstants.m_constants.at(2).m_constant +
                Et * scalingConstants.m_constants.at(2).m_linear    +
                Et * Et * scalingConstants.m_constants.at(2).m_quadratic);
    }

}  // namespace hltp2

#endif  // HLTfilters_HLTL1TScaling_h
