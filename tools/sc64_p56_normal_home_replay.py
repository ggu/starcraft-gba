#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay a verified P5/P6 controller tape on a pinned normal P10 cartridge."""
import argparse
from collections import defaultdict
import json
from pathlib import Path
import resource
import shutil
import struct

from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import PhysicalGame, entity_rows
from sc64_z7_normal_replay import input_hashes, require_unchanged


def require(ok, message):
    if not ok:
        raise ValueError(message)


def plan(trace, pages, mission):
    require(trace.get("schema") == 1 and trace.get("mission") == mission, "wrong tape")
    keys, checks, briefs, modals = {}, {}, {}, {}
    held, last = 0, 0
    for row in trace["events"]:
        tick, kind = row["host_tick"], row["kind"]
        require(type(tick) is int and last <= tick <= 190000 and tick > 0, "invalid call")
        last = tick
        if kind == "input":
            value = row["held"]
            require(type(value) is int and 0 <= value <= 1023 and value != held
                    and value & 48 != 48 and value & 192 != 192 and tick - 1 not in keys
                    and row["pressed"] == value & ~held, "invalid input edge")
            keys[tick - 1] = held = value
        else:
            require(tick not in checks and kind in ("start", "periodic", "won"), "invalid checkpoint")
            checks[tick] = row
    stop = last
    require(len(checks) >= 3 and next(iter(checks.values()))["kind"] == "start"
            and checks[stop]["kind"] == "won"
            and not any(r["kind"] != "periodic" for r in list(checks.values())[1:-1])
            and checks[stop]["won"] == checks[stop]["outcome"] == 1
            and checks[stop]["lost"] == checks[stop]["fault"] == 0
            and checks[stop]["screen"] == 3 and keys.get(0) == 1 and keys.get(1) == 0,
            "tape does not establish HOME to victory")
    for line in pages.splitlines():
        cells = line.split()
        if len(cells) == 4 and cells[1] == "briefing":
            tick, page, count = map(int, (cells[0], cells[2], cells[3]))
            require(2 < tick < min(checks) and tick not in briefs, "invalid briefing call")
            briefs[tick] = (page, count)
        elif len(cells) == 8 and all(v.isdecimal() for v in cells):
            tick, frame, box, page, trigger, action, old, key = map(int, cells)
            require(min(checks) <= tick < stop - 1 and 0 <= frame <= checks[stop]["frame"]
                    and 0 <= box <= 255 and 0 <= page <= 255 and 0 <= trigger <= 255 and action < 64
                    and old <= 1023 and key == (8 if old & 1 else 1)
                    and keys.get(tick) == old | key and keys.get(tick + 1) == old
                    and not any(k in modals for k in range(tick - 1, tick + 2)),
                    f"invalid dialogue call {tick}")
            modals[tick] = (frame, box, page, trigger, action, old, key)
    require(len(briefs) == (12 if mission == 24 else 6)
            and list(briefs) == sorted(briefs)
            and list(briefs.values()) == [(i, len(briefs)) for i in range(len(briefs))]
            and modals, "incomplete authored pages")
    return stop, list(keys.items()), checks, briefs, modals


def run(args):
    names = ("trace", "pages", "rom", "elf", "runner")
    files = tuple(getattr(args, k) for k in names) + (Path(__file__),)
    hashes = input_hashes(files)
    for name, path in zip(names, files):
        require(hashes[str(path.resolve())] == getattr(args, name + "_sha256"), f"{name} hash mismatch")
    mission = args.mission
    stop, edges, checks, briefs, modals = plan(json.loads(args.trace.read_text()), args.pages.read_text(), mission)
    if args.plan:
        return dict(calls=stop, checks=len(checks), briefing=len(briefs), dialogue=len(modals))
    require(args.out and shutil.disk_usage(args.out.parent).free >= int(5.25 * 2**30), "need new output and 5.25 GiB")
    args.out.mkdir()
    resource.setrlimit(resource.RLIMIT_FSIZE, (1 << 20,) * 2)
    n = mission - 19
    runtime_name, state_name = f"Sc64P{n}RuntimeState", f"Sc64P{n}State"
    _, persist = dwarf_struct_layout(args.elf, f"Sc64P{n}PersistentState")
    _, runtime = dwarf_struct_layout(args.elf, runtime_name)
    _, model = dwarf_struct_layout(args.elf, state_name)
    _, campaign = dwarf_struct_layout(args.elf, "Sc64RaceCampaignState")
    g = PhysicalGame(args.runner, args.rom, args.elf, args.out / "guest")
    r = g.rsp
    observations = []
    briefing_seen, dialogue_seen = [], []
    timing_start = r.clock()
    def scalar(name):
        return int.from_bytes(r.read(g.sym[name], g.sizes[name]), "little")
    def field(raw, base, layout, name):
        offset, width = base + layout[name], layout.widths[name]
        require(0 <= offset <= len(raw) - width and width in (1, 2, 4), f"bad layout {name}")
        return int.from_bytes(raw[offset:offset + width], "little", signed=layout.signed[name])
    unit_fields = ("owner", "raw_type", "raw_owner", "x", "y", "hp", "order", "selected")
    def observe():
        data = r.read(g.sym[f"race_p{n}"], g.sizes[f"race_p{n}"])
        def read_model(k):
            return field(data, persist["runtime"] + runtime["model"], model, k)
        units = entity_rows(r.read(g.sym["u"], g.sizes["u"]), g.unit_stride, g.unit_layout, unit_fields)
        def find(raw):
            return next((u for u in units if u[1:3] == (0, raw)), None)
        hero = find(79)
        result = dict((k, scalar(k)) for k in ("frame", "screen", "won", "lost", "mode"))
        result.update(started=read_model("started"), outcome=read_model("outcome"),
                      fault=field(data, persist["runtime"], runtime, "fault"),
                      cursor=[scalar("cx"), scalar("cy")],
                      selected=sum(u[-1] != 0 for u in units))
        count, width, signed = model.arrays["action_cursor"]
        offset = persist["runtime"] + runtime["model"] + model["action_cursor"]
        result["action_cursors"] = [int.from_bytes(data[j:j+width], "little", signed=signed)
                                    for j in range(offset, offset+width*count, width)]
        if mission == 24:
            result.update(hero=[1, hero[4], hero[5], hero[1]] if hero else [0, -1, -1, -1],
                          alive=len(units), minerals=scalar("minerals"))
        else:
            for name, unit in (("zeratul", find(75)), ("tassadar", hero)):
                result[name] = [1, *unit[2:6], unit[1], *unit[6:8]] if unit else [0,-1,-1,-1,-1,-1,0,-1]
            for key, owner in (("side", 0), ("enemy3", 1), ("enemy6", 4)):
                result[key] = sum(u[1:3] == (owner, 65) for u in units)
        return result
    caps = {i for i in range(500, stop, 500)} | set(checks) | set(briefs)
    caps |= {i for t in modals for i in (t, t+1, t+2)}
    modal_edges = defaultdict(list)
    for tick, row in modals.items():
        for phase in range(3):
            modal_edges[tick + phase].append((phase, row))
    try:
        require(scalar("screen") == scalar("frame") == g.front_field("page") == 0, "not fresh HOME")
        for tick in sorted(caps):
            require(shutil.disk_usage(args.out).free >= int(4.15 * 2**30), "disk floor")
            r.play_to(tick, edges)
            count, page, armed, box = struct.unpack("<4H", r.read(0x06013fe0, 8))
            if tick in briefs:
                q = tuple(g.front_field(k) for k in ("briefing_page", "briefing_pages", "briefing_revealed", "mission", "page"))
                require(q == (*briefs[tick], 1, mission, 3), f"briefing at {tick}: {q}")
                briefing_seen.append(tick)
            if tick in modal_edges:
                for phase, (frame, want_box, want_page, _, _, old, key) in modal_edges[tick]:
                    require(scalar("frame") == frame and (not armed if phase == 2 else
                            count and (box, page, armed, r.held) == (want_box, want_page, key if phase else 0, old | key if phase else old)),
                            f"dialogue differs at {tick}, phase {phase}: {(count, page, armed, box)}")
                    if phase == 2:
                        dialogue_seen.append(tick - 2)
            else:
                require(not count and not armed, f"unexpected dialogue at {tick}")
            if tick in checks:
                actual = observe()
                diff = {k: (checks[tick][k], v) for k, v in actual.items() if checks[tick][k] != v}
                observations.append(dict(call=tick, state=actual, differences=diff))
                require(not diff, f"checkpoint {tick}: {diff}")
        raw = r.read(g.sym["race_campaign"], g.sizes["race_campaign"])
        actual = observations[-1]["state"]
        terminal = dict(screen=actual["screen"], won=actual["won"], lost=actual["lost"],
                        mission=scalar("race_selected_mission"), campaign_mission=field(raw, 0, campaign, "mission"),
                        phase=field(raw, 0, campaign, "phase"), completed=scalar("race_completed"),
                        unsupported=scalar("mission_unsupported_count"))
        require(terminal["screen"] == 3 and terminal["won"] == 1 and terminal["lost"] == 0
                and terminal["completed"] & (1 << mission) and terminal["mission"] == mission
                and terminal["campaign_mission"] == mission and terminal["unsupported"] == 0
                and terminal["phase"] == 3 and len(briefing_seen) == len(briefs)
                and len(dialogue_seen) == len(modals), f"incomplete terminal state: {terminal}")
        require_unchanged(files, hashes)
        cycles = r.clock() - timing_start
        result = dict(result="won", mission=mission, calls=stop, cycles=cycles, updates_per_second=stop*16777216/cycles,
                      observations=observations, terminal=terminal, briefing=briefing_seen, dialogue=dialogue_seen, sha256=hashes)
        (args.out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
        return dict(calls=stop, checks=len(observations), briefing=len(briefs), dialogue=len(modals), rate=result["updates_per_second"])
    except Exception as exc:
        (args.out / "divergence.json").write_text(json.dumps(dict(call=locals().get("tick"), error=str(exc),
            sha256=hashes, observations=observations, briefing=briefing_seen, dialogue=dialogue_seen), indent=2)+"\n")
        raise
    finally:
        g.close()


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--mission", type=int, choices=(24, 25), required=True)
    for name in ("trace", "pages", "rom", "elf", "runner"):
        cli.add_argument("--" + name, type=Path, required=True)
        cli.add_argument("--" + name + "-sha256", required=True)
    cli.add_argument("--out", type=Path)
    cli.add_argument("--plan", action="store_true")
    print(json.dumps(run(cli.parse_args()), indent=2))
