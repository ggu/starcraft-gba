#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Translate a private Terran controller trace into an embedded normal-menu tape.

Usage: make_tape.py <trace.json> <name> <out.h>. The output is derived from a
private trace and must stay in an ignored folder.
"""
import json
import subprocess
import sys
from pathlib import Path

root = Path(subprocess.run(["git", "rev-parse", "--show-toplevel"], cwd=Path(__file__).parent,
                           capture_output=True, text=True, check=True).stdout.strip())
sys.path.insert(0, str(root / "tools"))
trace_path, name, out = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])
trace = json.loads(trace_path.read_text())
if trace["mission"] == 8:
    import sc64_t5_controller_replay as replay
    # Newer T5 probes add observation-only checkpoint kinds; the tape needs
    # only the inputs, so drop observations this validator does not know.
    known = replay.TRACE_CHECKPOINTS | {"input", "won", "lost", "limit"}
    trace["events"] = [e for e in trace["events"] if e.get("kind") in known]
    plan = replay.validate_trace(trace, capacity=256)
    launch = plan["start_tick"]
else:
    import sc64_terran_controller_replay as replay
    plan = replay.validate_trace(trace, capacity=256)
    launch = plan["launch_tick"]
inputs = plan["inputs"]
assert all(a[0] < b[0] for a, b in zip(inputs, inputs[1:]))
# The tape runs 300 ticks past the trace's terminal observation; the probe
# stops at the first result screen.
lines = [f"/* {name}: generated from a private controller trace; do not commit. */",
         f'#define TAPE_NAME "{name}"',
         f"#define TAPE_STOP_TICK {plan['stop_tick'] + 300}u",
         f"#define TAPE_LAUNCH_TICK {launch}u",
         f"#define TAPE_INPUT_COUNT {len(inputs)}u",
         "static const TapeInput tape_inputs[] = {"]
lines += [f"    {{{t}u, {h}u}}," for t, h in inputs]
lines.append("};")
out.write_text("\n".join(lines) + "\n")
final = plan["final"]
print(json.dumps({"name": name, "inputs": len(inputs), "launch": launch, "stop": plan["stop_tick"],
                  "final_kind": final["kind"], "final_frame": final["frame"]}))
