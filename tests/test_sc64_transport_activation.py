"""Production routing for the versioned shared-transport save namespace."""
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class TransportActivation(unittest.TestCase):
    def test_profile_flags_and_legacy_z6(self):
        names = ('TERRAN_ROM_FLAGS', 'ZERG_Z7_CANONICAL_FLAGS',
                 'ZERG_Z8_CANONICAL_FLAGS', 'PROTOSS_P7_CANONICAL_FLAGS',
                 'PROTOSS_P8_FLAGS', 'SHOWCASE_FLAGS', 'ZERG_ROM_FLAGS')
        with tempfile.TemporaryDirectory() as td:
            makefile = Path(td) / 'flags.mk'
            makefile.write_text('transport-flags:\n' + ''.join(
                "\t@printf '%s\\t%s\\n' '" + name + "' '$({})'\n".format(name)
                for name in names))
            output = subprocess.check_output(
                ['make', '--no-print-directory', '-s', '-f', 'Makefile',
                 '-f', str(makefile), 'transport-flags'], cwd=ROOT, text=True)
        values = dict(line.split('\t', 1) for line in output.splitlines())
        for name in names[:-1]:
            self.assertIn('-DSC64_SHARED_TRANSPORT', values[name].split(), name)
        self.assertNotIn('-DSC64_SHARED_TRANSPORT', values['ZERG_ROM_FLAGS'].split())


if __name__ == '__main__':
    unittest.main()
