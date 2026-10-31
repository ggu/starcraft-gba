"""The original-art opening assigns workers without periodic auto-mining."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class DefaultWorkerStartTest(unittest.TestCase):
    def test_opening_and_later_idle_worker(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = pathlib.Path(tmp) / "worker-start"
            subprocess.run(["clang", "-std=c11", "-O1", "-g", "-fsanitize=address,undefined",
                            "-DGBA_HOST", "-Icommon", "tests/sc64_default_worker_start.c",
                            "common/gba.c", "-o", str(exe)], cwd=ROOT, check=True)
            subprocess.run([str(exe)], cwd=ROOT, check=True)
