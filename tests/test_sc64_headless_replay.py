# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free safety tests for the headless replay transport."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_headless_replay as headless


class HeadlessTransportTests(unittest.TestCase):
    def test_rejects_wrong_breakpoint(self):
        transport = headless.HeadlessReplay(mock.Mock())
        with mock.patch.object(transport, "cmd", return_value="STOP 08000004"):
            with self.assertRaisesRegex(RuntimeError, "unexpected headless breakpoint"):
                transport.tick(expected=0x08000000)

    def test_rejects_short_read(self):
        transport = headless.HeadlessReplay(mock.Mock())
        with mock.patch.object(transport, "cmd", return_value="00"):
            with self.assertRaisesRegex(RuntimeError, "incomplete memory read"):
                transport.read(0x02000000, 2)

    def test_overlapping_scheduler_symbols_do_not_spawn(self):
        names = ("demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
                 "demo_schedule_active", "demo_schedule_index")
        symbols = {name: 0x02000000 for name in names}
        sizes = {name: 4 for name in names}
        with tempfile.TemporaryDirectory() as out, mock.patch.object(headless.subprocess, "Popen") as spawn:
            with self.assertRaisesRegex(ValueError, "overlap"):
                headless.start(Path("runner"), Path("rom"), Path(out), symbols, sizes)
            spawn.assert_not_called()


@unittest.skipUnless(os.environ.get("SC64_HEADLESS_RUNNER") and os.environ.get("SC64_HEADLESS_ROM"),
                     "private headless backend and ROM not supplied")
class HeadlessProtocolTests(unittest.TestCase):
    def setUp(self):
        self.runner = os.environ["SC64_HEADLESS_RUNNER"]
        self.rom = os.environ["SC64_HEADLESS_ROM"]
        self.ranges = ["02000000", "8", "02000010", "4", "02000020", "4",
                       "02000030", "4", "02000040", "4"]
        self.home = tempfile.TemporaryDirectory()
        self.addCleanup(self.home.cleanup)
        self.env = {**os.environ, "HOME": self.home.name,
                    "XDG_CONFIG_HOME": str(Path(self.home.name) / ".config")}

    def test_invalid_allowlist_rejected_before_rom_load(self):
        for start, size in (("", "1"), ("-1", "1"), ("2000000", "4294967296"),
                            ("2000000", "18446744073709551615"), ("203ffff", "2")):
            args = [self.runner, self.rom, start, size, *self.ranges[2:]]
            p = subprocess.run(args, capture_output=True, text=True, timeout=10, env=self.env)
            self.assertNotEqual(p.returncode, 0, (start, size))
            self.assertNotIn("READY", p.stdout)

    def test_malformed_commands_and_side_effecting_reads_are_rejected(self):
        commands = ["t", "m04000000,2", "m0e000000,1", "m02000000,1junk",
                    "m0203ffff,2", "mffffffff,2", "M02000000,1:ffjunk",
                    "M02000000,1:f", "M02000100,1:00", "canything", "tjunk",
                    "Z08000000junk", "z08000000junk", "qjunk", "x" * 5000, "t", "q"]
        p = subprocess.run([self.runner, self.rom, *self.ranges],
                           input="\n".join(commands) + "\n", capture_output=True,
                           text=True, timeout=10, env=self.env)
        self.assertEqual(p.returncode, 0, p.stderr)
        lines = p.stdout.splitlines()
        self.assertEqual(lines[0], "READY")
        self.assertEqual(lines[1], lines[-1])  # malformed commands did not advance the core
        self.assertTrue(all(line.startswith("ERR") for line in lines[2:-1]), lines)


if __name__ == "__main__":
    unittest.main()
