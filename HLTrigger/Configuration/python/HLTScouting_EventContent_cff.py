import FWCore.ParameterSet.Config as cms

# EventContent for HLT Scouting products.

# This file keep track of all the scouting objects used since Run-2 
# HLTScoutingAll is used in Configuration/EventContent to keep all scouting objects available in all AOD/MINIAOD[SIM]

HLTScoutingAll = cms.PSet(
    outputCommands = cms.vstring( *(
        ## Current scouting objects (from 2024)
        'keep *_hltFEDSelectorL1_*_*',  # PR#20739
        'keep *_hltScoutingEgammaPacker_*_*', # PR#20739
        'keep *_hltScoutingMuonPackerNoVtx_*_*', # PR#44302
        'keep *_hltScoutingMuonPackerVtx_*_*',  # PR#44302
        'keep *_hltScoutingPFPacker_*_*', # PR#20739
        'keep *_hltScoutingPrimaryVertexPacker_*_*',  # PR#20739
        'keep *_hltScoutingTrackPacker_*_*', # PR #23077
        
        ## Previous scouting objects
        # Run3 (2022-23)
        'keep *_hltScoutingMuonPacker_*_*', # added w/ PR#20739, removed w/ PR#44302
        
        # Run2 only
        'keep *_hltScoutingCaloPacker_*_*',  # added  w/ PR#20739, removed w/ PR#37114
        'keep *_hltScoutingMuonPackerCalo_*_*', # added  w/ PR#20739, removed w/  PR#37114
        'keep *_hltScoutingPrimaryVertexPackerCaloMuon_*_*', # added  w/ PR#20739, removed w/  PR#37114
    ) )
)
