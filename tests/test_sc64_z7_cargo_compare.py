import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
from tools import sc64_z7_cargo_compare as checker

class CargoCompareTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / 'log'
        self.valid = ('Z7 CHECK checkpoint host_tick=1\n'
                      'CARGO7 checkpoint tick=1 digest=123 end=1\n'
                      'Z7 CHECK won host_tick=2\n'
                      'CARGO7 won tick=2 digest=4294967295 end=1\n')

    def parse(self, text):
        self.path.write_text(text)
        return checker.read(self.path)

    def test_valid_host_and_emulator_prefix(self):
        rows = self.parse(self.valid)
        self.assertEqual(rows, self.parse(''.join('[WARN] GBA Debug: ' + line + '\n' for line in self.valid.splitlines())))

    def test_missing_duplicate_reordered_and_malformed(self):
        a = 'CARGO7 checkpoint tick=1 digest=123 end=1\n'
        b = 'CARGO7 won tick=2 digest=4294967295 end=1\n'
        for text in (self.valid.replace(a, ''), self.valid.replace(a, a+a),
                     self.valid.replace(a, 'PLACEHOLDER\n').replace(b, a).replace('PLACEHOLDER\n', b),
                     self.valid.replace('end=1', 'end=1 trailing', 1),
                     self.valid.replace('end=1', 'end=', 1),
                     self.valid.replace('123', '4294967296'),
                     self.valid.replace('CARGO7 checkpoint', 'CARGO7 unknown')):
            with self.subTest(text=text), self.assertRaises(ValueError):
                self.parse(text)

    def test_mismatch_rejected(self):
        first = self.path
        first.write_text(self.valid)
        second = first.with_name('other')
        second.write_text(self.valid.replace('digest=123', 'digest=124'))
        with patch('sys.argv', ['checker', str(first), str(second)]), self.assertRaises(ValueError):
            checker.main()
