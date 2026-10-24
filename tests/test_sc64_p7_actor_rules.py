import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tests" / "sc64_p7_actor_rules.c"

CONFIGS = {
    "base": ("SC64_ZP_CAMPAIGN_ACTORS",),
    "p7": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_P7_ACTORS"),
    "p7_p6": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_P6_ACTORS",
              "SC64_P7_ACTORS"),
    "p7_p6_combat": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_P6_ACTORS",
                      "SC64_P6_COMBAT_ENABLED", "SC64_P7_ACTORS"),
    "zp_full_p7_save_budget": ("SC64_ZP_CAMPAIGN_ACTORS", "SC64_Z5_ACTORS",
                                "SC64_Z5_DOODADS", "SC64_Z6_RUNTIME",
                                "SC64_Z6_ACTORS", "SC64_P5_ACTORS",
                                "SC64_P6_RUNTIME", "SC64_P6_ACTORS",
                                "SC64_P6_COMBAT_ENABLED", "SC64_P7_ACTORS"),
    "late_terran_p7": ("SC64_T4_RUNTIME", "SC64_T5_RUNTIME", "SC64_T6_ENABLED",
                        "SC64_TERRAN_LATE_ACTORS", "SC64_ZP_CAMPAIGN_ACTORS",
                        "SC64_Z5_ACTORS", "SC64_Z5_DOODADS", "SC64_Z6_ACTORS",
                        "SC64_P5_ACTORS", "SC64_P6_ACTORS",
                        "SC64_P6_COMBAT_ENABLED", "SC64_P7_ACTORS"),
}


class P7ActorRulesTest(unittest.TestCase):
    def test_p7_facts_and_mappings(self):
        compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
        self.assertIsNotNone(compiler, "clang or cc is required")
        with tempfile.TemporaryDirectory(prefix="sc64-p7-facts-") as directory:
            for name, defines in CONFIGS.items():
                with self.subTest(configuration=name):
                    executable = pathlib.Path(directory) / name
                    command = [compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                               "-Werror", "-fsanitize=address,undefined", "-I",
                               str(ROOT / "starcraft")]
                    command.extend(f"-D{macro}" for macro in defines)
                    command.extend((str(SOURCE), "-o", str(executable)))
                    subprocess.run(command, check=True, cwd=ROOT)
                    result = subprocess.run([str(executable)], check=True, cwd=ROOT,
                                            stdout=subprocess.PIPE).stdout
                    self.assertTrue(result.endswith(b"\n"))
                    self.assertIn(b"P7", result)
                    if name == "zp_full_p7_save_budget":
                        self.assertIn(b"ENUMS:49,60\n", result)


if __name__ == "__main__":
    unittest.main()
