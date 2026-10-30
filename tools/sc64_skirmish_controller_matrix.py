#!/usr/bin/env python3
"""Run bounded normal-input matches across maps, human races and AI counts.

Completed human losses are valid terminal outcomes. A tick limit, launch failure,
sanitizer failure or invalid result fails the matrix; this is not a test that
every configuration has a proven human winning strategy.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from sc64_skirmish_controller_replay import validate_trace


def run(controller: Path, output: Path, ticks: int = 120000) -> dict:
    if type(ticks) is not int or not 1 <= ticks <= 1000000:
        raise ValueError("ticks must be in [1,1000000]")
    controller = controller.resolve(strict=True)
    output.mkdir(parents=True, exist_ok=True)
    results = []
    failures = []
    def write_report() -> dict:
        report = {"controller": str(controller),
                  "controller_sha256": hashlib.sha256(controller.read_bytes()).hexdigest(),
                  "scope": "two maps x three human races x one to three bots; rotated bot races",
                  "wins": sum(row.get("outcome") == "won" for row in results),
                  "losses": sum(row.get("outcome") == "lost" for row in results),
                  "failures": failures, "matches": results}
        (output / "matrix.json").write_text(json.dumps(report, indent=2) + "\n")
        return report
    for map_id in range(2):
        for race in range(3):
            for bots in range(1, 4):
                label = f"m{map_id}-h{race}-n{bots}"
                trace_path = output / f"{label}.json"
                try:
                    # A successful exit must produce a fresh witness for this run.
                    trace_path.unlink(missing_ok=True)
                    with (output / f"{label}.log").open("w") as log:
                        process = subprocess.run(
                            [str(controller), str(map_id), str(race), str(bots),
                             str(trace_path), str(ticks)], stdout=log, stderr=subprocess.STDOUT,
                            timeout=180)
                    if process.returncode not in (0, 3):
                        raise RuntimeError(f"controller exit {process.returncode}; see {label}.log")
                    payload = trace_path.read_bytes()
                    plan = validate_trace(json.loads(payload), 256, checkpoint_interval=250)
                    terminal = plan["terminal"]
                    if process.returncode != (0 if terminal["kind"] == "won" else 3):
                        raise RuntimeError("exit code and terminal witness disagree")
                    if (terminal["map"], terminal["human_race"], terminal["bot_count"]) != (map_id, race, bots):
                        raise RuntimeError("controller launched a different configuration")
                except (OSError, ValueError, TypeError, KeyError, subprocess.TimeoutExpired, RuntimeError) as exc:
                    failures.append({"configuration": label, "error": str(exc)})
                    write_report()
                    print(f"{label}: incomplete ({exc})", flush=True)
                    continue
                results.append({"map": map_id, "human_race": race, "bots": bots,
                                "outcome": terminal["kind"], "host_ticks": plan["stop_tick"],
                                "frame": terminal["frame"], "trace": str(trace_path.resolve()),
                                "trace_sha256": hashlib.sha256(payload).hexdigest()})
                write_report()
                print(f"{label}: {terminal['kind']} at frame {terminal['frame']}", flush=True)
    report = write_report()
    if failures:
        raise RuntimeError(f"{len(failures)} matches incomplete; see {output / 'matrix.json'}")
    return report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--controller", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--ticks", type=int, default=120000)
    args = parser.parse_args()
    run(args.controller, args.output, args.ticks)


if __name__ == "__main__":
    main()
