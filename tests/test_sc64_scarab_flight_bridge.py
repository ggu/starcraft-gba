# SPDX-License-Identifier: GPL-3.0-or-later
"""Source-only bridge tests against real Unit layouts and four codec families."""
import pathlib, subprocess, tempfile, unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
class ScarabFlightBridgeTest(unittest.TestCase):
    def test_profiles(self):
        game = (ROOT / 'starcraft/game.c').read_text()
        end = game.index('} Unit;') + len('} Unit;')
        definition = game[game.rindex('typedef struct {', 0, end):end]
        fixture = (ROOT / 'tests/sc64_scarab_flight_bridge.c').read_text().replace('/* SC64_REAL_UNIT_DEFINITION */', definition).replace('"../starcraft/', '"' + str(ROOT / 'starcraft') + '/')
        save = (ROOT / 'starcraft/race_save_adapter.h').read_text()
        start = save.index('    for(int i=0;i<MAX_UNITS;i++)for(unsigned j=0;j<RACE_UNIT_WORDS;j++) {', save.index('static int race_save_game(void)'))
        end = save.index('\n#endif\n#if defined(SC64_T6_STORAGE)', start)
        fixture = fixture.replace('/* SC64_REAL_LEGACY_ENCODE */', save[start:end])
        start = save.index('    for(int i=0;i<MAX_UNITS;i++)for(unsigned j=0;j<RACE_UNIT_WORDS;j++) {', save.index('static int race_load_game(void)'))
        end = save.index('\n#endif\n#if defined(SC64_T6_STORAGE)', start)
        fixture = fixture.replace('/* SC64_REAL_LEGACY_DECODE */', save[start:end])
        profiles = {'terran':'T4_RUNTIME T5_RUNTIME TERRAN_LATE_ACTORS', 'zerg':'ZP_CAMPAIGN_ACTORS Z5_ACTORS Z6_ACTORS', 'p7':'ZP_CAMPAIGN_ACTORS Z5_ACTORS Z6_ACTORS P5_ACTORS P6_ACTORS P7_ACTORS P7_PROJECTILES_ENABLED', 'showcase':'ZP_CAMPAIGN_ACTORS Z5_ACTORS Z6_ACTORS P5_ACTORS P6_ACTORS P7_ACTORS'}
        with tempfile.TemporaryDirectory() as d:
            src=pathlib.Path(d)/'bridge.c'; exe=pathlib.Path(d)/'bridge';src.write_text(fixture)
            for name,features in profiles.items():
                for compact in (False,True):
                    with self.subTest(profile=name,compact=compact):
                        flags=['-DSC64_'+x for x in features.split()]
                        if compact: flags+=['-DSC64_LATE_TERRAN_STORAGE']
                        subprocess.run(['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-tautological-constant-out-of-range-compare','-fsanitize=address,undefined','-DSC64_SCARAB_RUNTIME','-DSC64_SKIRMISH_ROSTER','-DSC64_BATTLECRUISER_EXPERIMENT',*flags,str(src),'-o',str(exe)],check=True)
                        subprocess.run([str(exe)],check=True)
