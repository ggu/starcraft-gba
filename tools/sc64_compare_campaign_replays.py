#!/usr/bin/env python3
"""Compare reported gameplay state between two campaign replay reports.

This compares the state exposed by replay checkpoints. It does not claim RAM,
rendered pixels, or unreported game state are identical.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any


PROFILE = "protoss-p3"
MISSION = 22
CHECKPOINT_METADATA = {
    "elapsed_hardware_cycles",
    "frame_delta",
    "counted_live_play_interval",
    "effective_fps",
    "profile_cycles_last_tick",
    "route_counters",
    "route_interval",
    "capture",
}
REQUIRED_STATE = (
    "screen", "frame", "won", "lost", "race_mode", "race_selected_mission",
    "race_human", "race_completed", "units", "buildings", "minerals", "gas", "race_minerals",
    "race_gas", "supply", "supply_cap", "race_campaign", "race_p3",
)
P3_MODEL_FIELDS = ("fired", "outcome", "sequence", "transmission", "reserved", "action_ticks")
CAMPAIGN_FIELDS = ("mission", "elapsed_frames", "phase", "objective_flags", "objective_stage")
ENTITY_SUMMARY_FIELDS = ("alive", "by_owner", "by_owner_type", "entities")


class ReportError(ValueError):
    """A report is incomplete, malformed, or incompatible with the other report."""


def _integer(value: Any, label: str, *, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        raise ReportError(f"{label} must be an integer >= {minimum}")
    return value


def _nonempty_string(value: Any, label: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ReportError(f"{label} must be a nonempty string")
    return value


def _no_nulls(value: Any, path: str) -> None:
    if value is None:
        raise ReportError(f"required gameplay state {path} is null")
    if isinstance(value, dict):
        for key, child in value.items():
            _no_nulls(child, f"{path}.{key}")
    elif isinstance(value, list):
        for index, child in enumerate(value):
            _no_nulls(child, f"{path}[{index}]")


def _validate_actor_summary(summary: dict[str, Any], name: str, label: str) -> None:
    alive = _integer(summary["alive"], f"{label}.{name}.alive")
    if not isinstance(summary["entities"], list):
        raise ReportError(f"{label}.{name}.entities must be an array")
    if len(summary["entities"]) != alive:
        raise ReportError(f"{label}.{name}.alive does not match entities length")
    if not isinstance(summary["by_owner"], dict) or not isinstance(summary["by_owner_type"], dict):
        raise ReportError(f"{label}.{name}.owner summaries must be objects")
    for owner, count in summary["by_owner"].items():
        if not isinstance(owner, str):
            raise ReportError(f"{label}.{name}.by_owner keys must be strings")
        _integer(count, f"{label}.{name}.by_owner.{owner}", minimum=1)
    for owner, types in summary["by_owner_type"].items():
        if not isinstance(owner, str) or not isinstance(types, dict) or not types:
            raise ReportError(f"{label}.{name}.by_owner_type entries must be nonempty objects")
        for unit_type, count in types.items():
            if not isinstance(unit_type, str):
                raise ReportError(f"{label}.{name}.by_owner_type keys must be strings")
            _integer(count, f"{label}.{name}.by_owner_type.{owner}.{unit_type}", minimum=1)

    owners: dict[str, int] = {}
    owner_types: dict[str, dict[str, int]] = {}
    slots: set[int] = set()
    fields = (("slot", "owner", "type", "x", "y", "raw_type", "hp", "shield")
              if name == "units" else
              ("slot", "owner", "type", "x", "y", "raw_type", "under_construction"))
    for index, entity in enumerate(summary["entities"]):
        path = f"{label}.{name}.entities[{index}]"
        if not isinstance(entity, dict):
            raise ReportError(f"{path} must be an object")
        values: dict[str, int] = {}
        for field in fields:
            if field not in entity:
                raise ReportError(f"{path}.{field} is missing")
            values[field] = _integer(entity[field], f"{path}.{field}",
                                     minimum=-1 if field == "raw_type" else 0)
        if values["slot"] in slots:
            raise ReportError(f"{label}.{name}.entities contains duplicate slot {values['slot']}")
        slots.add(values["slot"])
        owner = str(values["owner"])
        unit_type = str(values["type"])
        owners[owner] = owners.get(owner, 0) + 1
        owner_types.setdefault(owner, {})[unit_type] = owner_types.setdefault(owner, {}).get(unit_type, 0) + 1

    if summary["by_owner"] != owners or summary["by_owner_type"] != owner_types:
        raise ReportError(f"{label}.{name} counts do not match its entity list")
    if name == "buildings" and "completed_by_owner_type" in summary:
        completed = summary["completed_by_owner_type"]
        if not isinstance(completed, dict):
            raise ReportError(f"{label}.buildings.completed_by_owner_type must be an object")
        for owner, types in completed.items():
            if not isinstance(owner, str) or not isinstance(types, dict):
                raise ReportError(f"{label}.buildings.completed_by_owner_type entries must be objects")
            for unit_type, count in types.items():
                if not isinstance(unit_type, str):
                    raise ReportError(f"{label}.buildings.completed_by_owner_type keys must be strings")
                _integer(count, f"{label}.buildings.completed_by_owner_type.{owner}.{unit_type}", minimum=1)
        completed_counts: dict[str, dict[str, int]] = {}
        for entity in summary["entities"]:
            if entity["under_construction"] == 0:
                owner, unit_type = str(entity["owner"]), str(entity["type"])
                completed_counts.setdefault(owner, {})[unit_type] = completed_counts.setdefault(owner, {}).get(unit_type, 0) + 1
        if completed != completed_counts:
            raise ReportError(f"{label}.buildings.completed_by_owner_type does not match entities")


def _state_projection(checkpoint: Any, label: str) -> dict[str, Any]:
    if not isinstance(checkpoint, dict):
        raise ReportError(f"{label} must be an object")
    for key in REQUIRED_STATE:
        if key not in checkpoint:
            raise ReportError(f"{label}.{key} is missing")
    for key in REQUIRED_STATE:
        _no_nulls(checkpoint[key], f"{label}.{key}")
    for name in ("units", "buildings"):
        summary = checkpoint[name]
        if not isinstance(summary, dict):
            raise ReportError(f"{label}.{name} must be an object")
        for field in ENTITY_SUMMARY_FIELDS:
            if field not in summary:
                raise ReportError(f"{label}.{name}.{field} is missing")
        _validate_actor_summary(summary, name, label)
    campaign = checkpoint["race_campaign"]
    model = checkpoint["race_p3"]
    if not isinstance(campaign, dict) or not isinstance(model, dict):
        raise ReportError(f"{label}.race_campaign and race_p3 must be objects")
    for field in CAMPAIGN_FIELDS:
        if field not in campaign:
            raise ReportError(f"{label}.race_campaign.{field} is missing")
        _integer(campaign[field], f"{label}.race_campaign.{field}")
    for field in P3_MODEL_FIELDS:
        if field not in model:
            raise ReportError(f"{label}.race_p3.{field} is missing")
        _integer(model[field], f"{label}.race_p3.{field}")
    # The allowlist is intentionally for metadata only. Every other reported
    # field is gameplay state and participates automatically, including fields
    # added by newer replay observers.
    projected = {key: value for key, value in checkpoint.items()
                 if key not in CHECKPOINT_METADATA}
    for key, value in projected.items():
        _no_nulls(value, f"{label}.{key}")
    return projected


def _diff(left: Any, right: Any, path: str, tick: int, out: list[dict[str, Any]], limit: int) -> None:
    if len(out) >= limit:
        return
    if type(left) is not type(right):
        out.append({"tick": tick, "path": path, "baseline": _brief(left), "candidate": _brief(right)})
    elif isinstance(left, dict):
        for key in sorted(left.keys() | right.keys(), key=str):
            if len(out) >= limit:
                return
            child_path = f"{path}.{key}" if path else str(key)
            if key not in left:
                out.append({"tick": tick, "path": child_path, "baseline": "<missing>", "candidate": _brief(right[key])})
            elif key not in right:
                out.append({"tick": tick, "path": child_path, "baseline": _brief(left[key]), "candidate": "<missing>"})
            else:
                _diff(left[key], right[key], child_path, tick, out, limit)
    elif isinstance(left, list):
        common = min(len(left), len(right))
        for index in range(common):
            _diff(left[index], right[index], f"{path}[{index}]", tick, out, limit)
            if len(out) >= limit:
                return
        if len(left) != len(right):
            out.append({"tick": tick, "path": f"{path}.length", "baseline": len(left), "candidate": len(right)})
    elif left != right:
        out.append({"tick": tick, "path": path, "baseline": _brief(left), "candidate": _brief(right)})


def _brief(value: Any, limit: int = 240) -> Any:
    """Keep each diagnostic bounded even for a mismatched entity array."""
    if isinstance(value, (dict, list, str)):
        encoded = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
        if len(encoded) > limit:
            return encoded[:limit] + f"… <{len(encoded)} chars>"
        return value
    return value


def _validate_report(report: Any, label: str, allow_prefix: bool) -> tuple[list[dict[str, Any]], list[int]]:
    if not isinstance(report, dict):
        raise ReportError(f"{label} report must be an object")
    if type(report.get("mission")) is not int or report["mission"] != MISSION:
        raise ReportError(f"{label}.mission must be P3 global ID {MISSION}")
    raw_profile = report.get("profile")
    if raw_profile not in (None, "protoss", PROFILE):
        raise ReportError(f"{label}.profile conflicts with P3 mission identity")
    _nonempty_string(report.get("trace_sha256"), f"{label}.trace_sha256")
    requested = _integer(report.get("requested_ticks"), f"{label}.requested_ticks", minimum=1)
    observed = _integer(report.get("observed_ticks"), f"{label}.observed_ticks", minimum=1)
    trace_stop = _integer(report.get("trace_stop_tick"), f"{label}.trace_stop_tick", minimum=1)
    if observed != requested:
        raise ReportError(f"{label}.observed_ticks must equal requested_ticks")
    is_prefix = report.get("prefix")
    if type(is_prefix) is not bool:
        raise ReportError(f"{label}.prefix must be boolean")
    expected_status = "prefix" if is_prefix else "won"
    if report.get("status") != expected_status:
        raise ReportError(f"{label}.status must be {expected_status!r} for its prefix flag")
    if is_prefix:
        if not allow_prefix:
            raise ReportError(f"{label} is a prefix; pass --allow-prefix to compare prefixes")
        if requested >= trace_stop:
            raise ReportError(f"{label} prefix must end before trace_stop_tick")
        if report.get("verified_win") is not False:
            raise ReportError(f"{label} prefix must not claim a verified win")
    elif report.get("verified_win") is not True:
        raise ReportError(f"{label} full report must have verified_win=true")
    elif requested != trace_stop:
        raise ReportError(f"{label} full report requested_ticks must equal trace_stop_tick")

    checkpoints = report.get("checkpoints")
    if not isinstance(checkpoints, list) or not checkpoints:
        raise ReportError(f"{label}.checkpoints must be a nonempty array")
    ticks: list[int] = []
    for index, checkpoint in enumerate(checkpoints):
        if not isinstance(checkpoint, dict):
            raise ReportError(f"{label}.checkpoints[{index}] must be an object")
        tick = _integer(checkpoint.get("tick"), f"{label}.checkpoints[{index}].tick")
        ticks.append(tick)
        _state_projection(checkpoint, f"{label}.checkpoints[{index}]")
        expected_identity = (("race_human", 2), ("race_mode", 0),
                             ("race_selected_mission", 2 if index == 0 else MISSION))
        if index == 0 and (checkpoint.get("tick") != 0 or checkpoint.get("frame") != 0
                           or checkpoint.get("screen") != 0 or checkpoint.get("won") != 0
                           or checkpoint.get("lost") != 0):
            raise ReportError(f"{label}.checkpoints[0] must be the fresh HOME state")
        for field, expected in expected_identity:
            if checkpoint.get(field) != expected:
                raise ReportError(f"{label}.checkpoints[{index}].{field} must be {expected}")
        if index > 0 and checkpoint["race_campaign"].get("mission") != MISSION:
            raise ReportError(f"{label}.checkpoints[{index}].race_campaign.mission must be {MISSION}")
    if ticks[0] != 0 or ticks[-1] != observed or any(a >= b for a, b in zip(ticks, ticks[1:])):
        raise ReportError(f"{label} checkpoint ticks must be unique, increasing, and span 0..observed_ticks")
    final = report.get("final")
    if final is None:
        raise ReportError(f"{label}.final is missing")
    if _state_projection(final, f"{label}.final") != _state_projection(checkpoints[-1], f"{label}.checkpoints[-1]"):
        raise ReportError(f"{label}.final does not match its last checkpoint gameplay state")
    if not is_prefix:
        final_campaign = final["race_campaign"]
        final_p3 = final["race_p3"]
        if not isinstance(final_campaign, dict) or not isinstance(final_p3, dict):
            raise ReportError(f"{label} final campaign/model state must be objects")
        if (final["screen"], final["won"], final["lost"]) != (3, 1, 0):
            raise ReportError(f"{label} full report final checkpoint is not a won result")
        completed = _integer(final["race_completed"], f"{label}.final.race_completed")
        objective_flags = _integer(final_campaign.get("objective_flags"),
                                   f"{label}.final.race_campaign.objective_flags")
        if not (completed & (1 << MISSION)):
            raise ReportError(f"{label} full report lacks the P3 completion bit")
        if (final_campaign.get("mission"), final_campaign.get("phase")) != (MISSION, 3):
            raise ReportError(f"{label} full report has invalid final campaign mission or phase")
        if not (objective_flags & 1) or final_p3.get("outcome") != 1:
            raise ReportError(f"{label} full report lacks the P3 victory objective or model outcome")
    return checkpoints, ticks


def _profile_identity(report: dict[str, Any]) -> str:
    # The original P3 replay writer labeled profile reports simply "protoss";
    # mission 22 disambiguates it as the P3 campaign profile.
    return PROFILE


def compare_reports(baseline: Any, candidate: Any, *, allow_prefix: bool = False,
                    max_diffs: int = 50) -> dict[str, Any]:
    """Validate and compare two P3 replay reports, returning bounded differences."""
    if type(max_diffs) is not int or not 1 <= max_diffs <= 1000:
        raise ReportError("max_diffs must be between 1 and 1000")
    left_checkpoints, left_ticks = _validate_report(baseline, "baseline", allow_prefix)
    right_checkpoints, right_ticks = _validate_report(candidate, "candidate", allow_prefix)
    if _profile_identity(baseline) != _profile_identity(candidate):
        raise ReportError("reports have different profile identities")
    for field in ("mission", "trace_sha256", "trace_stop_tick", "requested_ticks", "observed_ticks", "prefix", "status"):
        if baseline[field] != candidate[field]:
            raise ReportError(f"reports have different {field}")
    if left_ticks != right_ticks:
        raise ReportError("reports have different checkpoint tick sequences")

    differences: list[dict[str, Any]] = []
    for tick, left, right in zip(left_ticks, left_checkpoints, right_checkpoints):
        _diff(_state_projection(left, "baseline checkpoint"),
              _state_projection(right, "candidate checkpoint"), "", tick, differences, max_diffs)
        if len(differences) >= max_diffs:
            break
    return {
        "equal": not differences,
        "comparison_scope": "reported gameplay state",
        "claim": ("The reports match for all required and otherwise reported gameplay fields at every checkpoint."
                  if not differences else
                  "The reports differ in the reported gameplay fields listed below."),
        "full_ram_equality_claimed": False,
        "profile": PROFILE,
        "profile_identity_inferred": (baseline.get("profile") != PROFILE
                                       or candidate.get("profile") != PROFILE),
        "profile_identity_basis": "P3 global mission ID 22; legacy 'protoss' profile label is accepted",
        "mission": baseline["mission"],
        "trace_sha256": baseline["trace_sha256"],
        "trace_stop_tick": baseline["trace_stop_tick"],
        "status": baseline["status"],
        "requested_ticks": baseline["requested_ticks"],
        "observed_ticks": baseline["observed_ticks"],
        "checkpoint_count": len(left_ticks),
        "differences": differences,
        "differences_truncated": bool(differences and len(differences) == max_diffs),
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path, help="baseline campaign replay profile.json")
    parser.add_argument("candidate", type=Path, help="candidate campaign replay profile.json")
    parser.add_argument("--allow-prefix", action="store_true", help="allow matching non-winning prefix reports")
    parser.add_argument("--max-diffs", type=int, default=50, help="maximum differences to emit (1..1000)")
    args = parser.parse_args(argv)
    try:
        left = json.loads(args.baseline.read_text(encoding="utf-8"))
        right = json.loads(args.candidate.read_text(encoding="utf-8"))
        result = compare_reports(left, right, allow_prefix=args.allow_prefix, max_diffs=args.max_diffs)
    except (OSError, json.JSONDecodeError, ReportError) as exc:
        print(json.dumps({"equal": False, "error": str(exc),
                          "comparison_scope": "reported gameplay state",
                          "full_ram_equality_claimed": False}, indent=2), file=sys.stderr)
        return 2
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["equal"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
