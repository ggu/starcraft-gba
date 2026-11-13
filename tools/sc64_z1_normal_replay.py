#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay a retained Z1 HOME tape, reading briefing and authored messages normally."""

import argparse
import json
import shutil
import struct
import subprocess
from pathlib import Path

from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import BUILDING_FIELDS, UNIT_FIELDS, PhysicalGame
from sc64_z7_normal_replay import (
    input_hashes,
    require_unchanged,
    timing_delta,
    timing_snapshot,
)


def main():
    if not __debug__:
        raise RuntimeError("Replay validation requires Python assertions")

    ap = argparse.ArgumentParser(description=__doc__)
    for k in ("trace", "rom", "elf", "runner", "out"):
        ap.add_argument("--" + k, type=Path, required=True)
    args = ap.parse_args()
    files = (args.trace, args.rom, args.elf, args.runner, Path(__file__))
    pins = input_hashes(files)
    t = json.loads(args.trace.read_text())
    stop = t["total_ticks"]
    assert t["mission"] == "Z1" and type(stop) is int and 0 < stop <= 240000
    assert t["expected"]["won"] == 1 and t["expected"]["lost"] == 0
    trans = {}
    prior = 0
    for row in t["events"]:
        tick, held = row["tick"], row["held"]
        assert (
            type(tick) is int
            and type(held) is int
            and 0 <= tick < stop
            and 0 <= held <= 1023
        )
        assert held != prior and (not trans or tick > next(reversed(trans)))
        assert held & 48 != 48 and held & 192 != 192
        trans[tick] = held
        prior = held
    assert list(trans.items())[:8] == [
        (0, 1),
        (1, 0),
        (2, 1),
        (3, 0),
        (4, 1),
        (5, 0),
        (6, 8),
        (7, 0),
    ]
    checks = {q["tick"]: q for q in t["checkpoints"]}
    assert len(checks) == len(t["checkpoints"])
    assert list(checks) == list(range(1000, stop + 1, 1000))
    unit = {k: i + 1 for i, k in enumerate(UNIT_FIELDS)}
    building = {k: i + 1 for i, k in enumerate(BUILDING_FIELDS)}
    campaign = dwarf_struct_layout(args.elf, "Sc64RaceCampaignState")[1]
    front = dwarf_struct_layout(args.elf, "Sc64Frontend")[1]
    dwarf = subprocess.check_output(
        ["arm-none-eabi-readelf", "--debug-dump=info", str(args.elf)], text=True
    )
    dwarf_lines = dwarf.splitlines()

    def enum(name):
        matches = {
            int(row.split(":")[-1])
            for line, row in zip(dwarf_lines, dwarf_lines[1:])
            if "DW_AT_name" in line
            and line.split()[-1] == name
            and "DW_AT_const_value" in row
        }
        if len(matches) != 1:
            raise ValueError(name)
        return matches.pop()

    types = {
        k: enum(k)
        for k in (
            "U_DRONE",
            "U_HYDRALISK",
            "U_ZERGLING",
            "U_OVERLORD",
            "U_CHRYSALIS",
            "B_EXTRACTOR",
            "B_SPAWNING_POOL",
            "B_HYDRALISK_DEN",
        )
    }

    args.out.mkdir()
    g = PhysicalGame(args.runner, args.rom, args.elf, args.out / "guest")
    r = g.rsp

    def n(k, field=None):
        layout = front if k == "race_front" else campaign
        off = layout[field] if field else 0
        size = layout.widths[field] if field else g.sizes[k]
        assert size in (1, 2, 4) and size + off <= g.sizes[k]
        return int.from_bytes(r.read(g.sym[k] + off, size), "little")

    def modal():
        return struct.unpack("<4H", r.read(0x06013FE0, 8))

    def observe():
        s = g.state()
        us = s["units"]
        bs = s["buildings"]

        def count(rows, layout, owner, actor=None):
            return sum(
                q[layout["owner"]] == owner
                and (actor is None or q[layout["type"]] == types[actor])
                for q in rows
            )

        return dict(
            minerals=s["minerals"],
            gas=s["gas"],
            drones=count(us, unit, 0, "U_DRONE"),
            hydras=count(us, unit, 0, "U_HYDRALISK"),
            zerglings=count(us, unit, 0, "U_ZERGLING"),
            overlords=count(us, unit, 0, "U_OVERLORD"),
            extractors=count(bs, building, 0, "B_EXTRACTOR"),
            pools=count(bs, building, 0, "B_SPAWNING_POOL"),
            dens=count(bs, building, 0, "B_HYDRALISK_DEN"),
            enemy_buildings=count(bs, building, 1),
            enemy_units=count(us, unit, 1),
            won=n("won"),
            lost=n("lost"),
            phase=n("race_campaign", "phase"),
            chrysalis=sum(q[unit["type"]] == types["U_CHRYSALIS"] for q in us),
        )

    def step(key):
        if key != r.held:
            r.set_keys(key)
        r._advance(1)

    def tap(key):
        step(key)
        step(0)

    briefing, pages, matched = [], [], []
    try:
        assert n("screen") == n("frame") == g.front_field("page") == 0
        before = timing_snapshot(r, g.sym, g.sizes)
        # The older host controller skips its briefing. Read all pages on the
        # real cart, and replace its START call with the final A press/release.
        for tick in range(6):
            step(trans[tick])
        assert g.front_field("page") == 3 and g.front_field("mission") == 0
        count = g.front_field("briefing_pages")
        assert 1 <= count <= 64

        def revealed():
            return n("race_front", "briefing_revealed") or (
                n("race_front", "ticks") - n("race_front", "briefing_page_tick")
            ) * 2 >= n("race_front", "briefing_reveal_length")

        for i in range(count):
            assert g.front_field("briefing_page") == i
            if not revealed():
                tap(1)
            assert revealed() and g.front_field("briefing_page") == i
            briefing.append(dict(page=i, pages=count, call=r.completed))
            if i + 1 < count:
                tap(1)
        step(1)
        assert n("screen") == 1 and n("race_human") == 1 and n("race_mode") == 0
        assert n("race_selected_mission") == 0 and n("frame") == 0
        briefing_extra = r.completed - 7
        for tick in range(7, stop):
            if tick % 1000 == 0 and shutil.disk_usage(args.out).free < 4 << 30:
                raise RuntimeError("disk guard")
            step(trans.get(tick, r.held))
            for _ in range(48):
                count, page, armed, row = modal()
                if not count:
                    break
                frame, held = n("frame"), r.held
                assert 0 < count <= 13 and not armed
                key = 8 if held & 1 else 1
                step(held | key)
                assert n("frame") == frame and modal() == (count, page, key, row)
                step(held)
                next_count, next_page, next_armed, next_row = modal()
                assert n("frame") == frame and r.held == held and next_armed == 0
                assert not next_count or (next_page, next_row) != (page, row)
                pages.append(
                    dict(
                        tape_call=tick + 1,
                        call=r.completed,
                        frame=frame,
                        page=page,
                        row=row,
                    )
                )
                print("dialogue", pages[-1], flush=True)
            else:
                raise RuntimeError("authored text did not clear")
            call = tick + 1
            if call in checks or call == stop:
                s = observe()
                expected = [checks[call]] if call in checks else []
                if call == stop:
                    expected.append(t["expected"])
                for observation in expected:
                    if any(s[k] != v for k, v in observation.items() if k != "tick"):
                        (args.out / "divergence.json").write_text(
                            json.dumps(
                                dict(
                                    call=call,
                                    expected=observation,
                                    observed=s,
                                    pages=pages,
                                ),
                                indent=2,
                            )
                            + "\n"
                        )
                        raise ValueError((call, observation, s))
                matched.append(
                    dict(call=call, physical_call=r.completed, frame=n("frame"), **s)
                )
                if call % 5000 == 0 or call == stop:
                    print("matched", call, matched[-1]["frame"], flush=True)
        assert r.completed == stop + briefing_extra + 2 * len(pages)
        final = matched[-1]
        assert (
            n("screen") == 3
            and n("race_completed") & 1
            and n("mission_unsupported_count") == 0
        )
        assert (
            final["chrysalis"]
            and min(final[k] for k in ("hydras", "extractors", "pools", "dens")) > 0
        ), final
        assert n("race_campaign", "objective_flags") & 3 == 3
        require_unchanged(files, pins)
        proof = dict(
            status="won",
            tape_calls=stop,
            physical_calls=r.completed,
            briefing=briefing,
            pages=pages,
            checkpoints=matched,
            completion=n("race_completed"),
            faults=n("mission_unsupported_count"),
            objective_flags=n("race_campaign", "objective_flags"),
            sha256=pins,
            timing=timing_delta(
                before, timing_snapshot(r, g.sym, g.sizes), r.completed
            ),
        )
        (args.out / "result.json").write_text(json.dumps(proof, indent=2) + "\n")
        print("won", final, proof["timing"], flush=True)
    finally:
        g.close()


if __name__ == "__main__":
    main()
