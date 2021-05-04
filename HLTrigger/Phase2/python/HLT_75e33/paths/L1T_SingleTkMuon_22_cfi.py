import FWCore.ParameterSet.Config as cms

from ..modules.hltL1TkSingleMuFiltered22_cfi import *
from ..sequences.HLTL1Sequence_cfi import *

L1T_SingleTkMuon_22 = cms.Path(HLTL1Sequence+hltL1TkSingleMuFiltered22)
