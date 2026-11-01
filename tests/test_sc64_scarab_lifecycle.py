# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks for the unintegrated Scarab lifecycle component."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class ScarabLifecycleTest(unittest.TestCase):
    def test_source_tick_lifecycle(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = pathlib.Path(directory) / "scarab-lifecycle"
            subprocess.run(["clang", "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                            "-Werror", "-fsanitize=address,undefined",
                            str(ROOT / "tests/sc64_scarab_lifecycle.c"), "-o", str(executable)],
                           check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()
