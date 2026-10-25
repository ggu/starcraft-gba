"""Reject incomplete Arbiter profiles in the actual game translation unit."""
import pathlib
import shutil
import subprocess
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
GATE = "SC64_P7_ARBITER_CLOAK_CACHE_ENABLED"
DEPENDENCIES = (
    "SC64_P7_OBSERVER_ENABLED", "SC64_P7_BATTERY_ENABLED", "SC64_RACES",
    "SC64_P7_RUNTIME", "SC64_P7_ACTORS", "SC64_P7_DATA_ENABLED",
    "SC64_ZP_CAMPAIGN_ACTORS",
)


class ArbiterGateTests(unittest.TestCase):
    def rejects(self, defines, diagnostic):
        compiler = shutil.which("clang") or shutil.which("cc")
        self.assertIsNotNone(compiler, "clang or cc is required")
        # Later includes may also reject unavailable private data. Require the
        # specific early profile diagnostic, not merely a failed compilation.
        command = [compiler, "-E", "-DGBA_HOST", "-Icommon"]
        command.extend("-D" + macro for macro in defines)
        command.append(str(ROOT / "starcraft/game.c"))
        result = subprocess.run(command, cwd=ROOT, stdout=subprocess.DEVNULL,
                                stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(diagnostic, result.stderr)

    def test_bare_cache_gate(self):
        self.rejects([GATE], "P7 Arbiter cloak requires Observer, Battery state")

    def test_each_required_dependency(self):
        for missing in DEPENDENCIES:
            with self.subTest(missing=missing):
                self.rejects([GATE] + [d for d in DEPENDENCIES if d != missing],
                             "P7 Arbiter cloak requires Observer, Battery state")

    def test_legacy_split_gate(self):
        self.rejects(["SC64_P7_ARBITER_CLOAK_ENABLED"],
                     "Use the unified P7 Arbiter cloak/cache feature gate")


if __name__ == "__main__":
    unittest.main()
