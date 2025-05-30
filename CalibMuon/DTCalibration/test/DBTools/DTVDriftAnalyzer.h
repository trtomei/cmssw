#ifndef DTVDriftAnalyzer_H
#define DTVDriftAnalyzer_H

/** \class DTVDriftAnalyzer
 *  Plot the vdrift from the DB
 *
 *  \author S. Bolognesi - INFN Torino
 */

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "DataFormats/MuonDetId/interface/DTWireId.h"
#include "CondFormats/DataRecord/interface/DTMtimeRcd.h"
#include "CondFormats/DataRecord/interface/DTRecoConditionsVdriftRcd.h"

#include <string>
#include <fstream>
#include <map>
#include <vector>

class DTMtime;
class DTRecoConditions;
class TFile;
class TH1D;

class DTVDriftAnalyzer : public edm::one::EDAnalyzer<edm::one::WatchRuns> {
public:
  DTVDriftAnalyzer(const edm::ParameterSet& pset);

  virtual ~DTVDriftAnalyzer();
  void beginJob() override {}
  void beginRun(const edm::Run& run, const edm::EventSetup& setup) override;
  void analyze(const edm::Event& event, const edm::EventSetup& setup) override {}
  void endRun(const edm::Run& run, const edm::EventSetup& setup) override {}
  void endJob() override;

protected:
private:
  std::string getHistoName(const DTWireId& lId) const;
  std::string getDistribName(const DTWireId& wId) const;

  // The file which will contain the histos
  TFile* theFile;

  //The t0 map
  const DTMtime* mTimeMap;             // legacy DB object
  const DTRecoConditions* vDriftMap_;  // DB object in new format
  bool readLegacyVDriftDB;             // which one to use

  // Map of the vdrift, reso histos by wheel/sector/SL
  std::map<std::pair<int, int>, TH1D*> theVDriftHistoMap;
  std::map<std::pair<int, int>, TH1D*> theResoHistoMap;
  // Map of the vdrift, reso distributions by wheel/station/SL
  std::map<std::vector<int>, TH1D*> theVDriftDistribMap;
  std::map<std::vector<int>, TH1D*> theResoDistribMap;

  edm::ESGetToken<DTMtime, DTMtimeRcd> mTimeMapToken_;
  edm::ESGetToken<DTRecoConditions, DTRecoConditionsVdriftRcd> vDriftMapToken_;
};
#endif
