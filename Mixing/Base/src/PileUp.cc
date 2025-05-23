#include "Mixing/Base/interface/PileUp.h"
#include "DataFormats/Provenance/interface/BranchIDListHelper.h"
#include "DataFormats/Provenance/interface/ModuleDescription.h"
#include "DataFormats/Provenance/interface/ThinnedAssociationsHelper.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/SignallingProductRegistryFiller.h"
#include "FWCore/Framework/interface/ESRecordsToProductResolverIndices.h"
#include "FWCore/Framework/interface/ProductResolversFactory.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"
#include "FWCore/ServiceRegistry/interface/GlobalContext.h"
#include "FWCore/ServiceRegistry/interface/ProcessContext.h"
#include "FWCore/Sources/interface/VectorInputSourceDescription.h"
#include "FWCore/Sources/interface/VectorInputSourceFactory.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "FWCore/Version/interface/GetReleaseVersion.h"

#include "FWCore/AbstractServices/interface/RandomNumberGenerator.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "Mixing/Base/src/SecondaryEventProvider.h"
#include "CondFormats/DataRecord/interface/MixingRcd.h"
#include "CondFormats/RunInfo/interface/MixingModuleConfig.h"

#include "CLHEP/Random/RandPoissonQ.h"
#include "CLHEP/Random/RandPoisson.h"

#include "PileupRandomNumberGenerator.h"

#include <algorithm>
#include <memory>
#include "TMath.h"

////////////////////////////////////////////////////////////////////////////////
/// return a random number distributed according the histogram bin contents.
///
/// This routine is derived from root/hist/hist/src/TH1.cxx
/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

static Double_t GetRandom(TH1* th1, CLHEP::HepRandomEngine* rng) {
  Int_t nbinsx = th1->GetNbinsX();
  Double_t* fIntegral = th1->GetIntegral();
  Double_t integral = fIntegral[nbinsx];

  if (integral == 0)
    return 0;

  Double_t r1 = rng->flat();
  Int_t ibin = TMath::BinarySearch(nbinsx, fIntegral, r1);
  Double_t x = th1->GetBinLowEdge(ibin + 1);
  if (r1 > fIntegral[ibin])
    x += th1->GetBinWidth(ibin + 1) * (r1 - fIntegral[ibin]) / (fIntegral[ibin + 1] - fIntegral[ibin]);
  return x;
}
////////////////////////////////////////////////////////////////////////////////

namespace edm {
  PileUp::PileUp(ParameterSet const& pset,
                 const std::shared_ptr<PileUpConfig>& config,
                 edm::ConsumesCollector iC,
                 const bool mixingConfigFromDB)
      : type_(pset.getParameter<std::string>("type")),
        Source_type_(config->sourcename_),
        averageNumber_(config->averageNumber_),
        intAverage_(static_cast<int>(averageNumber_)),
        histo_(std::make_shared<TH1F>(*config->histo_)),
        histoDistribution_(type_ == "histo"),
        probFunctionDistribution_(type_ == "probFunction"),
        poisson_(type_ == "poisson"),
        fixed_(type_ == "fixed"),
        none_(type_ == "none"),
        fileNameHash_(0U),
        productRegistry_(),
        input_(VectorInputSourceFactory::get()->makeVectorInputSource(
            pset,
            VectorInputSourceDescription(std::make_shared<edm::ProductRegistry>(), edm::PreallocationConfiguration()))),
        // hardware information is not needed for the "overlay"
        processConfiguration_(std::make_shared<ProcessConfiguration>(
            "@MIXING", getReleaseVersion(), edm::HardwareResourcesDescription())),
        processContext_(new ProcessContext()),
        eventPrincipal_(),
        lumiPrincipal_(),
        runPrincipal_(),
        provider_(),
        PoissonDistribution_(),
        PoissonDistr_OOT_(),
        randomEngine_(),
        playback_(config->playback_),
        sequential_(pset.getUntrackedParameter<bool>("sequential", false)) {
    if (mixingConfigFromDB) {
      configToken_ = iC.esConsumes<edm::Transition::BeginLuminosityBlock>();
    }

    // Use the empty parameter set for the parameter set ID of our "@MIXING" process.
    processConfiguration_->setParameterSetID(ParameterSet::emptyParameterSetID());
    processContext_->setProcessConfiguration(processConfiguration_.get());

    SignallingProductRegistryFiller filler;
    if (pset.existsAs<std::vector<ParameterSet> >("producers", true)) {
      std::vector<ParameterSet> producers = pset.getParameter<std::vector<ParameterSet> >("producers");

      std::vector<std::string> names;
      names.reserve(producers.size());
      std::transform(producers.begin(), producers.end(), std::back_inserter(names), [](edm::ParameterSet const& iPSet) {
        return iPSet.getParameter<std::string>("@module_label");
      });
      auto randomGenerator = std::make_unique<PileupRandomNumberGenerator>(names);
      randomGenerator_ = randomGenerator.get();
      std::unique_ptr<edm::RandomNumberGenerator> baseGen = std::move(randomGenerator);
      serviceToken_ = edm::ServiceRegistry::createContaining(
          std::move(baseGen), edm::ServiceRegistry::instance().presentToken(), true);

      provider_ = std::make_unique<SecondaryEventProvider>(producers, filler, processConfiguration_);
    }
    filler.addFromInput(*input_->productRegistry());
    filler.setFrozen();
    productRegistry_ = std::make_shared<ProductRegistry>(filler.moveTo());

    // A modified HistoryAppender must be used for unscheduled processing.
    eventPrincipal_ = std::make_unique<EventPrincipal>(productRegistry_,
                                                       edm::productResolversFactory::makePrimary,
                                                       std::make_shared<BranchIDListHelper>(),
                                                       std::make_shared<ThinnedAssociationsHelper>(),
                                                       *processConfiguration_,
                                                       nullptr);

    bool DB = type_ == "readDB";

    if (pset.exists("nbPileupEvents")) {
      if (0 != pset.getParameter<edm::ParameterSet>("nbPileupEvents").getUntrackedParameter<int>("seed", 0)) {
        edm::LogWarning("MixingModule") << "Parameter nbPileupEvents.seed is not supported";
      }
    }

    edm::Service<edm::RandomNumberGenerator> rng;
    if (!rng.isAvailable()) {
      throw cms::Exception("Configuration")
          << "PileUp requires the RandomNumberGeneratorService\n"
             "which is not present in the configuration file.  You must add the service\n"
             "in the configuration file or remove the modules that require it.";
    }

    if (!(histoDistribution_ || probFunctionDistribution_ || poisson_ || fixed_ || none_) && !DB) {
      throw cms::Exception("Illegal parameter value", "PileUp::PileUp(ParameterSet const& pset)")
          << "'type' parameter (a string) has a value of '" << type_ << "'.\n"
          << "Legal values are 'poisson', 'fixed', or 'none'\n";
    }

    if (!DB) {
      manage_OOT_ = pset.getUntrackedParameter<bool>("manage_OOT", false);

      // Check for string describing special processing.  Add these here for individual cases
      PU_Study_ = false;
      Study_type_ = pset.getUntrackedParameter<std::string>("Special_Pileup_Studies", "");

      if (Study_type_ == "Fixed_ITPU_Vary_OOTPU") {
        PU_Study_ = true;
        intFixed_ITPU_ = pset.getUntrackedParameter<int>("intFixed_ITPU", 0);
      }

      if (manage_OOT_) {  // figure out what the parameters are

        //      if (playback_) throw cms::Exception("Illegal parameter clash","PileUp::PileUp(ParameterSet const& pset)")
        // << " manage_OOT option not allowed with playback ";

        std::string OOT_type = pset.getUntrackedParameter<std::string>("OOT_type");

        if (OOT_type == "Poisson" || OOT_type == "poisson") {
          poisson_OOT_ = true;
        } else if (OOT_type == "Fixed" || OOT_type == "fixed") {
          fixed_OOT_ = true;
          // read back the fixed number requested out-of-time
          intFixed_OOT_ = pset.getUntrackedParameter<int>("intFixed_OOT", -1);
          if (intFixed_OOT_ < 0) {
            throw cms::Exception("Illegal parameter value", "PileUp::PileUp(ParameterSet const& pset)")
                << " Fixed out-of-time pileup requested, but no fixed value given ";
          }
        } else {
          throw cms::Exception("Illegal parameter value", "PileUp::PileUp(ParameterSet const& pset)")
              << "'OOT_type' parameter (a string) has a value of '" << OOT_type << "'.\n"
              << "Legal values are 'poisson' or 'fixed'\n";
        }
        edm::LogInfo("MixingModule") << " Out-of-time pileup will be generated with a " << OOT_type
                                     << " distribution. ";
      }
    }

    if (Source_type_ == "cosmics") {  // allow for some extra flexibility for mixing
      minBunch_cosmics_ = pset.getUntrackedParameter<int>("minBunch_cosmics", -1000);
      maxBunch_cosmics_ = pset.getUntrackedParameter<int>("maxBunch_cosmics", 1000);
    }
  }  // end of constructor

  void PileUp::beginJob(eventsetup::ESRecordsToProductResolverIndices const& iES) {
    input_->doBeginJob();
    if (provider_.get() != nullptr) {
      edm::ServiceRegistry::Operate guard(*serviceToken_);
      GlobalContext globalContext(GlobalContext::Transition::kBeginJob, processContext_.get());
      provider_->beginJob(*productRegistry_, iES, globalContext);
    }
  }

  void PileUp::beginStream(edm::StreamID) {
    auto iID = eventPrincipal_->streamID();  // each producer has its own workermanager, so use default streamid
    streamContext_ = std::make_shared<StreamContext>(iID, processContext_.get());
    streamContext_->setTransition(StreamContext::Transition::kBeginStream);
    if (provider_.get() != nullptr) {
      edm::ServiceRegistry::Operate guard(*serviceToken_);
      provider_->beginStream(iID, *streamContext_);
    }
  }

  void PileUp::endStream() {
    ExceptionCollector exceptionCollector(
        "Multiple exceptions were thrown while executing PileUp::endStream. An exception message follows for "
        "each.\n");
    endStream(exceptionCollector);

    if (exceptionCollector.hasThrown()) {
      exceptionCollector.rethrow();
    }
  }

  void PileUp::endStream(ExceptionCollector& exceptionCollector) {
    if (provider_.get() != nullptr) {
      edm::ServiceRegistry::Operate guard(*serviceToken_);
      streamContext_->setTransition(StreamContext::Transition::kEndStream);
      provider_->endStream(streamContext_->streamID(), *streamContext_, exceptionCollector);
      // This is kind of strange, end of job running as part of endStream multiple times...
      // For the moment, I'm leaving this as is but maybe we should think about this...
      // I think nothing uses this code anymore anyway...
      GlobalContext globalContext(GlobalContext::Transition::kEndJob, processContext_.get());
      provider_->endJob(exceptionCollector, globalContext);
    }
    input_->doEndJob();
  }

  void PileUp::beginRun(const edm::Run& run, const edm::EventSetup& setup) {
    if (provider_.get() != nullptr) {
      runPrincipal_ = std::make_shared<RunPrincipal>(
          productRegistry_, edm::productResolversFactory::makePrimary, *processConfiguration_, nullptr, 0);
      runPrincipal_->setAux(run.runAuxiliary());
      edm::ServiceRegistry::Operate guard(*serviceToken_);
      streamContext_->setTransition(StreamContext::Transition::kBeginRun);
      provider_->beginRun(*runPrincipal_, setup.impl(), run.moduleCallingContext(), *streamContext_);
    }
  }
  void PileUp::beginLuminosityBlock(const edm::LuminosityBlock& lumi, const edm::EventSetup& setup) {
    if (provider_.get() != nullptr) {
      lumiPrincipal_ = std::make_shared<LuminosityBlockPrincipal>(
          productRegistry_, edm::productResolversFactory::makePrimary, *processConfiguration_, nullptr, 0);
      lumiPrincipal_->setAux(lumi.luminosityBlockAuxiliary());
      lumiPrincipal_->setRunPrincipal(runPrincipal_);
      setRandomEngine(lumi);
      edm::ServiceRegistry::Operate guard(*serviceToken_);
      streamContext_->setTransition(StreamContext::Transition::kBeginLuminosityBlock);
      provider_->beginLuminosityBlock(*lumiPrincipal_, setup.impl(), lumi.moduleCallingContext(), *streamContext_);
    }
  }

  void PileUp::endRun(const edm::Run& run, const edm::EventSetup& setup) {
    if (provider_.get() != nullptr) {
      edm::ServiceRegistry::Operate guard(*serviceToken_);
      streamContext_->setTransition(StreamContext::Transition::kEndRun);
      provider_->endRun(*runPrincipal_, setup.impl(), run.moduleCallingContext(), *streamContext_);
    }
  }
  void PileUp::endLuminosityBlock(const edm::LuminosityBlock& lumi, const edm::EventSetup& setup) {
    if (provider_.get() != nullptr) {
      edm::ServiceRegistry::Operate guard(*serviceToken_);
      streamContext_->setTransition(StreamContext::Transition::kEndLuminosityBlock);
      provider_->endLuminosityBlock(*lumiPrincipal_, setup.impl(), lumi.moduleCallingContext(), *streamContext_);
    }
  }

  void PileUp::setupPileUpEvent(const edm::EventSetup& setup) {
    if (provider_.get() != nullptr) {
      // note:  run and lumi numbers must be modified to match lumiPrincipal_
      eventPrincipal_->setLuminosityBlockPrincipal(lumiPrincipal_.get());
      eventPrincipal_->setRunAndLumiNumber(lumiPrincipal_->run(), lumiPrincipal_->luminosityBlock());
      edm::ServiceRegistry::Operate guard(*serviceToken_);
      streamContext_->setTransition(StreamContext::Transition::kEvent);
      provider_->setupPileUpEvent(*eventPrincipal_, setup.impl(), *streamContext_);
    }
  }

  void PileUp::reload(const edm::EventSetup& setup) {
    //get the required parameters from DB.
    const MixingInputConfig& config = setup.getData(configToken_).config(inputType_);

    //get the type
    type_ = config.type();
    //set booleans
    histoDistribution_ = type_ == "histo";
    probFunctionDistribution_ = type_ == "probFunction";
    poisson_ = type_ == "poisson";
    fixed_ = type_ == "fixed";
    none_ = type_ == "none";

    if (histoDistribution_)
      edm::LogError("MisConfiguration") << "type histo cannot be reloaded from DB, yet";

    if (fixed_) {
      averageNumber_ = averageNumber();
    } else if (poisson_) {
      averageNumber_ = config.averageNumber();
      if (PoissonDistribution_) {
        PoissonDistribution_ = std::make_unique<CLHEP::RandPoissonQ>(PoissonDistribution_->engine(), averageNumber_);
      }
    } else if (probFunctionDistribution_) {
      //need to reload the histogram from DB
      const std::vector<int>& dataProbFunctionVar = config.probFunctionVariable();
      std::vector<double> dataProb = config.probValue();

      int varSize = (int)dataProbFunctionVar.size();
      int probSize = (int)dataProb.size();

      if ((dataProbFunctionVar[0] != 0) || (dataProbFunctionVar[varSize - 1] != (varSize - 1)))
        throw cms::Exception("BadProbFunction")
            << "Please, check the variables of the probability function! The first variable should be 0 and the "
               "difference between two variables should be 1."
            << std::endl;

      // Complete the vector containing the probability  function data
      // with the values "0"
      if (probSize < varSize) {
        edm::LogWarning("MixingModule") << " The probability function data will be completed with "
                                        << (varSize - probSize) << " values 0.";

        for (int i = 0; i < (varSize - probSize); i++)
          dataProb.push_back(0);

        probSize = dataProb.size();
        edm::LogInfo("MixingModule") << " The number of the P(x) data set after adding the values 0 is " << probSize;
      }

      // Create an histogram with the data from the probability function provided by the user
      int xmin = (int)dataProbFunctionVar[0];
      int xmax = (int)dataProbFunctionVar[varSize - 1] + 1;  // need upper edge to be one beyond last value
      int numBins = varSize;

      edm::LogInfo("MixingModule") << "An histogram will be created with " << numBins << " bins in the range (" << xmin
                                   << "," << xmax << ")." << std::endl;

      histo_ = std::make_shared<TH1F>("h", "Histo from the user's probability function", numBins, xmin, xmax);

      LogDebug("MixingModule") << "Filling histogram with the following data:" << std::endl;

      for (int j = 0; j < numBins; j++) {
        LogDebug("MixingModule") << " x = " << dataProbFunctionVar[j] << " P(x) = " << dataProb[j];
        histo_->Fill(dataProbFunctionVar[j] + 0.5,
                     dataProb[j]);  // assuming integer values for the bins, fill bin centers, not edges
      }

      // Check if the histogram is normalized
      if (std::abs(histo_->Integral() - 1) > 1.0e-02) {
        throw cms::Exception("BadProbFunction") << "The probability function should be normalized!!! " << std::endl;
      }
      averageNumber_ = histo_->GetMean();
    }

    int oot = config.outOfTime();
    manage_OOT_ = false;
    if (oot == 1) {
      manage_OOT_ = true;
      poisson_OOT_ = false;
      fixed_OOT_ = true;
      intFixed_OOT_ = config.fixedOutOfTime();
    } else if (oot == 2) {
      manage_OOT_ = true;
      poisson_OOT_ = true;
      fixed_OOT_ = false;
    }
  }
  PileUp::~PileUp() {}

  std::unique_ptr<CLHEP::RandPoissonQ> const& PileUp::poissonDistribution(StreamID const& streamID) {
    if (!PoissonDistribution_) {
      CLHEP::HepRandomEngine& engine = *randomEngine(streamID);
      PoissonDistribution_ = std::make_unique<CLHEP::RandPoissonQ>(engine, averageNumber_);
    }
    return PoissonDistribution_;
  }

  std::unique_ptr<CLHEP::RandPoisson> const& PileUp::poissonDistr_OOT(StreamID const& streamID) {
    if (!PoissonDistr_OOT_) {
      CLHEP::HepRandomEngine& engine = *randomEngine(streamID);
      PoissonDistr_OOT_ = std::make_unique<CLHEP::RandPoisson>(engine);
    }
    return PoissonDistr_OOT_;
  }

  CLHEP::HepRandomEngine* PileUp::randomEngine(StreamID const& streamID) {
    if (!randomEngine_) {
      Service<RandomNumberGenerator> rng;
      randomEngine_ = &rng->getEngine(streamID);
    }
    return randomEngine_;
  }

  void PileUp::setRandomEngine(StreamID streamID) { randomGenerator_->setEngine(*randomEngine(streamID)); }
  void PileUp::setRandomEngine(LuminosityBlock const& iLumi) {
    Service<RandomNumberGenerator> rng;
    randomGenerator_->setSeed(rng->mySeed());
    randomGenerator_->setEngine(rng->getEngine(iLumi.index()));
  }

  void PileUp::CalculatePileup(int MinBunch,
                               int MaxBunch,
                               std::vector<int>& PileupSelection,
                               std::vector<float>& TrueNumInteractions,
                               StreamID const& streamID) {
    // if we are managing the distribution of out-of-time pileup separately, select the distribution for bunch
    // crossing zero first, save it for later.

    int nzero_crossing = -1;
    double Fnzero_crossing = -1;

    if (provider_) {
      setRandomEngine(streamID);
    }

    if (manage_OOT_) {
      if (none_) {
        nzero_crossing = 0;
      } else if (poisson_) {
        nzero_crossing = poissonDistribution(streamID)->fire();
      } else if (fixed_) {
        nzero_crossing = intAverage_;
      } else if (histoDistribution_ || probFunctionDistribution_) {
        // RANDOM_NUMBER_ERROR
        // Random number should be generated by the engines from the
        // RandomNumberGeneratorService. This appears to use the global
        // engine in ROOT. This is not thread safe unless the module using
        // it is a one module and declares a shared resource and all
        // other modules using it also declare the same shared resource.
        // This also breaks replay.
        double d = GetRandom(histo_.get(), randomEngine(streamID));
        //n = (int) floor(d + 0.5);  // incorrect for bins with integer edges
        Fnzero_crossing = d;
        nzero_crossing = int(d);
      }
    }

    for (int bx = MinBunch; bx < MaxBunch + 1; ++bx) {
      if (manage_OOT_) {
        if (bx == 0 && !poisson_OOT_) {
          PileupSelection.push_back(nzero_crossing);
          TrueNumInteractions.push_back(nzero_crossing);
        } else {
          if (poisson_OOT_) {
            if (PU_Study_ && (Study_type_ == "Fixed_ITPU_Vary_OOTPU") && bx == 0) {
              PileupSelection.push_back(intFixed_ITPU_);
            } else {
              PileupSelection.push_back(poissonDistr_OOT(streamID)->fire(Fnzero_crossing));
            }
            TrueNumInteractions.push_back(Fnzero_crossing);
          } else {
            PileupSelection.push_back(intFixed_OOT_);
            TrueNumInteractions.push_back(intFixed_OOT_);
          }
        }
      } else {
        if (none_) {
          PileupSelection.push_back(0);
          TrueNumInteractions.push_back(0.);
        } else if (poisson_) {
          PileupSelection.push_back(poissonDistribution(streamID)->fire());
          TrueNumInteractions.push_back(averageNumber_);
        } else if (fixed_) {
          PileupSelection.push_back(intAverage_);
          TrueNumInteractions.push_back(intAverage_);
        } else if (histoDistribution_ || probFunctionDistribution_) {
          // RANDOM_NUMBER_ERROR
          // Random number should be generated by the engines from the
          // RandomNumberGeneratorService. This appears to use the global
          // engine in ROOT. This is not thread safe unless the module using
          // it is a one module and declares a shared resource and all
          // other modules using it also declare the same shared resource.
          // This also breaks replay.
          double d = GetRandom(histo_.get(), randomEngine(streamID));
          PileupSelection.push_back(int(d));
          TrueNumInteractions.push_back(d);
        }
      }
    }
  }

}  //namespace edm
