# SPDX-License-Identifier: GPL-3.0-or-later
"""Private-input pixel fixture for completed, firing and constructing cannons."""
from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
PRIVATE_HEADER = Path(os.environ.get(
    "SC64_RACE_SPRITES_INCLUDE",
    ROOT / ".local/sc64/races/generated",
)).resolve()
FULL_PROFILE_HEADER = Path(os.environ.get(
    "SC64_RACE_SPRITES_FULL_HEADER",
    ROOT / ".local/build/stock-art/all-profile",
)).resolve()


class PhotonCannonRendererTests(unittest.TestCase):
    def test_base_race_enums_have_unconditional_opaque_sprites(self) -> None:
        compiler = shutil.which("clang") or shutil.which("cc")
        header = PRIVATE_HEADER / "sc64_race_sprites_generated.h"
        if not compiler or not header.is_file():
            self.skipTest("compiler and private base race sprite header are required")
        with tempfile.TemporaryDirectory(prefix="sc64-base-roster-") as temp:
            directory = Path(temp)
            source = directory / "base-roster.c"
            executable = directory / "base-roster"
            source.write_text('''#include <assert.h>
#include "sc64_race_sprites_generated.h"
#include "race_rules.h"
static void check(unsigned short raw) {
    const Sc64SpriteFrame *s = sc64_race_unit_sprite(raw);
    assert(s && s->pixels && s->w && s->h);
    unsigned opaque = 0;
    for (unsigned i = 0; i < (unsigned)s->w * s->h; ++i)
        opaque += s->pixels[i] != 0;
    assert(opaque);
}
int main(void) {
    for (int type = 0; type < SC_RU_COUNT; ++type) {
        int raw = sc64_race_raw_unit(type);
        if (raw >= 0 && raw != 101) check((unsigned short)raw);
    }
    for (int type = 0; type < SC_RB_COUNT; ++type) {
        int raw = sc64_race_raw_building(type);
        if (raw >= 0) check((unsigned short)raw);
    }
    check(19); /* T6/T1 Raynor exists even in a base-only compile. */
    check(20); /* The base U_RAYNOR enum resolves to raw 20. */
    check(79); /* U_TASSADAR's source hero art is also part of the base set. */
    return 0;
}
''')
            subprocess.run(
                [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                 "-Wno-unused-function", "-DSC64_RACES",
                 f"-I{ROOT / 'starcraft'}", f"-I{PRIVATE_HEADER}",
                 str(source), "-o", str(executable)],
                check=True, capture_output=True, text=True)
            subprocess.run([str(executable)], check=True,
                           capture_output=True, text=True)

    def test_native_cannon_pixels_reach_game_renderer(self) -> None:
        compiler = shutil.which("clang") or shutil.which("cc")
        if not compiler:
            self.skipTest("clang or cc is required for the renderer fixture")
        required = (
            PRIVATE_HEADER / "sc64_race_sprites_generated.h",
            ROOT / ".local/sc64/backwater/generated/sc64_mission_generated.h",
            ROOT / ".local/sc64/backwater/generated/sc64_assets_generated.h",
            ROOT / ".local/sc64/backwater/generated/sc64_sprites_generated.h",
            ROOT / ".local/sc64/races/generated/sc64_races_generated.h",
        )
        if not all(path.is_file() for path in required):
            self.skipTest("private imported race sprites and maps are unavailable")
        with tempfile.TemporaryDirectory(prefix="sc64-photon-render-") as temp:
            executable = Path(temp) / "renderer-fixture"
            command = [
                compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                "-Werror", "-Wno-unused-function", "-Wno-unused-variable",
                "-DGBA_HOST", f"-I{ROOT / 'common'}", f"-I{ROOT / 'starcraft'}",
                "-DSC64_IMPORTED_MISSION", "-DSC64_HAS_TERRAIN_ART",
                "-DSC64_HAS_SPRITES", f"-I{ROOT / '.local/sc64/backwater/generated'}",
                "-DSC64_RACES", f"-I{PRIVATE_HEADER}",
                f"-I{ROOT / '.local/sc64/races/generated'}",
                str(ROOT / "tests/sc64_photon_cannon_renderer.c"),
                str(ROOT / "common/gba.c"), "-o", str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run([str(executable)], check=True, capture_output=True, text=True)

    def test_every_full_profile_sprite_mapping_has_opaque_pixels(self) -> None:
        compiler = shutil.which("clang") or shutil.which("cc")
        header = FULL_PROFILE_HEADER / "sc64_race_sprites_generated.h"
        if not compiler or not header.is_file():
            self.skipTest("compiler and complete private race sprite header are required")
        from tools.sc64_race_sprites import (
            LATE_BUILDING_IDS, LATE_UNIT_IDS, P6_DOODAD_IDS, P6_UNIT_IDS,
            P7_BUILDING_IDS, P7_UNIT_IDS, RACE_UNIT_IDS, T4_BUILDING_IDS,
            T4_UNIT_IDS, T6_UNIT_IDS, Z5_BUILDING_IDS, Z5_DOODAD_IDS,
            Z5_UNIT_IDS, Z6_BUILDING_IDS, Z6_UNIT_IDS, ZP_BUILDING_IDS,
            ZP_UNIT_IDS,
        )

        def check_call(call: str) -> str:
            return (f"{{ const Sc64SpriteFrame *s = {call}; unsigned n = 0; "
                    "assert(s && s->pixels && s->w && s->h); "
                    "for (unsigned i = 0; i < (unsigned)s->w * s->h; ++i) "
                    "n += s->pixels[i] != 0; assert(n > 0); }\n")

        base_ids = sorted(set(RACE_UNIT_IDS.values()))
        scenario_units = sorted(set(T4_UNIT_IDS.values()) | set(T6_UNIT_IDS.values()) |
                                set(LATE_UNIT_IDS.values()) | set(ZP_UNIT_IDS.values()) |
                                set(P6_UNIT_IDS.values()) | set(P7_UNIT_IDS.values()))
        calls = [check_call(f"sc64_race_unit_sprite({raw})") for raw in base_ids]
        calls += [check_call(f"sc64_race_unit_sprite({raw})") for raw in scenario_units]
        calls += [check_call(f"sc64_z5_actor_sprite({raw})")
                  for raw in sorted(set(Z5_UNIT_IDS.values()))]
        calls += [check_call(f"sc64_z6_actor_sprite({raw})")
                  for raw in sorted(set(Z6_UNIT_IDS.values()))]
        building_ids = {162} | set(T4_BUILDING_IDS.values()) | set(LATE_BUILDING_IDS.values())
        building_ids |= set(ZP_BUILDING_IDS.values()) | set(Z5_BUILDING_IDS.values())
        building_ids |= set(Z5_DOODAD_IDS.values()) | set(P6_DOODAD_IDS.values())
        building_ids |= set(Z6_BUILDING_IDS.values()) | set(P7_BUILDING_IDS.values())
        calls += [check_call(f"sc64_race_building_sprite({raw}, 0)")
                  for raw in sorted(building_ids)]

        with tempfile.TemporaryDirectory(prefix="sc64-race-atlas-") as temp:
            directory = Path(temp)
            source = directory / "atlas.c"
            executable = directory / "atlas"
            source.write_text(
                '#include <assert.h>\n#include "sc64_race_sprites_generated.h"\n'
                'int main(void) {\n' + ''.join(calls) + 'return 0;\n}\n')
            flags = [
                "-DSC64_RACES", "-DSC64_T4_ENABLED", "-DSC64_T6_ENABLED",
                "-DSC64_TERRAN_LATE_ENABLED", "-DSC64_ZP_CAMPAIGN_ACTORS",
                "-DSC64_Z5_ACTORS", "-DSC64_Z5_DOODADS", "-DSC64_Z6_RUNTIME",
                "-DSC64_Z6_ACTORS", "-DSC64_P6_ACTORS", "-DSC64_P7_ACTORS",
            ]
            compiled = subprocess.run(
                [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", *flags,
                 f"-I{FULL_PROFILE_HEADER}", str(source), "-o", str(executable)],
                capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            subprocess.run([str(executable)], check=True, capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()
