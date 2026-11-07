# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_q8_paid_research_physical as paid


class PaidResearchContracts(unittest.TestCase):
    def test_payment_restore_and_completion_reject_mismatch(self):
        before={'screen':1,'lost':0,'bots':1,'minerals':476,'gas':112,'jobs':[],
                'levels':[0]*48,'frame':3005,'debt':'00'}
        active=copy.deepcopy(before)
        active.update(minerals=376,gas=12,jobs=[{'owner':0,'family_plus_one':3,
                      'queue':[0,1,0,0,0],'timer':4999}])
        paid.validate_paid(before,active)
        for key in ('minerals','gas'):
            bad=copy.deepcopy(active); bad[key]+=1
            with self.assertRaises(ValueError): paid.validate_paid(before,bad)
        saved=copy.deepcopy(active); saved['screen']=2
        paid.validate_restored(saved,active)
        for mutate in ('minerals','gas','frame','jobs','levels','debt'):
            bad=copy.deepcopy(active)
            if mutate in ('jobs','levels'): bad[mutate]=[]
            elif mutate=='debt': bad[mutate]='01'
            else: bad[mutate]+=1
            with self.assertRaises(ValueError): paid.validate_restored(saved,bad)
        finished=copy.deepcopy(active); finished['levels'][2]=1; finished['jobs']=[]
        paid.validate_finished(finished)
        for bad in (active,dict(finished,lost=1),dict(finished,jobs=active['jobs'])):
            with self.assertRaises(ValueError): paid.validate_finished(bad)

if __name__=='__main__': unittest.main()

class ContextContracts(unittest.TestCase):
    def test_context_is_part_of_restored_comparison(self):
        saved={'screen':2,'context':{'race_mode':1,'race_skirmish_map':0,'race_human':2,
               'race_bots':1,'players':[2,0,0,0,0],'bot_races':[0,1,2]}}
        loaded=copy.deepcopy(saved); loaded['screen']=1
        paid.validate_restored(saved,loaded)
        for key in ('race_mode','race_skirmish_map','race_human','race_bots','players','bot_races'):
            bad=copy.deepcopy(loaded); bad['context'][key]=None
            with self.assertRaises(ValueError):paid.validate_restored(saved,bad)

    def test_deadline_handler_is_bound(self):
        self.assertTrue(callable(paid.timeout))
        with self.assertRaises(TimeoutError):paid.timeout(None,None)
