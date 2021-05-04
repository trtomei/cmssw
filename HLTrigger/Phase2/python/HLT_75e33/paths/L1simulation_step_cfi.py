import FWCore.ParameterSet.Config as cms

from ..modules.simCscTriggerPrimitiveDigis_cfi import *
from ..modules.simDtTriggerPrimitiveDigis_cfi import *

L1simulation_step = cms.Path(
    cms.Task(
        simCscTriggerPrimitiveDigis,
        simDtTriggerPrimitiveDigis,
    )
)
