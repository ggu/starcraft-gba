import importlib.util, os, pathlib, signal, subprocess, sys, tempfile, time, unittest
from unittest.mock import patch
root=pathlib.Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('observer',root/'experiments/ares-headless/marine_snapshot.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class StopTests(unittest.TestCase):
 def test_real_stop_owned_fd_and_kill(self):
  exe=root/'.local/stop-test/signal-stop'
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d); child=subprocess.Popen([str(exe),str(p/'record'),str(p/'target')])
   try:
    deadline=time.monotonic()+5
    while not m.stopped_child(child) and time.monotonic()<deadline:
     self.assertIsNone(child.poll());time.sleep(.01)
    self.assertTrue(m.stopped_child(child)); self.assertEqual((p/'record').stat().st_size,40)
    result={};m.inspect_stopped_child(child,p,5,result)
    self.assertEqual(result['fd_inspection'],'complete')
    self.assertIn(str(p/'target'),(p/'stopped-fds.txt').read_text())
    self.assertLessEqual((p/'stopped-fds.txt').stat().st_size,65536)
    self.assertIsNone(child.poll())
   finally:
    if child.poll() is None:child.kill()
    child.wait(timeout=2)
   self.assertEqual(child.returncode,-signal.SIGKILL)
 def test_term_cleanup_never_resumes_stopped_child(self):
  exe=root/'.local/stop-test/signal-stop'
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d);child=subprocess.Popen([str(exe),str(p/'record'),str(p/'target')])
   try:
    deadline=time.monotonic()+5
    while not m.stopped_child(child) and time.monotonic()<deadline:time.sleep(.01)
    self.assertTrue(m.stopped_child(child))
    m.terminate_child(child)
    self.assertIn(child.returncode,(-signal.SIGTERM,-signal.SIGKILL))
   finally:
    if child.poll() is None:child.kill();child.wait(timeout=2)
 def test_hung_inspector_killed_and_reaped(self):
  real=subprocess.Popen;helpers=[]
  def spawn(_args,**kw):
   p=real([sys.executable,'-c','import time;time.sleep(60)'],**kw);helpers.append(p);return p
  with tempfile.TemporaryDirectory() as d, patch.object(m.subprocess,'Popen',side_effect=spawn):
   result={};m.inspect_stopped_child(type('Child',(),{'pid':os.getpid()})(),d,2.2,result)
   self.assertEqual(result['fd_inspection'],'timeout');self.assertEqual(len(helpers),1)
   self.assertIsNotNone(helpers[0].poll())
   with self.assertRaises(ProcessLookupError):os.kill(helpers[0].pid,0)
 def test_deadline_avoids_helper(self):
  result={};m.inspect_stopped_child(None,'.',1,result)
  self.assertEqual(result['fd_inspection'],'deadline')
if __name__=='__main__':unittest.main()
