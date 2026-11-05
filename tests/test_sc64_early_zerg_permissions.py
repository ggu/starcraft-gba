# SPDX-License-Identifier: GPL-3.0-or-later
import struct
import unittest
from tools.sc64_early_zerg_permissions import decode, render_header


def section(tag, data):
    return tag + struct.pack('<I', len(data)) + data


def chk(puni):
    return (section(b'PUNI', puni) + section(b'OWNR', bytes(range(12))) +
            section(b'SIDE', bytes([1] * 12)) + section(b'DIM ', b'\x40\0\x40\0'))


class EarlyPermissionsTest(unittest.TestCase):
    def test_player_and_default_selectors_and_final_bit(self):
        source = bytearray(5700)
        source[0] = 1
        source[1] = 1
        source[2964 + 1] = 1  # Default raw1 is zero: disables player raw1.
        source[2736 + 227] = 1
        source[2964 + 455] = 1  # Player1/raw227 inherits default availability.
        source[2735] = 1  # Player11/raw227, final bit, explicit availability.
        row = decode(chk(source))
        expected = bytearray(342)
        expected[0] = 1
        expected[455 >> 3] |= 1 << (455 & 7)
        expected[-1] |= 1 << 7
        self.assertEqual(bytes(expected), row['effective'])
        self.assertEqual(bytes(range(12)), row['owners'])
        self.assertEqual(bytes([1] * 12), row['races'])
        self.assertEqual(bytes(source), row['puni'])

    def test_missing_wrong_duplicate_and_truncated_inputs_reject(self):
        valid = chk(bytes(5700))
        for data in (valid[:-1], valid + section(b'PUNI', bytes(5700)),
                     chk(bytes(5699)), section(b'PUNI', bytes(5700))):
            with self.subTest(length=len(data)):
                with self.assertRaises(ValueError):
                    decode(data)

    def test_header_rejects_reordered_or_incomplete_mission_set(self):
        base = decode(chk(bytes(5700)))
        rows = [dict(base, mission=i) for i in [0, 1, 14, 15]]
        self.assertIn('sc64_early_zerg_permissions[4]', render_header(rows))
        for bad in (rows[:3], rows[::-1], [dict(row, effective=b'') for row in rows]):
            with self.assertRaises(ValueError):
                render_header(bad)


class EarlyPermissionAccessorTest(unittest.TestCase):
    def test_actual_c_accessor_stable_ids_and_fail_closed_bounds(self):
        import pathlib
        import shutil
        import subprocess
        import tempfile
        compiler = shutil.which('clang') or shutil.which('cc')
        if not compiler:
            self.skipTest('C compiler unavailable')
        root = pathlib.Path(__file__).resolve().parents[1]
        header = '''typedef struct { unsigned char mission, owner_kind[12], source_race[12], bits[342]; } Sc64EarlyZergPermission;
static const Sc64EarlyZergPermission sc64_early_zerg_permissions[4] = {
 { .mission=0, .bits={[0]=1,[341]=128} },
 { .mission=1, .bits={[5]=8} },
 { .mission=14, .bits={[0]=2} },
 { .mission=15, .bits={[341]=1} }
};
'''
        program = '''#define SC64_EARLY_ZERG_PERMISSIONS 1
#include <assert.h>
#include "sc64_early_zerg_permissions.h"
int main(void) {
 assert(sc64_early_zerg_permission_available(0,0,0));
 assert(sc64_early_zerg_permission_available(0,11,227));
 assert(!sc64_early_zerg_permission_available(0,0,1));
 assert(sc64_early_zerg_permission_available(1,0,43));
 assert(!sc64_early_zerg_permission_available(0,0,43));
 assert(sc64_early_zerg_permission_available(14,0,1));
 assert(sc64_early_zerg_permission_available(15,11,220));
 assert(!sc64_early_zerg_permission_available(2,0,0));
 assert(!sc64_early_zerg_permission_available(0,12,0));
 assert(!sc64_early_zerg_permission_available(0,0,228));
 assert(!sc64_early_zerg_permission_available(0,~0u,0));
 assert(!sc64_early_zerg_permission_available(~0u,0,0));
 assert(!sc64_early_zerg_permission_available(0,0,~0u));
 return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='sc64-early-permission-') as name:
            temp = pathlib.Path(name)
            (temp/'sc64_early_zerg_permissions_generated.h').write_text(header)
            (temp/'accessor.c').write_text(program)
            subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-I'+str(root/'starcraft'),
                            '-I'+str(temp), str(temp/'accessor.c'), '-o', str(temp/'accessor')],
                           check=True, capture_output=True, text=True)
            subprocess.run([str(temp/'accessor')], check=True, capture_output=True, text=True)
