#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""HOST check of a paid Scarab in flight on the Protoss campaign, mission 2.

usage: sc64_paid_scarab_p2_host.py PROGRAM CONTROLLER_TAPE OUTDIR [FRAME]

PROGRAM is the build of tests/sc64_paid_scarab_p2.c; CONTROLLER_TAPE is the tape the P2
controller records. Steps, all through game_update() with pad input only:

  1. expand   the controller tape with its text-box acknowledgements as explicit inputs,
              so the tape alone drives any player, including the ROM
  2. find     the first call at or after FRAME (default 29395) with a human Scarab in flight
  3. insert   an ordinary pause-menu save (tools/sc64_paid_scarab_p2_tape.py) at the first
              neutral call at or after that call whose pause-menu save the campaign writer accepts
  4. ref/run  the uninterrupted tape and the tape with the save must give identical samples
  5. resume   a fresh process loads the SRAM from the title screen and continues through impact

The samples (frame counter, minerals, gas, Reaver stock, Scarab position, unit count, total
hit points and shields, a state digest, Scarab hits) are written to OUTDIR/run.out for
tools/sc64_paid_scarab_p2_arm.py to compare against the ROM. Nothing here is game data."""
import pathlib
import re
import subprocess
import sys


def run(args, **kw):
    return subprocess.run([str(a) for a in args], capture_output=True, text=True, **kw)


def samples(path):
    return [l[2:] for l in path.read_text().splitlines() if l.startswith("S ")]


def main():
    program, tape, out = (pathlib.Path(a) for a in sys.argv[1:4])
    frame = sys.argv[4] if len(sys.argv) > 4 else "29395"
    out.mkdir(parents=True, exist_ok=True)
    here = pathlib.Path(__file__).resolve().parent
    expanded = out / "p2x.json"
    r = run([program, "expand", tape, expanded])
    print(r.stdout.strip().splitlines()[-1])
    if r.returncode:
        sys.exit(r.stderr)
    import os
    env = dict(os.environ, FIND_FRAME=frame)
    r = run([program, "find", expanded], env=env)
    m = re.search(r"after call (\d+): frame (\d+), Reaver stock (\d+)", r.stdout)
    if not m:
        sys.exit("no human Scarab in flight found\n" + r.stdout + r.stderr)
    call = int(m.group(1))
    print(f"first human Scarab in flight: call {call}, frame {m.group(2)}, Reaver stock {m.group(3)}")
    chosen = None
    for after in range(call, call + 120):
        run([sys.executable, here / "sc64_paid_scarab_p2_tape.py", expanded, out / "p2", "--after", after]).check_returncode()
        (out / "p2.sav").unlink(missing_ok=True)
        r = run([program, "run", out / "p2-save.json", out / "p2.sav"])
        (out / "run.out").write_text(r.stdout)
        if r.returncode == 0:
            chosen = after
            break
        if r.returncode != 3:
            sys.exit(f"run failed with {r.returncode}\n{r.stdout}\n{r.stderr}")
    if chosen is None:
        sys.exit("no neutral call near the flight lets the campaign writer save")
    ref = run([program, "ref", out / "p2-ref.json"])
    (out / "ref.out").write_text(ref.stdout)
    res = run([program, "resume", out / "p2-save.json", out / "p2.sav"])
    (out / "resume.out").write_text(res.stdout)
    a, b, c = (samples(out / n) for n in ("ref.out", "run.out", "resume.out"))
    saved = re.search(r"SAVED by the pause menu at call (\d+), frame (\d+)", (out / "run.out").read_text())
    print(f"save written by the pause menu at call {saved.group(1)}, frame {saved.group(2)}, Scarab in flight")
    ok = ref.returncode == 0 and res.returncode == 0 and len(a) == len(b) == len(c) == 12 and a == b == c
    print(f"{len(b)} samples: uninterrupted tape == tape with the save: {a == b}; fresh process == saving process: {b == c}")
    hits = re.findall(r"hits=(\d+)", (out / "run.out").read_text())
    print(f"Scarab hits at the last sample: {hits[-1] if hits else '?'}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
