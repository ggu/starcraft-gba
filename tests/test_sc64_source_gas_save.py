"""Asset-free sanitizer checks for the source gas save wire."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SourceGasSaveTests(unittest.TestCase):
    def test_wire_round_trip_and_validation(self):
        compiler = shutil.which('clang')
        if compiler is None:
            self.skipTest('clang is unavailable')
        with tempfile.TemporaryDirectory() as directory:
            binary = str(Path(directory) / 'gas-save')
            subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined',
                            str(ROOT / 'tests/sc64_source_gas_save.c'), '-o', binary],
                           check=True, cwd=ROOT)
            subprocess.run([binary], check=True, cwd=ROOT, capture_output=True, text=True)


if __name__ == '__main__':
    unittest.main()
