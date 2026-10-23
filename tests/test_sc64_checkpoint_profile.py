"""Reject unusable cartridge saves before launching the campaign profiler."""
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_split_profile as profile


class CampaignCheckpointInputTests(unittest.TestCase):
    def test_sram_validation_keeps_exact_32k_input_unchanged(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "campaign.sav"
            with self.assertRaises(ValueError):
                profile.checked_sram(path)
            for length in (0, 32767, 32769):
                path.write_bytes(b"x" * length)
                with self.assertRaises(ValueError):
                    profile.checked_sram(path)
            data = bytes(range(256)) * 128
            path.write_bytes(data)
            self.assertEqual(profile.checked_sram(path), path.resolve())
            self.assertEqual(path.read_bytes(), data)

    def test_invalid_checkpoint_modes_fail_before_any_emulator_work(self):
        with self.assertRaisesRegex(ValueError, "cannot be combined"):
            profile.run("terran", 0, 0, 1, 1, 1000, Path("unused"),
                        mission=6, require_win=True, save_at_end=True)


if __name__ == "__main__":
    unittest.main()
