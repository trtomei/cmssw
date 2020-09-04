#ifndef HLTfilters_HLTL1TScaling_h
#define HLTfilters_HLTL1TScaling_h

/// Small collection of functions to convert the L1T objects' values
/// into something we can cut with "offline" values.

namespace hltp2 {

  // L1TkMuon scaling
  double TkMuonOfflineEt(double Et, double Eta) {
    if (std::abs(Eta) < 0.9)
      return (Et * 1.05326 + 0.480586);
    else if (std::abs(Eta) < 1.2)
      return (Et * 1.03509 + 0.789258);
    else
      return (Et * 1.03251 + 0.784553);
  }

}  // namespace hltp2

#endif  // HLTfilters_HLTL1TScaling_h
