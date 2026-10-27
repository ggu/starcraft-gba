#!/usr/bin/env python3
"""Validate an offline P7 normal-input controller JSONL trace.

This checks the trace's JSON shape, held/pressed edge contract, frame/call
ordering, and (optionally) its recorded source-win fields. It does not run or
authenticate an emulator, ROM, or game process.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


class TraceValidationError(ValueError):
    """Raised when a P7 controller trace violates the offline contract."""


def _unique_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise TraceValidationError(f"duplicate JSON key {key!r}")
        result[key] = value
    return result


def parse_jsonl(text: str) -> list[dict[str, Any]]:
    """Parse JSONL while rejecting duplicate keys and non-object rows."""
    rows: list[dict[str, Any]] = []
    for line_number, line in enumerate(text.splitlines(), 1):
        if not line.strip():
            continue
        try:
            row = json.loads(line, object_pairs_hook=_unique_object)
        except (json.JSONDecodeError, TraceValidationError) as exc:
            raise TraceValidationError(f"line {line_number}: {exc}") from exc
        if not isinstance(row, dict):
            raise TraceValidationError(f"line {line_number}: row must be an object")
        rows.append(row)
    return rows


def _int(row: dict[str, Any], key: str, *, minimum: int = 0) -> int:
    value = row.get(key)
    if type(value) is not int or value < minimum:
        raise TraceValidationError(f"{row.get('kind', 'row')}.{key} must be an integer >= {minimum}")
    return value


def validate_trace(rows: list[dict[str, Any]], *, require_win: bool = False) -> dict[str, Any]:
    """Validate event structure and held-input edges; return a compact summary."""
    if not isinstance(rows, list) or not rows:
        raise TraceValidationError("trace must contain at least one JSON object")
    if any(not isinstance(row, dict) for row in rows):
        raise TraceValidationError("every trace row must be an object")

    metadata = rows[0]
    if metadata.get("kind") != "metadata":
        raise TraceValidationError("trace must begin with metadata")
    if sum(row.get("kind") == "metadata" for row in rows) != 1:
        raise TraceValidationError("trace must contain exactly one metadata row")
    if type(metadata.get("schema")) is not int or metadata["schema"] != 1:
        raise TraceValidationError("metadata schema must be integer 1")
    if type(metadata.get("mission")) is not int or metadata["mission"] != 26:
        raise TraceValidationError("metadata mission must be integer 26")
    if metadata.get("input_policy") != "ordinary game_update held/pressed only":
        raise TraceValidationError("metadata must identify ordinary game_update held/pressed input")

    finals = [row for row in rows if row.get("kind") == "final"]
    verdicts = [row for row in rows if row.get("kind") == "strict-verdict"]
    if len(finals) != 1 or len(verdicts) != 1:
        raise TraceValidationError("trace must contain exactly one final census and one strict verdict")
    final, verdict = finals[0], verdicts[0]
    if rows[-1] is not verdict:
        raise TraceValidationError("strict-verdict must be the final JSONL row")
    if rows.index(final) >= rows.index(verdict):
        raise TraceValidationError("final census must precede strict-verdict")

    inputs = [row for row in rows if row.get("kind") == "input"]
    if not inputs:
        raise TraceValidationError("trace must contain input transitions")
    previous_held = 0
    previous_call = -1
    previous_frame_after = 0
    for index, row in enumerate(inputs):
        call = _int(row, "call_index")
        held = _int(row, "held")
        pressed = _int(row, "pressed")
        frame_before = _int(row, "frame_before")
        frame_after = _int(row, "frame_after")
        if call <= previous_call:
            raise TraceValidationError(f"input {index}: call_index must be strictly increasing")
        if held > 0x3FF or held == previous_held:
            raise TraceValidationError(f"input {index}: held must be a changed mask in 0..0x3ff")
        if pressed != (held & ~previous_held):
            raise TraceValidationError(f"input {index}: pressed does not match the held transition")
        if frame_before < previous_frame_after:
            raise TraceValidationError(f"input {index}: frames move backwards between transitions")
        if index == 0:
            if frame_before > call:
                raise TraceValidationError("first input frame_before cannot exceed call_index")
        elif frame_before - previous_frame_after > call - previous_call - 1:
            raise TraceValidationError(
                f"input {index}: frame advance between input calls is impossible")
        delta = frame_after - frame_before
        if delta not in (0, 1):
            raise TraceValidationError(f"input {index}: frame delta must be zero or one")
        previous_held = held
        previous_call = call
        previous_frame_after = frame_after

    total_calls = _int(final, "total_calls", minimum=1)
    final_frame = _int(final, "frame")
    if previous_call >= total_calls:
        raise TraceValidationError("every input call_index must be less than final.total_calls")
    if final_frame < previous_frame_after or final_frame > total_calls:
        raise TraceValidationError("final.frame is incoherent with input frames and total_calls")
    if final_frame - previous_frame_after > total_calls - previous_call - 1:
        raise TraceValidationError("final.frame advances too far after the last input call")
    if any(row.get("kind") == "input" for row in rows[rows.index(final) + 1:]):
        raise TraceValidationError("input rows cannot follow the final census")

    result: dict[str, Any] = {
        "input_transitions": len(inputs),
        "last_input_call_index": previous_call,
        "total_calls": total_calls,
        "final_frame": final_frame,
        "edge_errors": 0,
        "strict_ok": verdict.get("ok"),
    }
    if require_win:
        required_ints = {
            "ok": 1,
            "screen_result": 1,
            "won": 1,
            "lost": 0,
            "outcome": 1,
            "fault": 0,
            "defeat_started": 0,
            "victory_started": 1,
            "victory_cursor": 0,
            "action_timers_clear": 1,
            "heroes_alive": 3,
        }
        for key, expected in required_ints.items():
            if type(verdict.get(key)) is not int or verdict[key] != expected:
                raise TraceValidationError(f"strict-verdict.{key} must be integer {expected}")
        switch_bits = _int(verdict, "switch_bits")
        if not (switch_bits & 1):
            raise TraceValidationError("strict-verdict.switch_bits must include the victory gate")
        force2 = _int(verdict, "force2_nexus_condition")
        force3 = _int(verdict, "force3_nexus_condition")
        if force2 not in (0, 1) or force3 not in (0, 1) or not (force2 or force3):
            raise TraceValidationError("a recorded source Nexus victory condition must be true")
        if (_int(final, "screen") != 3 or _int(final, "won") != 1 or
                _int(final, "lost") != 0 or _int(final, "heroes_alive") != 3):
            raise TraceValidationError("final census does not record the required source win state")
        if inputs[-1]["held"] != 0:
            raise TraceValidationError("winning input tape must end with neutral held input")
    return result


def validate_file(path: str | Path, *, require_win: bool = False) -> dict[str, Any]:
    """Read and validate one JSONL trace file."""
    raw = Path(path).read_bytes()
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise TraceValidationError("trace must be UTF-8 JSONL") from exc
    result = validate_trace(parse_jsonl(text), require_win=require_win)
    result["sha256"] = hashlib.sha256(raw).hexdigest()
    return result


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path, help="JSONL controller trace")
    parser.add_argument("--require-win", action="store_true",
                        help="require the recorded source-win fields (offline contract only)")
    args = parser.parse_args(argv)
    try:
        result = validate_file(args.trace, require_win=args.require_win)
    except (OSError, TraceValidationError) as exc:
        parser.error(str(exc))
    print(json.dumps({"trace": str(args.trace), **result}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
