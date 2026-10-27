#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Host checks for the isolated Z8 trigger scheduler.

The synthetic C program is asset-free. The optional descriptor probe runs only
when SC64_Z8_DESCRIPTOR names a private descriptor written by
``tools/sc64_z8_pack.py``; its generated C tables stay in a temporary
directory and are never written to the source tree.
"""
from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / 'starcraft/sc64_z8_triggers.h'
SYNTHETIC = ROOT / 'tests/sc64_z8_triggers.c'
FLAGS = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-pedantic',
         '-fsanitize=address,undefined', '-fno-sanitize-recover=all']


def _compile_and_run(clang: str, source: Path, output: Path) -> str:
    subprocess.run([clang, *FLAGS, f'-I{ROOT}', str(source), '-o', str(output)],
                   check=True, capture_output=True, text=True)
    result = subprocess.run([str(output)], check=True, capture_output=True,
                            text=True, timeout=300)
    return result.stdout


def _c_rows(rows: list[dict], fields: list[str]) -> str:
    return ',\n'.join('{' + ', '.join(f'.{f} = {int(row[f])}u' for f in fields) + '}'
                      for row in rows)


def descriptor_probe_source(descriptor: dict) -> str:
    """Render one private descriptor as C tables plus a fault-checking run."""
    from tools import sc64_z8_pack
    sc64_z8_pack.validate_structure(descriptor)
    triggers = []
    for trigger in descriptor['triggers']:
        enabled = ', '.join(str(v) for v in trigger['enabled_players'])
        triggers.append(
            '{' + ', '.join(f'.{f} = {trigger[f]}u' for f in (
                'first_condition', 'condition_count', 'first_action',
                'action_count', 'execution_flags', 'initial_cursor')) +
            f', .enabled_players = {{{enabled}}}}}')
    conditions = _c_rows(descriptor['conditions'], [
        'location', 'player', 'amount', 'unit', 'mask', 'comparison',
        'opcode', 'resource', 'flags', 'index'])
    actions = _c_rows(descriptor['actions'], [
        'location', 'string_id', 'wav_id', 'time', 'group1', 'group2',
        'unit', 'opcode', 'modifier', 'flags', 'index'])
    locations = _c_rows(descriptor['locations'], [
        'left', 'top', 'right', 'bottom', 'flags'])
    owners = descriptor['owners']
    forces = descriptor['forces']
    # OWNR 5 is a computer and 6 a human; FORC's first eight bytes are
    # zero-based force numbers.
    active = sum(1 << slot for slot in range(8) if owners[slot] in (5, 6))
    force = ', '.join(str(forces[slot] + 1) for slot in range(8))
    width = descriptor['dimensions']['width'] * 32
    height = descriptor['dimensions']['height'] * 32
    return f'''
#include <stdio.h>
#include "starcraft/sc64_z8_triggers.h"
static const Sc64Z8Trigger T[] = {{{', '.join(triggers)}}};
static const Sc64Z8Condition C[] = {{{conditions}}};
static const Sc64Z8Action A[] = {{{actions}}};
static const Sc64Z8SourceLocation L[] = {{{locations}}};
static int count(void *c, const Sc64Z8Condition *x, unsigned o,
                 const Sc64Z8Rect *r, u32 *n) {{ (void)c; (void)x; (void)o; (void)r; *n = 0; return 1; }}
static int act(void *c, const Sc64Z8Action *x, unsigned o, const Sc64Z8Rect *r)
{{ (void)c; (void)x; (void)o; (void)r; return 1; }}
static int timed(void *c, int ph, const Sc64Z8Action *x, unsigned o,
                 const Sc64Z8Rect *r, u32 *d)
{{ (void)c; (void)ph; (void)x; (void)o; (void)r; *d = 0; return 1; }}
static int find(void *c, const Sc64Z8Action *x, unsigned o,
                const Sc64Z8Rect *r, s16 *px, s16 *py)
{{ (void)c; (void)x; (void)o; (void)r; *px = *py = 0; return 0; }}
int main(void) {{
    const Sc64Z8Program p = {{T, C, A, L, {len(descriptor['triggers'])}, {len(descriptor['conditions'])},
        {len(descriptor['actions'])}, {len(descriptor['locations'])}, {width}, {height}}};
    const Sc64Z8Setup setup = {{{active}u, {{{force}}}, 42}};
    const Sc64Z8WorldOps ops = {{count, act, timed, find}};
    static Sc64Z8SchedulerState s;
    if (!sc64_z8_program_valid(&p)) {{ puts("program rejected"); return 1; }}
    if (!sc64_z8_init(&s, &p, &setup)) {{ puts("init rejected"); return 1; }}
    for (unsigned f = 0; f < 20000; ++f)
        if (!sc64_z8_tick(&s, &p, &ops, 0, 0)) {{
            printf("fault %u owner %u trigger %u slot %u\\n", s.fault,
                   s.fault_owner, s.fault_trigger, s.fault_slot);
            return 1;
        }}
    printf("instances %u passes %u\\n", s.instance_count, s.passes);
    return 0;
}}
'''


class Z8TriggerSchedulerTest(unittest.TestCase):
    def setUp(self) -> None:
        self.clang = shutil.which('clang')
        if not self.clang:
            self.skipTest('clang is not available')

    def test_synthetic_scheduler(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            out = _compile_and_run(self.clang, SYNTHETIC, Path(tmp) / 'z8')
        self.assertIn('sc64_z8_triggers: ok (state 876 bytes', out)

    def test_private_descriptor_rows_are_supported(self) -> None:
        path = os.environ.get('SC64_Z8_DESCRIPTOR')
        if not path:
            self.skipTest('SC64_Z8_DESCRIPTOR is not set')
        descriptor = json.loads(Path(path).read_text(encoding='utf-8'))
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / 'probe.c'
            source.write_text(descriptor_probe_source(descriptor), encoding='ascii')
            out = _compile_and_run(self.clang, source, Path(tmp) / 'probe')
        self.assertRegex(out, r'^instances \d+ passes \d+$')


if __name__ == '__main__':
    unittest.main()
