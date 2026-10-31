import copy
import unittest
from tools.sc64_t1_trace_export import validate

class T1TraceExportTest(unittest.TestCase):
    def trace(self):
        return {"schema":2,"complete":True,"total_ticks":1000,
                "event_count":2,"checkpoint_count":1,
                "events":[{"tick":0,"held":1},{"tick":3,"held":0}],
                "checkpoints":[dict(tick=1000,frame=900,minerals=10,marines=10,scv=1,zerg=0,raynor_owner=0,raynor_alive=1,state_hash=7)],
                "expected":dict(won=1,lost=0,marines=10,scv=1,minerals=10,frame=900,state_hash=7)}
    def test_complete(self):
        validate(self.trace())
    def test_missing_completeness_or_checkpoint(self):
        for change in (lambda x:x.pop("schema"),lambda x:x.update(complete=False),lambda x:x["checkpoints"].clear(),lambda x:x["events"].pop()):
            trace=self.trace();change(trace)
            with self.assertRaises(ValueError):validate(trace)
    def test_bad_event_order_or_bounds(self):
        for events in ([{"tick":0,"held":1},{"tick":0,"held":0}], [{"tick":1000,"held":0}], [{"tick":0,"held":1024}]):
            trace=self.trace();trace["events"]=events
            with self.assertRaises(ValueError):validate(trace)
