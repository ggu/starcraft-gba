# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free differential of both target-index builders."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class TargetOwnerIndexTest(unittest.TestCase):
    def test_randomized_builds(self):
        profiles = {
            "24x24": ("-DSC64_TARGET_OCCUPANCY",),
            "32x24": ("-DSC64_LARGE_MAPS",),
            "32x32": ("-DSC64_ZP_CAMPAIGN_STORAGE", "-DSC64_TARGET_OCCUPANCY"),
        }
        with tempfile.TemporaryDirectory() as directory:
            exe = pathlib.Path(directory) / "index-diff"
            for profile, flags in profiles.items():
                for label, options in (
                    ("stock", ()),
                    ("bulk", ("-DSC64_TARGET_BULK_REBUILD_PROBE",)),
                    ("fast", ("-DSC64_TARGET_BULK_REBUILD_PROBE", "-DSC64_TARGET_FAST_LINK_PROBE")),
                    ("retained", ("-DSC64_TARGET_BULK_REBUILD_PROBE", "-DSC64_TARGET_FAST_LINK_PROBE",
                                  "-DSC64_TARGET_RETAIN_PROBE")),
                    ("retained-p10", ("-DSC64_TARGET_BULK_REBUILD_PROBE", "-DSC64_TARGET_FAST_LINK_PROBE",
                                      "-DSC64_TARGET_RETAIN_PROBE", "-DSC64_TARGET_TEST_P10")),
                ):
                    with self.subTest(profile=profile, implementation=label):
                        subprocess.run(["clang", "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                                        "-Werror", "-fsanitize=address,undefined", *flags, *options,
                                        str(ROOT / "tests/sc64_target_owner_index_diff.c"),
                                        "-o", str(exe)], check=True)
                        subprocess.run([str(exe)], check=True)

if __name__ == "__main__":
    unittest.main()
