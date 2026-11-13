#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only normal ROM replay of Z2 or P1/P2 HOME input and authored pages."""
import argparse
from collections import defaultdict
import json
from pathlib import Path
import resource
import shutil
import struct

from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import BUILDING_FIELDS, UNIT_FIELDS, PhysicalGame
from sc64_z7_normal_replay import input_hashes, require_unchanged, timing_delta, timing_snapshot


# Bound the authenticated, all-neutral HOST controller unwind after WIN.
MAX_TERMINAL_NEUTRAL_CALLS = 5


def terminal_modal_release(call, stop, keys, checks, modal, frame, final):
    """Recognize the final HOST neutral bookkeeping update after a modal WIN."""
    count, _, armed, _ = modal
    if (not 1 <= stop - call <= MAX_TERMINAL_NEUTRAL_CALLS
            or keys.get(call) != 0
            or any(keys.get(tick, 0) != 0 or checks.get(tick)
                   for tick in range(call + 1, stop + 1))
            or count or armed
            or frame != final["frame"] or final["won"] != 1 or final["lost"]):
        return False
    for kind, *row in checks.get(call - 2, ()):
        if kind != "modal":
            continue
        frozen, box, page, _, _, held, key = row
        if (held and keys.get(call - 2) == held | key
                and keys.get(call - 1) == held and frozen == frame
                and ("armed", frame, box, page, key) in checks.get(call - 1, ())
                and ("released", frame) in checks.get(call, ())):
            return True
    return False


def run(args):
    if not __debug__:
        raise RuntimeError("Replay checks require Python assertions")
    resource.setrlimit(resource.RLIMIT_FSIZE, (16 * 1024**2, 16 * 1024**2))
    files = [args.trace, args.rom, args.elf, args.runner, Path(__file__)]
    files.append(args.pages)
    pins = input_hashes(files)
    data = json.loads(args.trace.read_text())
    mission = args.mission
    human = 1 if mission == 1 else 2
    if args.pins:
        for path, expected in zip(files[:4] + [args.pages], args.pins):
            if pins[str(path.resolve())] != expected:
                raise ValueError(f"SHA-256 mismatch: {path}")
    for key, value in dict(event_overflow=0, mission=mission, race_selected_mission=mission,
                           race_human=human, race_mode=0, race_completed=1 << mission).items():
        assert data[key] == value
    final = data["expected"]
    outcome = dict(screen=3, won=1, lost=0, phase=3)
    if mission == 1:
        outcome.update(objective_flags=17, chrysalis_owner=0, chrysalis_alive=1)
    else:
        outcome.update(rescue_owner=0, rescue_alive=1)
    for key, value in outcome.items():
        assert final[key] == value
    stop = data["total_ticks"]
    assert type(stop) is int and 1000 < stop < 128000
    events = [(r["tick"], r["held"]) for r in data["events"]]
    opening = [1,0,1,0] if mission == 2 else [1,0,128,0,1,0]
    assert events[:len(opening)] == list(enumerate(opening))
    assert all(a[0] < b[0] and a[1] != b[1] for a,b in zip(events,events[1:]))
    assert all(type(i) is int and type(k) is int and 0 <= i < stop for i, k in events)
    keys = dict(events)
    checks = defaultdict(list)
    checkpoints = {r["tick"]: r for r in data["checkpoints"]}
    assert sorted(checkpoints) == list(range(1000, stop, 1000))
    briefing = []
    for line in args.pages.read_text().splitlines():
        fields = line.split()
        if not fields or not fields[0].isdigit():
            continue
        call = int(fields[0])
        assert 6 <= call < stop
        if fields[1] == "briefing":
            page, total = map(int, fields[2:])
            assert page == len(briefing) and 0 <= page < total <= 32
            briefing.append(total)
            checks[call].append(("briefing", page, total))
        else:
            frame, row, page, trigger, action, held, key = map(int, fields[1:])
            assert key == (8 if held & 1 else 1)
            assert keys[call] == held | key and keys[call+1] == held
            checks[call].append(("modal", frame, row, page, trigger, action, held, key))
            checks[call+1].append(("armed", frame, row, page, key))
            checks[call+2].append(("released", frame))
    assert 0 < len(briefing) <= 32 and all(n == len(briefing) for n in briefing)
    authored = [(check[2],check[3],check[4],check[5]) for call in sorted(checks)
                for check in checks[call] if check[0] == "modal"]
    if mission == 1:
        assert authored == [(1,0,2,0), (1,1,2,0), (0,0,1,0)]
    if args.plan:
        print(json.dumps(dict(mission=mission,calls=stop,edges=len(events),checkpoints=len(checkpoints)+1,
                              briefings=len(briefing),authored=authored,sha256=pins),indent=2))
        return
    stop_at=args.stop or stop
    assert 1 <= stop_at <= stop
    if args.out is None:
        raise ValueError("--out is required to replay")
    if shutil.disk_usage(args.out.parent).free < int(5.25 * 1024**3):
        raise RuntimeError("launch requires 5.25 GiB free")
    campaign = dwarf_struct_layout(args.elf, "Sc64RaceCampaignState")[1]
    u = {k: i + 1 for i, k in enumerate(UNIT_FIELDS)}
    b = {k: i + 1 for i, k in enumerate(BUILDING_FIELDS)}
    assert shutil.disk_usage(args.out.parent).free >= 4 * 1024**3
    args.out.mkdir()
    guest = args.out / "guest"
    guest.mkdir()
    (guest / "headless.log").symlink_to("/dev/null")
    g = PhysicalGame(args.runner, args.rom, args.elf, guest)
    r = g.rsp

    def n(k):
        return int.from_bytes(r.read(g.sym[k], g.sizes[k]), "little")

    def observe():
        raw = r.read(g.sym["race_campaign"], g.sizes["race_campaign"])
        def c(k):
            start = campaign[k]
            return int.from_bytes(raw[start:start+campaign.widths[k]], "little")
        s = g.state()
        units, buildings = s["units"], s["buildings"]
        slots = {unit[0]: unit for unit in units}
        # The old controller reads source slots even after an actor dies.
        def unit(slot, field, fallback):
            if slot >= g.sizes["u"] // g.unit_stride:
                return fallback
            if slot in slots:
                return 1 if field == "alive" else slots[slot][u[field]]
            offset, width = g.unit_layout[field], g.unit_layout.widths[field]
            return int.from_bytes(r.read(g.sym["u"] + slot*g.unit_stride + offset, width),
                                  "little", signed=g.unit_layout.signed.get(field, False))
        rescue, chrys, carrier = (c(k) for k in ("rescue_unit", "chrysalis_unit", "carrier_unit"))
        return dict(mission=c("mission"), race_selected_mission=n("race_selected_mission"),
                    race_human=n("race_human"), race_mode=n("race_mode"),
                    race_completed=n("race_completed"), screen=s["screen"], frame=s["frame"],
                    won=n("won"), lost=n("lost"), elapsed_frames=c("elapsed_frames"),
                    objective_flags=c("objective_flags"), phase=c("phase"),
                    rescue_owner=unit(rescue,"owner",-1), rescue_alive=unit(rescue,"alive",0),
                    chrysalis_owner=unit(chrys,"owner",-1), chrysalis_alive=unit(chrys,"alive",0),
                    chrysalis_x=unit(chrys,"x",-1), chrysalis_y=unit(chrys,"y",-1),
                    carrier_unit=carrier, carrier_x=unit(carrier,"x",-1), carrier_y=unit(carrier,"y",-1),
                    side_units=sum(x[u["owner"]]==0 for x in units),
                    enemy_units=sum(x[u["owner"]]==1 for x in units),
                    side_buildings=sum(x[b["owner"]]==0 for x in buildings),
                    enemy_buildings=sum(x[b["owner"]]==1 for x in buildings))

    try:
        assert n("screen") == n("frame") == g.front_field("page") == 0
        start = timing_snapshot(r, g.sym, g.sizes)
        pages, reveals, observations = [], [], []
        boundaries = set(checks) | set(checkpoints) | {stop_at}
        if mission != 1:
            # The runner cannot stop a batch when RESULT or a modal opens.
            # Never let a later tape edge restart a failed attempt.
            boundaries.update(range(1, stop_at + 1))
        terminal_release = None
        terminal_snapshot = None
        for call in sorted(n for n in boundaries if n <= stop_at):
            if call % 500 == 0 or call == stop_at:
                if shutil.disk_usage(args.out).free < int(4.15 * 1024**3):
                    (args.out/"partial.json").write_text(json.dumps(dict(call=call, observations=observations,
                        briefing=reveals, pages=pages, sha256=pins), indent=2))
                    raise RuntimeError("runtime disk floor reached")
            if mission == 1:
                r.play_to(call, events)
            else:
                if call-1 in keys:
                    r.set_keys(keys[call-1])
                r._advance(1)
            for check in checks.get(call, ()):
                kind, *row = check
                count, page, armed, current = struct.unpack("<4H", r.read(0x06013fe0, 8))
                if kind == "briefing":
                    assert (g.front_field("page"),g.front_field("mission"),
                            g.front_field("briefing_revealed"),n("screen")) == (3,mission,1,0)
                    assert [g.front_field("briefing_page"),g.front_field("briefing_pages")] == row
                    reveals.append([call,*row])
                elif kind == "modal":
                    frame, expected_row, expected_page, trigger, action, held, key = row
                    assert 0 < count <= 13 and (page,armed,current) == (expected_page,0,expected_row)
                    assert n("frame") == frame and r.held == held
                    pages.append([call,*row])
                elif kind == "armed":
                    frame, expected_row, expected_page, key = row
                    assert count and (page,armed,current,n("frame")) == (expected_page,key,expected_row,frame)
                else:
                    assert kind == "released" and not armed and n("frame") == row[0]
            if terminal_snapshot is not None:
                actual = observe()
                count, _, armed, _ = struct.unpack("<4H", r.read(0x06013fe0, 8))
                if actual != terminal_snapshot or r.held or count or armed:
                    (args.out/"divergence.json").write_text(json.dumps(
                        dict(call=call, terminal_before=terminal_snapshot, terminal_after=actual,
                             held=r.held, modal_count=count, modal_armed=armed), indent=2))
                    raise RuntimeError(f"terminal state changed during neutral tail at call {call}")
            elif mission != 1 and call < stop and n("screen") == 3:
                actual = observe()
                active = struct.unpack("<4H", r.read(0x06013fe0, 8))
                differences = {k: [v, actual[k]] for k, v in final.items() if actual[k] != v}
                if differences or not terminal_modal_release(call, stop, keys, checks, active,
                                                              actual["frame"], final):
                    (args.out/"divergence.json").write_text(json.dumps(
                        dict(call=call, unexpected_result=actual, differences=differences), indent=2))
                    raise RuntimeError(f"unexpected RESULT at call {call}")
                terminal_release = call
                terminal_snapshot = actual
            if call in checkpoints or call == stop:
                assert not int.from_bytes(r.read(0x06013fe0, 2), "little"), (call, "unacknowledged modal")
                expected = final if call == stop else checkpoints[call]
                actual = observe() | {"tick":call}
                mismatches = {k:[expected[k],actual[k]] for k in expected if expected[k] != actual[k]}
                if mismatches:
                    (args.out/"divergence.json").write_text(json.dumps(dict(call=call, differences=mismatches),indent=2))
                    raise RuntimeError(f"Z2 diverged at call {call}: {mismatches}")
                observations.append({k:actual[k] for k in expected})
                print(f"Matched mission {mission} call {call}, frame {actual['frame']}",flush=True)
        require_unchanged(files, pins)
        result = dict(status="won" if stop_at == stop else "prefix", calls=stop_at, observations=observations, briefing=reveals,
                      pages=pages, terminal_modal_release=terminal_release, sha256=pins,
                      timing=timing_delta(start,timing_snapshot(r,g.sym,g.sizes),stop_at))
        (args.out/"result.json").write_text(json.dumps(result,indent=2)+"\n")
    finally:
        g.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("trace","rom","elf","runner"):
        parser.add_argument("--"+name, type=Path, required=True)
    parser.add_argument("--pages", type=Path, required=True, help="authored HOST briefing and modal witnesses")
    parser.add_argument("--out", type=Path)
    parser.add_argument("--mission", type=int, choices=(1,2,3), default=1, help="global mission ID")
    parser.add_argument("--pins", nargs=5, metavar="SHA256", help="trace ROM ELF runner pages, in order")
    parser.add_argument("--plan", action="store_true")
    parser.add_argument("--stop", type=int)
    args=parser.parse_args()
    if args.mission != 1 and not args.pins:
        parser.error("P1/P2 require exact input SHA-256 pins")
    run(args)
