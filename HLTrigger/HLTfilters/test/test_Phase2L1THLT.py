import FWCore.ParameterSet.Config as cms

process = cms.Process("HLTX")

### Load all ESSources, ESProducers and PSets
# process.load("HLTrigger.Configuration.Phase2.hltPhase2Setup_cff")

### GlobalTag
# process.load("Configuration.StandardSequences.CondDBESSource_cff")
# process.GlobalTag.globaltag = "112X_mcRun4_realistic_T15_v2"

process.l1tEle7 = cms.EDFilter(
    "L1TkEleFilter",
    MinPt=cms.double(7.0),
    MinEta=cms.double(-2.4),
    MaxEta=cms.double(2.4),
    inputTag1=cms.InputTag("L1TkElectronsEllipticMatchCrystal", "EG"),
    inputTag2=cms.InputTag("L1TkElectronsEllipticMatchHGC", "EG"),
    Scalings=cms.PSet(
        barrel=cms.vdouble(0.805095, 1.18336, 0.0),
        endcap=cms.vdouble(0.453144, 1.26205, 0.0),
    ),
    EtaBinsForIsolation=cms.vdouble(0.0, 9999.9),
    TrkIsolation=cms.vdouble(99999.9),  # No isolation
    ApplyQual1=cms.bool(True),
    ApplyQual2=cms.bool(True),
    Quality1=cms.int32(2),  # 0x2
    Quality2=cms.int32(5),
    Qual1IsMask=cms.bool(True),
    Qual2IsMask=cms.bool(False),
)

process.l1tIsoEle7 = cms.EDFilter(
    "L1TkEleFilter",
    MinPt=cms.double(7.0),
    MinEta=cms.double(-2.4),
    MaxEta=cms.double(2.4),
    inputTag1=cms.InputTag("L1TkElectronsEllipticMatchCrystal", "EG"),
    inputTag2=cms.InputTag("L1TkElectronsEllipticMatchHGC", "EG"),
    Scalings=cms.PSet(
        barrel=cms.vdouble(0.434262, 1.20586, 0.0),
        endcap=cms.vdouble(0.266186, 1.25976, 0.0),
    ),
    EtaBinsForIsolation=cms.vdouble(0.0, 1.479, 9999.9),
    TrkIsolation=cms.vdouble(0.12, 0.2),
    ApplyQual1=cms.bool(False),
    ApplyQual2=cms.bool(True),
    Quality1=cms.int32(-1),
    Quality2=cms.int32(5),
    Qual1IsMask=cms.bool(False),
    Qual2IsMask=cms.bool(False),
)

process.l1tIsoPho7 = cms.EDFilter(
    "L1TkEmFilter",
    MinPt=cms.double(7.0),
    MinEta=cms.double(-2.4),
    MaxEta=cms.double(2.4),
    inputTag1=cms.InputTag("L1TkPhotonsCrystal", "EG"),
    inputTag2=cms.InputTag("L1TkPhotonsHGC", "EG"),
    Scalings=cms.PSet(
        barrel=cms.vdouble(2.54255, 1.08749, 0.0),
        endcap=cms.vdouble(2.11186, 1.15524, 0.0),
    ),    
    EtaBinsForIsolation=cms.vdouble(0.0, 1.479, 9999.9),
    TrkIsolation=cms.vdouble(0.28, 0.35),
    ApplyQual1=cms.bool(False),
    ApplyQual2=cms.bool(True),
    Quality1=cms.int32(2),  # 0x2 "second bit"
    Quality2=cms.int32(5),
    Qual1IsMask=cms.bool(True),
    Qual2IsMask=cms.bool(False),
)

process.l1tMuon7 = cms.EDFilter(
    "L1TkMuonFilter",
    MinPt=cms.double(7.0),
    MinEta=cms.double(-2.4),
    MaxEta=cms.double(2.4),
    inputTag=cms.InputTag("L1TkMuons"),
    Scalings=cms.PSet(
        barrel=cms.vdouble(0.802461, 1.04193, 0.0),
        overlap=cms.vdouble(0.921315, 1.03611, 0.0),
        endcap=cms.vdouble(0.828802, 1.03447, 0.0),
    ),
  )

# process.l1tPFJet64 = cms.EDFilter("L1PFJetFilter",
#     MinPt=cms.double(64.0),
# )

# process.L1PFHtMht = cms.EDProducer("HLTHtMhtProducer",
#     jetsLabel=cms.InputTag("ak4PFL1PuppiCorrected"),
#     minPtJetHt=cms.double(30),
#     maxEtaJetHt=cms.double(2.4),
# )

# ### Notice that there is no MHT seed in the Phase-II Level-1 Menu...
# # Possible choices for TypeOfSum are: MET, MHT, ETT, HT
# # but notice that if you are using a MET seed you
# # should probably use the precomputed one.

# # We don't have scaling for MHT...
# process.l1tPFMht120 = cms.EDFilter("L1EnergySumFilter",
#     inputTag=cms.InputTag("L1PFHtMht"),
#     esScalingTag=cms.ESInputTag("L1TScalingESSource", "L1PFHTScaling"),
#     TypeOfSum=cms.string("MHT"),
#     MinPt=cms.double(120.0),
# )

# process.l1tPFHt120 = cms.EDFilter("L1EnergySumFilter",
#     inputTag=cms.InputTag("L1PFHtMht"),
#     esScalingTag=cms.ESInputTag("L1TScalingESSource", "L1PFHTScaling"),
#     TypeOfSum=cms.string("HT"),
#     MinPt=cms.double(120.0),
# )

# process.l1tPFMet120 = cms.EDFilter("L1PFEnergySumFilter",
#     inputTag=cms.InputTag("l1PFMetPuppi"),
#     esScalingTag=cms.ESInputTag("L1TScalingESSource", "L1PuppiMETScaling"),
#     TypeOfSum=cms.string("MET"),
#     MinPt=cms.double(120.0),
# )

# process.l1tDoubleMuon7 = cms.EDFilter(
#     "L1TkMuonFilter",
#     MinPt=cms.double(7.0),
#     MinN=cms.int32(2),
#     MinEta=cms.double(-2.4),
#     MaxEta=cms.double(2.4),
# )

# process.l1tSingleMuon15 = cms.EDFilter(
#     "L1TkMuonFilter",
#     MinPt=cms.double(15.0),
#     MinN=cms.int32(2),
#     MinEta=cms.double(-2.4),
#     MaxEta=cms.double(2.4),
# )

# # Not working yet
# process.l1tDoubleMuonPt15PtDZ1p0 = cms.EDFilter(
#     "L1T2TkMuonTkMuonDZ",
#     saveTags=cms.bool(True),
#     originTag1=cms.VInputTag("L1TkMuons"),
#     originTag2=cms.VInputTag("L1TkMuons"),
#     MinPixHitsForDZ=cms.int32(0),
#     MinN=cms.int32(1),
#     triggerType1=cms.int32(-114),
#     triggerType2=cms.int32(-114),
#     MinDR=cms.double(0.001),
#     MaxDZ=cms.double(9999.0),
#     inputTag1=cms.InputTag("l1tDoubleMuon7"),
#     checkSC=cms.bool(False),
#     inputTag2=cms.InputTag("l1tSingleMuon15"),
# )


process.HLT_SingleEle7 = cms.Path(process.l1tEle7)
process.HLT_SingleIsoEle7 = cms.Path(process.l1tIsoEle7)
process.HLT_SingleIsoPhoton7 = cms.Path(process.l1tIsoPho7)
process.HLT_SingleMu7 = cms.Path(process.l1tMuon7)

process.source = cms.Source(
    "PoolSource",
    fileNames=cms.untracked.vstring(
        "/store/mc/Phase2HLTTDRSummer20ReRECOMiniAOD/DYToLL_M-50_TuneCP5_14TeV-pythia8/FEVT/PU200_pilot_111X_mcRun4_realistic_T15_v1-v1/270000/FF7BF0E2-1380-2D48-BB19-F79E6907CD5D.root",
        # "/store/mc/Phase2HLTTDRSummer20ReRECOMiniAOD/SingleElectron_PT2to200/FEVT/PU200_111X_mcRun4_realistic_T15_v1_ext2-v1/270000/0064D31F-F48B-3144-8CB9-17F820065E01.root",
    ),
)

process.maxEvents.input = cms.untracked.int32(-1)
process.options = cms.untracked.PSet(wantSummary=cms.untracked.bool(True))
