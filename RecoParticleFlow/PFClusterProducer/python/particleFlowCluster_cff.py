import FWCore.ParameterSet.Config as cms


#from RecoParticleFlow.PFClusterProducer.towerMakerPF_cfi import *
#from RecoParticleFlow.PFClusterProducer.particleFlowCaloResolution_cfi import _timeResolutionHCALMaxSample

from RecoParticleFlow.PFClusterProducer.particleFlowRecHitECAL_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowRecHitHBHE_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowRecHitHF_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowRecHitHO_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowRecHitPS_cfi import *

from RecoParticleFlow.PFClusterProducer.particleFlowClusterECALUncorrected_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowClusterECAL_cff import *

from RecoParticleFlow.PFClusterProducer.particleFlowClusterHBHE_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowClusterHF_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowClusterHCAL_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowClusterHO_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowClusterPS_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowBadHcalPseudoCluster_cff import *

particleFlowClusterECALTask = cms.Task(particleFlowClusterECAL)
particleFlowClusterECALSequence = cms.Sequence(particleFlowClusterECALTask)

pfClusteringECALTask = cms.Task(particleFlowRecHitECAL,
                                particleFlowClusterECALUncorrected,
                                particleFlowClusterECALTask)
pfClusteringECAL = cms.Sequence(pfClusteringECALTask)

pfClusteringPSTask = cms.Task(particleFlowRecHitPS,particleFlowClusterPS)
pfClusteringPS = cms.Sequence(pfClusteringPSTask)

pfClusteringHBHEHFTask = cms.Task(particleFlowRecHitHBHE,
                                  particleFlowRecHitHF,
                                  particleFlowClusterHBHE,
                                  particleFlowClusterHF,
                                  particleFlowClusterHCAL)
pfClusteringHBHEHF = cms.Sequence(pfClusteringHBHEHFTask)

pfClusteringHBHEHFOnlyTask = cms.Task(particleFlowRecHitHBHEOnly,
                                      particleFlowRecHitHF,
                                      particleFlowClusterHBHEOnly,
                                      particleFlowClusterHF,
                                      particleFlowClusterHCALOnly)

#--- Legacy HCAL Only Task
pfClusteringHBHEHFOnlyLegacyTask = pfClusteringHBHEHFOnlyTask.copy()

pfClusteringHOTask = cms.Task(particleFlowRecHitHO,particleFlowClusterHO)
pfClusteringHO = cms.Sequence(pfClusteringHOTask)

particleFlowClusterWithoutHOTask = cms.Task(particleFlowBadHcalPseudoCluster,
                                            pfClusteringPSTask,
                                            pfClusteringECALTask,
                                            pfClusteringHBHEHFTask)
particleFlowClusterWithoutHO = cms.Sequence(particleFlowClusterWithoutHOTask)

particleFlowClusterTask = cms.Task(particleFlowBadHcalPseudoCluster,
                                   pfClusteringPSTask,
                                   pfClusteringECALTask,
                                   pfClusteringHBHEHFTask,
                                   pfClusteringHOTask)
particleFlowCluster = cms.Sequence(particleFlowClusterTask)

#HGCal

from RecoParticleFlow.PFClusterProducer.particleFlowRecHitHGC_cfi import particleFlowRecHitHGC
pfClusteringHGCalTask = cms.Task(particleFlowRecHitHGC)
pfClusteringHGCal = cms.Sequence(pfClusteringHGCalTask)

_phase2_hgcal_particleFlowClusterTask = particleFlowClusterTask.copy()
_phase2_hgcal_particleFlowClusterTask.add(pfClusteringHGCalTask)

from Configuration.Eras.Modifier_phase2_hgcal_cff import phase2_hgcal
phase2_hgcal.toReplaceWith( particleFlowClusterTask, _phase2_hgcal_particleFlowClusterTask )

#timing

from RecoParticleFlow.PFClusterProducer.particleFlowClusterTimeAssigner_cfi import particleFlowTimeAssignerECAL
from RecoParticleFlow.PFSimProducer.ecalBarrelClusterFastTimer_cfi import ecalBarrelClusterFastTimer
_phase2_timing_particleFlowClusterECALTask = particleFlowClusterECALTask.copy()
_phase2_timing_particleFlowClusterECALTask.add(cms.Task(ecalBarrelClusterFastTimer,
                                                        particleFlowTimeAssignerECAL))

from Configuration.Eras.Modifier_phase2_timing_cff import phase2_timing
phase2_timing.toReplaceWith(particleFlowClusterECALTask,
                                  _phase2_timing_particleFlowClusterECALTask)
phase2_timing.toModify(particleFlowClusterECAL,
                            inputECAL = 'particleFlowTimeAssignerECAL')

# Replace HBHE rechit and clustering with Alpaka modules

from Configuration.ProcessModifiers.alpaka_cff import alpaka

def _addProcessPFClusterAlpaka(process):
    process.load("RecoParticleFlow.PFClusterProducer.pfClusterHBHEAlpaka_cff")

modifyConfigurationPFClusterAlpaka_ = alpaka.makeProcessModifier(_addProcessPFClusterAlpaka)

from RecoParticleFlow.PFClusterProducer.barrelLayerClusters_cff import barrelLayerClustersEB, barrelLayerClustersHB 
_pfClusteringECALTask = pfClusteringECALTask.copy()
_pfClusteringECALTask.add(barrelLayerClustersEB)

_pfClusteringHBHEHFTask = pfClusteringHBHEHFTask.copy()
_pfClusteringHBHEHFTask.add(barrelLayerClustersHB)

from Configuration.ProcessModifiers.ticl_barrel_cff import ticl_barrel
ticl_barrel.toReplaceWith(pfClusteringECALTask, _pfClusteringECALTask)
ticl_barrel.toReplaceWith(pfClusteringHBHEHFTask, _pfClusteringHBHEHFTask)
