#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free host checks for the typed Z8 live-world callback bridge."""
from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'tests/sc64_z8_world.c'
FLAGS = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-pedantic',
         '-fsanitize=address,undefined', '-fno-sanitize-recover=all']


class Z8WorldAdapterTest(unittest.TestCase):
    def test_typed_live_world_callbacks(self) -> None:
        clang = shutil.which('clang')
        if not clang:
            self.skipTest('clang is not available')
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'sc64-z8-world'
            subprocess.run([clang, *FLAGS, f'-I{ROOT}', str(SOURCE),
                            '-o', str(binary)], check=True, capture_output=True,
                            text=True)
            result = subprocess.run([str(binary)], check=True, capture_output=True,
                                    text=True, timeout=30)
        self.assertIn('sc64_z8_world: ok', result.stdout)

    def test_verified_descriptor_action_targets(self) -> None:
        descriptor_path = os.environ.get('SC64_Z8_DESCRIPTOR')
        if not descriptor_path:
            self.skipTest('SC64_Z8_DESCRIPTOR is not set')
        descriptor = json.loads(Path(descriptor_path).read_text(encoding='utf-8'))
        from tools import sc64_z8_pack
        sc64_z8_pack.validate_structure(descriptor)

        def rows(opcode: int) -> str:
            result = []
            for row in descriptor['actions']:
                if row['opcode'] != opcode:
                    continue
                fields = ('location', 'group1', 'group2', 'unit', 'opcode',
                          'modifier', 'index')
                result.append('{' + ','.join(f'.{f}={row[f]}u' for f in fields) + '}')
            return ',\n'.join(result)

        force = ','.join(str(int(v) + 1) for v in descriptor['forces'][:8])
        active = sum(1 << slot for slot, owner in enumerate(descriptor['owners'][:8])
                     if owner in (5, 6, 7))
        create_count = sum(row['opcode'] == 11 for row in descriptor['actions'])
        resource_count = sum(row['opcode'] == 26 for row in descriptor['actions'])
        invincible_count = sum(row['opcode'] == 43 for row in descriptor['actions'])
        force_setup = ' '.join(f'w.force[{i}]={int(value) + 1};'
                               for i, value in enumerate(descriptor['forces'][:8]))
        source = f'''#include <assert.h>
#include <stdio.h>
#include "starcraft/sc64_z8_world_adapter.h"
static const Sc64Z8Action creates[] = {{{rows(11)}}};
static const Sc64Z8Action resources[] = {{{rows(26)}}};
static const Sc64Z8Action invincibility[] = {{{rows(43)}}};
static unsigned create_calls, seen_property, seen_owner;
static int create(void *c,const Sc64Z8Action *a,unsigned owner,unsigned count,
 unsigned property,unsigned x1,unsigned y1,unsigned x2,unsigned y2) {{
 (void)c;(void)a;(void)x1;(void)y1;(void)x2;(void)y2;
 assert(count==1); ++create_calls; seen_property=property; seen_owner=owner; return 1;
}}
int main(void) {{
 Sc64Z8WorldView w={{0}}; Sc64ResourceActionState wallet[8]={{{{0}}}};
 u8 inv=0; Sc64Z8ActorView actor[2]={{{{.raw_type=74,.source_owner=1,.x=20,.y=20,
 .left=8,.top=8,.right=8,.bottom=8,.alive=1,.completed=1,.type_known=1,.invincible=&inv}},
 {{.raw_type=74,.source_owner=11,.x=24,.y=24,.left=8,.top=8,.right=8,.bottom=8,
 .alive=1,.completed=1,.type_known=1}}}};
 Sc64Z8Rect rect={{0,0,128,128}};
 w.active_mask={active}u; w.actor_count=2; w.actors=actor;
 {force_setup}
 w.services.create=create;
 for(unsigned i=0;i<8;++i) w.wallet[i]=&wallet[i];
 for(unsigned i=0;i<{create_count}u;++i) {{
   assert(creates[i].group2==0u);
   assert(sc64_z8_world_action(&w,&creates[i],3,&rect)==1);
   unsigned target=255;
   assert(sc64_z8_world_resolve_one(&w,creates[i].group1,3,&target));
   assert(seen_owner==target && seen_property==creates[i].group2);
 }}
 assert(create_calls=={create_count}u);
 for(unsigned i=0;i<{resource_count}u;++i)
   assert(sc64_z8_world_action(&w,&resources[i],3,0)==1);
 assert(wallet[3].current_minerals==350u && wallet[3].current_gas==350u);
 assert(wallet[2].current_minerals==8000u && wallet[2].current_gas==8000u);
 assert(wallet[1].current_minerals==8000u && wallet[1].current_gas==8000u);
 for(unsigned i=0;i<{invincible_count}u;++i) {{
   assert(sc64_z8_world_action(&w,&invincibility[i],3,&rect)==1 && inv==1u);
   inv=0;
 }}
 {{ Sc64Z8Condition c={{.opcode=2,.player=17,.unit=74,.comparison=0}}; u32 count=0;
   assert(sc64_z8_world_count(&w,&c,3,&rect,&count)==1 && count==1u); }}
 printf("native rows: create %u resource %u invincibility %u\\n",
        create_calls,{resource_count}u,{invincible_count}u);
 return 0;
}}
'''

        clang = shutil.which('clang')
        if not clang:
            self.skipTest('clang is not available')
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            c_file = directory / 'native_rows.c'
            binary = directory / 'native_rows'
            c_file.write_text(source, encoding='ascii')
            compiled = subprocess.run([clang, *FLAGS, f'-I{ROOT}', str(c_file), '-o',
                                      str(binary)], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            result = subprocess.run([str(binary)], check=True, capture_output=True,
                                    text=True, timeout=30)
        self.assertIn(f'native rows: create {create_count}', result.stdout)

    def test_game_pool_actor_kinds_match_verified_descriptor(self) -> None:
        descriptor_path = os.environ.get('SC64_Z8_DESCRIPTOR')
        if not descriptor_path:
            self.skipTest('SC64_Z8_DESCRIPTOR is not set')
        descriptor = json.loads(Path(descriptor_path).read_text(encoding='utf-8'))
        from tools import sc64_z8_pack
        sc64_z8_pack.validate_structure(descriptor)

        units = {37, 38, 39, 41, 42, 51, 64, 65, 66, 67, 70, 83, 84}
        buildings = {131, 134, 143, 154, 155, 156, 159, 160,
                     162, 163, 164, 165, 166, 167, 171, 194}
        source_types = {int(raw) for raw in
                        descriptor['histograms']['unit_raw_type']}
        non_actors = {176, 177, 178, 188, 214}
        self.assertEqual(source_types - non_actors, units | buildings)


if __name__ == '__main__':
    unittest.main()
