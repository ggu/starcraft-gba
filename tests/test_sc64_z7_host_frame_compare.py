# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from sc64_z7_host_frame_compare import RECORD, compare


class HostFrameCompareTests(unittest.TestCase):
    def test_rejects_a_missing_reordered_or_changed_call(self):
        rows = [RECORD.pack(i, i-1, 1, 0, 10+i, 20+i, 30+i) for i in (1, 2, 3)]
        with tempfile.TemporaryDirectory() as tmp:
            a, b = Path(tmp) / "a", Path(tmp) / "b"
            a.write_bytes(b"".join(rows)); b.write_bytes(b"".join(rows))
            self.assertEqual(compare(a, b, 3), 3)
            for damaged in (rows[:-1], [rows[0], rows[2], rows[1]],
                            [rows[0], RECORD.pack(2, 1, 1, 0, 12, 22, 999), rows[2]],
                            rows + [rows[2]]):
                b.write_bytes(b"".join(damaged))
                with self.assertRaises(ValueError):
                    compare(a, b, 3)


if __name__ == "__main__":
    unittest.main()
