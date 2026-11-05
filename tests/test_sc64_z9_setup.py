"""Asset-free setup guard tests; no supplied ROM or extracted data is needed."""
from pathlib import Path
import importlib.machinery
import importlib.util
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import sc64_z9_setup as setup


class Z9Setup(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def bank(self):
        p = self.root / setup.BANK
        p.parent.mkdir(parents=True)
        p.write_text("\n".join(f"#define {m} 1" for m in setup.BANK_MARKERS) + "\n")
        return p

    def test_empty_workspace_is_not_prepared(self):
        rows = setup.readiness(self.root)
        self.assertEqual(len(rows), 5)
        self.assertFalse(any(ready for _, _, ready in rows))

    def test_missing_late_art_does_not_pass_as_inherited_bank(self):
        bank = self.bank()
        self.assertTrue(setup.bank_ready(bank))
        bank.write_text(bank.read_text().replace("#define SC64_ZERG_LATE_ART_COMPLETE 1", "#define SC64_ZERG_LATE_ART_COMPLETE 0"))
        self.assertFalse(setup.bank_ready(bank))

    def test_program_must_match_authenticated_render_not_just_guard(self):
        self.bank()
        for mission in ("Z8", "Z9"):
            _, header = setup.paths(self.root, mission)
            header.parent.mkdir(parents=True)
            header.write_text("trusted authored test program\n")
        with patch.object(setup, "authenticated", return_value={}), \
             patch.object(setup.z8_program, "render", return_value="trusted authored test program\n"), \
             patch.object(setup.late_program, "render", return_value="trusted authored test program\n"):
            self.assertTrue(all(ready for _, _, ready in setup.readiness(self.root)))
            setup.paths(self.root, "Z9")[1].write_text("edited program\n")
            rows = setup.readiness(self.root)
            self.assertTrue(rows[-2][2])  # Descriptor still authenticates.
            self.assertFalse(rows[-1][2])

    def test_untrusted_descriptor_does_not_authenticate_its_header(self):
        with patch.object(setup, "authenticated", side_effect=ValueError("source mismatch")):
            self.assertFalse(any(ready for _, _, ready in setup.readiness(self.root)))

    def test_existing_descriptors_verify_before_regenerable_outputs(self):
        for mission in ("Z8", "Z9"):
            descriptor, _ = setup.paths(self.root, mission)
            descriptor.parent.mkdir(parents=True)
            descriptor.write_text("retained evidence")
        calls = []
        with patch.object(setup, "readiness", return_value=[]):
            setup.prepare(self.root, calls.append)
        verifies = [c for c in calls if "verify" in c]
        self.assertEqual(len(verifies), 2)
        self.assertFalse(any("pack" in c for c in calls))
        self.assertIn("--include-zerg-late-art", calls[-1])
        self.assertNotIn("--base-only", calls[-1])  # P6 needs inherited T4 props.
        for mission in ("Z8", "Z9"):
            self.assertEqual(setup.paths(self.root, mission)[0].read_text(), "retained evidence")

    def test_failed_verification_preserves_existing_descriptor(self):
        descriptor, _ = setup.paths(self.root, "Z8")
        descriptor.parent.mkdir(parents=True)
        descriptor.write_text("retained evidence")
        calls = []
        def reject(args):
            calls.append(args)
            raise RuntimeError("source mismatch")
        with self.assertRaisesRegex(RuntimeError, "source mismatch"):
            setup.prepare(self.root, reject)
        self.assertEqual(len(calls), 1)
        self.assertIn("verify", calls[0])
        self.assertEqual(descriptor.read_text(), "retained evidence")

    def test_explicit_profile_does_not_promote_canonical_or_change_save_directory(self):
        loader = importlib.machinery.SourceFileLoader("dev_z9_setup_test", str(ROOT / "dev"))
        spec = importlib.util.spec_from_loader(loader.name, loader)
        dev = importlib.util.module_from_spec(spec)
        loader.exec_module(dev)
        self.assertIn("zerg-z9", dev.SC64_PROFILES)
        self.assertEqual(dev.SC64_TEST_TARGETS["zerg-z9"], "test-sc64-zerg-z9")
        self.assertIn("Zerg-Z7", dev.GAMES["zerg"][1])
        self.assertEqual(dev.PLAY_SAVE_PROFILES.get("zerg-z9", "zerg-z9"), "zerg-z9")

    def test_fresh_build_checks_inputs_without_requiring_an_existing_rom(self):
        loader = importlib.machinery.SourceFileLoader("dev_z9_fresh_build_test", str(ROOT / "dev"))
        spec = importlib.util.spec_from_loader(loader.name, loader)
        dev = importlib.util.module_from_spec(spec)
        loader.exec_module(dev)
        with patch.object(dev, "status", return_value=0) as status, patch.object(dev, "run") as run:
            dev.build("zerg-z9")
        status.assert_called_once_with("zerg-z9", include_built=False)
        run.assert_called_once_with(["make", "zerg-z9"])


if __name__ == "__main__":
    unittest.main()
