###############################################################################
# Way to use this:
#   cmsRun SimRun4_cfg.py geometry=D110 type=DDD data=Muon
#
#   Options for geometry: D110, D120
#               type: DDD, DD4hep
#               data: Muon, MinBias
#
###############################################################################
import FWCore.ParameterSet.Config as cms
import os, sys, importlib, re, random
import FWCore.ParameterSet.VarParsing as VarParsing

####################################################################
### SETUP OPTIONS
options = VarParsing.VarParsing('standard')
options.register('geometry',
                 "D110",
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.string,
                  "geometry of operations: D110, D120")
options.register('type',
                 "DDD",
                  VarParsing.VarParsing.multiplicity.singleton,
                  VarParsing.VarParsing.varType.string,
                  "type of operations: DDD, DD4hep")
options.register('data',
                 "Muon",
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 "data of operations: Muon, MinBias")

### get and parse the command line arguments
options.parseArguments()

print(options)

####################################################################
# Use the options

geomName = "Run4" + options.geometry
import Configuration.Geometry.defaultPhase2ConditionsEra_cff as _settings
GLOBAL_TAG, ERA = _settings.get_era_and_conditions(geomName)

if (options.type == "DD4hep"):
    from Configuration.ProcessModifiers.dd4hep_cff import dd4hep
    process = cms.Process('SimRun4',ERA,dd4hep)
    geomFile = "Configuration.Geometry.Geometry" + options.type +"Extended" + geomName + "Reco_cff"
else:
    process = cms.Process('SimRun4',ERA)
    geomFile = "Configuration.Geometry.GeometryExtended" + geomName + "Reco_cff"

inFile = "file:step0" + options.data + ".root"
outFile = "file:step1" + options.type + options.geometry + options.data + ".root"
tFile = "file:" + options.type + options.geometry + options.data + ".root"

print("Geometry file:   ", geomFile)
print("Global Tag Name: ", GLOBAL_TAG)
print("Era Name:        ", ERA)
print("Input file:      ", inFile)
print("Output file:     ", outFile)
print("Histogram file:  ", tFile)

process.load(geomFile)
process.load("SimGeneral.HepPDTESSource.pythiapdt_cfi")
process.load("FWCore.MessageService.MessageLogger_cfi")
process.load("IOMC.EventVertexGenerators.VtxSmearedGauss_cfi")
process.load("Configuration.StandardSequences.MagneticField_cff")
process.load("Configuration.EventContent.EventContent_cff")
process.load("Configuration.StandardSequences.SimIdeal_cff")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, GLOBAL_TAG, '')

process.source = cms.Source("PoolSource",
    dropDescendantsOfDroppedBranches = cms.untracked.bool(False),
    fileNames = cms.untracked.vstring(inFile),
    secondaryFileNames = cms.untracked.vstring()
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1),
    output = cms.optional.untracked.allowed(cms.int32,cms.PSet)
)

if 'MessageLogger' in process.__dict__:
    process.MessageLogger.G4cerr=dict()
    process.MessageLogger.HitStudy=dict()
    process.MessageLogger.HGCSim=dict()
#   process.MessageLogger.SensitiveDetector=dict()

process.Timing = cms.Service("Timing")

process.load("IOMC.RandomEngine.IOMC_cff")
process.RandomNumberGeneratorService.generator.initialSeed = 456789
process.RandomNumberGeneratorService.g4SimHits.initialSeed = 9876
process.RandomNumberGeneratorService.VtxSmeared.initialSeed = 123456789
process.rndmStore = cms.EDProducer("RandomEngineStateProducer")

# Event output
process.output = cms.OutputModule("PoolOutputModule",
    process.FEVTSIMEventContent,
    fileName = cms.untracked.string(outFile)
)

process.load("SimG4CMS.Calo.hgcalHitCheck_cff")
process.TFileService = cms.Service("TFileService",
                                   fileName = cms.string(tFile)
)

process.simulation_step = cms.Path(process.psim)
process.out_step = cms.EndPath(process.output)
process.analysis_step1 = cms.EndPath(process.hgcalHitCheckEE)
process.analysis_step2 = cms.EndPath(process.hgcalHitCheckHEF)
process.analysis_step3 = cms.EndPath(process.hgcalHitCheckHEB)

process.g4SimHits.Physics.type = 'SimG4Core/Physics/FTFP_BERT_EMM'
process.g4SimHits.HGCSD.Verbosity = 0

# Schedule definition
process.schedule = cms.Schedule(process.simulation_step,
                                process.out_step,
                                process.analysis_step1,
                                process.analysis_step2,
                                process.analysis_step3,
                                )
