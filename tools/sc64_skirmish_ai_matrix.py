#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run and validate custom-game AI progression, recovery and resume checks.

Each case runs build/test-sc64-skirmish-ai-progress-<profile>, which plays a
match through the ordinary Custom Game menu with a passive human. Every record
is validated strictly; a missing, duplicate, malformed or out-of-order record
fails the case. Progression criteria apply to opponents that still own a depot
when the offensive window opens, because an opponent destroyed earlier by
another opponent has no later tech to show.

"match" cases run with --mirror (a HOST fixture: the human owner is planned by
the same planner) to the engine's own victory or defeat and check, from the
snapshots and the spending audit the harness records, that the match ends,
that nothing exists before its prerequisites, that nobody gets anything
without paying, that supply is admitted, that every owner develops and that no
owner lets its economy sit idle or its bank pile up. A run that reaches the tick limit is a failure
while any owner could still act. If nothing in the world can act any more it
is reported as its own outcome, "draw: exhausted"; a "match-draw" case
requires exactly that outcome.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

RACES = "tzp"
PROFILES = ("terran", "zerg", "protoss", "showcase")
BOT_OWNERS = (1, 3, 4)
OWNER_FIELDS = ("owner", "race", "minerals", "gas", "supply", "cap", "workers", "mining",
                "gas_workers", "idle_workers", "building_workers", "army", "army_supply",
                "air_army", "anti_air", "depots", "refineries", "buildings", "complete", "queued",
                "trainable")
OWNER_MAPS = ("unit_raw", "building_raw", "first_made", "first_complete")
# Structures beyond the first producer and supply that show tech progress.
TECH = {0: {112, 113, 114, 120, 122, 123}, 1: {132, 135, 139, 141, 144, 146}, 2: {162, 164, 166, 167, 169}}
WORKERS = {0: 7, 1: 41, 2: 64}
OFFENSIVE_FRAME = 10800
# Full-match cases: the harness cap and the snapshot spacing the thresholds
# below are expressed in (one snapshot is one minute of game time).
MATCH_TICKS = 216000
MATCH_EVERY = 1800
# An owner with a depot, workers and ore left on the map must not keep two or
# more workers idle for IDLE_LIMIT snapshots in a row, nor bank FLOAT_MINERALS
# for FLOAT_LIMIT snapshots in a row below its unit share. The sound lobbies
# of the reference sweep peak at 3 idle and 9 floating snapshots; the defects
# these catch last 20 to 118.
IDLE_LIMIT = 8
FLOAT_LIMIT = 15
FLOAT_MINERALS = 1500
# Snapshots at the end of a run that must all show a world nothing can move.
DEAD_WINDOW = 3

# Structures that must stand complete before a unit or structure may be made,
# by raw ID, as the engine's skirmish catalog defines them (checked against
# the engine by the "catalog" case). A Zerg Lair or Hive satisfies a Hatchery
# prerequisite, a Hive a Lair, and a Greater Spire a Spire.
ALIASES = {131: (131, 132, 133), 132: (132, 133), 141: (141, 137)}
PREREQS = {
    # Terran units, then structures.
    7: {106}, 0: {111}, 32: {111, 112}, 2: {113}, 3: {113, 123}, 5: {113, 120}, 8: {114},
    106: set(), 109: set(), 110: set(), 111: {106}, 112: {111}, 113: {111}, 114: {113},
    120: {113}, 122: {106}, 123: {113}, 124: {122},
    # Zerg.
    41: {131}, 42: {131}, 37: {131, 142}, 38: {131, 135}, 43: {131, 141}, 47: {131, 141},
    131: set(), 132: {131, 142}, 135: {142}, 139: {131}, 141: {132}, 142: {131}, 143: {131},
    144: {143, 139}, 146: {143, 142}, 149: set(),
    # Protoss.
    64: {154}, 65: {160}, 66: {160, 164}, 70: {167}, 72: {167, 169},
    154: set(), 156: set(), 157: set(), 160: {154}, 162: {166}, 164: {160}, 166: {154}, 167: {164}, 169: {167},
}
# Protoss structures, and the units their producers train, also need a
# completed Pylon (the psi field itself is a position and is not checked).
POWERED = {160, 162, 164, 166, 167, 169, 65, 66, 70, 72}
PYLON = 156
RACE_OF = {**{r: 0 for r in (7, 0, 32, 2, 3, 5, 8, 106, 109, 110, 111, 112, 113, 114, 120, 122, 123, 124)},
           **{r: 1 for r in (41, 42, 37, 38, 43, 47, 131, 132, 135, 139, 141, 142, 143, 144, 146, 149)},
           **{r: 2 for r in (64, 65, 66, 70, 72, 154, 156, 157, 160, 162, 164, 166, 167, 169)}}


class RecordError(ValueError):
    pass


SANITIZER_MARK = "SANITIZER REPORT:"


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


def _frames(value, name, frame):
    if type(value) is not dict:
        raise RecordError(f"{name} must be an object")
    out = {}
    for key, at in value.items():
        if type(key) is not str or not key.isdigit() or str(int(key)) != key or int(key) > 255:
            raise RecordError(f"{name} key {key!r} is not a raw ID")
        out[int(key)] = _int(at, name, 0, frame if frame is not None else 10_000_000)
    return out


def validate_owner(row, expected_owner, expected_race, frame=None):
    if type(row) is not dict:
        raise RecordError("owner row must be an object")
    missing = [f for f in OWNER_FIELDS + OWNER_MAPS if f not in row]
    if missing:
        raise RecordError(f"owner row lacks {missing}")
    extra = set(row) - set(OWNER_FIELDS) - set(OWNER_MAPS)
    if extra:
        raise RecordError(f"owner row has unknown fields {sorted(extra)}")
    for f in OWNER_FIELDS:
        # A human owner's bank is a plain integer; a computer owner's clamps.
        _int(row[f], f, 0, 1 << 24 if f in ("minerals", "gas") else 65535)
    if row["owner"] != expected_owner or row["race"] != expected_race:
        raise RecordError(f"owner {row['owner']}/race {row['race']} != {expected_owner}/{expected_race}")
    if row["mining"] + row["gas_workers"] + row["idle_workers"] + row["building_workers"] > row["workers"]:
        raise RecordError("worker order counts exceed workers")
    if row["air_army"] > row["army"] or row["anti_air"] > row["army"] or row["complete"] > row["buildings"]:
        raise RecordError("army or building subtotal exceeds its total")
    if row["trainable"] > 1:
        raise RecordError("trainable must be 0 or 1")
    units = _counts(row["unit_raw"], "unit_raw", False)
    buildings = _counts(row["building_raw"], "building_raw", True)
    if sum(units.values()) < row["workers"] + row["army"]:
        raise RecordError("unit_raw total is below workers + army")
    if sum(c + p for c, p in buildings.values()) != row["buildings"]:
        raise RecordError("building_raw total does not match buildings")
    made = _frames(row["first_made"], "first_made", frame)
    done = _frames(row["first_complete"], "first_complete", frame)
    for raw, at in done.items():
        if raw not in made or made[raw] > at:
            raise RecordError(f"structure {raw} completed before it was made")
    return {**row, "unit_raw": units, "building_raw": buildings, "first_made": made, "first_complete": done}


AUDIT_FIELDS = ("owner", "start", "spent", "spent_gas", "income", "vanished", "low", "high", "first_bad",
                "supply_bad", "supply_over", "model_errors")


def validate_audit(audit, owners):
    """The spending audit that closes a run; owners is [(id, race)]."""
    if type(audit) is not dict or set(audit) not in ({"ore_start", "owners"}, {"ore_start", "owners", "carrier"}):
        raise RecordError("final record lacks a well-formed audit")
    out = {"ore_start": _int(audit["ore_start"], "ore_start", 0, 1 << 24), "owners": []}
    rows = audit["owners"]
    if type(rows) is not list or len(rows) != len(owners):
        raise RecordError("audit owner list does not match the lobby")
    for row, (owner, _) in zip(rows, owners):
        if type(row) is not dict or set(row) != set(AUDIT_FIELDS) | {"made_n"}:
            raise RecordError("audit owner row is malformed")
        if row["owner"] != owner:
            raise RecordError(f"audit owner {row['owner']} != {owner}")
        for f in AUDIT_FIELDS:
            # Income is signed: an unexplained charge makes an update's negative.
            _int(row[f], f, -1 if f == "first_bad" else -(1 << 30) if f == "income" else 0, 1 << 30)
        out["owners"].append({**row, "made_n": _counts(row["made_n"], "made_n", False)})
    if "carrier" in audit:
        out["carrier"] = validate_carrier_use(audit["carrier"], owners)
    return out


CARRIER_FIELDS = ("owner", "outside_updates", "max_outside", "fire_updates")


def validate_carrier_use(rows, owners):
    """Per owner, how the Carrier service was used: updates with an own
    Interceptor in flight, the most in flight at once, and updates with an own
    shot in the projectile pool. Only profiles that compile the service emit it."""
    if type(rows) is not list or len(rows) != len(owners):
        raise RecordError("carrier use list does not match the lobby")
    out = []
    for row, (owner, _) in zip(rows, owners):
        if type(row) is not dict or set(row) != set(CARRIER_FIELDS):
            raise RecordError("carrier use row is malformed")
        if row["owner"] != owner:
            raise RecordError(f"carrier use owner {row['owner']} != {owner}")
        for f in CARRIER_FIELDS:
            _int(row[f], f, 0, 1 << 30)
        out.append(dict(row))
    return out


def validate_run(lines, human, bot_races, every, ticks):
    """Validate a --every run; returns parsed snapshots ending in a final record.

    Every snapshot carries the ore left on the map and, per owner, the first
    frame each unit or structure kind was made and each structure completed;
    the final record carries the spending audit."""
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
        # A match that ends on a snapshot frame prints the final record at the
        # same frame; it must then repeat that snapshot exactly.
        repeat = record["kind"] == "final" and frame == last_frame and records and records[-1]["kind"] == "tick"
        if frame <= last_frame and not repeat:
            raise RecordError(f"line {n + 1}: duplicate or out-of-order frame {frame}")
        if record["kind"] == "tick":
            if frame != last_frame + every:
                raise RecordError(f"line {n + 1}: missing or misaligned snapshot at frame {frame}")
        else:
            if frame > last_frame + every and not repeat:
                raise RecordError(f"line {n + 1}: snapshots missing before the final record")
            finals += 1
        for f in ("won", "lost"):
            _int(record.get(f), f, 0, 1)
        _int(record.get("max_units"), "max_units", 1, 1024)
        _int(record.get("max_buildings"), "max_buildings", 1, 1024)
        _int(record.get("ore_left"), "ore_left", 0, 1 << 24)
        rows = record.get("owners")
        if type(rows) is not list or len(rows) != len(owners):
            raise RecordError(f"line {n + 1}: owner list does not match the lobby")
        record["owners"] = [validate_owner(r, o, race, frame) for r, (o, race) in zip(rows, owners)]
        if records:
            for before, after in zip(records[-1]["owners"], record["owners"]):
                for name in ("first_made", "first_complete"):
                    if any(after[name].get(raw) != at for raw, at in before[name].items()):
                        raise RecordError(f"line {n + 1}: {name} changed after it was recorded")
        if record["kind"] == "final":
            record["audit"] = validate_audit(record.get("audit"), owners)
        elif "audit" in record:
            raise RecordError(f"line {n + 1}: audit outside the final record")
        if repeat and any(record[f] != records[-1][f] for f in ("won", "lost", "ore_left", "owners")):
            raise RecordError(f"line {n + 1}: final record differs from the snapshot at its frame")
        records.append(record)
        last_frame = frame
    if finals != 1:
        raise RecordError("missing final record")
    final = records[-1]
    if not final["won"] and not final["lost"] and final["frame"] < ticks:
        raise RecordError("nonterminal run ended before its tick limit")
    return records


def progression(records, criteria="development", first=1):
    """Per-opponent milestones and the criteria results.

    "economy" checks workers, gas and a basic army; "development" also needs
    two tech structures and two army unit types. A mirror match plans the
    human owner too, so it passes first=0 to judge every owner."""
    out = {}
    for r in records:
        for row in r["owners"][first:]:
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


def dead_world(records):
    """Nothing in the world can act or earn any more: for DEAD_WINDOW snapshots
    in a row no owner has an army, a unit it could afford to train, or a worker
    that mines, idles or builds, and no mineral bank moved."""
    tail = records[-DEAD_WINDOW:]
    if len(tail) < DEAD_WINDOW:
        return False
    for r in tail:
        for row in r["owners"]:
            if row["army"] or row["trainable"] or row["mining"] or row["idle_workers"] or row["building_workers"]:
                return False
    return all(len({r["owners"][i]["minerals"] for r in tail}) == 1 for i in range(len(tail[0]["owners"])))


def outcome(records):
    """"won" or "lost" (the engine's own end), "draw: exhausted" or "stuck"."""
    final = records[-1]
    if final["won"] or final["lost"]:
        return "won" if final["won"] else "lost"
    return "draw: exhausted" if dead_world(records) else "stuck"


def prerequisite_violations(records):
    """Anything made, or standing, before its prerequisite structures were
    complete. The event check compares the frame an owner first made a kind
    (queue entry, foundation, morph or addon start) with the frame its
    prerequisites first stood complete; the snapshot check asks the same of
    every kind present in a snapshot."""
    out = []

    def met(done, raw, at):
        need = set(PREREQS[raw]) | ({PYLON} if raw in POWERED else set())
        return sorted(r for r in need if not any(done.get(a, at + 1) <= at for a in ALIASES.get(r, (r,))))

    for rec in records:
        for row in rec["owners"]:
            made, done = row["first_made"], row["first_complete"]
            present = set(row["unit_raw"]) | set(row["building_raw"])
            for raw in sorted(present & set(PREREQS)):
                if raw not in made:
                    out.append(f"owner {row['owner']}: kind {raw} stands at {rec['frame']} but was never made")
                for r in met(done, raw, rec["frame"]):
                    out.append(f"owner {row['owner']}: kind {raw} stands at {rec['frame']} without {r}")
    final = records[-1]
    for row in final["owners"]:
        for raw, at in sorted(row["first_made"].items()):
            if raw in PREREQS:
                for r in met(row["first_complete"], raw, at):
                    out.append(f"owner {row['owner']}: kind {raw} made at {at} without {r} complete")
    return sorted(set(out))


def audit_violations(final):
    """The bank bound of the spending audit.

    Per update the harness derives each owner's income as bank change plus the
    rule-table prices of everything made, and holds it between zero and the
    mineral cargo that left workers. Here the totals must balance and the
    world must not have spent more than it started with plus what its fields
    gave up (cargo in transit makes that an inequality in the safe direction)."""
    audit, out = final["audit"], []
    banks = spent = starts = vanished = 0
    for row, owner in zip(audit["owners"], final["owners"]):
        who = f"owner {row['owner']}"
        if row["start"] + row["income"] - row["spent"] != owner["minerals"]:
            out.append(f"{who}: start + income - spent does not equal its bank")
        if not 0 <= row["income"] <= row["vanished"]:
            out.append(f"{who}: income {row['income']} outside [0, cargo {row['vanished']}]")
        for name, text in (("low", "updates with a charge no creation explains"),
                           ("high", "updates with money no cargo explains"),
                           ("model_errors", "queue or structure changes the audit could not read"),
                           ("supply_bad", "admissions beyond the supply cap")):
            if row[name]:
                out.append(f"{who}: {row[name]} {text}, first at frame {row['first_bad']}"
                           if name in ("low", "high") else f"{who}: {row[name]} {text}")
        banks += owner["minerals"]; spent += row["spent"]; starts += row["start"]; vanished += row["vanished"]
    removed = audit["ore_start"] - final["ore_left"]
    if banks + spent > starts + removed:
        out.append(f"world spent {banks + spent} but started with {starts} and mined {removed}")
    if vanished > removed:
        out.append(f"cargo {vanished} exceeds ore removed {removed}")
    return out


def economy_streaks(records):
    """Longest runs of snapshots, per owner, with two or more idle workers while
    the owner could put them to work, and with a pile of minerals while its unit
    share was open."""
    final = records[-1]
    share = min(56, (final["max_units"] - 16) // len(final["owners"]))
    out = {}
    for i, row in enumerate(final["owners"]):
        idle = float_ = best_idle = best_float = 0
        for rec in records:
            r = rec["owners"][i]
            units = sum(r["unit_raw"].values()) + r["queued"]
            idle = idle + 1 if r["depots"] and r["workers"] and rec["ore_left"] and r["idle_workers"] >= 2 else 0
            float_ = float_ + 1 if r["buildings"] and r["minerals"] >= FLOAT_MINERALS and units < share - 1 else 0
            best_idle, best_float = max(best_idle, idle), max(best_float, float_)
        out[row["owner"]] = {"idle": best_idle, "float": best_float}
    return out


def match_checks(records, expect_draw=False):
    """Every criterion of a full match; ok only if all hold."""
    final = records[-1]
    end = outcome(records)
    streaks = economy_streaks(records)
    prog = progression(records, "development", 0)
    failed = sorted(o for o, p in prog.items() if p["ok"] is False)
    checks = {
        # "stuck" (the limit reached while some owner could still act) always
        # fails. A dead world is not a stall; it is reported as its own
        # outcome, and a match-draw case requires exactly that.
        "outcome": end == "draw: exhausted" if expect_draw else end != "stuck",
        "prerequisites": not prerequisite_violations(records),
        "audit": not audit_violations(final),
        "development": not failed,
        "idle": all(v["idle"] < IDLE_LIMIT for v in streaks.values()),
        "float": all(v["float"] < FLOAT_LIMIT for v in streaks.values()),
    }
    return {"ok": all(checks.values()), "checks": checks, "outcome": end, "end_frame": final["frame"],
            "streaks": {str(k): v for k, v in streaks.items()}, "failed_owners": failed,
            "prerequisites": prerequisite_violations(records)[:8], "audit": audit_violations(final),
            "progression": {str(k): v for k, v in prog.items()}}


def carrier_evidence(records):
    """Whether a Protoss owner of a finished match used Carriers: it made at
    least one (so a Fleet Beacon and a Stargate stood, and the Carrier was
    paid for like any unit), ordered at least one Interceptor through the
    shared service (a made-count of raw 73 in the spending audit), had its
    Interceptors in flight and had its own shots in the projectile pool."""
    audit = records[-1]["audit"]
    use = {row["owner"]: row for row in audit.get("carrier", [])}
    owners = {}
    for row, snapshot in zip(audit["owners"], records[-1]["owners"]):
        if snapshot["race"] != 2:
            continue
        mine = use.get(row["owner"], {})
        made, orders = row["made_n"].get(CARRIER, 0), row["made_n"].get(INTERCEPTOR, 0)
        owners[row["owner"]] = {"carriers": made, "interceptor_orders": orders,
                                "outside_updates": mine.get("outside_updates", 0),
                                "fire_updates": mine.get("fire_updates", 0),
                                "used": bool(made and orders and mine.get("outside_updates") and mine.get("fire_updates"))}
    return {"ok": any(v["used"] for v in owners.values()), "owners": owners}


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


MORPH_SCENARIOS = ("full", "damaged-during", "low-hp", "low-hp-dies", "save-resume", "save-resume-low-hp")
ADDON_SCENARIOS = ("first-blocked-second-free", "no-feasible-parent")
COMMIT_SCENARIOS = ("exhausted-small-army", "late-small-army", "exhausted-small-army-resume",
                    "minerals-small-army-stays")
BUILDER_SCENARIOS = ("walk-save-resume", "timeout-replan", "timeout-save-resume")
WORKER_SCENARIOS = ("bot-stop-returns", "human-stop-stays", "depleted-patch-reassigned",
                    "constructed-scv-returns")
PLAN_PERIOD, PLAN_PHASE, STALL_PASSES = 48, 16, 40
THREAT_RANGE = 320


def _one(lines, kind):
    rows = [r for r in _records(lines) if r["kind"] == kind]
    if len(rows) != 1:
        raise RecordError(f"expected exactly one {kind} record")
    return rows[0]


def _hex(value, name):
    if type(value) is not str or len(value) != 8 or any(c not in "0123456789abcdef" for c in value):
        raise RecordError(f"{name} must be an 8-digit hex digest")
    return value


def validate_morph_hp(lines, scenario):
    """Recompute the BW morph rule from the record; its ok flag must agree."""
    r = _one(lines, "morph-hp")
    if r.get("scenario") != MORPH_SCENARIOS[scenario]:
        raise RecordError("morph-hp record does not match its scenario")
    for f in ("start_hp", "during_hp", "during_max", "final_hp", "final_max", "final_raw", "completed_at"):
        _int(r.get(f), f, -1, 1 << 20)
    for f in ("alive", "ok"):
        _int(r.get(f), f, 0, 1)
    _int(r.get("charged"), "charged", -1, 1 << 16)
    reference, resumed = _hex(r.get("reference"), "reference"), _hex(r.get("resumed"), "resumed")
    name = MORPH_SCENARIOS[scenario]
    if name == "low-hp-dies":
        good = (r["alive"] == 0 and r["during_hp"] == 0 and r["final_raw"] == 143 and
                r["charged"] == 50 and r["start_hp"] > 0)
    else:
        expected = min(300, max(1, r["during_hp"] - 100))
        good = (r["alive"] == 1 and r["completed_at"] > 0 and r["charged"] == 50 and
                r["during_max"] == 400 and r["final_max"] == 300 and r["final_raw"] == 146 and
                r["final_hp"] == expected and 0 < r["during_hp"] <= 400)
        if name in ("low-hp", "save-resume-low-hp"):
            good = good and r["during_hp"] == r["start_hp"] and r["start_hp"] < 100
        if name == "damaged-during":
            good = good and 0 < r["during_hp"] <= 100
        if name.startswith("save-resume"):
            good = good and reference == resumed and reference != "00000000"
    if r["ok"] != int(good):
        raise RecordError("morph-hp ok flag disagrees with the recorded hit points")
    return r


def validate_addon(lines, scenario):
    r = _one(lines, "addon")
    if r.get("scenario") != ADDON_SCENARIOS[scenario]:
        raise RecordError("addon record does not match its scenario")
    for f in ("factories", "reserved", "charged"):
        v = r.get(f)
        if type(v) is not list or len(v) != 2:
            raise RecordError(f"{f} must be a pair")
        for x in v:
            _int(x, f, -1, 1 << 16)
    for f in ("blocked", "parent", "started_on"):
        _int(r.get(f), f, -2, 1 << 16)
    _int(r.get("ok"), "ok", 0, 1)
    if scenario == 0:
        good = (r["blocked"] == 1 and r["parent"] == r["factories"][1] and r["reserved"] == [50, 50] and
                r["started_on"] == 1 and r["charged"] == [50, 50])
    else:
        good = (r["blocked"] == 2 and r["parent"] == -1 and r["reserved"][1] == 0 and
                r["reserved"][0] != 50 and r["started_on"] == -1 and r["charged"] == [0, 0])
    if r["ok"] != int(good):
        raise RecordError("addon ok flag disagrees with the recorded reservation")
    return r


def validate_commit(lines, scenario):
    """A forced commit sends a small army; the normal threshold keeps it home."""
    r = _one(lines, "commit")
    if r.get("scenario") != COMMIT_SCENARIOS[scenario]:
        raise RecordError("commit record does not match its scenario")
    for f in ("kept", "army_supply", "committed", "commit_frame"):
        _int(r.get(f), f, -1, 1 << 20)
    _int(r.get("ok"), "ok", 0, 1)
    reference, resumed = _hex(r.get("reference"), "reference"), _hex(r.get("resumed"), "resumed")
    small = r["kept"] == 3 and 0 < r["army_supply"] < 12
    if COMMIT_SCENARIOS[scenario] == "minerals-small-army-stays":
        good = small and r["commit_frame"] == -1 and r["committed"] == 0
    else:
        good = small and r["commit_frame"] > 0 and r["committed"] == 3
        if scenario == 2:
            good = good and reference == resumed and reference != "00000000"
    if r["ok"] != int(good):
        raise RecordError("commit ok flag disagrees with the recorded orders")
    return r


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


REPOWER_SCENARIOS = ("army-and-home-nexus-lost",)
AIR_PIN_SCENARIOS = ("ground-army-leaves", "anti-air-defends")
NO_ORE_SCENARIOS = ("spends-bank", "workers-cut-spends-bank")
AUDIT_CONTROL_SCENARIOS = ("free-structure", "free-queued-worker")
AUDIT_CONTROL_FRAME = 7200
AUDIT_CONTROL_TICKS = 9000


def validate_repower(lines, scenario):
    """After its army, first Nexus and every Pylon near the surviving Nexus are
    lost, with supply still above need, the opponent starts a Pylon beside the
    survivor within 12 planning passes. The ok flag is recomputed."""
    r = _one(lines, "repower")
    if r.get("scenario") != REPOWER_SCENARIOS[scenario]:
        raise RecordError("repower record does not match its scenario")
    for f in ("killed_nexus", "lost_pylons", "pylons_left", "gateways", "workers", "free_supply", "margin",
              "near_at_start", "kill_frame", "order_frame", "free_at_order", "margin_at_order",
              "start_frame", "window", "price", "audit_first_bad"):
        _int(r.get(f), f, -1, 1 << 24)
    for f in ("unpowered", "ok"):
        _int(r.get(f), f, 0, 1)
    good = (r["killed_nexus"] == 1 and r["unpowered"] == 1 and r["near_at_start"] == 0 and
            r["free_supply"] >= r["margin"] and r["gateways"] >= 1 and r["workers"] >= 9 and
            r["pylons_left"] >= 1 and r["price"] > 0 and r["window"] == 12 * PLAN_PERIOD and
            r["kill_frame"] > 0 and r["kill_frame"] <= r["order_frame"] and
            r["free_at_order"] >= r["margin_at_order"] > 0 and r["order_frame"] <= r["start_frame"] and
            r["start_frame"] - r["kill_frame"] <= r["window"] and r["audit_first_bad"] == -1)
    if r["ok"] != int(good):
        raise RecordError("repower ok flag disagrees with the recorded observations")
    return r


def validate_air_pin(lines, scenario):
    """An armed flyer over the depot of an owner whose army has been cut to
    three units. Without anti-air at home the army must still go; with it, the
    flyer is a threat and the army answers it."""
    r = _one(lines, "air-pin")
    if r.get("scenario") != AIR_PIN_SCENARIOS[scenario]:
        raise RecordError("air-pin record does not match its scenario")
    for f in ("kept", "anti_air_home", "enemy_air", "threat_dist", "army_supply", "committed",
              "commit_frame", "defend_frame"):
        _int(r.get(f), f, -1, 1 << 24)
    _int(r.get("ok"), "ok", 0, 1)
    flyer = r["enemy_air"] >= 1 and 0 <= r["threat_dist"] < THREAT_RANGE and r["kept"] == 3
    if scenario == 0:
        good = (flyer and r["anti_air_home"] == 0 and r["army_supply"] > 0 and r["commit_frame"] > 0 and
                r["committed"] == 3 and r["defend_frame"] == -1)
    else:
        good = flyer and r["anti_air_home"] >= 1 and r["defend_frame"] > 0
    if r["ok"] != int(good):
        raise RecordError("air-pin ok flag disagrees with the recorded orders")
    return r


def validate_no_ore(lines, scenario):
    """With every mineral field gone, no worker is queued and the bank goes to
    army units: below the cheapest army unit, or the unit share is full."""
    r = _one(lines, "no-ore")
    if r.get("scenario") != NO_ORE_SCENARIOS[scenario]:
        raise RecordError("no-ore record does not match its scenario")
    for f in ("start_frame", "ore_start", "ore_end", "workers_start", "bank_start", "bank_end", "cheapest_army",
              "workers_made", "army_made", "units_end", "queued_end", "share", "audit_first_bad"):
        _int(r.get(f), f, -1, 1 << 24)
    _int(r.get("ok"), "ok", 0, 1)
    good = (r["start_frame"] > 0 and r["ore_start"] == 0 and r["ore_end"] == 0 and 0 < r["cheapest_army"] <= r["bank_start"] and
            r["workers_made"] == 0 and r["army_made"] >= 1 and
            (r["bank_end"] < r["cheapest_army"] or r["units_end"] + r["queued_end"] >= r["share"]) and
            (scenario == 0 or r["workers_start"] == 5) and r["audit_first_bad"] == -1)
    if r["ok"] != int(good):
        raise RecordError("no-ore ok flag disagrees with the recorded observations")
    return r


def audit_control_detected(records):
    """The negative control for the spending audit. In one update the harness
    hands the human a structure or a queued worker that nothing paid for; the
    audit must flag that update, for that owner, and nothing else."""
    rows = records[-1]["audit"]["owners"]
    human, others = rows[0], rows[1:]
    return (human["high"] == 1 and human["first_bad"] == AUDIT_CONTROL_FRAME and human["low"] == 0 and
            human["model_errors"] == 0 and human["supply_bad"] == 0 and
            all(not (r["low"] or r["high"] or r["model_errors"] or r["supply_bad"]) for r in others) and
            any(v.startswith("owner 0:") for v in audit_violations(records[-1])))


def validate_catalog(stdout):
    """The prerequisite table above against the engine's catalog rows."""
    try:
        rows = json.loads(stdout)["audit"]
    except (json.JSONDecodeError, KeyError, TypeError):
        raise RecordError("--audit output is not a catalog document") from None
    if type(rows) is not list:
        raise RecordError("--audit rows must be a list")
    seen = {}
    for row in rows:
        raw = row.get("raw")
        if type(raw) is not int or raw not in PREREQS:
            continue
        kind = row.get("kind")
        need = {row[f] for f in ("prereq1", "prereq2") if row.get(f) != 255}
        if kind != 1 and row.get("producer_raw") != 255:       # a worker builds kind 1
            need.add(row["producer_raw"])
        if kind in (2, 3) and row.get("parent_raw") != 255:
            need.add(row["parent_raw"])
        need = {131 if r == 35 else r for r in need}           # 35: any Zerg hall
        if raw in seen or need != PREREQS[raw] or row.get("race") != RACE_OF[raw]:
            raise RecordError(f"kind {raw}: engine requires {sorted(need)} for race {row.get('race')}, "
                              f"table has {sorted(PREREQS[raw])} for race {RACE_OF[raw]}"
                              if raw not in seen else f"kind {raw} appears twice")
        seen[raw] = need
    missing = sorted(set(PREREQS) - set(seen))
    if missing:
        raise RecordError(f"kinds {missing} are missing from the engine catalog")
    return seen


def classify_end(lines):
    """Why a save case produced no resume record (never counted as a save)."""
    kinds = [r["kind"] for r in _records(lines)
             if r["kind"] in ("ended-before-save", "save-not-possible", "no-morph-before-save")]
    return kinds[-1] if kinds else "no-record"


# Full-match lobbies, (map, human, opponents), the same for every profile: both
# maps, all three human races, one to three opponents, homogeneous and mixed
# opponent races. The first five were unresolved at baseline for one profile or
# more. All pass every criterion in all three profiles with at most 3 idle and
# 9 floating snapshots, against limits of 8 and 15.
MATCH_LOBBIES = (
    (0, "p", "tzp"), (0, "t", "ppp"), (1, "z", "zz"), (1, "t", "pt"), (1, "p", "z"),
    (0, "z", "t"), (0, "t", "z"), (0, "z", "pp"), (0, "p", "ttt"), (0, "z", "zpt"),
    (1, "z", "tzp"), (1, "p", "pt"), (1, "t", "t"), (1, "z", "pp"), (0, "p", "pp"),
    (1, "t", "zzz"), (0, "t", "ttt"), (1, "p", "tt"),
)
# Unresolved at baseline too, but within the limits only in some profiles (the
# Zerg profile's human Protoss banks minerals for 17 snapshots).
MATCH_EXTRA = {"terran": ((1, "p", "pp"),), "protoss": ((1, "p", "pp"),)}
# Long mirrored matches in which a Protoss owner must put Carriers to use: the
# planner's Fleet Beacon, a Carrier from the Stargate (paid like any unit), its
# Interceptors ordered through the same service the human's command uses,
# launched and firing. Only the Protoss profile compiles the Carrier service;
# the lobbies are those of the sweep in which an owner did. They run every
# match criterion too, so the spending audit covers the Interceptor orders.
CARRIER_LOBBIES = ((0, "p", "pp"), (1, "t", "pt"), (1, "p", "pp"), (0, "z", "zpt"))
CARRIER = 72
INTERCEPTOR = 73
# A lobby that ends with nothing left in the world that can act, by profile:
# four owners on the small map with every field mined out. Which lobbies end
# this way shifts with any planner change; when this one starts to resolve,
# pick another from the report.
MATCH_DRAWS = {"zerg": (0, "t", "ppp")}


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
            # Focused fixtures: Creep-to-Sunken hit points (human Zerg colony)
            # and addon feasibility before reservation (Terran opponent).
            for s in range(len(MORPH_SCENARIOS)):
                out.append(("morph-hp", profile, 1, 1, "t", s))
            for s in range(len(ADDON_SCENARIOS)):
                out.append(("addon", profile, 1, 2, "t", s))
            # Forced commit with an army below the normal threshold.
            for s in range(len(COMMIT_SCENARIOS)):
                out.append(("commit", profile, 1, 2, "t", s))
    for profile in PROFILES:
        for s in range(len(BUILDER_SCENARIOS)):
            out.append(("builder", profile, 1, 2, "t", s))
        for s in range(len(WORKER_SCENARIOS)):
            out.append(("workers", profile, 1, 2, "t", s))
    for profile in PROFILES:
        for map_id, human, bots in MATCH_LOBBIES + MATCH_EXTRA.get(profile, ()):
            out.append(("match", profile, map_id, RACES.index(human), bots, MATCH_TICKS))
        if profile in MATCH_DRAWS:
            map_id, human, bots = MATCH_DRAWS[profile]
            out.append(("match-draw", profile, map_id, RACES.index(human), bots, MATCH_TICKS))
        if profile == "protoss":
            for map_id, human, bots in CARRIER_LOBBIES:
                out.append(("match-carrier", profile, map_id, RACES.index(human), bots, MATCH_TICKS))
    if criteria == "development":
        for profile in PROFILES:
            out.append(("catalog", profile, 1, 0, "t", 0))
            for s in range(len(AUDIT_CONTROL_SCENARIOS)):
                for human in range(3):
                    out.append(("audit-control", profile, 0, human, "z", s))
            for human in range(3):
                out.append(("repower", profile, 0, human, "p", 0))
            for s in range(len(AIR_PIN_SCENARIOS)):
                for human, bots in ((0, "z"), (1, "p"), (2, "z")):
                    out.append(("air-pin", profile, 1, human, bots, s))
            for s in range(len(NO_ORE_SCENARIOS)):
                for map_id, human, bots in ((1, 1, "t"), (0, 0, "z"), (1, 1, "p")):
                    out.append(("no-ore", profile, map_id, human, bots, s))
    scenarios = ("pending-builder-killed", "constructing-scv-killed", "foundation-destroyed", "depot-destroyed")
    for profile in PROFILES:
        for map_id in (0, 1):
            for race in range(3):
                for s, name in enumerate(scenarios):
                    if s == 1 and race != 0:
                        continue
                    out.append(("recovery", profile, map_id, (race + 1) % 3, RACES[race], s))
    return out


def sanitizer_report(stderr):
    """The first line of an AddressSanitizer or UBSan report, as a marker that
    fails the case whatever its exit code was (UBSan reports and goes on)."""
    for line in stderr.splitlines():
        if "AddressSanitizer" in line or "runtime error:" in line or "UndefinedBehaviorSanitizer" in line:
            return f"{SANITIZER_MARK} {line.strip()}\n"
    return ""


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
        # Mirrored, so the human owner is still standing when the first
        # Sunken Colony morph starts (about update 18,500) and for the
        # continuation after it; a passive human has lost by then.
        argv += ["--mirror", "--resume-at-morph", "--resume-len", "1800"]
    elif kind == "morph-hp":
        argv += ["--morph-hp", str(arg)]
    elif kind == "addon":
        argv += ["--addon", str(arg)]
    elif kind == "commit":
        argv += ["--commit", str(arg)]
    elif kind == "builder":
        argv += ["--builder", str(arg)]
    elif kind == "workers":
        argv += ["--workers", str(arg)]
    elif kind in ("match", "match-draw", "match-carrier"):
        argv += ["--mirror", "--ticks", str(arg), "--every", str(MATCH_EVERY)]
    elif kind == "catalog":
        argv = [str(binary), "--audit"]
    elif kind == "audit-control":
        argv += ["--audit-control", str(arg), "--ticks", str(AUDIT_CONTROL_TICKS), "--every", str(MATCH_EVERY)]
    elif kind == "repower":
        argv += ["--repower", str(arg)]
    elif kind == "air-pin":
        argv += ["--air-pin", str(arg)]
    elif kind == "no-ore":
        argv += ["--no-ore", str(arg)]
    elif kind == "run":
        argv += ["--ticks", str(arg), "--every", str(every)]
    else:
        argv += ["--recovery", str(arg)]
    proc = subprocess.run(argv, capture_output=True, text=True,
                          timeout=3600 if kind in ("match", "match-draw", "match-carrier") else 900)
    return case, argv, proc.returncode, proc.stdout, sanitizer_report(proc.stderr) + proc.stderr[-2000:]


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
        # Nobody gets anything without paying, with a passive human as well.
        audit = audit_violations(final)
        return {"ok": not failed and not audit, "end_frame": final["frame"], "won": final["won"],
                "lost": final["lost"], "progression": {str(k): v for k, v in prog.items()},
                "failed_owners": failed, "audit": audit}
    if kind in ("match", "match-draw"):
        if returncode != 0:
            raise RecordError(f"harness exit {returncode}")
        records = validate_run(lines, human, bot_races, MATCH_EVERY, arg)
        result = match_checks(records, expect_draw=kind == "match-draw")
        return {"ok": result.pop("ok"), **result}
    if kind == "match-carrier":
        if returncode != 0:
            raise RecordError(f"harness exit {returncode}")
        records = validate_run(lines, human, bot_races, MATCH_EVERY, arg)
        result = match_checks(records)
        evidence = carrier_evidence(records)
        result["checks"]["carriers"] = evidence["ok"]
        return {"ok": result.pop("ok") and evidence["ok"], "carrier": evidence["owners"], **result}
    if kind == "catalog":
        if returncode != 0:
            raise RecordError(f"harness exit {returncode}")
        return {"ok": True, "kinds": len(validate_catalog(stdout))}
    if kind == "audit-control":
        if returncode != 0:
            raise RecordError(f"harness exit {returncode}")
        records = validate_run(lines, human, bot_races, MATCH_EVERY, AUDIT_CONTROL_TICKS)
        return {"ok": audit_control_detected(records), "scenario": AUDIT_CONTROL_SCENARIOS[arg],
                "audit": audit_violations(records[-1])}
    if kind in ("repower", "air-pin", "no-ore"):
        check, code = {"repower": (validate_repower, 15), "air-pin": (validate_air_pin, 16),
                       "no-ore": (validate_no_ore, 17)}[kind]
        r = check(lines, arg)
        if (returncode == 0) != bool(r["ok"]) or returncode not in (0, code):
            raise RecordError(f"exit code and {kind} record disagree")
        return {"ok": bool(r["ok"]), kind: r}
    if kind == "morph-hp":
        r = validate_morph_hp(lines, arg)
        if (returncode == 0) != bool(r["ok"]) or returncode not in (0, 8):
            raise RecordError("exit code and morph-hp record disagree")
        return {"ok": bool(r["ok"]), "morph_hp": r}
    if kind == "addon":
        r = validate_addon(lines, arg)
        if (returncode == 0) != bool(r["ok"]) or returncode not in (0, 9):
            raise RecordError("exit code and addon record disagree")
        return {"ok": bool(r["ok"]), "addon": r}
    if kind == "commit":
        r = validate_commit(lines, arg)
        if (returncode == 0) != bool(r["ok"]) or returncode not in (0, 10):
            raise RecordError("exit code and commit record disagree")
        return {"ok": bool(r["ok"]), "commit": r}
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
    parser.add_argument("--profiles", default=",".join(PROFILES),
                        help="comma-separated profiles to run (default: all of %(default)s)")
    parser.add_argument("--kinds", default="", help="comma-separated case kinds to run (default: all)")
    args = parser.parse_args(argv)
    if not 1800 <= args.ticks <= 200000 or args.every < 60:
        parser.error("ticks must be in [1800,200000] and every >= 60")
    profiles = tuple(p for p in args.profiles.split(",") if p)
    if not profiles or any(p not in PROFILES for p in profiles) or len(set(profiles)) != len(profiles):
        parser.error(f"profiles must be distinct names from {','.join(PROFILES)}")
    kinds = {k for k in args.kinds.split(",") if k}
    selected = [c for c in cases(args.ticks, args.criteria) if c[1] in profiles and (not kinds or c[0] in kinds)]
    if not selected:
        parser.error("no cases selected")
    hashes = {}
    for profile in profiles:
        binary = args.build / f"test-sc64-skirmish-ai-progress-{profile}"
        hashes[profile] = hashlib.sha256(binary.read_bytes()).hexdigest()
    args.output.mkdir(parents=True, exist_ok=True)
    report = {"binaries": hashes, "ticks": args.ticks, "every": args.every,
              "criteria": args.criteria, "profiles": list(profiles), "cases": []}
    started = time.time()
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        for case, argv_used, rc, out, err in pool.map(lambda c: run_case(args.build, c, args.every), selected):
            label = "-".join(str(x) for x in case)
            (args.output / f"{label}.jsonl").write_text(out)
            row = {"case": list(case), "argv": argv_used, "returncode": rc}
            try:
                row.update(evaluate(case, rc, out, args.every, args.criteria))
                if err.startswith(SANITIZER_MARK):
                    row.update(ok=False, error=err.splitlines()[0], stderr=err)
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
            elif r.get("audit"):
                reason = "audit: " + r["audit"][0]
            elif r["case"][0] in ("match", "match-draw"):
                reason = "match: " + ",".join(k for k, v in r["checks"].items() if not v)
            else:
                reason = f"{r['case'][0]} fixture reported failure"
            report["failures_by_reason"].setdefault(reason, []).append("-".join(str(x) for x in r["case"]))
    report["failed"] = len(report["cases"]) - report["passed"]
    report["seconds"] = round(time.time() - started, 1)
    report["by_profile"] = {p: {"passed": sum(r["ok"] for r in report["cases"] if r["case"][1] == p),
                                "failed": sum(not r["ok"] for r in report["cases"] if r["case"][1] == p)}
                            for p in profiles}
    (args.output / "report.json").write_text(json.dumps(report, indent=1, default=list) + "\n")
    for p, counts in report["by_profile"].items():
        print(f"{p}: {counts['passed']} passed, {counts['failed']} failed")
    print(f"{report['passed']} passed, {report['failed']} failed in {report['seconds']} s")
    return 0 if report["failed"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
