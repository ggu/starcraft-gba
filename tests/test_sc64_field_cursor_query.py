"""Compare the asset-free field cursor probe with the original queries."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


class FieldCursorQuery(unittest.TestCase):
    def test_cursor_results_and_rendered_frames(self):
        root = Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as directory:
            receipts = []
            for option in ([], ["-DSC64_FIELD_CURSOR_QUERY_PROBE"]):
                exe = Path(directory) / "cursor"
                subprocess.run([
                    os.environ.get("CLANG", "clang"), "-std=c11", "-O1",
                    "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
                    "-Wno-unused-variable", "-fsanitize=address,undefined",
                    "-DGBA_HOST", "-Icommon", *option,
                    "tests/field_cursor_query_probe.c", "common/gba.c",
                    "-o", str(exe),
                ], cwd=root, check=True)
                receipts.append(subprocess.check_output([str(exe)], cwd=root))
            self.assertEqual(receipts[0], receipts[1])
