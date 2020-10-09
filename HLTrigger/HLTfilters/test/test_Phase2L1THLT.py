import FWCore.ParameterSet.Config as cms

process = cms.Process("HLTX")

### Load all ESSources, ESProducers and PSets
# process.load("HLTrigger.Configuration.Phase2.hltPhase2Setup_cff")

### GlobalTag
process.load("Configuration.StandardSequences.CondDBESSource_cff")
process.GlobalTag.globaltag = "111X_mcRun4_realistic_T15_v2"

# input database (in this case the local sqlite file)
process.load("CondCore.CondDB.CondDB_cfi")
process.CondDB.connect = "sqlite_file:L1TObjScaling.db"

process.L1TScalingESSource = cms.ESSource(
    "PoolDBESSource",
    process.CondDB,
    DumpStat=cms.untracked.bool(True),
    toGet=cms.VPSet(
        cms.PSet(
            record=cms.string("L1TObjScalingRcd"),
            tag=cms.string("L1TkMuonScaling"),
            label=cms.untracked.string("L1TkMuonScaling"),
        ),
        cms.PSet(
            record=cms.string("L1TObjScalingRcd"),
            tag=cms.string("L1PFJetScaling"),
            label=cms.untracked.string("L1PFJetScaling"),
        ),
        cms.PSet(
            record=cms.string("L1TObjScalingRcd"),
            tag=cms.string("L1TkElectronScaling"),
            label=cms.untracked.string("L1TkEleScaling"),
        ),
    ),
)
process.es_prefer_l1tscaling = cms.ESPrefer("PoolDBESSource", "L1TScalingESSource")

process.l1tMuon7 = cms.EDFilter(
    "L1TkMuonFilter",
    MinPt=cms.double(7.0),
)

process.l1tEle7 = cms.EDFilter(
    "L1TkEleFilter",
    MinPt=cms.double(7.0),
    MinEta=cms.double(-2.4),
    MaxEta=cms.double(2.4),
    inputTag1 = cms.InputTag("L1TkElectronsEllipticMatchCrystal","EG"), 
    inputTag2 = cms.InputTag("L1TkElectronsEllipticMatchHGC","EG"), 
    esScalingTag = cms.ESInputTag("L1TScalingESSource","L1TkEleScaling"),
    Quality1 = cms.int32(2), #0x2
    Quality2 = cms.int32(5),
    Qual1IsMask = cms.bool(True),
    Qual2IsMask = cms.bool(False),
)

process.l1tPFJet64 = cms.EDFilter(
    "L1PFJetFilter",
    MinPt=cms.double(64.0),
)

process.hltTestSeq = cms.Sequence(
    cms.ignore(process.l1tMuon7) + cms.ignore(process.l1tPFJet64) + cms.ignore(process.l1tEle7)
)

process.HLT_Test_Path = cms.Path(process.hltTestSeq)

process.source = cms.Source(
    "PoolSource",
    fileNames=cms.untracked.vstring(
        "/store/mc/Phase2HLTTDRSummer20ReRECOMiniAOD/DYToLL_M-50_TuneCP5_14TeV-pythia8/FEVT/PU200_pilot_111X_mcRun4_realistic_T15_v1-v1/270000/FF7BF0E2-1380-2D48-BB19-F79E6907CD5D.root",
#        "/store/mc/Phase2HLTTDRSummer20ReRECOMiniAOD/SingleElectron_PT2to200/FEVT/PU200_111X_mcRun4_realistic_T15_v1_ext2-v1/270000/0064D31F-F48B-3144-8CB9-17F820065E01.root",
    ),
)

process.maxEvents.input = cms.untracked.int32(-1)
process.options = cms.untracked.PSet(wantSummary=cms.untracked.bool(True))
