# SPDX-License-Identifier: GPL-3.0-or-later
import sys
import os
import signal
import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch
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

    def test_success_handshake_and_early_exit(self):
        for success in (True,False):
            done_r,done_w=os.pipe();ack_r,ack_w=os.pipe()
            pid=os.fork()
            if pid==0:
                os.close(done_r);os.close(ack_w);os.setsid()
                if success:
                    matrix.require_no_children();os.write(done_w,b'D')
                    if os.read(ack_r,1)!=b'A':os._exit(2)
                os._exit(0)
            os.close(done_w);os.close(ack_r)
            try:
                with patch.object(matrix,'guard',return_value=0):
                    if success:matrix.await_completion(pid,done_r,ack_w,Path('.'))
                    else:
                        with self.assertRaises(RuntimeError):matrix.await_completion(pid,done_r,ack_w,Path('.'))
                _,status=os.waitpid(pid,0)
                self.assertTrue(os.WIFEXITED(status))
            finally:os.close(done_r);os.close(ack_w)

    def test_timeout_and_forced_eperm_fail_closed(self):
        for force_eperm in (False,True):
            pid=os.fork()
            if pid==0:
                os.setsid();signal.pause();os._exit(0)
            if force_eperm:
                with patch.object(matrix.os,'killpg',side_effect=PermissionError('forced')):
                    with self.assertRaisesRegex(RuntimeError,'cleanup unverified'):
                        matrix.terminate_and_reap(pid)
            else:
                try:
                    with patch.object(matrix,'guard',side_effect=TimeoutError('deadline')):
                        readfd,writefd=os.pipe()
                        try:
                            with self.assertRaises(TimeoutError):matrix.await_completion(pid,readfd,writefd,Path('.'))
                        finally:os.close(readfd);os.close(writefd)
                finally:
                    try:matrix.terminate_and_reap(pid)
                    except RuntimeError:pass
            with self.assertRaises(ChildProcessError):os.waitpid(pid,os.WNOHANG)

    def test_worker_cannot_ack_with_live_child(self):
        pid=os.fork()
        if pid==0:
            child=os.fork()
            if child==0:
                signal.pause();os._exit(0)
            try:
                try:matrix.require_no_children();rc=1
                except RuntimeError:rc=0
            finally:
                os.kill(child,signal.SIGKILL);os.waitpid(child,0)
            os._exit(rc)
        _,status=os.waitpid(pid,0)
        self.assertTrue(os.WIFEXITED(status) and os.WEXITSTATUS(status)==0)

    def test_supervisor_reaps_its_worker(self):
        pid=os.fork()
        if pid==0:
            os.setsid()
            signal.pause()
            os._exit(0)
        try:
            try:
                status=matrix.terminate_and_reap(pid)
                self.assertTrue(os.WIFSIGNALED(status) or os.WIFEXITED(status))
            except RuntimeError as exc:
                self.assertIn('cleanup unverified',str(exc))
        finally:
            try:os.waitpid(pid,os.WNOHANG)
            except ChildProcessError:pass

    def test_receipt_roles_and_completion(self):
        with tempfile.TemporaryDirectory() as d:
            rom=Path(d)/'a.gba';elf=Path(d)/'a.elf';rom.write_bytes(b'rom');elf.write_bytes(b'elf')
            product={k:{'path':str(p.resolve()),'sha256':matrix.sha(p)} for k,p in [('rom',rom),('elf',elf)]}
            receipt={'head':matrix.COMPILED_HEAD,'unchanged':True,'products':{'p10':product},
                     'results':[{'name':name,'rc':0} for name in ('arm-p10','rom','header','boundary')]}
            matrix.validate_receipt(receipt,rom,elf)
            with self.assertRaises(ValueError):matrix.validate_receipt(receipt,elf,rom)
            receipt['results'][-1]['rc']=1
            with self.assertRaises(ValueError):matrix.validate_receipt(receipt,rom,elf)


if __name__=='__main__':unittest.main()
