# SPDX-License-Identifier: GPL-3.0-or-later
"""Private-input pixel fixture for completed, firing and constructing cannons."""
from __future__ import annotations

import os
from pathlib import Path
import shlex
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


def renderer_dry_run(target: str) -> subprocess.CompletedProcess[str]:
    """Resolve the target's real compile recipe without rebuilding private inputs."""
    database = subprocess.run(
        ["make", "-rRqp", "--no-print-directory", target],
        cwd=ROOT, capture_output=True, text=True)
    matches = [line for line in database.stdout.splitlines()
               if line.startswith(target + ":")]
    if len(matches) != 1:
        raise AssertionError(f"Expected one Make target for {target}: {database.stderr}")
    prerequisites = shlex.split(matches[0].split(":", 1)[1])
    generated = sorted({path for path in prerequisites
                        if path.startswith((".local/", "build/"))})
    old_args = [item for path in generated for item in ("-o", path)]
    return subprocess.run(
        ["make", "-Bn", "--no-print-directory", *old_args, target],
        cwd=ROOT, capture_output=True, text=True)


class PhotonCannonRendererTests(unittest.TestCase):
    def test_canonical_zerg_dry_run_uses_z7_and_keeps_z4(self) -> None:
        z7 = renderer_dry_run("build/test-sc64-z7-canonical-frame")
        self.assertEqual(z7.returncode, 0, z7.stderr)
        self.assertIn("-DSC64_Z7_RUNTIME", z7.stdout)
        self.assertIn("-I.local/sc64/zerg-z6/generated", z7.stdout)
        self.assertIn("tests/sc64_z3_frame_coverage.c", z7.stdout)
        z4 = renderer_dry_run("build/test-sc64-z4-frame-coverage")
        self.assertEqual(z4.returncode, 0, z4.stderr)
        self.assertNotIn("-DSC64_Z7_RUNTIME", z4.stdout)
        self.assertIn("-DSC64_Z4_RUNTIME", z4.stdout)

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
            ROOT / ".local/sc64/briefings/generated/sc64_briefing_generated.h",
            ROOT / ".local/sc64/interface/generated/sc64_font_generated.h",
            ROOT / ".local/sc64/interface/generated/sc64_icons_generated.h",
            ROOT / ".local/sc64/interface/generated/sc64_chrome_generated.h",
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
                f"-I{ROOT / '.local/sc64/briefings/generated'}",
                f"-I{ROOT / '.local/sc64/interface/generated'}",
                "-DSC64_RACES", f"-I{PRIVATE_HEADER}",
                f"-I{ROOT / '.local/sc64/races/generated'}",
                str(ROOT / "tests/sc64_photon_cannon_renderer.c"),
                str(ROOT / "common/gba.c"), "-o", str(executable),
            ]
            compiled = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0,
                             compiled.stderr or compiled.stdout or "renderer fixture did not compile")
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

    def test_canonical_profiles_resolve_their_runtime_sprite_rosters(self) -> None:
        """Check each shipped profile's actual enum-to-draw resolver.

        The all-profile atlas above deliberately enables every group, which can
        hide missing conditional cases in a canonical ROM's selected groups.
        Reuse each canonical renderer test's exact Make flags and include order.
        """
        compiler = shutil.which("clang") or shutil.which("cc")
        if not compiler:
            self.skipTest("clang or cc is required for the canonical sprite fixture")

        from tools.sc64_race_sprites import (
            LATE_BUILDING_IDS, LATE_UNIT_IDS, RACE_UNIT_IDS,
            T4_BUILDING_IDS, T4_UNIT_IDS, T6_UNIT_IDS,
            Z5_BUILDING_IDS, Z5_DOODAD_IDS, Z5_UNIT_IDS, Z6_BUILDING_IDS,
            Z6_UNIT_IDS, ZP_BUILDING_IDS, ZP_UNIT_IDS,
        )

        profiles = (
            ("terran", "build/test-sc64-terran-full-frame",
             Path(".local/sc64/terran-late/import-stage/sc64_race_sprites_generated.h"),
             (".local/sc64/terran-late/import-stage/sc64_races_generated.h",
              ".local/sc64/races/generated/sc64_t4_scenario_generated.h",
              ".local/sc64/races/generated/sc64_t5_scenario_generated.h",
              ".local/sc64/races/generated/sc64_t5_availability_generated.h",
              ".local/sc64/races/generated/sc64_t6_scenario_generated.h",
              ".local/sc64/races/generated/sc64_t6_availability_generated.h",
              *tuple(f".local/sc64/terran-late/import-stage/sc64_t{n}_{suffix}_generated.h"
                     for n in range(7, 11) for suffix in ("scenario", "availability"))),
             set(T4_UNIT_IDS.values()) | set(T6_UNIT_IDS.values()) |
             set(LATE_UNIT_IDS.values()),
             set(T4_BUILDING_IDS.values()) | set(LATE_BUILDING_IDS.values()), set()),
            ("zerg-z4", "build/test-sc64-z4-frame-coverage",
             Path(".local/sc64/zp-stage/generated/sc64_race_sprites_generated.h"),
             (".local/sc64/zp-stage/generated/sc64_races_generated.h",
              ".local/sc64/z4/generated/sc64_z4_scenario_generated.h"),
             set(ZP_UNIT_IDS.values()), set(), set(ZP_BUILDING_IDS.values())),
            ("zerg", "build/test-sc64-z7-canonical-frame",
             Path(".local/sc64/zerg-z6/generated/sc64_race_sprites_generated.h"),
             (".local/sc64/zp-stage/generated/sc64_races_generated.h",
              ".local/sc64/zp-stage/generated/scenarios/sc64_z7_scenario_generated.h"),
             set(ZP_UNIT_IDS.values()),
             set(Z5_BUILDING_IDS.values()) | set(Z5_DOODAD_IDS.values()) |
             set(Z6_BUILDING_IDS.values()), set(ZP_BUILDING_IDS.values())),
            ("protoss", "build/test-sc64-protoss-canonical-frame",
             Path(".local/sc64/zp-stage/generated/sc64_race_sprites_generated.h"),
             (".local/sc64/zp-stage/generated/sc64_races_generated.h",
              ".local/sc64/p4/generated/sc64_p4_scenario_generated.h"),
             set(ZP_UNIT_IDS.values()), set(), set(ZP_BUILDING_IDS.values())),
        )

        def opaque_check(call: str, label: str) -> str:
            return (f'check_sprite({call}, "{label}");\n')

        common_inputs = (
            ".local/sc64/backwater/generated/sc64_mission_generated.h",
            ".local/sc64/backwater/generated/sc64_assets_generated.h",
            ".local/sc64/backwater/generated/sc64_sprites_generated.h",
            ".local/sc64/races/generated/sc64_races_generated.h",
            ".local/sc64/races/generated/sc64_race_sprites_generated.h",
            ".local/sc64/races/generated/sc64_menu_generated.h",
            ".local/sc64/briefings/generated/sc64_briefing_generated.h",
            ".local/sc64/interface/generated/sc64_font_generated.h",
            ".local/sc64/interface/generated/sc64_icons_generated.h",
            ".local/sc64/interface/generated/sc64_chrome_generated.h",
            ".local/sc64/skirmish/generated/sc64_skirmish_generated.h",
        )

        for profile, target, header, profile_inputs, extra_units, extra_buildings, zp_buildings in profiles:
            with self.subTest(profile=profile):
                if not (ROOT / header).is_file():
                    self.skipTest(f"prepared {profile} sprite header is unavailable: {header}")
                missing_inputs = [path for path in common_inputs + profile_inputs
                                  if not (ROOT / path).is_file()]
                if missing_inputs:
                    self.skipTest("prepared private inputs are incomplete: " +
                                  ", ".join(missing_inputs))

                explicit_units = sorted(set(RACE_UNIT_IDS.values()) | extra_units)
                explicit_buildings = sorted(extra_buildings | zp_buildings)
                source_lines = [
                    '#include <assert.h>\n#include <stdio.h>\n#include <string.h>\n',
                    f'#include "{ROOT / "starcraft/game.c"}"\n',
                    'static unsigned char actual[240 * 160];\n',
                    'static unsigned char expected_pixels[240 * 160];\n',
                    'static void check_sprite(const Sc64SpriteFrame *s, const char *id) {\n',
                    '  if(!s||!s->pixels||!s->w||!s->h) fprintf(stderr,"missing sprite: %s\\n",id); '
                    'assert(s && s->pixels && s->w && s->h); unsigned n=0;\n',
                    '  for (unsigned i=0;i<(unsigned)s->w*s->h;++i) n += s->pixels[i] != 0;\n',
                    '  if (!n) fprintf(stderr,"empty sprite: %s\\n",id); assert(n);\n',
                    '}\n',
                    'static void capture(unsigned char *p) { for(int y=0;y<160;++y) '
                    'for(int x=0;x<240;++x) p[y*240+x]=gfx_test_pixel(x,y); }\n',
                    'static void compose(unsigned char *p,const Sc64SpriteFrame *s,int cx,int cy) {\n',
                    '  int l=cx-s->canvas_w/2+s->offset_x, t=cy-s->canvas_h/2+s->offset_y;\n',
                    '  for(int y=0;y<s->h;++y) for(int x=0;x<s->w;++x) { unsigned char c=s->pixels[y*s->w+x]; '
                    'int dx=l+x,dy=t+y; if(c&&dx>=0&&dx<240&&dy>=0&&dy<160) p[dy*240+dx]=c; }\n',
                    '}\n',
                    'static void check_building_pixels(int type,int raw,const Sc64SpriteFrame *s) {\n',
                    '  Building q; memset(&q,0,sizeof(q)); q.x=120; q.y=72; q.owner=SIDE; '
                    'q.type=type; q.raw_type=(short)raw; q.alive=1; q.hp=q.maxhp=1000;\n',
                    '  camx=camy=0; gfx_begin(C_BLACK); draw_building_art(&q); capture(actual);\n',
                    '  memset(expected_pixels,C_BLACK,sizeof(expected_pixels)); compose(expected_pixels,s,q.x,q.y+12);\n',
                    '  if(memcmp(actual,expected_pixels,sizeof(actual))) fprintf(stderr,"building pixels differ type=%d raw=%d\\n",type,raw); '
                    'assert(memcmp(actual,expected_pixels,sizeof(actual))==0);\n',
                    '  b[0]=q; selected_building=0; gfx_begin(C_BLACK); draw_building_overlay(&b[0]); '
                    'capture(actual); selected_building=-1;\n',
                    '  int sy=q.y+12-s->canvas_h/2+s->offset_y;\n',
                    '  gfx_begin(C_BLACK);\n',
                    '#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)\n',
                    '  if(!sc64_game_building_has_flag(q.type,SC64_GAME_ACTOR_INVINCIBLE)) {\n',
                    '#else\n',
                    '  {\n',
                    '#endif\n',
                    '  draw_actor_vitals(q.x,sy+s->h+3,s->w>46?46:s->w,q.hp,q.maxhp,q.shield, '
                    'sc64_actor_max_shields(q.type,1),0,0); } capture(expected_pixels);\n',
                    '  if(memcmp(actual,expected_pixels,sizeof(actual))) fprintf(stderr,"building vitals differ type=%d raw=%d\\n",type,raw); '
                    'assert(memcmp(actual,expected_pixels,sizeof(actual))==0);\n',
                    '}\n',
                    'int main(void) { unsigned invisible=0;\n',
                    '  for(int t=0;t<SC_RU_COUNT;++t) { int raw=sc64_race_raw_unit(t); if(raw<0) continue; '
                    'if(raw==101) { assert(mission_unit_sprite(t)==0); ++invisible; continue; } '
                    'check_sprite(mission_unit_sprite(t),sc64_race_unit_name(t)); }\n',
                    '  for(int t=0;t<SC_RB_COUNT;++t) { int raw=sc64_race_raw_building(t); if(raw<0) continue; '
                    f'if({"||".join(f"raw=={raw}" for raw in sorted(T4_BUILDING_IDS.values())) or "0"}) '
                    'check_sprite(sc64_race_building_sprite((unsigned short)raw,0),sc64_race_building_name(t)); '
                    'else check_sprite(mission_building_sprite(t),sc64_race_building_name(t)); }\n',
                    '  assert(invisible <= 2);\n',
                ]
                for raw in explicit_units:
                    source_lines.append(opaque_check(f"sc64_race_unit_sprite({raw})",
                                                     f"unit raw {raw}"))
                for raw in explicit_buildings:
                    source_lines.append(opaque_check(f"sc64_race_building_sprite({raw},0)",
                                                     f"building raw {raw}"))
                if profile == "zerg":
                    for raw in sorted(set(Z5_UNIT_IDS.values())):
                        source_lines.append(opaque_check(f"sc64_z5_actor_sprite({raw})",
                                                         f"Z5 unit raw {raw}"))
                    for raw in sorted(set(Z6_UNIT_IDS.values())):
                        source_lines.append(opaque_check(f"sc64_z6_actor_sprite({raw})",
                                                         f"Z6 unit raw {raw}"))
                if zp_buildings:
                    for name, raw in sorted(ZP_BUILDING_IDS.items(), key=lambda pair: pair[1]):
                        enum = {
                            115: "B_ZP_CONTROL_TOWER", 120: "B_ZP_MACHINE_SHOP",
                            123: "B_ZP_ARMORY", 138: "B_ZP_QUEENS_NEST",
                            140: "B_ZP_ULTRALISK_CAVERN", 196: "B_ZP_PROTOSS_BEACON",
                        }[raw]
                        source_lines.append(
                            f'  check_building_pixels({enum},{raw},sc64_race_building_sprite({raw},0));\n'
                            f'  assert(mission_building_sprite({enum}) == sc64_race_building_sprite({raw},0));\n')
                source_lines.append('  return 0; }\n')

                with tempfile.TemporaryDirectory(prefix=f"sc64-{profile}-sprite-roster-") as temp:
                    directory = Path(temp)
                    source = directory / "canonical-roster.c"
                    executable = directory / "canonical-roster"
                    source.write_text("".join(source_lines), encoding="ascii")
                    dry_run = renderer_dry_run(target)
                    optional_banks = (
                        ".local/sc64/audio/generated/sc64_audio_generated.bin",
                        ".local/sc64/construction/generated/sc64_construction_sprites_generated.h",
                    )
                    if (dry_run.returncode and "No rule to make target" in dry_run.stderr
                            and any(path in dry_run.stderr for path in optional_banks)):
                        self.skipTest("prepared private audio or construction bank is incomplete")
                    self.assertEqual(dry_run.returncode, 0, dry_run.stderr)
                    compile_commands = [
                        shlex.split(line)
                        for line in dry_run.stdout.replace("\\\n", " ").splitlines()]
                    compile_commands = [argv for argv in compile_commands
                                        if "common/gba.c" in argv and "-o" in argv and
                                        any(item.endswith(".c") and
                                            ("frame_coverage.c" in item)
                                            for item in argv)]
                    self.assertTrue(compile_commands, dry_run.stdout)
                    command = compile_commands[-1]
                    command[0] = compiler

                    required = [ROOT / path for path in common_inputs + profile_inputs]
                    if "-DSC64_CONSTRUCTION_SOURCE_ART" in command:
                        required.append(ROOT / ".local/sc64/construction/generated/"
                                        "sc64_construction_sprites_generated.h")
                    if "-DSC64_AUDIO_IMPORTED" in command:
                        required.extend((ROOT / ".local/sc64/audio/generated/"
                                         "sc64_audio_generated.h",
                                         ROOT / ".local/sc64/audio/generated/"
                                         "sc64_audio_generated.bin"))
                    missing = [path.relative_to(ROOT).as_posix()
                               for path in required if not path.is_file()]
                    if missing:
                        self.skipTest("prepared private inputs are incomplete: " +
                                       ", ".join(missing))

                    command = [str(source) if item.startswith("tests/") and item.endswith(".c")
                               else item for item in command]
                    out = command.index("-o")
                    command[out + 1] = str(executable)
                    compiled = subprocess.run(command, cwd=ROOT,
                                              capture_output=True, text=True)
                    self.assertEqual(compiled.returncode, 0, compiled.stderr)
                    executed = subprocess.run([str(executable)], cwd=ROOT,
                                              capture_output=True, text=True)
                    self.assertEqual(executed.returncode, 0,
                                     executed.stderr or executed.stdout)


if __name__ == "__main__":
    unittest.main()
