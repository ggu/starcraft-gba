# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free production-header tests for the P7 sight-radius wrapper."""
import json
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests" / "sc64_p7_sight_runtime_fixture.c"
PROJECT = pathlib.Path(os.environ.get("SC64_FACT_V2_SOURCE_ROOT", ROOT)).resolve()
GENERATOR = ROOT / "tools" / "sc64_fact_tables_gen.c"
GATES = (
    "SC64_RACES", "SC64_UNIFIED", "SC64_P3_ENABLED",
    "SC64_ZP_CAMPAIGN_ACTORS", "SC64_ZP_CAMPAIGN_STORAGE",
    "SC64_P5_ACTORS", "SC64_Z6_ACTORS", "SC64_P6_ACTORS",
    "SC64_Z5_ACTORS", "SC64_Z5_DOODADS", "SC64_P6_COMBAT_ENABLED",
    "SC64_P7_ACTORS", "SC64_P7_BATTERY_ENABLED",
    "SC64_P7_PROJECTILES_ENABLED", "SC64_P7_CARRIER_TRAINING_ENABLED",
    "SC64_P7_INTERCEPTOR_FLIGHT_ENABLED",
)

class SightRuntimeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
        if not cls.cc:
            raise RuntimeError("clang or cc required")
        artifact = os.environ.get("SC64_P7_SIGHT_RUNTIME_ARTIFACTS")
        if artifact:
            cls.work = pathlib.Path(artifact).resolve()
            if cls.work.exists():
                raise RuntimeError(f"refusing existing output directory: {cls.work}")
            cls.work.mkdir(parents=True)
            cls.temporary = None
        else:
            cls.temporary = tempfile.TemporaryDirectory(prefix="sc64-p7-sight-runtime-")
            cls.work = pathlib.Path(cls.temporary.name)

    @classmethod
    def tearDownClass(cls):
        if cls.temporary:
            cls.temporary.cleanup()

    def command(self, name, argv, expected=0, diagnostic=None):
        (self.work / f"{name}.argv.json").write_text(json.dumps(argv, indent=2)+"\n")
        result = subprocess.run(argv, cwd=ROOT, capture_output=True, text=True)
        (self.work / f"{name}.stdout.log").write_text(result.stdout)
        (self.work / f"{name}.stderr.log").write_text(result.stderr)
        self.assertEqual(result.returncode, expected,
                         f"{name}: rc={result.returncode}\n{result.stderr}")
        if diagnostic:
            self.assertIn(diagnostic, result.stderr)
        return result

    def compile(self, name, source, defines, output, generated=None, expected=0,
                diagnostic=None):
        argv = [self.cc, "-std=c11", "-O1", "-Wall", "-Wextra", "-Werror",
                "-Wno-unused-function", "-Wno-unused-variable",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                "-I", str(ROOT / "starcraft"), "-I", str(PROJECT / "starcraft")]
        if generated:
            argv += ["-I", str(generated)]
        argv += [f"-D{x}" for x in defines]
        argv += [str(source), "-o", str(output)]
        return self.command(name, argv, expected, diagnostic)

    def test_actual_production_wrapper_on_and_off(self):
        # Off build exercises the exact shared wrapper's source-provider fallback.
        off = self.work / "policy-fallback"
        self.compile("wrapper-off-build", FIXTURE, GATES, off)
        offrun = self.command("wrapper-off-run", [str(off)])
        self.assertIn("checks=142", offrun.stdout)

        # Generate a V2 SIGHT table from the production fact generator into this
        # test's private directory, then compile the same wrapper against the
        # real production sc64_fact_tables.h accessor and its payload.
        gen_dir = self.work / "generated"
        gen_dir.mkdir()
        generator = self.work / "fact-gen"
        self.compile("generator-build", GENERATOR,
                     GATES + ("SC64_FACT_SIGHT_TABLES",), generator)
        argv_file = json.loads((self.work / "generator-build.argv.json").read_text())
        self.assertNotIn("-DSC64_FACT_TABLE_GENERATOR", argv_file)
        generated = self.command("generator-run", [str(generator)])
        self.assertNotIn("SC64_FACT_TABLE_GENERATOR", generated.stdout)
        (gen_dir / "sc64_fact_tables_generated.h").write_text(generated.stdout)
        on = self.work / "cached-wrapper"
        self.compile("wrapper-on-build", FIXTURE,
                     GATES + ("SC64_FACT_TABLES", "SC64_FACT_SIGHT_TABLES"),
                     on, gen_dir)
        onrun = self.command("wrapper-on-run", [str(on)])
        self.assertIn("checks=142", onrun.stdout)
        self.assertIn("production fact-table wrapper exact", onrun.stdout)

    def test_sight_feature_requires_fact_tables(self):
        self.compile("missing-fact-gate", FIXTURE,
                     GATES + ("SC64_FACT_SIGHT_TABLES",), self.work / "must-not-build",
                     expected=1,
                     diagnostic="SC64_FACT_SIGHT_TABLES requires SC64_FACT_TABLES")

if __name__ == "__main__":
    unittest.main()
