import FWCore.ParameterSet.Config as cms

hltFilteredLayerClustersRecoveryL1Seeded = cms.EDProducer("FilteredLayerClustersProducer",
    LayerClusters = cms.InputTag("hltMergeLayerClustersL1Seeded"),
    LayerClustersInputMask = cms.InputTag("hltTiclTrackstersCLUE3DHighL1Seeded"),
    algo_number = cms.vint32(6, 7, 8),
    clusterFilter = cms.string('ClusterFilterBySize'),
    iteration_label = cms.string('Recovery'),
    max_cluster_size = cms.int32(9999),
    max_layerId = cms.int32(9999),
    mightGet = cms.optional.untracked.vstring,
    min_cluster_size = cms.int32(2),
    min_layerId = cms.int32(0)
)
