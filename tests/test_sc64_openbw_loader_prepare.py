"""Asset-free safety checks for the private OpenBW loader preparer."""
import contextlib
import hashlib
import io
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from tools import sc64_openbw_iscript_oracle_prepare as prepare

ROOT = Path(prepare.__file__).resolve().parents[1]
SAMPLE = b'prefix\n' + prepare.START + b'\n};\n' + prepare.END + b'\n'

class LoaderPrepareTest(unittest.TestCase):
    def setUp(self):
        (ROOT / '.local').mkdir(exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(dir=ROOT / '.local')
        self.addCleanup(self.tmp.cleanup)
        self.dir = Path(self.tmp.name)
        self.source = self.dir / 'bwgame.h'
        self.source.write_bytes(SAMPLE)

    def invoke(self, source, output, digest=None):
        digest = digest or hashlib.sha256(SAMPLE).hexdigest()
        argv = ['prepare', '--bwgame', str(source), '--out', str(output)]
        with mock.patch.object(prepare, 'PINNED_SHA256', digest), mock.patch.object(sys, 'argv', argv):
            with contextlib.redirect_stderr(io.StringIO()), contextlib.redirect_stdout(io.StringIO()):
                prepare.main()

    def test_valid_output_and_existing_output_rejected(self):
        out = self.dir / 'include'
        self.invoke(self.source, out)
        first = out.read_bytes()
        with self.assertRaises(SystemExit): self.invoke(self.source, out)
        self.assertEqual(out.read_bytes(), first)
        self.assertEqual(self.source.read_bytes(), SAMPLE)

    def test_alias_and_symlink_alias_rejected(self):
        for out in (self.source, self.dir / 'alias'):
            if out != self.source: out.symlink_to(self.source)
            with self.assertRaises(SystemExit): self.invoke(self.source, out)
            self.assertEqual(self.source.read_bytes(), SAMPLE)

    def test_outside_local_and_escaping_symlink_rejected(self):
        outside = ROOT / 'sc64-loader-forbidden-test-output'
        link = self.dir / 'escape'
        link.symlink_to(ROOT, target_is_directory=True)
        for out in (outside, link / outside.name):
            with self.assertRaises(SystemExit): self.invoke(self.source, out)
            self.assertFalse(outside.exists())
        self.assertEqual(self.source.read_bytes(), SAMPLE)

    def test_bad_hash_and_ambiguous_anchors_rejected(self):
        out = self.dir / 'include'
        with self.assertRaises(SystemExit): self.invoke(self.source, out, '0' * 64)
        self.assertFalse(out.exists())
        self.source.write_bytes(SAMPLE + prepare.START)
        before = self.source.read_bytes()
        with self.assertRaises(SystemExit): self.invoke(self.source, out, hashlib.sha256(before).hexdigest())
        self.assertEqual(self.source.read_bytes(), before)
        self.assertFalse(out.exists())

if __name__ == '__main__': unittest.main()
