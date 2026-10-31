#!/usr/bin/env python3
"""Validate a complete schema-2 T1 input trace and emit a C replay tape."""
import argparse
import json
import pathlib


def validate(trace):
    if trace.get("schema") != 2 or trace.get("complete") is not True:
        raise ValueError("only explicitly complete schema-2 traces are accepted")
    total = trace.get("total_ticks")
    if type(total) is not int or not 0 < total <= 1000000:
        raise ValueError("invalid total tick count")
    events = trace.get("events")
    points = trace.get("checkpoints")
    end = trace.get("expected")
    if not isinstance(events, list) or not events or len(events) > 4096 or trace.get("event_count") != len(events):
        raise ValueError("invalid input events")
    last = -1
    for event in events:
        tick, held = event.get("tick"), event.get("held")
        if type(tick) is not int or type(held) is not int or not last < tick < total or not 0 <= held <= 1023:
            raise ValueError("invalid or out-of-order input event")
        last = tick
    if not isinstance(points, list) or len(points) != total // 1000 or len(points) > 256 or trace.get("checkpoint_count") != len(points):
        raise ValueError("missing checkpoints")
    fields = ("frame", "minerals", "marines", "scv", "zerg", "raynor_owner", "raynor_alive", "state_hash")
    for i, point in enumerate(points):
        if point.get("tick") != (i + 1) * 1000 or any(type(point.get(k)) is not int for k in fields):
            raise ValueError("invalid checkpoint")
    if not isinstance(end, dict) or any(type(end.get(k)) is not int for k in ("won", "lost", "marines", "scv", "minerals", "frame", "state_hash")):
        raise ValueError("invalid expected result")
    if end["won"] != 1 or end["lost"] != 0:
        raise ValueError("trace does not describe a win")
    return total, events, points, end


def export(trace):
    total, events, points, end = validate(trace)
    lines = ["/* Generated host-only replay tape. */", f"#define T1_TOTAL_TICKS {total}u", "static const T1Event t1_events[] = {"]
    lines += [f'    {{{e["tick"]}u, {e["held"]}u}},' for e in events]
    lines += ["};", "static const T1Checkpoint t1_checkpoints[] = {"]
    fields = ("tick", "frame", "minerals", "marines", "scv", "zerg", "raynor_owner", "raynor_alive", "state_hash")
    lines += ["    {" + ", ".join(str(p[k]) for k in fields) + "}," for p in points]
    lines += ["};", "static const T1Expected t1_expected = {" + ", ".join(str(end[k]) for k in ("won", "lost", "marines", "scv", "minerals", "frame", "state_hash")) + "};", ""]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    try:
        text = export(json.loads(args.trace.read_text()))
    except (ValueError, KeyError, TypeError, json.JSONDecodeError) as exc:
        parser.error(str(exc))
    args.output.write_text(text)


if __name__ == "__main__":
    main()
