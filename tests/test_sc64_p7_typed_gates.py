"""Asset-free rejection checks for incomplete typed-save profiles."""
import pathlib
import shutil
import subprocess
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
GATE = "SC64_P7_TYPED_COMPACT_ENABLED"
DEPENDENCIES = (
    "SC64_P7_BATTERY_ENABLED", "SC64_P7_ARBITER_CLOAK_CACHE_ENABLED",
    "SC64_P7_RUNTIME", "SC64_UNIFIED", "SC64_ZP_FULL_STORAGE",
    "SC64_ZP_CAMPAIGN_STORAGE", "SC64_CAMPAIGN_RACE=2",
)


class TypedSaveGateTests(unittest.TestCase):
    def rejects(self, defines):
        compiler = shutil.which("clang") or shutil.which("cc")
        self.assertIsNotNone(compiler, "clang or cc is required")
        command = [compiler, "-E", "-DGBA_HOST", "-Icommon"]
        command.extend("-D" + define for define in defines)
        command.append(str(ROOT / "starcraft/game.c"))
        result = subprocess.run(command, cwd=ROOT, stdout=subprocess.DEVNULL,
                                stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(result.returncode, 0)
        # Missing private includes alone cannot satisfy this assertion.
        self.assertIn("P7 typed compact save requires unified P7 battery "
                      "and Arbiter cache profile", result.stderr)

    def test_bare_typed_gate(self):
        self.rejects([GATE])

    def test_each_dependency_required(self):
        for missing in DEPENDENCIES:
            with self.subTest(missing=missing):
                self.rejects([GATE] + [d for d in DEPENDENCIES if d != missing])

    def test_wrong_campaign_race(self):
        self.rejects([GATE] + [d.replace("RACE=2", "RACE=1")
                              for d in DEPENDENCIES])


if __name__ == "__main__":
    unittest.main()
