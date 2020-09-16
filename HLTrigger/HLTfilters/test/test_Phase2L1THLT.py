import FWCore.ParameterSet.Config as cms

process = cms.Process("HLTX")

### Load all ESSources, ESProducers and PSets
#process.load("HLTrigger.Configuration.Phase2.hltPhase2Setup_cff")

### GlobalTag
#process.load("Configuration.StandardSequences.CondDBESSource_cff")
#process.GlobalTag.globaltag = "111X_mcRun4_realistic_T15_v1"

process.load("CondCore.CondDB.CondDB_cfi")
# input database (in this case the local sqlite file)
process.CondDB.connect = 'sqlite_file:L1TObjScaling.db'

process.L1TkMuonScalingESSource = cms.ESSource("PoolDBESSource",
    process.CondDB,
    DumpStat=cms.untracked.bool(True),
    toGet = cms.VPSet(
        cms.PSet(
            record = cms.string('L1TObjScalingRcd'),
            tag = cms.string("L1TkMuonScaling")
        )
    )
                                   )

### Don't rerun the module that makes TrackTrigger tracks
# process.TTTracksFromTrackletEmulation = cms.EDProducer

process.offlineBeamSpot = cms.EDProducer("BeamSpotProducer")

process.l1tMuon5 = cms.EDFilter(
    "L1TkMuonFilter",
    MinPt = cms.double(5.0),
    )

process.hltTestSeq = cms.Sequence(
    process.l1tMuon5
)

process.HLT_Test_Path = cms.Path(process.hltTestSeq)

process.source = cms.Source(
    "PoolSource",
    fileNames=cms.untracked.vstring(
        "/store/mc/Phase2HLTTDRSummer20ReRECOMiniAOD/DYToLL_M-50_TuneCP5_14TeV-pythia8/FEVT/PU200_pilot_111X_mcRun4_realistic_T15_v1-v1/270000/FF7BF0E2-1380-2D48-BB19-F79E6907CD5D.root",
    ),
)

process.maxEvents.input = cms.untracked.int32(100)
process.options = cms.untracked.PSet(wantSummary = cms.untracked.bool(True))
