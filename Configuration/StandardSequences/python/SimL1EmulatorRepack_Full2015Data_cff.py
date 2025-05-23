import FWCore.ParameterSet.Config as cms

## L1REPACK Full2015Data : Re-Emulate all of L1 and repack into RAW

from Configuration.Eras.Modifier_stage2L1Trigger_cff import stage2L1Trigger

(~stage2L1Trigger).toModify(None, lambda x:
    print("# L1T WARN:  L1REPACK:Full2015Data only supports Stage-2 eras for now.\n# L1T WARN:  Use a legacy version of L1REPACK for now."))
stage2L1Trigger.toModify(None, lambda x:
    print("# L1T INFO:  L1REPACK:Full2015Data will unpack all L1T inputs, re-emulated (Stage-2), and pack uGT, uGMT, and Calo Stage-2 output."))

# First, Unpack all inputs to L1:
import EventFilter.DTTFRawToDigi.dttfunpacker_cfi
unpackDttf = EventFilter.DTTFRawToDigi.dttfunpacker_cfi.dttfunpacker.clone(
    DTTF_FED_Source = cms.InputTag( 'rawDataCollector', processName=cms.InputTag.skipCurrentProcess()))

import EventFilter.CSCTFRawToDigi.csctfunpacker_cfi
unpackCsctf = EventFilter.CSCTFRawToDigi.csctfunpacker_cfi.csctfunpacker.clone(
    producer = cms.InputTag( 'rawDataCollector', processName=cms.InputTag.skipCurrentProcess()))

import EventFilter.CSCRawToDigi.cscUnpacker_cfi
unpackCSC = EventFilter.CSCRawToDigi.cscUnpacker_cfi.muonCSCDigis.clone(
    InputObjects = cms.InputTag( 'rawDataCollector', processName=cms.InputTag.skipCurrentProcess()))

import EventFilter.DTRawToDigi.dtunpacker_cfi
unpackDT = EventFilter.DTRawToDigi.dtunpacker_cfi.muonDTDigis.clone(
    inputLabel = cms.InputTag( 'rawDataCollector', processName=cms.InputTag.skipCurrentProcess()))

import EventFilter.RPCRawToDigi.rpcUnpacker_cfi
unpackRPC = EventFilter.RPCRawToDigi.rpcUnpacker_cfi.rpcunpacker.clone(
    InputLabel = cms.InputTag( 'rawDataCollector', processName=cms.InputTag.skipCurrentProcess()))

import EventFilter.EcalRawToDigi.EcalUnpackerData_cfi
unpackEcal = EventFilter.EcalRawToDigi.EcalUnpackerData_cfi.ecalEBunpacker.clone(
    InputLabel = cms.InputTag( 'rawDataCollector', processName=cms.InputTag.skipCurrentProcess()))

import EventFilter.HcalRawToDigi.HcalRawToDigi_cfi
unpackHcal = EventFilter.HcalRawToDigi.HcalRawToDigi_cfi.hcalDigis.clone(
    InputLabel = cms.InputTag( 'rawDataCollector', processName=cms.InputTag.skipCurrentProcess()))

# Second, Re-Emulate the entire L1T

# NOTE:  2016 HCAL HF TPs require a new emulation
from SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff import *
simHcalTriggerPrimitiveDigis.inputLabel = [
    'unpackHcal',
    'unpackHcal'
]

from L1Trigger.Configuration.SimL1Emulator_cff import *
simDtTriggerPrimitiveDigis.digiTag = 'unpackDT'
simCscTriggerPrimitiveDigis.CSCComparatorDigiProducer = 'unpackCSC:MuonCSCComparatorDigi'
simCscTriggerPrimitiveDigis.CSCWireDigiProducer       = 'unpackCSC:MuonCSCWireDigi'

simTwinMuxDigis.RPC_Source = 'unpackRPC'
simTwinMuxDigis.DTDigi_Source = "unpackDttf"
simTwinMuxDigis.DTThetaDigi_Source = "unpackDttf"

# BMTF
simBmtfDigis.DTDigi_Source       = "simTwinMuxDigis"
simBmtfDigis.DTDigi_Theta_Source = "unpackDttf"

# OMTF
simOmtfDigis.srcRPC                = 'unpackRPC'
simOmtfDigis.srcDTPh               = "unpackDttf"
simOmtfDigis.srcDTTh               = "unpackDttf"
simOmtfDigis.srcCSC                = "unpackCsctf"

# EMTF
simEmtfDigis.CSCInput              = "unpackCsctf"
simEmtfDigis.RPCInput              = 'unpackRPC'

simCaloStage2Layer1Digis.ecalToken = 'unpackEcal:EcalTriggerPrimitives'
simCaloStage2Layer1Digis.hcalToken = 'simHcalTriggerPrimitiveDigis'


# Finally, pack the new L1T output back into RAW
    
from EventFilter.L1TRawToDigi.caloStage2Raw_cfi import caloStage2Raw as packCaloStage2
from EventFilter.L1TRawToDigi.gmtStage2Raw_cfi import gmtStage2Raw as packGmtStage2
from EventFilter.L1TRawToDigi.gtStage2Raw_cfi import gtStage2Raw as packGtStage2

# combine the new L1 RAW with existing RAW for other FEDs
import EventFilter.RawDataCollector.rawDataCollectorByLabel_cfi
rawDataCollector = EventFilter.RawDataCollector.rawDataCollectorByLabel_cfi.rawDataCollector.clone(
    verbose = 0,
    RawCollectionList = [
        'packCaloStage2',
        'packGmtStage2',
        'packGtStage2',
        cms.InputTag('rawDataCollector', processName=cms.InputTag.skipCurrentProcess()),
        ]
    )


SimL1EmulatorTask = cms.Task()
stage2L1Trigger.toReplaceWith(SimL1EmulatorTask, cms.Task(unpackEcal,unpackHcal,unpackCSC,unpackDT,unpackRPC,unpackDttf,unpackCsctf
                                 ,simHcalTriggerPrimitiveDigis,SimL1EmulatorCoreTask,packCaloStage2
                                 ,packGmtStage2,packGtStage2,rawDataCollector))
SimL1Emulator = cms.Sequence(SimL1EmulatorTask)
