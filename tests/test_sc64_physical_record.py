# SPDX-License-Identifier: GPL-3.0-or-later
"""The physical-replay recorder logs inputs, samples and saves, and compares two records."""
import json
import sys
import tempfile
import unittest
import unittest.mock
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_headless_physical as physical
import sc64_physical_record as record

SYMBOLS = {"frame": 0x03000010, "screen": 0x03000014, "u": 0x02000000, "__iwram_end": 0x03005000,
           "flip_pending": 0x03000018}
SIZES = {"frame": 4, "screen": 4, "u": 600, "flip_pending": 1}


class FakePipe:
    """The runner's stdin and stdout: queued `m` commands are answered in order."""

    def __init__(self, machine):
        self.machine, self.answers, self.closed = machine, [], False

    def write(self, text):
        for line in text.splitlines():
            if line == "SP":
                self.answers.append(self.machine.stack_answer())
                continue
            address, length = line[1:].split(",")
            if self.machine.broken:
                raise BrokenPipeError("runner is gone")
            self.answers.append(self.machine.memory(int(address, 16), int(length)).hex())

    def flush(self):
        pass

    def readline(self):
        return self.answers.pop(0) + "\n"

    def close(self):
        self.closed = True


class FakeProc:
    def __init__(self, machine):
        self.stdin = self.stdout = FakePipe(machine)
        self.gone = False
        self.shutdowns = []

    def poll(self):
        return 0 if self.stdin.closed or self.gone else None

    def terminate(self):
        self.gone = True
        self.shutdowns.append("terminate")

    def kill(self):
        self.gone = True
        self.shutdowns.append("kill")

    def wait(self):
        self.shutdowns.append("wait")


class FakeReplay(physical.PhysicalReplay):
    """A deterministic machine: its frame follows the inputs, its pixels follow `tint`."""
    made = []

    def __init__(self, tint, save_output, stack, flip=0, queued=False):
        self.proc = FakeProc(self)
        self.keys_pc = self.counter_address = 0
        self.completed = self.held = self.history = self.ticks = 0
        self.tint, self.save_output, self.stack, self.flip = tint, save_output, stack, flip
        self.stack_now = stack - 0x40  # boot went 64 bytes deeper than play
        self.broken, self.queued = False, queued
        FakeReplay.made.append(self)

    def memory(self, address, length):
        if address == SYMBOLS["frame"]:
            return self.completed.to_bytes(4, "little")
        if address == SYMBOLS["screen"]:
            return (1).to_bytes(4, "little")
        late = self.ticks >= 30
        if address == 0x04000000:  # a queued flip reaches DISPCNT only when the interrupt ran
            return (0x0404 | (self.flip if late and not self.queued else 0)).to_bytes(2, "little")
        if address == SYMBOLS["flip_pending"]:
            return bytes([1 if late and self.queued and self.flip else 0])
        if SYMBOLS["__iwram_end"] <= address < record.STACK_TOP:
            return bytes(a >= record.STACK_TOP - 64 for a in range(address, address + length))
        if record.STACK_TOP <= address < record.IRQ_TOP:
            return bytes(a >= record.IRQ_TOP - 40 for a in range(address, address + length))
        if 0x06000000 <= address < 0x0600A000 and self.completed >= 20:
            return bytes([self.tint]) * length
        return bytes([self.history & 255]) * length

    def cmd(self, command):
        if command[0] == "K":
            return "OK"
        if command[0] == "U":
            count = int(command[1:].split(",")[0])
            self.ticks += count  # the transport itself advances `completed` from this answer
            self.history += self.held * count
            return f"STEPS {count} {self.ticks} 0"
        if command == "c":
            self.ticks += 1
            return "STOP 0"
        if command == "s":
            Path(self.save_output).write_bytes(bytes([self.history & 255]) * 32768)
            return "OK 32768"
        raise AssertionError(command)

    def stack_answer(self):  # lowest since the previous query; the first covers boot
        low, self.stack_now = self.stack_now, self.stack
        return f"SP {low:08x} 03007e78 08000100"


TOOL = """
import sys
from pathlib import Path
import sc64_headless_physical as physical
out = Path(sys.argv[1])
out.mkdir()
proc, rsp, log = physical.start(None, None, out, SYMBOLS, SIZES, save_output=out / "pause.sav")
rsp.set_keys(mask=1)
rsp._advance(15)
rsp.set_keys(int(sys.argv[2]))
rsp._advance(25)
rsp.export_sram()
ENDING
"""


class TestPhysicalRecord(unittest.TestCase):
    def play(self, folder, *, tint=7, key=0, stack=0x03006800, tracked=True, flip=0,
             ending="proc.stdin.close()", status=0, broken=False, queued=False):
        """Run the little tool above against a fake machine and return its record."""
        folder.mkdir()
        tool = folder / "tool.py"
        tool.write_text(TOOL.replace("SYMBOLS", repr(SYMBOLS)).replace("SIZES", repr(SIZES))
                        .replace("ENDING", ending))

        def fake_start(runner, rom, out, symbols, sizes, *, save_input=None, save_output=None):
            replay = FakeReplay(tint, save_output, stack, flip, queued)
            replay.broken = broken
            return replay.proc, replay, None
        advance = physical.PhysicalReplay._advance
        environment = dict(PF_SYMS="functions") if tracked else {}
        with unittest.mock.patch.object(physical, "start", fake_start), \
                unittest.mock.patch.dict("os.environ", environment, clear=not tracked):
            path = list(sys.path)
            try:
                got = record.main(["run", "--out", str(folder / "rec"), "--every", "10",
                                   "--path", str(folder), str(tool), str(folder / "guest"), str(key)])
            finally:
                # The recorder puts back everything it changed, whatever happened.
                self.assertIs(physical.start, fake_start)
                self.assertIs(physical.PhysicalReplay._advance, advance)
                self.assertEqual(sys.path, path)
        self.assertEqual(got, status)
        return record.load(folder / "rec" / "rec.jsonl")

    def test_record_holds_inputs_periodic_samples_saves_and_end(self):
        with tempfile.TemporaryDirectory() as tmp:
            rows = self.play(Path(tmp) / "a")
        kinds = [(r["t"], r.get("at"), r.get("kind")) for r in rows]
        self.assertEqual(kinds, [
            ("start", None, None), ("sample", 0, "start"), ("keys", 0, None),
            ("sample", 10, "periodic"), ("keys", 15, None), ("sample", 20, "periodic"),
            ("sample", 30, "periodic"), ("sample", 40, "periodic"), ("sample", 40, "export"),
            ("sram", 40, None), ("sample", 40, "close"), ("end", 40, None), ("done", None, None)])
        self.assertEqual(rows[3]["frame"], 10)
        # The start sample keeps the boot low-water that the capability probe read.
        self.assertEqual(rows[1]["sp"], [0x03006800 - 0x40, 0x03007E78, 0x08000100])
        self.assertEqual(rows[3]["sp"], [0x03006800, 0x03007E78, 0x08000100])
        self.assertEqual(rows[-2]["stack_mark"], record.STACK_TOP - 64)
        self.assertEqual(rows[-2]["irq_mark"], record.IRQ_TOP - 40)
        self.assertEqual(record.stack_summary(rows)["written_mark_bytes"], 64)

    def test_same_machine_matches_whatever_the_link_dependent_stack_is(self):
        with tempfile.TemporaryDirectory() as tmp:
            first = self.play(Path(tmp) / "a", stack=0x03006800)
            second = self.play(Path(tmp) / "b", stack=0x03006400)
            untracked = self.play(Path(tmp) / "c", tracked=False)
        result = record.compare(first, second)
        self.assertTrue(result["match"], result)
        self.assertEqual((result["processes"], result["updates"], result["samples"],
                          result["exports"]), (1, 40, 7, 1))
        self.assertEqual(record.stack_summary(second)["deepest"]["bytes"], 0x03007E00 - 0x03006400 + 0x40)
        self.assertEqual(record.stack_summary(second)["irq_bytes"], 0x03007FA0 - 0x03007E78)
        self.assertTrue(record.compare(first, untracked)["match"])
        self.assertIsNone(record.stack_summary(untracked)["deepest"])

    def test_first_pixel_difference_is_located(self):
        with tempfile.TemporaryDirectory() as tmp:
            first = self.play(Path(tmp) / "a", tint=7)
            second = self.play(Path(tmp) / "b", tint=8)
        result = record.compare(first, second)
        self.assertFalse(result["match"])
        mismatch = result["first_mismatch"]
        self.assertEqual(mismatch["differing"], ["page0"])
        self.assertEqual((mismatch["first"]["t"], mismatch["first"]["at"]), ("sample", 20))

    def test_different_inputs_and_saves_do_not_match(self):
        with tempfile.TemporaryDirectory() as tmp:
            first = self.play(Path(tmp) / "a", key=0)
            second = self.play(Path(tmp) / "b", key=2)
        mismatch = record.compare(first, second)["first_mismatch"]
        self.assertEqual((mismatch["first"]["t"], mismatch["differing"]), ("keys", ["mask"]))
        saves = [[r["sha"] for r in rows if r["t"] == "sram"] for rows in (first, second)]
        self.assertNotEqual(saves[0], saves[1])

    def test_displayed_page_is_compared_with_a_queued_flip_applied(self):
        with tempfile.TemporaryDirectory() as tmp:
            stays = self.play(Path(tmp) / "a")
            flipped = self.play(Path(tmp) / "b", flip=0x10)
            queued = self.play(Path(tmp) / "c", flip=0x10, queued=True)
        mismatch = record.compare(stays, flipped)["first_mismatch"]
        self.assertEqual(mismatch["differing"], ["display"])
        self.assertEqual(mismatch["first"]["at"], 30)
        # The interrupt has not run yet in one record: DISPCNT differs, the page due does not.
        late = next(r for r in queued if r["t"] == "sample" and r["at"] == 30)
        done = next(r for r in flipped if r["t"] == "sample" and r["at"] == 30)
        self.assertEqual((late["dispcnt"], late["flip_pending"], late["display"]), (0x0404, 1, 0x0414))
        self.assertEqual((done["dispcnt"], done["flip_pending"], done["display"]), (0x0414, 0, 0x0414))
        self.assertTrue(record.compare(flipped, queued)["match"])
        self.assertFalse(record.compare(stays, queued)["match"])

    def test_output_folder_name_is_not_compared(self):
        with tempfile.TemporaryDirectory() as tmp:
            rows = self.play(Path(tmp) / "a")
        renamed = [dict(r, label="relinked") if r["t"] == "start" else r for r in rows]
        self.assertTrue(record.compare(rows, renamed)["match"])

    def test_tool_that_terminates_the_emulator_still_gets_a_final_sample(self):
        with tempfile.TemporaryDirectory() as tmp:
            closed = self.play(Path(tmp) / "a")
            ended = self.play(Path(tmp) / "b", ending="proc.terminate(); proc.stdin.close()")
        self.assertEqual([r["t"] for r in ended[-3:]], ["sample", "end", "done"])
        self.assertTrue(record.compare(closed, ended)["match"])

    def test_emulator_that_died_is_recorded_as_lost_and_never_matches(self):
        with tempfile.TemporaryDirectory() as tmp:
            rows = self.play(Path(tmp) / "a", ending="proc.gone = True; proc.stdin.close()")
        self.assertEqual([r["t"] for r in rows[-2:]], ["lost", "done"])
        result = record.compare(rows, rows)
        self.assertTrue(result["identical"])
        self.assertFalse(result["match"])
        self.assertIn("process 0 has no final sample", result["gaps"][0])

    def test_truncated_empty_or_failed_record_never_matches(self):
        with tempfile.TemporaryDirectory() as tmp:
            rows = self.play(Path(tmp) / "a")
            failed = self.play(Path(tmp) / "b", ending="raise SystemExit('')", status=1)
            with self.assertRaises(ZeroDivisionError):
                self.play(Path(tmp) / "c", ending="1 / 0")
            crashed = record.load(Path(tmp) / "c" / "rec" / "rec.jsonl")
        self.assertFalse(record.compare(rows, rows[:-3])["match"])
        self.assertFalse(record.compare(rows[:-1], rows[:-1])["match"])
        self.assertEqual(failed[-1], {"t": "done", "status": 1})
        self.assertFalse(record.compare(failed, failed)["match"])
        self.assertEqual([r["t"] for r in crashed[-3:]], ["tool_error", "sample", "end"])
        self.assertFalse(record.compare(crashed, crashed)["match"])
        done = [{"t": "done", "status": 0}]
        self.assertFalse(record.compare(done, done)["match"])
        self.assertEqual(json.loads(json.dumps(record.compare(rows, rows)))["match"], True)

    def test_record_missing_any_expected_row_or_pixel_hash_is_not_whole(self):
        with tempfile.TemporaryDirectory() as tmp:
            rows = self.play(Path(tmp) / "a")
        cuts = {
            "no start sample": [r for r in rows if r.get("kind") != "start"],
            "lacks periodic samples": [r for r in rows if not (r.get("kind") == "periodic" and r["at"] == 20)],
            "no final sample": [r for r in rows if r.get("kind") != "close"],
            "without its export sample": [r for r in rows if r.get("kind") != "export"],
            "without its save hash": [r for r in rows if r["t"] != "sram"],
            "without pixel hashes": [{k: v for k, v in r.items() if k != "page1"} for r in rows],
            "sample without pixel hashes": [dict(r, page0=None) if r["t"] == "sample" else r for r in rows],
            "a sample without pixel hashes": [dict(r, display="0404") if r["t"] == "sample" else r for r in rows],
            "lacks periodic samples ": [dict(r, at=10 ** 12) if r["t"] == "end" or r.get("kind") == "close" else r
                                        for r in rows],
            "no sample period": [{k: v for k, v in r.items() if k != "every"} for r in rows],
        }
        for problem, cut in cuts.items():
            with self.subTest(problem=problem):
                result = record.compare(cut, cut)
                self.assertTrue(result["identical"])
                self.assertFalse(result["match"])
                self.assertTrue(any(problem.strip() in text for text in result["gaps"][0]), result["gaps"])
        # Records written before the period was logged can be checked with it supplied.
        self.assertTrue(record.compare(cuts["no sample period"], cuts["no sample period"], every=10)["match"])
        self.assertFalse(record.compare(cuts["no sample period"], cuts["no sample period"], every=5)["match"])

    def test_changed_save_is_the_first_mismatch(self):
        with tempfile.TemporaryDirectory() as tmp:
            rows = self.play(Path(tmp) / "a")
        other = [dict(r, sha="0" * 64) if r["t"] == "sram" else r for r in rows]
        mismatch = record.compare(rows, other)["first_mismatch"]
        self.assertEqual((mismatch["first"]["t"], mismatch["differing"]), ("sram", ["sha"]))

    def test_commands_sent_around_the_recorder_make_the_record_incomplete(self):
        with tempfile.TemporaryDirectory() as tmp:
            for i, command in enumerate(("K2", "U5,0,0", "c", "s")):
                with self.subTest(command=command):
                    rows = self.play(Path(tmp) / str(i), ending=f"rsp.cmd({command!r}); proc.stdin.close()")
                    self.assertIn({"t": "bypass", "proc": 0, "command": command[0]}, rows)
                    result = record.compare(rows, rows)
                    self.assertFalse(result["match"])
                    self.assertIn("process 0 was sent commands around the recorder", result["gaps"][0])

    def test_emulator_left_running_by_the_tool_is_sampled_and_reaped(self):
        FakeReplay.made.clear()
        with tempfile.TemporaryDirectory() as tmp:
            rows = self.play(Path(tmp) / "a", ending="pass")
        self.assertEqual([r["t"] for r in rows[-3:]], ["sample", "end", "done"])
        self.assertEqual(FakeReplay.made[-1].proc.shutdowns, ["kill", "wait"])
        self.assertTrue(record.compare(rows, rows)["match"])

    def test_every_shutdown_path_samples_once_and_still_shuts_down(self):
        FakeReplay.made.clear()
        with tempfile.TemporaryDirectory() as tmp:
            rows = self.play(Path(tmp) / "a", ending="proc.kill(); proc.terminate(); proc.stdin.close()")
        self.assertEqual(sum(r.get("kind") == "close" for r in rows), 1)
        self.assertEqual(sum(r["t"] == "end" for r in rows), 1)
        self.assertEqual(FakeReplay.made[-1].proc.shutdowns, ["kill", "terminate"])
        self.assertTrue(FakeReplay.made[-1].proc.stdin.closed)
        self.assertTrue(record.compare(rows, rows)["match"])

    def test_failed_first_sample_reaps_the_emulator_and_restores_the_transport(self):
        FakeReplay.made.clear()
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(BrokenPipeError):
                self.play(Path(tmp) / "a", broken=True)
            rows = record.load(Path(tmp) / "a" / "rec" / "rec.jsonl")
            again = self.play(Path(tmp) / "b")
            with self.assertRaises(FileExistsError):
                record.Recorder(Path(tmp) / "b" / "rec")
        self.assertEqual(FakeReplay.made[0].proc.shutdowns, ["kill", "wait"])
        self.assertEqual([r["t"] for r in rows], ["start", "tool_error"])
        self.assertFalse(record.compare(rows, rows)["match"])
        self.assertTrue(record.compare(again, again)["match"])


if __name__ == "__main__":
    unittest.main()
