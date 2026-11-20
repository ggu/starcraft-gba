#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay the P2 pad tape with an ordinary pause-menu save on the P10 ROM in mGBA.

usage: sc64_paid_scarab_p2_arm.py ROM ELF RUNNER SAVE_TAPE HOST_OUT OUTDIR

SAVE_TAPE is the tape written by tools/sc64_paid_scarab_p2_tape.py (the P2 controller's
inputs with the four save inputs inserted at "save_start"). HOST_OUT is the output of
`tests/sc64_paid_scarab_p2.c run`, whose "T" and "S" lines give the type ids and the
sampled HOST state. Process 1 plays the tape from the title screen through the save and
on to the last sample; SRAM is exported right after the pause-menu save. Process 2 is a
fresh mGBA process: it loads that SRAM from the title screen with L and plays the same
tape on from the call after the save. Both are compared with the HOST samples, which
are taken at fixed frame counter values. Only physical buttons are used; game memory is
read, never written. Emulator evidence only."""
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sc64_skirmish_sram_continuation as cont  # noqa: E402

SAVE_LEN = 4
KEY_L = 512


def parse_host(text: str):
    ids = dict(re.findall(r"(\w+)=(-?\d+)", next(l for l in text.splitlines() if l.startswith("T "))))
    samples = []
    for line in text.splitlines():
        if line.startswith("S "):
            samples.append({k: int(v) for k, v in re.findall(r"(\w+)=(-?\d+)", line)})
    return {k: int(v) for k, v in ids.items()}, samples


def sample(game, ids):
    st = game.state()
    rows = st["units"]
    owner, typ, x, y, hp, sh, carry = (cont.UNIT_FIELDS.index(n) + 1 for n in
                                       ("owner", "type", "x", "y", "hp", "shield", "carry"))
    scarabs = [r for r in rows if r[typ] == ids["U_SCARAB"] and r[owner] == ids["SIDE"]]
    reavers = [r for r in rows if r[typ] == ids["U_REAVER"] and r[owner] == ids["SIDE"]]
    return {"frame": st["frame"], "minerals": st["minerals"], "gas": st["gas"],
            "stock": (reavers[0][carry] & 15) if reavers else -1,
            "scarabs": 1 if scarabs else 0,
            "sx": scarabs[0][x] if scarabs else -1, "sy": scarabs[0][y] if scarabs else -1,
            "units": len(rows), "allhp": sum(r[hp] for r in rows), "allsh": sum(r[sh] for r in rows)}


def play_samples(game, events, first_call, frames, ids, out):
    """Advance one update at a time from first_call, sampling when the frame counter reaches each target."""
    call = first_call
    for target in frames:
        for _ in range(4000):
            st_frame = game.u32("frame")
            if st_frame >= target:
                break
            game.rsp.play_to(call + 1, events)
            call += 1
        else:
            raise RuntimeError(f"frame counter stuck at {st_frame}, screen {game.u32('screen')}, target {target}, call {call}")
        out.append(sample(game, ids))
    return call


def trace(rom, elf, runner, tape_path, outdir, every, until):
    """Print the same columns as `sc64_paid_scarab_p2 trace` at every call multiple of `every`."""
    tape = json.loads(tape_path.read_text())
    events = [(e["tick"], e["held"]) for e in tape["events"]]
    g = cont.PhysicalGame(runner, rom, elf, outdir / "trace", save_output=None)
    try:
        for end in range(every, until + 1, every):
            g.rsp.play_to(end, events)
            st = g.state()
            print(f"C call={end} frame={st['frame']} screen={st['screen']} minerals={st['minerals']} gas={st['gas']}", flush=True)
    finally:
        g.close()


def main():
    if sys.argv[1] == "trace":
        rom, elf, runner, tape_path, outdir = (Path(a) for a in sys.argv[2:7])
        trace(rom, elf, runner, tape_path, outdir, int(sys.argv[7]), int(sys.argv[8]))
        return 0
    rom, elf, runner, tape_path, host_path, outdir = (Path(a) for a in sys.argv[1:7])
    outdir.mkdir(parents=True, exist_ok=True)
    tape = json.loads(tape_path.read_text())
    events = [(e["tick"], e["held"]) for e in tape["events"]]
    start = tape["save_start"]
    ids, host = parse_host(host_path.read_text())
    frames = [h["frame"] for h in host]
    sram = outdir / "flight.sav"
    for p in (sram, outdir / "p2.sav"):
        p.unlink(missing_ok=True)

    g1 = cont.PhysicalGame(runner, rom, elf, outdir / "p1", save_output=sram)
    first = []
    try:
        g1.rsp.play_to(start + SAVE_LEN, events)          # through the unpause: calls 0 .. start+3
        st = g1.state()
        at_save = {"frame": st["frame"], "screen": st["screen"]}
        g1.rsp.export_sram()
        play_samples(g1, events, start + SAVE_LEN, frames, ids, first)
    finally:
        g1.close()

    second = []
    g2 = cont.PhysicalGame(runner, rom, elf, outdir / "p2", save_input=sram, save_output=outdir / "p2.sav")
    try:
        g2.step(0, 2)
        g2.step(KEY_L, 1)                                   # the real load action from the title
        st = g2.state()
        loaded = {"frame": st["frame"], "screen": st["screen"], "scarabs": sample(g2, ids)["scarabs"]}
        # the loaded game is at the saved frame; the tape continues at call start+SAVE_LEN
        shifted = [(t - (start + SAVE_LEN) + g2.rsp.completed, h) for t, h in events if t >= start + SAVE_LEN]
        # the held keys at the first continued call are the event at or before it
        prior = [h for t, h in events if t <= start + SAVE_LEN]
        if shifted and shifted[0][0] != g2.rsp.completed:
            shifted.insert(0, (g2.rsp.completed, prior[-1] if prior else 0))
        play_samples(g2, shifted, g2.rsp.completed, frames, ids, second)
        g2.rsp.export_sram()
    finally:
        g2.close()

    report = {"rom": rom.name, "tape": tape_path.name, "save_start_call": start, "at_save": at_save,
              "host_base_frame": frames[0], "loaded_in_fresh_process": loaded, "checks": []}
    keys = ("frame", "minerals", "gas", "stock", "scarabs", "sx", "sy", "units", "allhp", "allsh")
    ok = True
    first_divergent = None
    for i, h in enumerate(host):
        hv = {k: h[k] for k in keys}
        row = {"frame": h["frame"], "host": hv, "arm_p1": first[i], "arm_p2": second[i],
               "p1_eq_host": first[i] == hv, "p2_eq_p1": second[i] == first[i]}
        report["checks"].append(row)
        if not (row["p1_eq_host"] and row["p2_eq_p1"]) and first_divergent is None:
            first_divergent = h["frame"]
        ok = ok and row["p1_eq_host"] and row["p2_eq_p1"]
    report["all_match"] = ok
    report["first_divergent_frame"] = first_divergent
    (outdir / "report.json").write_text(json.dumps(report, indent=1))
    print(json.dumps({k: v for k, v in report.items() if k != "checks"}))
    for c in report["checks"]:
        print(c["frame"], "arm==host", c["p1_eq_host"], "fresh==arm", c["p2_eq_p1"], c["arm_p1"] if not c["p1_eq_host"] else "")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
