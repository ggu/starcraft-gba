#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Record what a physical-button replay tool does to a ROM, and compare two records.

    sc64_physical_record.py run --out DIR [--every N] [--runner RUNNER] [--path DIR]
        -- TOOL.py [tool arguments...]
    sc64_physical_record.py compare A/rec.jsonl B/rec.jsonl [--symbols-a FILE --symbols-b FILE]

`run` executes any tool that drives sc64_headless_physical unchanged and logs
every emulator process it starts to DIR/rec.jsonl:

  start   the save image the process was given (SHA-256) and the sample period
  keys    every held-mask change with its completed-update index
  sample  every N completed updates, at each SRAM export and at close: frame,
          screen and mission scalars, the raw unit, building and mineral
          banks, both framebuffer pages, the BG and OBJ palettes and OBJ VRAM
          (hashes), the display control register, and the runner's exact
          lowest main and IRQ stack pointer since the previous sample when the
          runner answers `SP`
  sram    SHA-256 of each exported save
  end     number of exports, lowest non-zero byte above __iwram_end and in the
          IRQ stack
  lost    the emulator was gone before its final sample could be taken
  bypass  a button, run, update or export command was sent around the recorder

`compare` checks that two records contain the same inputs, samples and saves,
and that each is whole: a zero exit, at least one emulator process, and for
every process a start sample, every periodic sample up to its last update, an
export sample with its save hash, and a final sample followed by its `end`
row, all with well-formed pixel hashes, and no command sent around the
recorder. Stack marks, `end` rows and output folder names depend on the link
or the caller and are not compared. Two links of the same code must produce
identical records. A tool that reads state to choose inputs is compared on
the buttons it pressed too, so a match also shows both ROMs were driven the
same way. Guest memory is only read.

With GBA_ASYNC_PRESENT the game queues a page flip (`flip_pending`) and its
VBlank interrupt performs it, so whether DISPCNT already selects the finished
page at a sample depends on cycle timing, which a relink changes. The compared
`display` value is DISPCNT with the queued flip applied; raw `dispcnt` and
`flip_pending` are logged beside it. A flip that never lands is still caught:
`gfx_begin` waits for it before drawing, so the frame stops and later samples
differ.

The recorder assumes a cooperative driver: one that talks to the runner
through the transport's methods (`set_keys`, `_advance`, `play_to`,
`export_sram`, `read`). It flags commands written to the runner's text
stream outside those methods, but not bytes written to its binary buffer.
"""
from __future__ import annotations

import argparse
import bisect
import hashlib
import json
import os
import runpy
import sys
import threading
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

STACK_TOP, IRQ_TOP = 0x03007E00, 0x03007FA0
SCALARS = ("frame", "screen", "won", "lost", "race_mode", "race_selected_mission",
           "race_completed", "minerals", "gas", "camx", "camy", "demo_frame")
BANKS = ("u", "b", "m", "race_minerals", "race_gas")
VIDEO = (("page0", 0x06000000, 38400), ("page1", 0x0600A000, 38400),
         ("palette", 0x05000000, 1024), ("objvram", 0x06014000, 0x4000))
DISPCNT, PAGE_SELECT = 0x04000000, 0x10
PIXELS = ("display",) + tuple(name for name, _, _ in VIDEO)
UNCOMPARED_FIELDS = ("sp", "label", "dispcnt", "flip_pending")
UNCOMPARED_ROWS = ("end", "tool_error")
READ_TIMEOUT = 120
FORMAT = 2  # 2: start rows carry the format, end rows the process's export count


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class Recorder:
    """Wraps the physical transport of this process; `restore()` undoes it."""

    def __init__(self, out: Path, every: int = 100, runner: Path | None = None):
        import sc64_headless_physical as physical
        import sc64_headless_replay as headless
        if every <= 0:
            raise ValueError("sample period must be positive")
        self.physical, self.headless = physical, headless
        self.every, self.runner = every, runner
        out.mkdir(parents=True, exist_ok=True)
        self.log = (out / "rec.jsonl").open("x")  # one run per record
        self.processes: list = []
        self.saved: list = []
        self.inside = 0  # depth of recorded calls; other K/U/s commands bypass the record
        try:
            self.install()
        except BaseException:
            self.restore()
            raise

    def patch(self, owner, name, replacement):
        self.saved.append((owner, name, getattr(owner, name)))
        setattr(owner, name, replacement)

    def install(self) -> None:
        physical, headless, recorder = self.physical, self.headless, self
        start, set_keys = physical.start, physical.PhysicalReplay.set_keys
        advance = physical.PhysicalReplay._advance
        export, write = headless.HeadlessReplay.export_sram, headless.HeadlessReplay.write

        def guarded(method):  # also used for the recorder's own reads, which are not bypasses
            def inner(self, *args, **kwargs):
                recorder.inside += 1
                try:
                    return method(self, *args, **kwargs)
                finally:
                    recorder.inside -= 1
            return inner

        def recorded_start(runner, rom, out, symbols, sizes, *, save_input=None, save_output=None):
            proc, rsp, log = start(recorder.runner or runner, rom, out, symbols, sizes,
                                   save_input=save_input, save_output=save_output)
            try:
                rsp._rec = dict(proc=len(recorder.processes), sym=symbols, sizes=sizes, dead=False,
                                sp=False, first_sp=None, save_output=save_output, kill=proc.kill,
                                exports=0, recorder=recorder)
                if os.environ.get("PF_SYMS"):
                    try:  # the first answer is the low-water of boot, kept for the start sample
                        rsp._rec["first_sp"] = recorder.stack_query(rsp)
                        rsp._rec["sp"] = True
                    except RuntimeError:
                        pass
                recorder.processes.append(rsp)
                stream = proc.stdin.write

                def watched(text, *args, **kwargs):
                    # K sets buttons, U and c run the emulator, s exports the save. Only the
                    # recorded methods may send them; anything else bypasses the record.
                    if not recorder.inside:
                        for line in str(text).splitlines():
                            if line[:1] in ("K", "U", "c", "s") or line == "SP":
                                recorder.emit({"t": "bypass", "proc": rsp._rec["proc"], "command": line[:1]})
                    return stream(text, *args, **kwargs)
                proc.stdin.write = watched
                recorder.emit({"t": "start", "proc": rsp._rec["proc"], "label": Path(out).name,
                               "save_in": sha(Path(save_input).read_bytes()) if save_input else None,
                               "saves_out": save_output is not None, "sp": rsp._rec["sp"],
                               "every": recorder.every, "format": FORMAT})
                # Tools end a process by closing stdin or by terminating it: sample before either.
                for owner, name in ((proc.stdin, "close"), (proc, "terminate"), (proc, "kill")):
                    def ending(*args, _original=getattr(owner, name), **kwargs):
                        try:
                            recorder.finish(rsp)
                        except Exception:
                            pass  # a record that cannot be written must not block the shutdown
                        return _original(*args, **kwargs)
                    setattr(owner, name, ending)
                recorder.sample(rsp, "start")
            except BaseException:
                # The tool never receives this process, so it is ours to reap.
                rsp._rec["dead"] = True
                rsp._rec["kill"]()
                proc.wait()
                if log is not None:
                    log.close()
                raise
            return proc, rsp, log

        def recorded_keys(self, mask):
            set_keys(self, mask=mask)
            recorder.emit({"t": "keys", "proc": self._rec["proc"], "at": self.completed, "mask": mask})

        def recorded_advance(self, count):
            while count:
                n = min(count, recorder.every - self.completed % recorder.every)
                try:
                    advance(self, n)
                except BaseException as exc:  # the runner may have run part of it
                    recorder.emit({"t": "advance_error", "proc": self._rec["proc"],
                                   "at": self.completed, "error": repr(exc)[:300]})
                    raise
                count -= n
                if self.completed % recorder.every == 0:
                    recorder.sample(self, "periodic")

        def recorded_export(self):
            export(self)
            self._rec["exports"] += 1
            recorder.sample(self, "export")
            path = self._rec["save_output"]
            recorder.emit({"t": "sram", "proc": self._rec["proc"], "at": self.completed,
                           "sha": sha(Path(path).read_bytes()) if path else None})

        def recorded_write(self, address, data):
            recorder.emit({"t": "guest_write", "proc": self._rec["proc"],
                           "at": getattr(self, "completed", -1), "address": address,
                           "length": len(data)})
            write(self, address, data)

        self.patch(physical, "start", recorded_start)
        self.patch(physical.PhysicalReplay, "set_keys", guarded(recorded_keys))
        self.patch(physical.PhysicalReplay, "_advance", guarded(recorded_advance))
        self.patch(headless.HeadlessReplay, "export_sram", guarded(recorded_export))
        self.patch(headless.HeadlessReplay, "write", recorded_write)

    def emit(self, row: dict) -> None:
        self.log.write(json.dumps(row, separators=(",", ":")) + "\n")
        self.log.flush()

    @staticmethod
    def bounded(rsp, exchange):
        """Run one pipe exchange; a runner that stops answering is killed, which ends the read.

        select() cannot bound these reads: earlier answers may already sit in the read buffer,
        and a partial line makes the pipe readable while readline() still blocks.
        """
        watchdog = threading.Timer(READ_TIMEOUT, rsp._rec["kill"])
        watchdog.daemon = True
        watchdog.start()
        recorder = rsp._rec["recorder"]
        recorder.inside += 1
        try:
            return exchange()
        finally:
            recorder.inside -= 1
            watchdog.cancel()

    @classmethod
    def stack_query(cls, rsp):
        def exchange():
            rsp.proc.stdin.write("SP\n")
            rsp.proc.stdin.flush()
            return rsp.proc.stdout.readline()
        fields = cls.bounded(rsp, exchange).split()
        if len(fields) != 4 or fields[0] != "SP":
            raise RuntimeError("runner does not report stack marks")
        return [int(x, 16) for x in fields[1:]]

    @classmethod
    def bulk_read(cls, rsp, ranges):
        """Read several ranges with one pipe hand-off; the runner answers in order."""
        commands, counts = [], []
        for address, length in ranges:
            for i in range(0, length, 512):
                commands.append(f"m{address + i:x},{min(512, length - i)}")
            counts.append((length + 511) // 512)

        def exchange():
            rsp.proc.stdin.write("\n".join(commands) + "\n")
            rsp.proc.stdin.flush()
            lines = []
            for command in commands:
                line = rsp.proc.stdout.readline().rstrip("\n")
                if not line or line.startswith("ERR"):
                    raise RuntimeError(f"headless backend rejected {command!r}: {line or 'closed pipe'}")
                lines.append(line)
            return lines
        lines = cls.bounded(rsp, exchange)
        out, at = [], 0
        for (_, length), count in zip(ranges, counts):
            data = bytes.fromhex("".join(lines[at:at + count]))
            if len(data) != length:
                raise RuntimeError("headless backend returned an incomplete memory read")
            out.append(data)
            at += count
        return out

    def sample(self, rsp, kind: str) -> None:
        info = rsp._rec
        if info["dead"]:
            return
        row = {"t": "sample", "proc": info["proc"], "at": rsp.completed, "kind": kind}
        scalars = [n for n in SCALARS if n in info["sym"] and info["sizes"].get(n) == 4]
        banks = [n for n in BANKS if n in info["sym"]]
        queued = info["sizes"].get("flip_pending") == 1 and "flip_pending" in info["sym"]
        data = self.bulk_read(rsp, [(info["sym"][n], 4) for n in scalars] +
                              [(info["sym"][n], info["sizes"][n]) for n in banks] +
                              [(address, length) for _, address, length in VIDEO] +
                              [(DISPCNT, 2)] + ([(info["sym"]["flip_pending"], 1)] if queued else []))
        for name, raw in zip(scalars, data):
            row[name] = int.from_bytes(raw, "little", signed=True)
        for name, raw in zip(banks + [v[0] for v in VIDEO], data[len(scalars):]):
            row[name] = sha(raw)
        row["dispcnt"] = row["display"] = int.from_bytes(data[len(scalars) + len(banks) + len(VIDEO)], "little")
        if queued:  # the page the VBlank interrupt is about to show counts as shown
            row["flip_pending"] = data[-1][0]
            if row["flip_pending"]:
                row["display"] ^= PAGE_SELECT
        if info["sp"]:
            first, info["first_sp"] = info["first_sp"], None
            row["sp"] = first or self.stack_query(rsp)
        self.emit(row)

    def finish(self, rsp) -> None:
        """Final sample and stack marks; a process that is already gone is recorded as lost."""
        info = rsp._rec
        if info["dead"]:
            return
        try:
            if rsp.proc.poll() is not None:
                raise RuntimeError("emulator exited before its final sample")
            self.sample(rsp, "close")
            row = {"t": "end", "proc": info["proc"], "at": rsp.completed, "exports": info["exports"]}
            end = info["sym"].get("__iwram_end")
            if end:
                stack, irq = self.bulk_read(rsp, [(end, STACK_TOP - end),
                                                  (STACK_TOP, IRQ_TOP - STACK_TOP)])
                row["iwram_end"] = end
                row["stack_mark"] = next((end + i for i, b in enumerate(stack) if b), STACK_TOP)
                row["irq_mark"] = next((STACK_TOP + i for i, b in enumerate(irq) if b), IRQ_TOP)
            self.emit(row)
        except Exception as exc:
            self.emit({"t": "lost", "proc": info["proc"], "error": repr(exc)[:300]})
        finally:
            info["dead"] = True

    def restore(self) -> None:
        try:
            for rsp in self.processes:
                self.finish(rsp)
                if rsp.proc.poll() is None:  # the tool left it running: nobody else will reap it
                    rsp._rec["kill"]()
                    rsp.proc.wait()
        finally:
            while self.saved:  # newest first, so a target patched twice ends as it began
                owner, name, original = self.saved.pop()
                setattr(owner, name, original)
            self.log.close()


def load(path: Path) -> list[dict]:
    return [json.loads(line) for line in Path(path).read_text().splitlines() if line]


def comparable(rows: list[dict]) -> list[dict]:
    return [{k: v for k, v in row.items() if k not in UNCOMPARED_FIELDS}
            for row in rows if row["t"] not in UNCOMPARED_ROWS]


def digest(value) -> bool:
    return isinstance(value, str) and len(value) == 64 and all(c in "0123456789abcdef" for c in value)


def well_formed(sample: dict, pixels: tuple[str, ...]) -> bool:
    return all(isinstance(sample.get(name), int) and 0 <= sample[name] <= 0xFFFF if name == "display"
               else digest(sample.get(name)) for name in pixels)


def gaps(rows: list[dict], every: int | None = None, pixels: tuple[str, ...] = PIXELS) -> list[str]:
    """What keeps one record from being whole evidence of a finished run.

    `every` and `pixels` are only for records written before the start row
    carried the sample period and before DISPCNT was sampled.
    """
    problems = []
    if not rows or rows[-1] != {"t": "done", "status": 0}:
        problems.append("tool did not finish with status 0")
    starts = [r for r in rows if r["t"] == "start"]
    if not starts:
        problems.append("no emulator process")
    for start in starts:
        proc, period = start["proc"], start.get("every", every)
        mine = [r for r in rows if r.get("proc") == proc and r["t"] != "start"]
        samples = [r for r in mine if r["t"] == "sample"]
        closes = [r for r in samples if r["kind"] == "close"]
        ends = [r for r in mine if r["t"] == "end"]
        if any(r["t"] == "lost" for r in mine) or len(closes) != 1 or len(ends) != 1:
            problems.append(f"process {proc} has no final sample")
            continue
        last = ends[0]["at"]
        if closes[0]["at"] != last or mine[-1] is not ends[0] or mine[-2] is not closes[0]:
            problems.append(f"process {proc} final sample is not its last row")
        if any(r["t"] == "advance_error" for r in mine):
            problems.append(f"process {proc} failed an advance")
        # Format-2 records must carry the export count; earlier records did not have it.
        legacy = sum(r["t"] == "sram" for r in mine) if start.get("format", 1) < 2 else None
        if ends[0].get("exports", legacy) != sum(r["t"] == "sram" for r in mine):
            problems.append(f"process {proc} lacks rows for some of its exports")
        if any(r["t"] == "bypass" for r in mine):
            problems.append(f"process {proc} was sent commands around the recorder")
        if not mine or mine[0].get("kind") != "start" or mine[0].get("at") != 0:
            problems.append(f"process {proc} has no start sample")
        if not all(well_formed(r, pixels) for r in samples):
            problems.append(f"process {proc} has a sample without pixel hashes")
        periodic = [r["at"] for r in samples if r["kind"] == "periodic"]
        if not isinstance(period, int) or period <= 0 or not isinstance(last, int) or last < 0:
            problems.append(f"process {proc} has no sample period")
        elif len(periodic) != last // period or any(at != period * (i + 1) for i, at in enumerate(periodic)):
            problems.append(f"process {proc} lacks periodic samples")
        for i, r in enumerate(mine):
            after = mine[i + 1] if i + 1 < len(mine) else {}
            if r["t"] == "sram" and not (i and mine[i - 1].get("kind") == "export" and
                                         mine[i - 1]["at"] == r["at"] and digest(r.get("sha"))):
                problems.append(f"process {proc} has a save hash without its export sample")
            if r.get("kind") == "export" and not (after.get("t") == "sram" and after.get("at") == r["at"]):
                problems.append(f"process {proc} has an export sample without its save hash")
    return problems


def compare(first: list[dict], second: list[dict], every: int | None = None,
            pixels: tuple[str, ...] = PIXELS) -> dict:
    """Whether two whole records agree on inputs, samples and saves, and where they first differ."""
    a, b = comparable(first), comparable(second)
    index = next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), None)
    if index is None and len(a) != len(b):
        index = min(len(a), len(b))
    problems = [gaps(first, every, pixels), gaps(second, every, pixels)]
    result = dict(match=index is None and not any(problems), identical=index is None,
                  complete=not any(problems), gaps=problems, rows=[len(a), len(b)],
                  processes=sum(r["t"] == "start" for r in a),
                  updates=sum(r["at"] for r in first if r["t"] == "end"),
                  samples=sum(r["t"] == "sample" for r in a),
                  exports=sum(r["t"] == "sram" for r in a),
                  guest_writes=sum(r["t"] == "guest_write" for r in a))
    if index is not None:
        x = a[index] if index < len(a) else None
        y = b[index] if index < len(b) else None
        fields = sorted(k for k in set(x or {}) | set(y or {}) if (x or {}).get(k) != (y or {}).get(k))
        result["first_mismatch"] = dict(index=index, differing=fields, first=x, second=y)
    return result


def stack_summary(rows: list[dict], symbols: list[tuple[int, str]] | None = None) -> dict:
    """Deepest exact stack marks of one record, overall and per screen and sample kind."""
    starts = [address for address, _ in symbols or ()]

    def function(pc):
        i = bisect.bisect_right(starts, pc) - 1
        return symbols[i][1] if symbols and i >= 0 else f"{pc:08x}"
    labels = {r["proc"]: r["label"] for r in rows if r["t"] == "start"}
    deepest, phases, irq, marks = None, {}, 0, 0
    for r in rows:
        if r["t"] != "sample" or "sp" not in r:
            continue
        user, irq_sp, pc = r["sp"]
        marks += 1
        if irq_sp != 0xFFFFFFFF:
            irq = max(irq, IRQ_TOP - irq_sp)
        if user == 0xFFFFFFFF:
            continue
        entry = dict(bytes=STACK_TOP - user, sp=f"{user:08x}", function=function(pc),
                     proc=labels.get(r["proc"]), at=r["at"], kind=r["kind"],
                     screen=r.get("screen"), frame=r.get("frame"),
                     mission=r.get("race_selected_mission"))
        if deepest is None or entry["bytes"] > deepest["bytes"]:
            deepest = entry
        key = f"{r.get('screen')}:{r['kind']}"
        if key not in phases or entry["bytes"] > phases[key]["bytes"]:
            phases[key] = entry
    ends = [r for r in rows if r["t"] == "end" and "stack_mark" in r]
    end = ends[0]["iwram_end"] if ends else None
    mark = min((r["stack_mark"] for r in ends), default=None)
    irq_mark = min((r["irq_mark"] for r in ends), default=None)
    return dict(marks=marks, deepest=deepest, by_screen_kind=phases, irq_bytes=irq,
                iwram_end=f"{end:08x}" if end else None,
                free_below_deepest=STACK_TOP - deepest["bytes"] - end if deepest and end else None,
                written_mark_bytes=STACK_TOP - mark if mark else None,
                irq_written_mark_bytes=IRQ_TOP - irq_mark if irq_mark else None)


def read_symbols(path: Path) -> list[tuple[int, str]]:
    """Function starts from the runner's PF_SYMS file: `address size flag name` per line."""
    rows = [line.split() for line in Path(path).read_text().splitlines() if line.strip()]
    return sorted((int(r[0], 16), r[3]) for r in rows)


def run(args) -> int:
    saved_path, saved_argv = list(sys.path), sys.argv
    try:
        for extra in filter(None, (args.path or "").split(":")):
            sys.path.insert(0, extra)
        recorder = Recorder(args.out, args.every, args.runner)
        try:
            sys.argv = [args.tool, *args.arguments]
            status = 0
            try:
                runpy.run_path(args.tool, run_name="__main__")
            except SystemExit as exc:
                status = 0 if exc.code is None else exc.code if isinstance(exc.code, int) else 1
                if not isinstance(exc.code, (int, type(None))):
                    recorder.emit({"t": "tool_error", "error": repr(exc)[:600]})
            except BaseException as exc:
                recorder.emit({"t": "tool_error", "error": repr(exc)[:600]})
                raise
            for rsp in recorder.processes:
                recorder.finish(rsp)
            recorder.emit({"t": "done", "status": status})
            return status
        finally:
            recorder.restore()
    finally:
        sys.argv, sys.path[:] = saved_argv, saved_path


def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    record = sub.add_parser("run")
    record.add_argument("--out", type=Path, required=True)
    record.add_argument("--every", type=int, default=100)
    record.add_argument("--runner", type=Path)
    record.add_argument("--path")
    record.add_argument("tool")
    record.add_argument("arguments", nargs=argparse.REMAINDER)
    check = sub.add_parser("compare")
    check.add_argument("first", type=Path)
    check.add_argument("second", type=Path)
    check.add_argument("--symbols-a", type=Path)
    check.add_argument("--symbols-b", type=Path)
    args = parser.parse_args(argv)
    if args.command == "run":
        if args.arguments[:1] == ["--"]:
            args.arguments = args.arguments[1:]
        return run(args)
    first, second = load(args.first), load(args.second)
    result = compare(first, second)
    result["stack"] = [stack_summary(rows, read_symbols(path) if path else None)
                       for rows, path in ((first, args.symbols_a), (second, args.symbols_b))]
    print(json.dumps(result, indent=1))
    return 0 if result["match"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
