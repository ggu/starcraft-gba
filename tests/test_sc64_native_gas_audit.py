# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks for the native gas audit's authentication gate."""
import hashlib
import unittest
from tools import sc64_native_gas_audit as audit


class NativeGasAuditTest(unittest.TestCase):
    def test_region_checks_reject_truncation_and_mutation(self):
        data = b'0123456789'
        digest = hashlib.sha256(b'3456').hexdigest()
        regions = {'sample': (3, 4, digest)}
        self.assertEqual(audit.verify_regions(data, regions), {'sample': digest})
        with self.assertRaisesRegex(ValueError, 'sample'):
            audit.verify_regions(data[:5], regions)
        with self.assertRaisesRegex(ValueError, 'sample'):
            audit.verify_regions(b'01234X6789', regions)

    def test_unknown_rom_is_rejected_before_interpretation(self):
        with self.assertRaisesRegex(ValueError, 'authenticated'):
            audit.audit(b'not a ROM')


if __name__ == '__main__':
    unittest.main()
