# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free differential of both target-index builders."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class TargetOwnerIndexTest(unittest.TestCase):
    def test_randomized_builds(self):
        with tempfile.TemporaryDirectory() as directory:
            exe = pathlib.Path(directory) / "index-diff"
            subprocess.run(["clang", "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                            "-Werror", "-fsanitize=address,undefined",
                            str(ROOT / "tests/sc64_target_owner_index_diff.c"),
                            "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

if __name__ == "__main__":
    unittest.main()
