"""Keep the allocated minimap cache inside profiles with qualified RAM room."""
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ("TERRAN_ROM_FLAGS", "ZERG_ROM_FLAGS", "PROTOSS_ROM_FLAGS")
NON_CACHE = ("SHOWCASE_FLAGS", "Z7_FULL_RENDER_FLAGS", "P7_RENDER_FLAGS")


def profile_defines(*overrides):
    # A dry, dependency-free target evaluates the actual Make variables. It
    # cannot build a ROM, run an importer or regenerate private inputs.
    with tempfile.TemporaryDirectory(prefix="sc64-cache-flags-") as directory:
        rules = Path(directory) / "print.mk"
        rules.write_text(
            "".join(f"$(info PROFILE_{name}=$({name}))\n"
                    for name in CANONICAL + NON_CACHE)
            + ".PHONY: print-cache-profile-flags\nprint-cache-profile-flags:\n"
        )
        result = subprocess.run(
            ["make", "-n", "--no-print-directory", "-f", str(ROOT / "Makefile"),
             "-f", str(rules), "print-cache-profile-flags", "SC64_AUDIO=0", *overrides],
            cwd=ROOT, check=True, capture_output=True, text=True,
        )
    profiles = {}
    for line in result.stdout.splitlines():
        if line.startswith("PROFILE_"):
            name, flags = line.removeprefix("PROFILE_").split("=", 1)
            profiles[name] = {flag.split("=", 1)[0]
                              for flag in shlex.split(flags) if flag.startswith("-D")}
    return profiles


class CacheProfileFlagsTests(unittest.TestCase):
    def test_canonical_profiles_keep_all_four_qualified_caches(self):
        profiles = profile_defines()
        for name in CANONICAL:
            with self.subTest(profile=name):
                self.assertTrue({"-DSC64_TERRAIN_CACHE_VRAM", "-DSC64_HUD_VRAM_BANDS",
                                 "-DSC64_TERRAIN_ROW_MASKS", "-DSC64_MINIMAP_RASTER"}
                                <= profiles[name])

    def test_showcase_and_diagnostics_keep_the_prior_minimap_memory_budget(self):
        profiles = profile_defines()
        for name in NON_CACHE:
            with self.subTest(profile=name):
                self.assertNotIn("-DSC64_MINIMAP_RASTER", profiles[name])
                self.assertIn("-DSC64_MINIMAP_BLIT", profiles[name])

    def test_disabling_cache_group_also_removes_the_persistent_raster(self):
        profiles = profile_defines("CANONICAL_CACHE_FLAGS=")
        for name in CANONICAL:
            with self.subTest(profile=name):
                self.assertNotIn("-DSC64_MINIMAP_RASTER", profiles[name])
                self.assertIn("-DSC64_MINIMAP_BLIT", profiles[name])


if __name__ == "__main__":
    unittest.main()
