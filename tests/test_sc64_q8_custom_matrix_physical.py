# SPDX-License-Identifier: GPL-3.0-or-later
import sys
import os
import signal
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_q8_custom_matrix_physical as matrix


def image():
    data = bytearray(b'\xff' * 131072); base = 32768; meta = base + 4096
    tag, payload = 0x1248505c, b'authored test payload'
    h = 2166136261
    for b in payload: h = ((h ^ b) * 16777619) & 0xffffffff
    for off, value in [(base,tag),(meta,tag),(meta+4,len(payload)),(meta+8,h),(meta+12,1)]:
        data[off:off+4] = value.to_bytes(4,'little')
    data[meta+16:meta+16+len(payload)] = payload
    return data


class MatrixTest(unittest.TestCase):
    def test_all_rosters(self):
        rows=matrix.scenarios()
        self.assertEqual(len(rows),234); self.assertEqual(len(set(rows)),234)
        self.assertEqual({len(x[2]) for x in rows},{1,2,3})
        self.assertEqual({x[0] for x in rows},{0,1}); self.assertEqual({x[1] for x in rows},{0,1,2})

    def test_flash_commit_checksum_legacy_and_length(self):
        data=image(); self.assertEqual(matrix.flash_record(data)['sequence'],1)
        for offset in (0,32768,36864,36864+8,36864+16):
            bad=bytearray(data);bad[offset]^=1
            with self.subTest(offset=offset),self.assertRaises(ValueError):matrix.flash_record(bad)
        with self.assertRaises(ValueError):matrix.flash_record(data[:-1])

    def test_supervisor_reaps_its_worker(self):
        pid=os.fork()
        if pid==0:
            os.setsid()
            signal.pause()
            os._exit(0)
        try:
            status=matrix.terminate_and_reap(pid)
            self.assertTrue(os.WIFSIGNALED(status) or os.WIFEXITED(status))
        finally:
            try:os.waitpid(pid,os.WNOHANG)
            except ChildProcessError:pass


if __name__=='__main__':unittest.main()
