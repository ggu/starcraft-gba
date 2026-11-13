#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only normal Z10 replay of the Z2 HOME controller and its authored pages."""
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


def run(args):
    if not __debug__:
        raise RuntimeError("Replay checks require Python assertions")
    resource.setrlimit(resource.RLIMIT_FSIZE, (16 * 1024**2, 16 * 1024**2))
    files = [args.trace, args.rom, args.elf, args.runner, Path(__file__)]
    files.append(args.pages)
    pins = input_hashes(files)
    data = json.loads(args.trace.read_text())
    for key, value in dict(event_overflow=0, mission=1, race_selected_mission=1,
                           race_human=1, race_mode=0, race_completed=2).items():
        assert data[key] == value
    final = data["expected"]
    for key, value in dict(screen=3, won=1, lost=0, phase=3, objective_flags=17,
                           chrysalis_owner=0, chrysalis_alive=1).items():
        assert final[key] == value
    stop = data["total_ticks"]
    assert type(stop) is int and 1000 < stop < 128000
    events = [(r["tick"], r["held"]) for r in data["events"]]
    assert events[:6] == [(0, 1), (1, 0), (2, 128), (3, 0), (4, 1), (5, 0)]
    assert all(type(i) is int and type(k) is int and 0 <= i < stop for i, k in events)
    keys = dict(events)
    checks = defaultdict(list)
    checkpoints = {r["tick"]: r for r in data["checkpoints"]}
    assert sorted(checkpoints) == list(range(1000, stop, 1000))
    briefing = []
    for line in args.pages.read_text().splitlines():
        fields = line.split()
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
    assert len(briefing) == 6 and all(n == len(briefing) for n in briefing)
    authored = [(check[2],check[3],check[4],check[5]) for call in sorted(checks)
                for check in checks[call] if check[0] == "modal"]
    assert authored == [(1,0,2,0), (1,1,2,0), (0,0,1,0)]
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
        for call in sorted(set(checks) | set(checkpoints) | {stop}):
            assert shutil.disk_usage(args.out).free >= 4 * 1024**3
            r.play_to(call, events)
            for check in checks[call]:
                kind, *row = check
                count, page, armed, current = struct.unpack("<4H", r.read(0x06013fe0, 8))
                if kind == "briefing":
                    assert (g.front_field("page"),g.front_field("mission"),
                            g.front_field("briefing_revealed"),n("screen")) == (3,1,1,0)
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
            if call in checkpoints or call == stop:
                assert not int.from_bytes(r.read(0x06013fe0, 2), "little"), (call, "unacknowledged modal")
                expected = final if call == stop else checkpoints[call]
                actual = observe() | {"tick":call}
                mismatches = {k:[expected[k],actual[k]] for k in expected if expected[k] != actual[k]}
                if mismatches:
                    (args.out/"divergence.json").write_text(json.dumps(dict(call=call, differences=mismatches),indent=2))
                    raise RuntimeError(f"Z2 diverged at call {call}: {mismatches}")
                observations.append({k:actual[k] for k in expected})
                print(f"Matched Z2 call {call}, frame {actual['frame']}",flush=True)
        require_unchanged(files, pins)
        result = dict(status="won", calls=stop, observations=observations, briefing=reveals,
                      pages=pages, sha256=pins,
                      timing=timing_delta(start,timing_snapshot(r,g.sym,g.sizes),stop))
        (args.out/"result.json").write_text(json.dumps(result,indent=2)+"\n")
    finally:
        g.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("trace","rom","elf","runner","out"):
        parser.add_argument("--"+name, type=Path, required=True)
    parser.add_argument("--pages", type=Path, required=True, help="authored HOST briefing and modal witnesses")
    run(parser.parse_args())
