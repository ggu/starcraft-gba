#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run and validate custom-game AI progression, recovery and resume checks.

Each case runs build/test-sc64-skirmish-ai-progress-<profile>, which plays a
match through the ordinary Custom Game menu with a passive human. Every record
is validated strictly; a missing, duplicate, malformed or out-of-order record
fails the case. Progression criteria apply to opponents that still own a depot
when the offensive window opens, because an opponent destroyed earlier by
another opponent has no later tech to show.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
from pathlib import Path
import subprocess
import sys

RACES = "tzp"
PROFILES = ("terran", "zerg", "protoss", "showcase")
BOT_OWNERS = (1, 3, 4)
OWNER_FIELDS = ("owner", "race", "minerals", "gas", "supply", "cap", "workers", "mining",
                "gas_workers", "idle_workers", "building_workers", "army", "army_supply",
                "air_army", "anti_air", "depots", "refineries", "buildings", "complete", "queued")
# Structures beyond the first producer and supply that show tech progress.
TECH = {0: {112, 113, 114, 120, 122, 123}, 1: {132, 135, 139, 141, 144, 146}, 2: {162, 164, 166, 167}}
WORKERS = {0: 7, 1: 41, 2: 64}
OFFENSIVE_FRAME = 10800


class RecordError(ValueError):
    pass


def _int(value, name, low=0, high=1 << 20):
    if type(value) is not int or not low <= value <= high:
        raise RecordError(f"{name} must be an integer in [{low},{high}]")
    return value


def _counts(value, name, pair):
    if type(value) is not dict:
        raise RecordError(f"{name} must be an object")
    out = {}
    for key, count in value.items():
        if type(key) is not str or not key.isdigit() or str(int(key)) != key or int(key) > 255:
            raise RecordError(f"{name} key {key!r} is not a raw ID")
        if pair:
            if type(count) is not list or len(count) != 2:
                raise RecordError(f"{name}[{key}] must be [complete, pending]")
            out[int(key)] = (_int(count[0], name), _int(count[1], name))
        else:
            out[int(key)] = _int(count, name, 1)
    return out


def validate_owner(row, expected_owner, expected_race):
    if type(row) is not dict:
        raise RecordError("owner row must be an object")
    missing = [f for f in OWNER_FIELDS + ("unit_raw", "building_raw") if f not in row]
    if missing:
        raise RecordError(f"owner row lacks {missing}")
    extra = set(row) - set(OWNER_FIELDS) - {"unit_raw", "building_raw"}
    if extra:
        raise RecordError(f"owner row has unknown fields {sorted(extra)}")
    for f in OWNER_FIELDS:
        _int(row[f], f, 0, 65535)
    if row["owner"] != expected_owner or row["race"] != expected_race:
        raise RecordError(f"owner {row['owner']}/race {row['race']} != {expected_owner}/{expected_race}")
    if row["mining"] + row["gas_workers"] + row["idle_workers"] + row["building_workers"] > row["workers"]:
        raise RecordError("worker order counts exceed workers")
    if row["air_army"] > row["army"] or row["anti_air"] > row["army"] or row["complete"] > row["buildings"]:
        raise RecordError("army or building subtotal exceeds its total")
    units = _counts(row["unit_raw"], "unit_raw", False)
    buildings = _counts(row["building_raw"], "building_raw", True)
    if sum(units.values()) < row["workers"] + row["army"]:
        raise RecordError("unit_raw total is below workers + army")
    if sum(c + p for c, p in buildings.values()) != row["buildings"]:
        raise RecordError("building_raw total does not match buildings")
    return {**row, "unit_raw": units, "building_raw": buildings}


def validate_run(lines, human, bot_races, every, ticks):
    """Validate a --every run; returns parsed snapshots ending in a final record."""
    owners = [(0, human)] + list(zip(BOT_OWNERS, bot_races))
    records, last_frame, finals = [], 0, 0
    for n, text in enumerate(lines):
        try:
            record = json.loads(text)
        except json.JSONDecodeError as exc:
            raise RecordError(f"line {n + 1}: malformed JSON ({exc.msg})") from None
        if type(record) is not dict or record.get("kind") not in ("tick", "final"):
            raise RecordError(f"line {n + 1}: unexpected record kind")
        if finals:
            raise RecordError(f"line {n + 1}: record after final")
        frame = _int(record.get("frame"), "frame", 1, 10_000_000)
        if frame <= last_frame:
            raise RecordError(f"line {n + 1}: duplicate or out-of-order frame {frame}")
        if record["kind"] == "tick":
            if frame != last_frame + every:
                raise RecordError(f"line {n + 1}: missing or misaligned snapshot at frame {frame}")
        else:
            if frame > last_frame + every:
                raise RecordError(f"line {n + 1}: snapshots missing before the final record")
            finals += 1
        for f in ("won", "lost"):
            _int(record.get(f), f, 0, 1)
        _int(record.get("max_units"), "max_units", 1, 1024)
        _int(record.get("max_buildings"), "max_buildings", 1, 1024)
        rows = record.get("owners")
        if type(rows) is not list or len(rows) != len(owners):
            raise RecordError(f"line {n + 1}: owner list does not match the lobby")
        record["owners"] = [validate_owner(r, o, race) for r, (o, race) in zip(rows, owners)]
        records.append(record)
        last_frame = frame
    if finals != 1:
        raise RecordError("missing final record")
    final = records[-1]
    if not final["won"] and not final["lost"] and final["frame"] < ticks:
        raise RecordError("nonterminal run ended before its tick limit")
    return records


def progression(records, criteria="development"):
    """Per-opponent milestones and the criteria results.

    "economy" checks workers, gas and a basic army; "development" also needs
    two tech structures and two army unit types."""
    out = {}
    for r in records:
        for row in r["owners"][1:]:
            p = out.setdefault(row["owner"], {"race": row["race"], "max_workers": 0, "max_army_supply": 0,
                                              "first_gas": None, "army_types": set(), "tech": set(),
                                              "max_depots": 0, "alive_at_offensive": False})
            p["max_workers"] = max(p["max_workers"], row["workers"])
            p["max_army_supply"] = max(p["max_army_supply"], row["army_supply"])
            p["max_depots"] = max(p["max_depots"], row["depots"])
            if p["first_gas"] is None and row["gas_workers"]:
                p["first_gas"] = r["frame"]
            p["army_types"] |= {raw for raw in row["unit_raw"] if raw not in (WORKERS[row["race"]], 42)}
            p["tech"] |= {raw for raw, (done, _) in row["building_raw"].items() if done and raw in TECH[row["race"]]}
            if r["frame"] >= OFFENSIVE_FRAME and row["depots"] and not p["alive_at_offensive"]:
                p["alive_at_offensive"] = True
    for p in out.values():
        p["army_types"] = sorted(p["army_types"])
        p["tech"] = sorted(p["tech"])
        checks = {"workers": p["max_workers"] >= 8, "gas": p["first_gas"] is not None,
                  "army": p["max_army_supply"] >= 8}
        if criteria == "development":
            checks.update(tech=len(p["tech"]) >= 2, army_types=len(p["army_types"]) >= 2,
                          army=p["max_army_supply"] >= 12)
        p["checks"] = checks
        p["ok"] = all(checks.values()) if p["alive_at_offensive"] else None
    return out


def _records(lines):
    out = []
    for n, text in enumerate(lines):
        try:
            record = json.loads(text)
        except json.JSONDecodeError as exc:
            raise RecordError(f"line {n + 1}: malformed JSON ({exc.msg})") from None
        if type(record) is not dict or type(record.get("kind")) is not str:
            raise RecordError(f"line {n + 1}: record has no kind")
        out.append(record)
    return out


def validate_resume(lines):
    resume = [r for r in _records(lines) if r["kind"] == "resume"]
    if len(resume) != 1:
        raise RecordError("expected exactly one resume record")
    r = resume[0]
    for f in ("saved_frame", "reference_frame", "resumed_frame"):
        _int(r.get(f), f, 1, 10_000_000)
    for f in ("saved", "restored", "reference", "resumed"):
        v = r.get(f)
        if type(v) is not str or len(v) != 8 or any(c not in "0123456789abcdef" for c in v):
            raise RecordError(f"{f} must be an 8-digit hex digest")
    if r.get("match") not in (0, 1):
        raise RecordError("match must be 0 or 1")
    consistent = (r["saved"] == r["restored"] and r["reference"] == r["resumed"] and
                  r["reference_frame"] == r["resumed_frame"] and r["reference_frame"] > r["saved_frame"])
    if r["match"] != int(consistent):
        raise RecordError("match flag disagrees with digests")
    return r


def validate_recovery(lines, scenario_name):
    rows = [r for r in _records(lines) if r["kind"] == "recovery"]
    if len(rows) != 1:
        raise RecordError("expected exactly one recovery record")
    r = rows[0]
    if r.get("scenario") != scenario_name or r.get("ok") not in (0, 1):
        raise RecordError("recovery record does not match its scenario")
    return r


BUILDER_SCENARIOS = ("walk-save-resume", "timeout-replan", "timeout-save-resume")
WORKER_SCENARIOS = ("bot-stop-returns", "human-stop-stays", "depleted-patch-reassigned",
                    "constructed-scv-returns")
PLAN_PERIOD, PLAN_PHASE, STALL_PASSES = 48, 16, 40


def _one(lines, kind):
    rows = [r for r in _records(lines) if r["kind"] == kind]
    if len(rows) != 1:
        raise RecordError(f"expected exactly one {kind} record")
    return rows[0]


def _hex(value, name):
    if type(value) is not str or len(value) != 8 or any(c not in "0123456789abcdef" for c in value):
        raise RecordError(f"{name} must be an 8-digit hex digest")
    return value


def validate_builder(lines, scenario):
    """Walking builders: arrival and timeout survive save/reload; a released
    builder costs nothing and its structure is planned again."""
    r = _one(lines, "builder")
    if r.get("scenario") != BUILDER_SCENARIOS[scenario]:
        raise RecordError("builder record does not match its scenario")
    for f in ("start", "timer_at_save", "event", "reference_event", "replanned"):
        _int(r.get(f), f, -1, 1 << 24)
    for f in ("arrived", "released", "ok"):
        _int(r.get(f), f, 0, 1)
    if r.get("stall_passes") != STALL_PASSES:
        raise RecordError("stall_passes differs from the planner constant")
    reference, resumed = _hex(r.get("reference"), "reference"), _hex(r.get("resumed"), "resumed")
    if scenario == 0:
        good = (r["arrived"] == 1 and r["timer_at_save"] >= 1 and r["event"] > 0 and
                r["event"] == r["reference_event"] and reference == resumed and reference != "00000000")
    else:
        good = (r["released"] == 1 and r["arrived"] == 0 and r["replanned"] >= r["event"] > r["start"] >= 0 and
                r["event"] - r["start"] <= (STALL_PASSES + 2) * PLAN_PERIOD)
        if scenario == 2:
            good = good and (r["timer_at_save"] == STALL_PASSES // 2 and r["event"] == r["reference_event"] and
                             reference == resumed and reference != "00000000")
    if r["ok"] != int(good):
        raise RecordError("builder ok flag disagrees with the recorded events")
    return r


def validate_workers(lines, scenario):
    r = _one(lines, "workers")
    if r.get("scenario") != WORKER_SCENARIOS[scenario]:
        raise RecordError("workers record does not match its scenario")
    for f in ("slot", "idle_at", "back_at", "old_patch", "new_patch", "order"):
        _int(r.get(f), f, -1, 1 << 24)
    _int(r.get("ok"), "ok", 0, 1)
    if scenario == 1:
        good = r["slot"] >= 0 and r["back_at"] == -1 and r["order"] == 0
    else:
        good = (r["slot"] >= 0 and r["idle_at"] > 0 and
                0 <= r["back_at"] - r["idle_at"] <= 2 * PLAN_PERIOD + PLAN_PHASE)
        if scenario == 2:
            good = good and r["new_patch"] != r["old_patch"]
    if r["ok"] != int(good):
        raise RecordError("workers ok flag disagrees with the recorded orders")
    return r


def classify_end(lines):
    """Why a save case produced no resume record (never counted as a save)."""
    kinds = [r["kind"] for r in _records(lines)
             if r["kind"] in ("ended-before-save", "save-not-possible", "no-morph-before-save")]
    return kinds[-1] if kinds else "no-record"


def cases(ticks, criteria="development"):
    out = []
    for profile in PROFILES:
        for map_id in (0, 1):
            for human in range(3):
                for bots in (1, 2, 3):
                    out.append(("run", profile, map_id, human, "".join(RACES[(human + 1 + i) % 3] for i in range(bots)), ticks))
    for profile, race in zip(PROFILES[:3], RACES):
        for map_id in (0, 1):
            out.append(("run", profile, map_id, (RACES.index(race) + 1) % 3, race * 3, ticks))
            # The passive human usually falls soon after the six-minute mark on
            # the smaller map; save before that while opponents are teching.
            out.append(("resume", profile, map_id, (RACES.index(race) + 1) % 3, race * 3,
                        9000 if map_id == 0 else 15000))
    for profile in PROFILES:
        for human in range(3):
            bots = "".join(RACES[(human + 1 + i) % 3] for i in range(3))
            out.append(("resume", profile, 1, human, bots, 15000))
            # Later saves hold larger armies and stress the save capacity.
            out.append(("resume-late", profile, 1, human, bots, 22000))
    if criteria == "development":
        # Save while a Creep Colony is morphing into a Sunken Colony; the
        # in-progress morph keeps its source hit points.
        for profile in PROFILES:
            for human in (0, 2):
                out.append(("resume-morph", profile, 1, human, "zz", 0))
    for profile in PROFILES:
        for s in range(len(BUILDER_SCENARIOS)):
            out.append(("builder", profile, 1, 2, "t", s))
        for s in range(len(WORKER_SCENARIOS)):
            out.append(("workers", profile, 1, 2, "t", s))
    scenarios = ("pending-builder-killed", "constructing-scv-killed", "foundation-destroyed", "depot-destroyed")
    for profile in PROFILES:
        for map_id in (0, 1):
            for race in range(3):
                for s, name in enumerate(scenarios):
                    if s == 1 and race != 0:
                        continue
                    out.append(("recovery", profile, map_id, (race + 1) % 3, RACES[race], s))
    return out


def run_case(build: Path, case, every):
    kind, profile, map_id, human, bots, arg = case
    binary = build / f"test-sc64-skirmish-ai-progress-{profile}"
    argv = [str(binary), "--map", str(map_id), "--human", RACES[human], "--bots", bots]
    if kind in ("resume", "resume-late"):
        # Save as late as the match allows, up to the requested tick: a probe
        # run finds the end frame with the passive human, then the save lands
        # at least 2,400 frames before it.
        probe = subprocess.run(argv + ["--ticks", str(arg), "--every", "0"],
                               capture_output=True, text=True, timeout=900)
        final = [json.loads(t) for t in probe.stdout.splitlines() if t.startswith('{"kind":"final"')]
        end = final[-1]["frame"] if probe.returncode == 0 and len(final) == 1 else 0
        arg = min(arg, end - 2400)
        if arg < (9000 if kind == "resume-late" else 6000):
            return case, argv, 9, "", f"match ends at {end}; no save point"
        argv += ["--resume-at", str(arg), "--resume-len", "1800"]
    elif kind == "resume-morph":
        argv += ["--resume-at-morph", "--resume-len", "1800"]
    elif kind == "builder":
        argv += ["--builder", str(arg)]
    elif kind == "workers":
        argv += ["--workers", str(arg)]
    elif kind == "run":
        argv += ["--ticks", str(arg), "--every", str(every)]
    else:
        argv += ["--recovery", str(arg)]
    proc = subprocess.run(argv, capture_output=True, text=True, timeout=900)
    return case, argv, proc.returncode, proc.stdout, proc.stderr[-2000:]


def evaluate(case, returncode, stdout, every, criteria="development"):
    kind, profile, map_id, human, bots, arg = case
    lines = [t for t in stdout.splitlines() if t.strip()]
    bot_races = [RACES.index(c) for c in bots]
    if kind == "run":
        if returncode != 0:
            raise RecordError(f"harness exit {returncode}")
        records = validate_run(lines, human, bot_races, every, arg)
        prog = progression(records, criteria)
        failed = [o for o, p in prog.items() if p["ok"] is False]
        final = records[-1]
        return {"ok": not failed, "end_frame": final["frame"], "won": final["won"], "lost": final["lost"],
                "progression": {str(k): v for k, v in prog.items()}, "failed_owners": failed}
    if kind in ("builder", "workers"):
        r = (validate_builder if kind == "builder" else validate_workers)(lines, arg)
        if (returncode == 0) != bool(r["ok"]) or returncode not in (0, 11 if kind == "builder" else 12):
            raise RecordError(f"exit code and {kind} record disagree")
        return {"ok": bool(r["ok"]), kind: r}
    if kind in ("resume", "resume-late", "resume-morph"):
        if returncode in (4, 9) and not any(r["kind"] == "resume" for r in _records(lines)):
            # The match ended (or no save point existed) before the save:
            # classified, and never counted as a successful save.
            raise RecordError(f"no save: {classify_end(lines) if returncode == 4 else 'no-save-point'}")
        r = validate_resume(lines)
        if returncode not in (0, 4, 6):
            raise RecordError(f"harness exit {returncode}")
        if (returncode == 0) != bool(r["match"]):
            raise RecordError("exit code and resume record disagree")
        return {"ok": bool(r["match"]), "resume": r}
    names = ("pending-builder-killed", "constructing-scv-killed", "foundation-destroyed", "depot-destroyed")
    r = validate_recovery(lines, names[arg])
    if (returncode == 0) != bool(r["ok"]):
        raise RecordError("exit code and recovery record disagree")
    return {"ok": bool(r["ok"]), "recovery": r}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=Path("build"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--ticks", type=int, default=36000)
    parser.add_argument("--every", type=int, default=1800)
    parser.add_argument("--jobs", type=int, default=6)
    parser.add_argument("--criteria", choices=("economy", "development"), default="development")
    args = parser.parse_args(argv)
    if not 1800 <= args.ticks <= 200000 or args.every < 60:
        parser.error("ticks must be in [1800,200000] and every >= 60")
    hashes = {}
    for profile in PROFILES:
        binary = args.build / f"test-sc64-skirmish-ai-progress-{profile}"
        hashes[profile] = hashlib.sha256(binary.read_bytes()).hexdigest()
    args.output.mkdir(parents=True, exist_ok=True)
    report = {"binaries": hashes, "ticks": args.ticks, "every": args.every,
              "criteria": args.criteria, "cases": []}
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        for case, argv_used, rc, out, err in pool.map(lambda c: run_case(args.build, c, args.every),
                                                     cases(args.ticks, args.criteria)):
            label = "-".join(str(x) for x in case)
            (args.output / f"{label}.jsonl").write_text(out)
            row = {"case": list(case), "argv": argv_used, "returncode": rc}
            try:
                row.update(evaluate(case, rc, out, args.every, args.criteria))
            except (RecordError, ValueError, KeyError, TypeError) as exc:
                row.update(ok=False, error=str(exc), stderr=err)
            report["cases"].append(row)
            print(f"{label}: {'ok' if row['ok'] else 'FAIL ' + row.get('error', '')}", flush=True)
    report["passed"] = sum(r["ok"] for r in report["cases"])
    # Every failure keeps its validator reason; group them for the summary.
    report["failures_by_reason"] = {}
    for r in report["cases"]:
        if not r["ok"]:
            if r.get("error"):
                reason = r["error"]
            elif r.get("failed_owners"):
                reason = "progression: owners " + ",".join(map(str, r["failed_owners"]))
            else:
                reason = f"{r['case'][0]} fixture reported failure"
            report["failures_by_reason"].setdefault(reason, []).append("-".join(str(x) for x in r["case"]))
    report["failed"] = len(report["cases"]) - report["passed"]
    (args.output / "report.json").write_text(json.dumps(report, indent=1, default=list) + "\n")
    print(f"{report['passed']} passed, {report['failed']} failed")
    return 0 if report["failed"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
