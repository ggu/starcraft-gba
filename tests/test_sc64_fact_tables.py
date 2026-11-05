import os
import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest

from test_sc64_zp_fact_range import CONFIGS as FACT_RANGE_CONFIGS


ROOT = pathlib.Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "tools" / "sc64_fact_tables_gen.c"
CHECK = ROOT / "tests" / "sc64_fact_tables_check.c"
FACT_HEADERS = ("race_rules.h", "sc64_zp_actor_rules.h", "sc64_p7_actor_rules.h",
                "sc64_z5_actor_rules.h", "sc64_p6_actor_rules.h",
                "sc64_t6_actor_rules.h", "sc64_terran_late_actor_rules.h",
                "sc64_z6_actor_rules.h", "sc64_game_actor_facts.h")
# Include guards and options that cannot change a fact answer.
NON_GATES = {"SC64_ZP_FACT_RANGE_CULL", "SC64_FACT_TABLES"}

# The fact-range gate matrix plus P7 extensions and the P7 movement profile's
# actor gates. Every configuration needs SC64_RACES and Z/P actor facts.
CONFIGS = {name: ("SC64_RACES",) + defines
           for name, defines in FACT_RANGE_CONFIGS.items()}
CONFIGS.update({
    "scarab_identity": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS", "SC64_SCARAB_RUNTIME"),
    "shared_battlecruiser": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS",
                            "SC64_BATTLECRUISER_EXPERIMENT"),
    "all_race_roster": ("SC64_RACES", "SC64_UNIFIED", "SC64_ZP_CAMPAIGN_ACTORS",
                        "SC64_Z5_ACTORS", "SC64_Z5_DOODADS", "SC64_Z6_RUNTIME",
                        "SC64_Z6_ACTORS", "SC64_SKIRMISH_ROSTER"),
    "zp_p7_projectiles": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS",
                          "SC64_P7_ACTORS", "SC64_P7_PROJECTILES_ENABLED"),
    "zp_p7_carrier": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS", "SC64_P7_ACTORS",
                      "SC64_P7_PROJECTILES_ENABLED",
                      "SC64_P7_CARRIER_TRAINING_ENABLED"),
    "zp_p7_carrier_flight": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS",
                              "SC64_P7_ACTORS", "SC64_P7_PROJECTILES_ENABLED",
                              "SC64_P7_CARRIER_TRAINING_ENABLED",
                              "SC64_P7_INTERCEPTOR_FLIGHT_ENABLED"),
    "zp_z6_shared_transport": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS",
                               "SC64_Z6_ACTORS", "SC64_Z6_RUNTIME",
                               "SC64_SHARED_TRANSPORT"),
    "zp_z6_runtime": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS", "SC64_Z6_ACTORS",
                      "SC64_Z6_RUNTIME"),
    "zp_z7_egg": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS", "SC64_Z6_ACTORS",
                  "SC64_Z7_EGG_PRODUCTION"),
    "zp_nydus": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS", "SC64_Z6_ACTORS",
                 "SC64_Z6_RUNTIME", "SC64_NYDUS_ENABLED"),
    "zp_z6_shared_scourge": ("SC64_RACES", "SC64_UNIFIED", "SC64_ZP_CAMPAIGN_ACTORS",
                              "SC64_Z6_ACTORS", "SC64_Z6_RUNTIME", "SC64_SHARED_SCOURGE_COMBAT"),
    "p7_shared_scourge": ("SC64_RACES", "SC64_UNIFIED", "SC64_P3_ENABLED",
                          "SC64_ZP_CAMPAIGN_ACTORS", "SC64_P7_ACTORS",
                          "SC64_SHARED_SCOURGE_COMBAT"),
    "zp_p8_data": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS",
                   "SC64_P8_DATA_ENABLED"),
    "zp_p9_data": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS",
                   "SC64_P8_DATA_ENABLED", "SC64_P9_DATA_ENABLED"),
    "zp_p10_data": ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS", "SC64_P8_DATA_ENABLED",
                    "SC64_P9_DATA_ENABLED", "SC64_P10_DATA_ENABLED"),
    "p7_move_profile": ("SC64_RACES", "SC64_UNIFIED", "SC64_P3_ENABLED",
                        "SC64_ZP_CAMPAIGN_ACTORS", "SC64_ZP_CAMPAIGN_STORAGE",
                        "SC64_P5_ACTORS", "SC64_Z6_ACTORS", "SC64_P6_ACTORS",
                        "SC64_Z5_ACTORS", "SC64_Z5_DOODADS",
                        "SC64_P6_COMBAT_ENABLED", "SC64_P7_ACTORS",
                        "SC64_P7_BATTERY_ENABLED"),
})


def compile_program(compiler, source, defines, directory, name, extra=()):
    executable = pathlib.Path(directory) / name
    command = [compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
               "-Wno-unused-function", "-fsanitize=address,undefined",
               "-I", str(ROOT / "starcraft")]
    command += [f"-D{define}" for define in defines]
    command += list(extra) + [str(source), "-o", str(executable)]
    return subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE), executable


def generate(compiler, defines, directory, name):
    built, generator = compile_program(compiler, GENERATOR, defines, directory,
                                       name + "-gen")
    if built.returncode:
        raise AssertionError(built.stderr.decode())
    table_dir = pathlib.Path(directory) / (name + "-tables")
    table_dir.mkdir()
    header = subprocess.run([str(generator)], check=True, cwd=ROOT,
                            stdout=subprocess.PIPE).stdout
    (table_dir / "sc64_fact_tables_generated.h").write_bytes(header)
    return table_dir


class FactTablesTest(unittest.TestCase):
    def setUp(self):
        self.compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
        self.assertIsNotNone(self.compiler, "clang or cc is required")

    def test_gate_list_covers_every_fact_condition(self):
        listed = set(re.findall(r"X\((SC64_[A-Z0-9_]+)\)", GENERATOR.read_text()))
        used = set()
        for header in FACT_HEADERS:
            source = (ROOT / "starcraft" / header).read_text()
            source = re.sub(r"\\\s*\n\s*", " ", source)
            for line in source.splitlines():
                if re.match(r"\s*#\s*(if|ifdef|ifndef|elif)\b", line):
                    used.update(re.findall(r"SC64_[A-Z0-9_]+", line))
        used = {name for name in used
                if not name.endswith("_H") and name not in NON_GATES}
        self.assertEqual(sorted(used - listed), [])

    def test_tables_match_reference_functions_across_gates(self):
        with tempfile.TemporaryDirectory(prefix="sc64-fact-tables-") as directory:
            for name, defines in CONFIGS.items():
                for cull in (False, True):
                    variant = name + ("-cull" if cull else "")
                    flags = defines + (("SC64_ZP_FACT_RANGE_CULL",) if cull else ())
                    with self.subTest(configuration=variant):
                        table_dir = generate(self.compiler, flags, directory, variant)
                        built, check = compile_program(
                            self.compiler, CHECK, flags + ("SC64_FACT_TABLES",),
                            directory, variant + "-check", ("-I", str(table_dir)))
                        self.assertEqual(built.returncode, 0, built.stderr.decode())
                        result = subprocess.run([str(check)], check=True, cwd=ROOT,
                                                stdout=subprocess.PIPE).stdout
                        self.assertIn(b"fact tables exact:", result)

    def test_tables_reject_a_build_with_different_gates(self):
        with tempfile.TemporaryDirectory(prefix="sc64-fact-tables-gate-") as directory:
            table_dir = generate(self.compiler, CONFIGS["zp_p7"], directory, "zp_p7")
            for other in ("zp_p7_projectiles", "zp_p6", "zp", "all_race_roster"):
                with self.subTest(build=other):
                    built, _ = compile_program(
                        self.compiler, CHECK, CONFIGS[other] + ("SC64_FACT_TABLES",),
                        directory, other + "-mismatch", ("-I", str(table_dir)))
                    self.assertNotEqual(built.returncode, 0)
                    self.assertIn(b"fact tables were generated", built.stderr)

    def test_combined_guardian_facts_require_exact_layout_marker(self):
        base = ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS",
                "SC64_GUARDIAN_COCOON_STAGE", "SC64_SHARED_EGG_WIRE_HOST_STAGE",
                "SC64_GUARDIAN_FULL_FACTS_STAGE")
        combined = base + ("SC64_SCARAB_RUNTIME", "SC64_SCARAB_STABLE_ROSTER_HOST_STAGE",
                           "SC64_SHARED_ACTOR_LAYOUT_HOST_STAGE", "SC64_SHARED_ACTOR_FULL_HOST_STAGE")
        with tempfile.TemporaryDirectory(prefix="sc64-guardian-fact-marker-") as directory:
            old = generate(self.compiler, base, directory, "old-guardian")
            built, _ = compile_program(self.compiler, CHECK, combined + ("SC64_FACT_TABLES",),
                                       directory, "wrong-layout", ("-I", str(old)))
            self.assertNotEqual(built.returncode, 0)
            self.assertIn(b"full Guardian facts require the Egg-first shared layout", built.stderr)
            current = generate(self.compiler, combined, directory, "combined")
            header = current / "sc64_fact_tables_generated.h"
            text = header.read_text()
            self.assertIn("#define SC64_FACT_GUARDIAN_LAYOUT_VERSION 2u\n", text)
            header.write_text(text.replace("#define SC64_FACT_GUARDIAN_LAYOUT_VERSION 2u\n", ""))
            built, _ = compile_program(self.compiler, CHECK, combined + ("SC64_FACT_TABLES",),
                                       directory, "missing-marker", ("-I", str(current)))
            self.assertNotEqual(built.returncode, 0)
            self.assertIn(b"full Guardian facts require a generated layout marker", built.stderr)

    def test_combined_guardian_flight_fingerprint_both_directions(self):
        combined = ("SC64_RACES", "SC64_ZP_CAMPAIGN_ACTORS", "SC64_GUARDIAN_COCOON_STAGE",
                    "SC64_SHARED_EGG_WIRE_HOST_STAGE", "SC64_GUARDIAN_FULL_FACTS_STAGE",
                    "SC64_SCARAB_RUNTIME", "SC64_SCARAB_STABLE_ROSTER_HOST_STAGE",
                    "SC64_SHARED_ACTOR_LAYOUT_HOST_STAGE", "SC64_SHARED_ACTOR_FULL_HOST_STAGE")
        flight = combined + ("SC64_SHARED_ACTOR_FLIGHT_HOST_STAGE", "SC64_SCARAB_FLIGHT_CONTROL_PROBE",
                             "SC64_SCARAB_PACKED_MOTION_PROBE", "SC64_SCARAB_AUTONOMOUS_PROBE",
                             "SC64_SCARAB_WEAPON_PROBE", "SC64_SCARAB_HUMAN_CONTROL_PROBE")
        with tempfile.TemporaryDirectory(prefix="sc64-guardian-flight-facts-") as directory:
            for name, generated, used, message in (
                ("old-to-flight", combined, flight, b"full Guardian facts require the shared flight layout"),
                ("flight-to-old", flight, combined, b"full Guardian facts require the Egg-first shared layout")):
                table = generate(self.compiler, generated, directory, name)
                built, _ = compile_program(self.compiler, CHECK, used + ("SC64_FACT_TABLES",),
                                           directory, name + "-check", ("-I", str(table)))
                self.assertNotEqual(built.returncode, 0)
                self.assertIn(message, built.stderr)
            table = generate(self.compiler, flight, directory, "flight-capability")
            missing = tuple(flag for flag in flight if flag != "SC64_SCARAB_AUTONOMOUS_PROBE")
            built, _ = compile_program(self.compiler, CHECK, missing + ("SC64_FACT_TABLES",),
                                       directory, "missing-autonomous", ("-I", str(table)))
            self.assertNotEqual(built.returncode, 0)
            self.assertIn(b"fact tables were generated with SC64_SCARAB_AUTONOMOUS_PROBE", built.stderr)

    def test_tables_reject_shared_scourge_mismatch_both_directions(self):
        base = CONFIGS["zp_z6_runtime"]
        shared = base + ("SC64_SHARED_SCOURGE_COMBAT",)
        with tempfile.TemporaryDirectory(prefix="sc64-fact-tables-scourge-") as directory:
            for name, generated, used, message in (
                ("off-to-on", base, shared, b"generated without SC64_SHARED_SCOURGE_COMBAT"),
                ("on-to-off", shared, base, b"generated with SC64_SHARED_SCOURGE_COMBAT"),
            ):
                with self.subTest(direction=name):
                    table_dir = generate(self.compiler, generated, directory, name)
                    built, _ = compile_program(self.compiler, CHECK, used + ("SC64_FACT_TABLES",),
                                               directory, name + "-check", ("-I", str(table_dir)))
                    self.assertNotEqual(built.returncode, 0)
                    self.assertIn(message, built.stderr)

    def test_tables_reject_bc_energy_yamato_mismatch_both_directions(self):
        base = CONFIGS["zp_p7"] + ("SC64_BATTLECRUISER_EXPERIMENT",)
        with tempfile.TemporaryDirectory(prefix="sc64-fact-tables-bc-") as directory:
            for gate in ("SC64_BATTLECRUISER_ENERGY_EXPERIMENT",
                         "SC64_BATTLECRUISER_YAMATO_EXPERIMENT"):
                for name, generated, used, state in (
                    ("off-on", base, base + (gate,), "without"),
                    ("on-off", base + (gate,), base, "with"),
                ):
                    label = gate + name
                    with self.subTest(gate=gate, direction=name):
                        table_dir = generate(self.compiler, generated, directory, label)
                        built, _ = compile_program(self.compiler, CHECK,
                            used + ("SC64_FACT_TABLES",), directory,
                            label + "-check", ("-I", str(table_dir)))
                        self.assertNotEqual(built.returncode, 0)
                        self.assertIn(f"generated {state} {gate}".encode(), built.stderr)

    def test_tables_reject_scarab_gate_mismatch_both_directions(self):
        base = CONFIGS["zp_p7"]
        with tempfile.TemporaryDirectory(prefix="sc64-fact-tables-scarab-") as directory:
            for name, generated, used, state in (
                ("off-on", base, base + ("SC64_SCARAB_RUNTIME",), "without"),
                ("on-off", base + ("SC64_SCARAB_RUNTIME",), base, "with"),
            ):
                with self.subTest(direction=name):
                    table_dir = generate(self.compiler, generated, directory, name)
                    built, _ = compile_program(self.compiler, CHECK,
                        used + ("SC64_FACT_TABLES",), directory,
                        name + "-check", ("-I", str(table_dir)))
                    self.assertNotEqual(built.returncode, 0)
                    self.assertIn(f"generated {state} SC64_SCARAB_RUNTIME".encode(), built.stderr)

    def test_tables_reject_carrier_flight_gate_mismatch(self):
        with tempfile.TemporaryDirectory(prefix="sc64-fact-tables-flight-") as directory:
            table_dir = generate(self.compiler, CONFIGS["zp_p7_carrier"],
                                 directory, "carrier-without-flight")
            built, _ = compile_program(
                self.compiler, CHECK,
                CONFIGS["zp_p7_carrier_flight"] + ("SC64_FACT_TABLES",),
                directory, "carrier-flight-mismatch", ("-I", str(table_dir)))
            self.assertNotEqual(built.returncode, 0)
            self.assertIn(b"fact tables were generated without SC64_P7_INTERCEPTOR_FLIGHT_ENABLED",
                          built.stderr)


if __name__ == "__main__":
    unittest.main()
