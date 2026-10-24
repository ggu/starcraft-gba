import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tests" / "sc64_zp_fact_range.c"

# Keep the optional actor gates independent in the matrix: later enum prefixes
# and the P5/P6 append points must preserve the same fact answers.
CONFIGS = {
    "zp": ("SC64_ZP_CAMPAIGN_ACTORS",),
    "zp_z5": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_Z5_ACTORS"),
    "zp_z5_props": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_Z5_ACTORS",
                    "SC64_Z5_DOODADS"),
    "zp_z6": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_Z6_ACTORS"),
    "zp_z5_z6": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_Z5_ACTORS",
                 "SC64_Z6_ACTORS"),
    "zp_p5": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_P5_ACTORS"),
    "zp_p6": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_P6_ACTORS"),
    "zp_p6_combat": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_P6_ACTORS",
                     "SC64_P6_COMBAT_ENABLED"),
    "zp_p7": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_P7_ACTORS"),
    "zp_p7_p6_prefix": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_P6_ACTORS",
                        "SC64_P7_ACTORS"),
    "zp_all_combat": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_Z5_ACTORS",
                      "SC64_Z5_DOODADS", "SC64_Z6_ACTORS", "SC64_P5_ACTORS",
                      "SC64_P6_ACTORS", "SC64_P6_COMBAT_ENABLED"),
    "t4_zp_z5": ("SC64_T4_RUNTIME", "SC64_ZP_CAMPAIGN_ACTORS",
                 "SC64_Z5_ACTORS"),
    "t4_zp_z5_props": ("SC64_T4_RUNTIME", "SC64_ZP_CAMPAIGN_ACTORS",
                       "SC64_Z5_ACTORS", "SC64_Z5_DOODADS"),
    "t4_zp_full": ("SC64_T4_RUNTIME", "SC64_ZP_CAMPAIGN_ACTORS",
                   "SC64_Z5_ACTORS", "SC64_Z5_DOODADS", "SC64_Z6_ACTORS",
                   "SC64_P5_ACTORS", "SC64_P6_ACTORS"),
    "late_terran_all_combat": ("SC64_T4_RUNTIME", "SC64_T5_RUNTIME",
                                "SC64_T6_ENABLED", "SC64_TERRAN_LATE_ACTORS",
                                "SC64_ZP_CAMPAIGN_ACTORS", "SC64_Z5_ACTORS",
                                "SC64_Z5_DOODADS", "SC64_Z6_ACTORS",
                                "SC64_P5_ACTORS", "SC64_P6_ACTORS",
                                "SC64_P6_COMBAT_ENABLED"),
    "late_terran_p7_prefix": ("SC64_T4_RUNTIME", "SC64_T5_RUNTIME",
                               "SC64_T6_ENABLED", "SC64_TERRAN_LATE_ACTORS",
                               "SC64_ZP_CAMPAIGN_ACTORS", "SC64_Z5_ACTORS",
                               "SC64_Z5_DOODADS", "SC64_Z6_ACTORS", "SC64_P5_ACTORS",
                               "SC64_P6_ACTORS", "SC64_P6_COMBAT_ENABLED",
                               "SC64_P7_ACTORS"),
}


def compile_and_run(compiler, defines, range_cull, directory, name):
    executable = pathlib.Path(directory) / name
    command = [compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
               "-fsanitize=address,undefined",
               "-I", str(ROOT / "starcraft")]
    command += [f"-D{define}" for define in defines]
    if range_cull:
        command.append("-DSC64_ZP_FACT_RANGE_CULL")
    command += [str(SOURCE), "-o", str(executable)]
    subprocess.run(command, check=True, cwd=ROOT)
    return subprocess.run([str(executable)], check=True, cwd=ROOT,
                          stdout=subprocess.PIPE).stdout


class FactRangeCullTest(unittest.TestCase):
    def test_exact_fact_results_across_gate_combinations(self):
        compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
        self.assertIsNotNone(compiler, "clang or cc is required")
        with tempfile.TemporaryDirectory(prefix="sc64-zp-fact-range-") as directory:
            for name, defines in CONFIGS.items():
                with self.subTest(configuration=name):
                    baseline = compile_and_run(compiler, defines, False,
                                               directory, name + "-baseline")
                    optimized = compile_and_run(compiler, defines, True,
                                                directory, name + "-cull")
                    self.assertEqual(optimized, baseline)
                    rows = baseline.splitlines()
                    actor_rows = [row for row in rows if row.startswith(b"U:")]
                    building_rows = [row for row in rows if row.startswith(b"B:")]
                    self.assertTrue(any(b":NULL|" not in row for row in actor_rows))
                    self.assertTrue(any(b":NULL|" not in row for row in building_rows))
                    self.assertIn(b"U:-2147483648:NULL|", baseline)
                    self.assertIn(b"U:2147483647:NULL|", baseline)
                    self.assertIn(b"B:-2147483648:NULL|", baseline)
                    self.assertIn(b"B:2147483647:NULL|", baseline)


if __name__ == "__main__":
    unittest.main()
