# SPDX-License-Identifier: GPL-3.0-or-later
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_headless_physical as physical
from emulator_smoke import symbols


class FakePhysical(physical.PhysicalReplay):
    def __init__(self):
        super().__init__(None, 0x08001000, 0x02000000)
        self.samples = []
        self.commands = []

    def cmd(self, text):
        self.commands.append(text)
        if text.startswith("K"):
            return "OK"
        if text.startswith("U"):
            count = int(text[1:].split(",")[0])
            self.samples.extend([self.held] * count)
            return f"STEPS {count} {self.completed + count} 100"
        raise AssertionError(text)


class PhysicalTransportTests(unittest.TestCase):
    def test_page_applies_transitions_before_sample_and_defers_end(self):
        p = FakePhysical()
        p.play_page({"first": 0, "end": 4, "events": [(0, 1), (1, 0), (3, 16), (4, 0)]})
        self.assertEqual(p.samples, [1, 0, 0, 16])
        self.assertEqual((p.completed, p.held), (4, 16))
        p.play_page({"first": 4, "end": 6, "events": [(4, 0)]})
        self.assertEqual(p.samples, [1, 0, 0, 16, 0, 0])

    def test_resume_prefix_reuses_first_page_and_checks_held(self):
        p = FakePhysical(); events = [(0, 512), (1, 0), (5, 1)]
        p.play_to(2, events)
        p.play_page({"first": 0, "end": 7, "events": events})
        self.assertEqual(p.samples, [512, 0, 0, 0, 0, 1, 1])
        with self.assertRaisesRegex(ValueError, "disagree"):
            p.play_to(8, [(0, 1), (1, 16)])

    def test_batch_bound_and_invalid_masks(self):
        p = FakePhysical(); p.play_to(600, [])
        self.assertEqual([c.split(",")[0] for c in p.commands], ["U256", "U256", "U88"])
        for mask in (-1, 1024, 48, 192, True):
            with self.subTest(mask=mask), self.assertRaises(ValueError):
                p.set_keys(mask)
        for events in ([(0, 1), (0, 0)], [(0, 48)], [(-1, 0)]):
            with self.subTest(events=events), self.assertRaises(ValueError):
                FakePhysical().play_to(2, events)

    def test_wrong_step_counter_rejected(self):
        p = FakePhysical()
        with mock.patch.object(p, "cmd", return_value="STEPS 2 3 12"):
            with self.assertRaisesRegex(RuntimeError, "counter mismatch"):
                p.play_to(2, [])

    def test_start_rejects_invalid_symbols_without_spawning(self):
        with tempfile.TemporaryDirectory() as out, mock.patch.object(physical.subprocess, "Popen") as spawn:
            with self.assertRaisesRegex(ValueError, "symbol"):
                physical.start(Path("runner"), Path("rom"), Path(out), {}, {})
            spawn.assert_not_called()


@unittest.skipUnless(os.environ.get("SC64_PHYSICAL_RUNNER") and os.environ.get("SC64_PHYSICAL_ROM") and
                     os.environ.get("SC64_PHYSICAL_ELF"), "physical runner and normal ROM not supplied")
class PhysicalProtocolTests(unittest.TestCase):
    def test_denies_writes_and_checks_bounds_and_breakpoint(self):
        runner, rom, elf = (os.environ[k] for k in ("SC64_PHYSICAL_RUNNER", "SC64_PHYSICAL_ROM", "SC64_PHYSICAL_ELF"))
        sym = symbols(Path(elf)); pc = sym["gba_keys"] & ~1; counter = sym["demo_frame"]
        commands = ["M02000000,1:ff", "K400", "K-1", "K1junk", f"U1,{pc:x},{counter:x}",
                    f"Z{pc:x}", "c", "K1", f"U0,{pc:x},{counter:x}",
                    f"U257,{pc:x},{counter:x}", f"U1,{pc:x},{counter+1:x}",
                    f"U1,{pc:x},04000000", f"U1,{pc:x},{counter:x}", "q"]
        with tempfile.TemporaryDirectory() as td:
            env = {**os.environ, "HOME": td, "XDG_CONFIG_HOME": str(Path(td) / ".config")}
            p = subprocess.run([runner, rom, "--physical"], input="\n".join(commands) + "\n",
                               capture_output=True, text=True, timeout=60, env=env)
        self.assertEqual(p.returncode, 0, p.stderr)
        rows = p.stdout.splitlines(); self.assertEqual(rows[0], "READY")
        self.assertTrue(all(x.startswith("ERR") for x in rows[1:6]), rows)
        self.assertEqual(rows[6], "OK"); self.assertEqual(rows[7], f"STOP {pc:08x}")
        self.assertEqual(rows[8], "OK")
        self.assertTrue(all(x.startswith("ERR") for x in rows[9:13]), rows)
        self.assertRegex(rows[13], r"^STEPS 1 1 [0-9]+$")


if __name__ == "__main__":
    unittest.main()
