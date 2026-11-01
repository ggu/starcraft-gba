# SPDX-License-Identifier: GPL-3.0-or-later
"""Host qualification for optional versioned actor, sight, and P7 rect tables."""
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest

TREE = pathlib.Path(__file__).resolve().parents[1]
PROJECT = pathlib.Path(os.environ.get("SC64_FACT_V2_SOURCE_ROOT", TREE)).resolve()
GENERATOR = TREE / "tools" / "sc64_fact_tables_gen.c"
FACT_CHECK = PROJECT / "tests" / "sc64_fact_tables_check.c"
SIGHT_CHECK = TREE / "tests" / "sc64_fact_v2_sight_check.c"
SIGHT_POLICY_CHECK = TREE / "tests" / "sc64_fact_v2_sight_policy_check.c"
GEOM_REF = TREE / "tests" / "sc64_fact_v2_geometry_reference.c"
GEOM_CAND = TREE / "tests" / "sc64_fact_v2_geometry_candidate.c"
GEOM_ORACLE = TREE / "tests" / "sc64_fact_v2_geometry_oracle.c"

# A broad authored P7 roster, still asset-free. Generator and consumer use the
# identical set, and its generated header fingerprints every listed gate.
GATES = (
    "SC64_RACES", "SC64_UNIFIED", "SC64_P3_ENABLED",
    "SC64_ZP_CAMPAIGN_ACTORS", "SC64_ZP_CAMPAIGN_STORAGE",
    "SC64_P5_ACTORS", "SC64_Z6_ACTORS", "SC64_P6_ACTORS",
    "SC64_Z5_ACTORS", "SC64_Z5_DOODADS", "SC64_P6_COMBAT_ENABLED",
    "SC64_P7_ACTORS", "SC64_P7_BATTERY_ENABLED",
    "SC64_P7_PROJECTILES_ENABLED", "SC64_P7_CARRIER_TRAINING_ENABLED",
    "SC64_P7_INTERCEPTOR_FLIGHT_ENABLED",
)
FEATURES = {
    "none": (),
    "sight": ("SC64_FACT_SIGHT_TABLES",),
    "rect": ("SC64_P7_RECT_TABLES",),
    "both": ("SC64_FACT_SIGHT_TABLES", "SC64_P7_RECT_TABLES"),
}


def sha(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


class FactV2Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
        if not cls.cc:
            raise RuntimeError("clang or cc is required")
        artifact = os.environ.get("SC64_FACT_V2_ARTIFACTS")
        if artifact:
            cls.work = pathlib.Path(artifact).resolve()
            if cls.work.exists():
                raise RuntimeError(f"refusing existing artifact directory: {cls.work}")
            cls.work.mkdir(parents=True)
            cls._temporary = None
        else:
            cls._temporary = tempfile.TemporaryDirectory(prefix="sc64-fact-v2-")
            cls.work = pathlib.Path(cls._temporary.name)
        cls.results = {}

    @classmethod
    def tearDownClass(cls):
        if cls._temporary:
            cls._temporary.cleanup()

    def command(self, name, argv, expected=True, stderr_text=None):
        command_path = self.current / (name + ".argv.json")
        if command_path.exists():
            self.fail(f"refusing existing command file: {command_path}")
        command_path.write_text(json.dumps(argv, indent=2) + "\n")
        result = subprocess.run(argv, cwd=PROJECT, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        (self.current / (name + ".stdout.log")).write_bytes(result.stdout)
        (self.current / (name + ".stderr.log")).write_bytes(result.stderr)
        if expected and result.returncode:
            self.fail(f"{name} failed ({result.returncode}): {result.stderr.decode(errors='replace')}")
        if not expected and not result.returncode:
            self.fail(f"{name} unexpectedly succeeded")
        if stderr_text and stderr_text.encode() not in result.stderr:
            self.fail(f"{name} omitted expected diagnostic {stderr_text!r}: {result.stderr.decode(errors='replace')}")
        return result

    def setUp(self):
        self.current = self.work / self._testMethodName
        if self.current.exists():
            self.fail(f"refusing existing test output: {self.current}")
        self.current.mkdir()

    def compile(self, name, source, defines, output, generated=None, expected=True,
                stderr_text=None, extra=()):
        argv = [self.cc, "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                "-Werror", "-Wno-unused-function", "-Wno-unused-variable",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                "-I", str(TREE / "starcraft"), "-I", str(PROJECT / "starcraft")]
        if generated:
            argv += ["-I", str(generated)]
        argv += [f"-D{x}" for x in defines]
        argv += list(extra) + [str(source), "-o", str(output)]
        return self.command(name, argv, expected, stderr_text)

    def generate(self, name, feature_flags):
        output = self.current / name
        output.mkdir()
        gen = output / "fact-gen"
        self.compile(name + "-generator-build", GENERATOR,
                     GATES + feature_flags, gen)
        # Internal reference mode must be owned by the generator source.
        argv_file = self.current / (name + "-generator-build.argv.json")
        argv = json.loads(argv_file.read_text())
        self.assertNotIn("-DSC64_FACT_TABLE_GENERATOR", argv)
        result = self.command(name + "-generator-run", [str(gen)])
        table_dir = output / "generated"
        table_dir.mkdir()
        header = table_dir / "sc64_fact_tables_generated.h"
        header.write_bytes(result.stdout)
        self.assertNotIn(b"SC64_FACT_TABLE_GENERATOR", result.stdout)
        return table_dir, header

    def run_executable(self, name, executable, expected_line):
        result = self.command(name, [str(executable)])
        self.assertIn(expected_line.encode(), result.stdout)
        return result.stdout.decode().strip()

    def test_feature_combinations_and_public_helpers(self):
        for name, feature_flags in FEATURES.items():
            with self.subTest(features=name):
                table_dir, header = self.generate(name, feature_flags)
                consumer = GATES + ("SC64_FACT_TABLES",) + feature_flags
                exe = self.current / (name + "-fact-check")
                self.compile(name + "-fact-check-build", FACT_CHECK, consumer,
                             exe, table_dir)
                fact_result = self.run_executable(name + "-fact-check-run", exe,
                    b"fact tables exact:".decode())
                self.assertIn("checks", fact_result)
                if "SC64_FACT_SIGHT_TABLES" in feature_flags:
                    sight = self.current / (name + "-sight-check")
                    self.compile(name + "-sight-build", SIGHT_CHECK, consumer,
                                 sight, table_dir)
                    self.run_executable(name + "-sight-run", sight,
                        "sight-radius table exact:")
                if "SC64_P7_RECT_TABLES" in feature_flags:
                    ref_obj = self.current / (name + "-geometry-reference.o")
                    cand_obj = self.current / (name + "-geometry-candidate.o")
                    driver_obj = self.current / (name + "-geometry-oracle.o")
                    base = GATES
                    self.compile(name + "-geometry-reference-build", GEOM_REF,
                                 base, ref_obj, extra=("-c",))
                    self.compile(name + "-geometry-candidate-build", GEOM_CAND,
                                 consumer, cand_obj, table_dir, extra=("-c",))
                    self.compile(name + "-geometry-driver-build", GEOM_ORACLE,
                                 (), driver_obj, extra=("-c",))
                    oracle = self.current / (name + "-geometry-oracle")
                    argv = [self.cc, "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                            str(ref_obj), str(cand_obj), str(driver_obj), "-o", str(oracle)]
                    self.command(name + "-geometry-link", argv)
                    self.run_executable(name + "-geometry-run", oracle,
                        "P7 raw rectangle table exact:")
                self.results[name] = {
                    "header": str(header), "header_sha256": sha(header),
                    "feature_flags": list(feature_flags),
                }
        report = self.current / "feature-combinations.json"
        report.write_text(json.dumps(self.results, indent=2) + "\n")

    def test_pure_sight_policy_10k(self):
        exe = self.current / "sight-policy-check"
        self.compile("sight-policy-build", SIGHT_POLICY_CHECK, GATES, exe)
        self.run_executable("sight-policy-run", exe, "P7 sight pure policy exact:")

    def test_stale_rectangle_geometry_revision_rejects(self):
        table_dir, header = self.generate("current-rect", FEATURES["rect"])
        text = header.read_text()
        marker = "#define SC64_FACT_P7_RECT_REVISION 2u\n"
        self.assertIn(marker, text)
        consumer = GATES + ("SC64_FACT_TABLES",) + FEATURES["rect"]
        current = self.current / "current-rect-check"
        self.compile("current-rect-check-build", FACT_CHECK, consumer, current, table_dir)
        self.run_executable("current-rect-check-run", current, "fact tables exact:")
        for name, replacement in (("unmarked", ""),
                                  ("old", "#define SC64_FACT_P7_RECT_REVISION 1u\n")):
            old_dir = self.current / name
            old_dir.mkdir()
            (old_dir / header.name).write_text(text.replace(marker, replacement))
            self.compile(name + "-rect-check-build", FACT_CHECK, consumer,
                         self.current / (name + "-rect-check"), old_dir,
                         expected=False,
                         stderr_text="rectangle tables require current geometry revision")

    def test_legacy_unversioned_v1_is_fact_only_compatible(self):
        table_dir, header = self.generate("legacy-source", ())
        text = header.read_text()
        text = re.sub(r"^#define SC64_FACT_TABLE_FORMAT_VERSION .*\n", "", text, flags=re.M)
        text = re.sub(r"^#define SC64_FACT_GENERATED_FEATURES .*\n", "", text, flags=re.M)
        legacy_dir = self.current / "legacy-generated"
        legacy_dir.mkdir()
        (legacy_dir / "sc64_fact_tables_generated.h").write_text(text)
        exe = self.current / "legacy-fact-check"
        self.compile("legacy-fact-check-build", FACT_CHECK,
                     GATES + ("SC64_FACT_TABLES",), exe, legacy_dir)
        self.run_executable("legacy-fact-check-run", exe, "fact tables exact:")

    def test_unknown_version_and_missing_feature_bits_reject(self):
        _, both_header = self.generate("both-source", FEATURES["both"])
        both_text = both_header.read_text()
        bad_version = both_text.replace(
            "#define SC64_FACT_TABLE_FORMAT_VERSION SC64_FACT_TABLE_FORMAT_V2",
            "#define SC64_FACT_TABLE_FORMAT_VERSION 77u")
        self.assertNotEqual(bad_version, both_text)
        bad_version_dir = self.current / "bad-version"
        bad_version_dir.mkdir()
        (bad_version_dir / "sc64_fact_tables_generated.h").write_text(bad_version)
        bad_version_exe = self.current / "bad-version-check"
        self.compile("unsupported-version-build", FACT_CHECK,
                     GATES + ("SC64_FACT_TABLES",), bad_version_exe,
                     bad_version_dir, expected=False,
                     stderr_text="unsupported generated fact table format version")

        for feature, mask, diagnostic in (
            ("rect", "SC64_FACT_FEATURE_SIGHT", "rectangle lookup requires V2"),
            ("sight", "SC64_FACT_FEATURE_P7_RECT", "sight lookup requires V2"),
        ):
            changed = re.sub(
                r"#define SC64_FACT_GENERATED_FEATURES \(.*?\)",
                f"#define SC64_FACT_GENERATED_FEATURES ({mask})",
                both_text, flags=re.S)
            self.assertNotEqual(changed, both_text)
            bad_dir = self.current / ("missing-" + feature)
            bad_dir.mkdir()
            (bad_dir / "sc64_fact_tables_generated.h").write_text(changed)
            exe = self.current / ("missing-" + feature + "-check")
            self.compile("missing-" + feature + "-feature-build", FACT_CHECK,
                         GATES + ("SC64_FACT_TABLES",) + FEATURES["both"], exe,
                         bad_dir, expected=False, stderr_text=diagnostic)

    def test_feature_gate_fingerprint_rejects_mismatch(self):
        table_dir, _ = self.generate("both-source", FEATURES["both"])
        exe = self.current / "sight-gate-mismatch"
        self.compile("sight-gate-mismatch-build", FACT_CHECK,
                     GATES + ("SC64_FACT_TABLES", "SC64_P7_RECT_TABLES"),
                     exe, table_dir, expected=False,
                     stderr_text="fact tables were generated with SC64_FACT_SIGHT_TABLES")


if __name__ == "__main__":
    unittest.main(verbosity=2)
