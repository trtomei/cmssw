#ifndef SimTracker_SiPixelDigitizer_SiPixelDigitizerAlgorithm_h
#define SimTracker_SiPixelDigitizer_SiPixelDigitizerAlgorithm_h

#include <map>
#include <memory>
#include <vector>
#include <iostream>
#include "DataFormats/GeometrySurface/interface/GloballyPositioned.h"
#include "DataFormats/GeometryVector/interface/LocalPoint.h"
#include "SimDataFormats/EncodedEventId/interface/EncodedEventId.h"
#include "SimDataFormats/TrackingHit/interface/PSimHit.h"
#include "SimTracker/Common/interface/DigitizerUtility.h"
#include "SimTracker/SiPixelDigitizer/plugins/PixelDigiAddTempInfo.h"
#include "SimDataFormats/TrackerDigiSimLink/interface/PixelSimHitExtraInfo.h"
#include "SimDataFormats/TrackerDigiSimLink/interface/PixelSimHitExtraInfoLite.h"
#include "DataFormats/Math/interface/approx_exp.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupMixingContent.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"
#include "Geometry/CommonTopologies/interface/PixelTopology.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "CondFormats/SiPixelTransient/interface/SiPixelTemplate2D.h"
#include "DataFormats/SiPixelDetId/interface/PixelFEDChannel.h"
#include "CalibTracker/Records/interface/SiPixelFEDChannelContainerESProducerRcd.h"
#include "CondFormats/DataRecord/interface/SiPixelQualityRcd.h"
#include "CondFormats/DataRecord/interface/SiPixelFedCablingMapRcd.h"
#include "CondFormats/DataRecord/interface/SiPixelLorentzAngleSimRcd.h"
#include "CondFormats/DataRecord/interface/SiPixelDynamicInefficiencyRcd.h"
#include "CondFormats/DataRecord/interface/SiPixelStatusScenarioProbabilityRcd.h"
#include "CondFormats/DataRecord/interface/SiPixelStatusScenariosRcd.h"
#include "FWCore/Framework/interface/FrameworkfwdMostUsed.h"
#include "boost/multi_array.hpp"

typedef boost::multi_array<float, 2> array_2d;

// forward declarations

// For the random numbers
namespace CLHEP {
  class HepRandomEngine;
}

class DetId;
class GaussianTailNoiseGenerator;
class PixelDigi;
class PixelDigiSimLink;
class PixelGeomDetUnit;
class SiG4UniversalFluctuation;
class SiPixelFedCablingMap;
class SiPixelGainCalibrationOfflineSimService;
class SiPixelLorentzAngle;
class SiPixelQuality;
class SiPixelDynamicInefficiency;
class TrackerGeometry;
class TrackerTopology;
class SiPixelFEDChannelContainer;
class SiPixelQualityProbabilities;
class SiPixelChargeReweightingAlgorithm;

class SiPixelDigitizerAlgorithm {
public:
  SiPixelDigitizerAlgorithm(const edm::ParameterSet& conf, edm::ConsumesCollector iC);
  ~SiPixelDigitizerAlgorithm();

  // initialization that cannot be done in the constructor
  void init(const edm::EventSetup& es);

  void initializeEvent() { _signal.clear(); }

  //run the algorithm to digitize a single det
  void accumulateSimHits(const std::vector<PSimHit>::const_iterator inputBegin,
                         const std::vector<PSimHit>::const_iterator inputEnd,
                         const size_t inputBeginGlobalIndex,
                         const unsigned int tofBin,
                         const PixelGeomDetUnit* pixdet,
                         const GlobalVector& bfield,
                         const TrackerTopology* tTopo,
                         CLHEP::HepRandomEngine*);
  void digitize(const PixelGeomDetUnit* pixdet,
                std::vector<PixelDigi>& digis,
                std::vector<PixelDigiSimLink>& simlinks,
                std::vector<PixelDigiAddTempInfo>& newClass_Digi_extra,
                const TrackerTopology* tTopo,
                CLHEP::HepRandomEngine*);
  void calculateInstlumiFactor(PileupMixingContent* puInfo);
  void fillSimHitMaps(std::vector<PSimHit> simHits, const unsigned int tofBin);
  void resetSimHitMaps();
  void init_DynIneffDB(const edm::EventSetup&);
  std::unique_ptr<PixelFEDChannelCollection> chooseScenario(PileupMixingContent* puInfo, CLHEP::HepRandomEngine*);

  void lateSignalReweight(const PixelGeomDetUnit* pixdet,
                          std::vector<PixelDigi>& digis,
                          std::vector<PixelSimHitExtraInfo>& newClass_Sim_extra,
                          const TrackerTopology* tTopo,
                          CLHEP::HepRandomEngine* engine);
  void lateSignalReweight(const PixelGeomDetUnit* pixdet,
                          std::vector<PixelDigi>& digis,
                          std::vector<PixelSimHitExtraInfoLite>& newClass_Sim_extra,
                          const TrackerTopology* tTopo,
                          CLHEP::HepRandomEngine* engine);

  // for premixing
  void calculateInstlumiFactor(const std::vector<PileupSummaryInfo>& ps,
                               int bunchSpacing);  // TODO: try to remove the duplication of logic...
  void setSimAccumulator(const std::map<uint32_t, std::map<int, int> >& signalMap);
  std::unique_ptr<PixelFEDChannelCollection> chooseScenario(const std::vector<PileupSummaryInfo>& ps,
                                                            CLHEP::HepRandomEngine* engine);

  bool killBadFEDChannels() const;
  typedef std::unordered_map<std::string, PixelFEDChannelCollection> PixelFEDChannelCollectionMap;
  const PixelFEDChannelCollectionMap* quality_map;

private:
  //Accessing Lorentz angle from DB:
  edm::ESGetToken<SiPixelLorentzAngle, SiPixelLorentzAngleSimRcd> SiPixelLorentzAngleToken_;
  const SiPixelLorentzAngle* SiPixelLorentzAngle_ = nullptr;

  //Accessing Dead pixel modules from DB:
  edm::ESGetToken<SiPixelQuality, SiPixelQualityRcd> SiPixelBadModuleToken_;
  const SiPixelQuality* SiPixelBadModule_ = nullptr;

  //Accessing Map and Geom:
  const edm::ESGetToken<SiPixelFedCablingMap, SiPixelFedCablingMapRcd> mapToken_;
  const edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> geomToken_;
  const SiPixelFedCablingMap* map_ = nullptr;
  const TrackerGeometry* geom_ = nullptr;

  // Get Dynamic Inefficiency scale factors from DB
  edm::ESGetToken<SiPixelDynamicInefficiency, SiPixelDynamicInefficiencyRcd> SiPixelDynamicInefficiencyToken_;
  const SiPixelDynamicInefficiency* SiPixelDynamicInefficiency_ = nullptr;

  // For BadFEDChannel simulation
  edm::ESGetToken<SiPixelQualityProbabilities, SiPixelStatusScenarioProbabilityRcd> scenarioProbabilityToken_;
  edm::ESGetToken<PixelFEDChannelCollectionMap, SiPixelFEDChannelContainerESProducerRcd>
      PixelFEDChannelCollectionMapToken_;
  const SiPixelQualityProbabilities* scenarioProbability_ = nullptr;
  // Define internal classes

  // definition class
  //

  // Define a class to hold the calibration parameters per pixel
  // Internal
  class CalParameters {
  public:
    float p0;
    float p1;
    float p2;
    float p3;
  };
  //
  // Define a class for 3D ionization points and energy
  //
  /**
   * Internal use only.
   */
  class EnergyDepositUnit {
  public:
    EnergyDepositUnit() : _energy(0), _position(0, 0, 0) {}
    EnergyDepositUnit(float energy, float x, float y, float z) : _energy(energy), _position(x, y, z) {}
    EnergyDepositUnit(float energy, Local3DPoint position) : _energy(energy), _position(position) {}
    float x() const { return _position.x(); }
    float y() const { return _position.y(); }
    float z() const { return _position.z(); }
    float energy() const { return _energy; }

  private:
    float _energy;
    Local3DPoint _position;
  };

  //
  // define class to store signals on the collection surface
  //
  /**
   * Internal use only.
   */

  class SignalPoint {
  public:
    SignalPoint() : _pos(0, 0), _time(0), _amplitude(0), _sigma_x(1.), _sigma_y(1.), _hitp(nullptr) {}

    SignalPoint(float x, float y, float sigma_x, float sigma_y, float t, float a = 1.0)
        : _pos(x, y), _time(t), _amplitude(a), _sigma_x(sigma_x), _sigma_y(sigma_y), _hitp(nullptr) {}

    SignalPoint(float x, float y, float sigma_x, float sigma_y, float t, const PSimHit& hit, float a = 1.0)
        : _pos(x, y), _time(t), _amplitude(a), _sigma_x(sigma_x), _sigma_y(sigma_y), _hitp(&hit) {}

    const LocalPoint& position() const { return _pos; }
    float x() const { return _pos.x(); }
    float y() const { return _pos.y(); }
    float sigma_x() const { return _sigma_x; }
    float sigma_y() const { return _sigma_y; }
    float time() const { return _time; }
    float amplitude() const { return _amplitude; }
    const PSimHit& hit() { return *_hitp; }
    SignalPoint& set_amplitude(float amp) {
      _amplitude = amp;
      return *this;
    }

  private:
    LocalPoint _pos;
    float _time;
    float _amplitude;
    float _sigma_x;  // gaussian sigma in the x direction (cm)
    float _sigma_y;  //    "       "          y direction (cm) */
    const PSimHit* _hitp;
  };

  //
  // PixelEfficiencies struct
  //
  /**
   * Internal use only.
   */
  struct PixelEfficiencies {
    PixelEfficiencies(const edm::ParameterSet& conf,
                      bool AddPixelInefficiency,
                      int NumberOfBarrelLayers,
                      int NumberOfEndcapDisks);
    bool FromConfig;  // If true read from Config, otherwise use Database

    double theInstLumiScaleFactor;
    std::vector<double> pu_scale;                       // in config: 0-3 BPix, 4-5 FPix (inner, outer)
    std::vector<std::vector<double> > thePUEfficiency;  // Instlumi dependent efficiency

    // Read factors from Configuration
    double thePixelEfficiency[20];                     // Single pixel effciency
    double thePixelColEfficiency[20];                  // Column effciency
    double thePixelChipEfficiency[20];                 // ROC efficiency
    std::vector<double> theLadderEfficiency_BPix[20];  // Ladder efficiency
    std::vector<double> theModuleEfficiency_BPix[20];  // Module efficiency
    double theInnerEfficiency_FPix[20];                // Fpix inner module efficiency
    double theOuterEfficiency_FPix[20];                // Fpix outer module efficiency
    unsigned int FPixIndex;                            // The Efficiency index for FPix Disks

    // Read factors from DB and fill containers
    std::map<uint32_t, double> PixelGeomFactors;
    std::map<uint32_t, std::vector<double> > PixelGeomFactorsROCStdPixels;
    std::map<uint32_t, std::vector<double> > PixelGeomFactorsROCBigPixels;
    std::map<uint32_t, double> ColGeomFactors;
    std::map<uint32_t, double> ChipGeomFactors;
    std::map<uint32_t, size_t> iPU;

    // constants for ROC level simulation for Phase1
    enum shiftEnumerator { FPixRocIdShift = 3, BPixRocIdShift = 6 };
    static const int rocIdMaskBits = 0x1F;
    void init_from_db(const TrackerGeometry*, const SiPixelDynamicInefficiency*);
    bool matches(const DetId&, const DetId&, const std::vector<uint32_t>&);
    std::unique_ptr<PixelFEDChannelCollection> PixelFEDChannelCollection_;
  };

  //
  // PixelAging struct
  //
  /**
    * Internal use only.
    */
  struct PixelAging {
    PixelAging(const edm::ParameterSet& conf, bool AddPixelAging, int NumberOfBarrelLayers, int NumberOfEndcapDisks);
    float thePixelPseudoRadDamage[20];  // PseudoRadiation Damage Values for aging studies
    unsigned int FPixIndex;             // The Efficiency index for FPix Disks
  };

private:
  // Internal typedefs
  typedef std::map<int, digitizerUtility::Amplitude, std::less<int> > signal_map_type;  // from Digi.Skel.
  typedef signal_map_type::iterator signal_map_iterator;                                // from Digi.Skel.
  typedef signal_map_type::const_iterator signal_map_const_iterator;                    // from Digi.Skel.
  typedef std::map<uint32_t, signal_map_type> signalMaps;
  typedef GloballyPositioned<double> Frame;
  typedef std::vector<edm::ParameterSet> Parameters;
  typedef boost::multi_array<float, 2> array_2d;

  typedef std::pair<unsigned int, unsigned int> subDetTofBin;
  typedef std::map<unsigned int, std::vector<PSimHit> > simhit_map;
  simhit_map SimHitMap;
  typedef std::map<subDetTofBin, unsigned int> simhit_collectionMap;
  simhit_collectionMap SimHitCollMap;

  // Contains the accumulated hit info.
  signalMaps _signal;

  const bool makeDigiSimLinks_;
  const bool store_SimHitEntryExitPoints_;
  const bool store_SimHitEntryExitPointsLite_;

  const bool use_ineff_from_db_;
  const bool use_module_killing_;   // remove or not the dead pixel modules
  const bool use_deadmodule_DB_;    // if we want to get dead pixel modules from the DataBase.
  const bool use_LorentzAngle_DB_;  // if we want to get Lorentz angle from the DataBase.

  const Parameters DeadModules;

  std::unique_ptr<SiPixelChargeReweightingAlgorithm> TheNewSiPixelChargeReweightingAlgorithmClass;

private:
  // Variables
  //external parameters
  // go from Geant energy GeV to number of electrons
  const float GeVperElectron;  // 3.7E-09

  //-- drift
  const float Sigma0;      //=0.0007  // Charge diffusion in microns for 300 micron Si
  const float Dist300;     //=0.0300  // Define 300microns for normalization
  const bool alpha2Order;  // Switch on/off of E.B effect

  //-- induce_signal
  const float ClusterWidth;  // Gaussian charge cutoff width in sigma units
  //-- Allow for upgrades
  const int NumberOfBarrelLayers;  // Default = 3
  const int NumberOfEndcapDisks;   // Default = 2

  //-- make_digis
  const float theElectronPerADC;    // Gain, number of electrons per adc count.
  const int theAdcFullScale;        // Saturation count, 255=8bit.
  const int theAdcFullScLateCR;     // Saturation count, 255=8bit.
  const float theNoiseInElectrons;  // Noise (RMS) in units of electrons.
  const float theReadoutNoise;      // Noise of the readount chain in elec,
                                    //inludes DCOL-Amp,TBM-Amp, Alt, AOH,OptRec.

  const float theThresholdInE_FPix;     // Pixel threshold in electrons FPix.
  const float theThresholdInE_BPix;     // Pixel threshold in electrons BPix.
  const float theThresholdInE_BPix_L1;  // In case the BPix layer1 gets a different threshold
  const float theThresholdInE_BPix_L2;  // In case the BPix layer2 gets a different threshold

  const double theThresholdSmearing_FPix;
  const double theThresholdSmearing_BPix;
  const double theThresholdSmearing_BPix_L1;
  const double theThresholdSmearing_BPix_L2;

  const float electronsPerVCAL;            // for electrons - VCAL conversion
  const float electronsPerVCAL_Offset;     // in misscalibrate()
  const float electronsPerVCAL_L1;         // same for Layer 1
  const float electronsPerVCAL_L1_Offset;  // same for Layer 1

  const float theTofLowerCut;                // Cut on the particle TOF
  const float theTofUpperCut;                // Cut on the particle TOF
  const float tanLorentzAnglePerTesla_FPix;  //FPix Lorentz angle tangent per Tesla
  const float tanLorentzAnglePerTesla_BPix;  //BPix Lorentz angle tangent per Tesla

  const float FPix_p0;
  const float FPix_p1;
  const float FPix_p2;
  const float FPix_p3;
  const float BPix_p0;
  const float BPix_p1;
  const float BPix_p2;
  const float BPix_p3;

  //-- add_noise
  const bool addNoise;
  const bool addChargeVCALSmearing;
  const bool addNoisyPixels;
  const bool fluctuateCharge;

  //-- pixel efficiency
  const bool AddPixelInefficiency;  // bool to read in inefficiencies
  const bool KillBadFEDChannels;
  const bool addThresholdSmearing;

  //-- calibration smearing
  const bool doMissCalibrate;     // Switch on the calibration smearing
  const bool doMissCalInLateCR;   // Switch on the calibration smearing
  const float theGainSmearing;    // The sigma of the gain fluctuation (around 1)
  const float theOffsetSmearing;  // The sigma of the offset fluct. (around 0)

  // pixel aging
  const bool AddPixelAging;
  const bool UseReweighting;

  // The PDTable
  //HepPDTable *particleTable;
  //ParticleDataTable *particleTable;

  //-- charge fluctuation
  const double tMax;  // The delta production cut, should be as in OSCAR = 30keV
  //                                           cmsim = 100keV

  // The eloss fluctuation class from G4. Is the right place?
  const std::unique_ptr<SiG4UniversalFluctuation> fluctuate;  // make a pointer
  const std::unique_ptr<GaussianTailNoiseGenerator> theNoiser;

  // To store calibration constants
  const std::map<int, CalParameters, std::less<int> > calmap;

  //-- additional member functions
  // Private methods
  std::map<int, CalParameters, std::less<int> > initCal() const;
  void primary_ionization(const PSimHit& hit,
                          std::vector<EnergyDepositUnit>& ionization_points,
                          CLHEP::HepRandomEngine*) const;
  void drift(const PSimHit& hit,
             const PixelGeomDetUnit* pixdet,
             const GlobalVector& bfield,
             const TrackerTopology* tTopo,
             const std::vector<EnergyDepositUnit>& ionization_points,
             std::vector<SignalPoint>& collection_points) const;
  void induce_signal(std::vector<PSimHit>::const_iterator inputBegin,
                     std::vector<PSimHit>::const_iterator inputEnd,
                     const PSimHit& hit,
                     const size_t hitIndex,
                     const size_t FirstHitIndex,
                     const unsigned int tofBin,
                     const PixelGeomDetUnit* pixdet,
                     const std::vector<SignalPoint>& collection_points);
  void fluctuateEloss(int particleId,
                      float momentum,
                      float eloss,
                      float length,
                      int NumberOfSegments,
                      float elossVector[],
                      CLHEP::HepRandomEngine*) const;
  void add_noise(const PixelGeomDetUnit* pixdet, float thePixelThreshold, CLHEP::HepRandomEngine*);
  void make_digis(float thePixelThresholdInE,
                  uint32_t detID,
                  const PixelGeomDetUnit* pixdet,
                  std::vector<PixelDigi>& digis,
                  std::vector<PixelDigiSimLink>& simlinks,
                  std::vector<PixelDigiAddTempInfo>& newClass_Digi_extra,
                  const TrackerTopology* tTopo) const;
  void pixel_inefficiency(const PixelEfficiencies& eff,
                          const PixelGeomDetUnit* pixdet,
                          const TrackerTopology* tTopo,
                          CLHEP::HepRandomEngine*);

  void pixel_inefficiency_db(uint32_t detID);

  float pixel_aging(const PixelAging& aging, const PixelGeomDetUnit* pixdet, const TrackerTopology* tTopo) const;

  // access to the gain calibration payloads in the db. Only gets initialized if check_dead_pixels_ is set to true.
  const std::unique_ptr<SiPixelGainCalibrationOfflineSimService> theSiPixelGainCalibrationService_;
  float missCalibrate(
      uint32_t detID, const TrackerTopology* tTopo, const PixelGeomDetUnit* pixdet, int col, int row, float amp) const;
  LocalVector DriftDirection(const PixelGeomDetUnit* pixdet, const GlobalVector& bfield, const DetId& detId) const;

  void module_killing_conf(
      uint32_t detID);  // remove dead modules using the list in the configuration file PixelDigi_cfi.py
  void module_killing_DB(uint32_t detID);  // remove dead modules uisng the list in the DB

  PixelEfficiencies pixelEfficiencies_;
  const PixelAging pixelAging_;

  double calcQ(float x) const {
    // need erf(x/sqrt2)
    //float x2=0.5*x*x;
    //float a=0.147;
    //double erf=sqrt(1.0f-exp( -1.0f*x2*( (4/M_PI)+a*x2)/(1.0+a*x2)));
    //if (x<0.) erf*=-1.0;
    //return 0.5*(1.0-erf);

    auto xx = std::min(0.5f * x * x, 12.5f);
    return 0.5 * (1.0 - std::copysign(std::sqrt(1.f - unsafe_expf<4>(-xx * (1.f + 0.2733f / (1.f + 0.147f * xx)))), x));
  }
};

#endif
