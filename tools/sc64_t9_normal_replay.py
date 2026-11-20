#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check T9 or T10 on a normal Terran ROM using physical buttons."""
import argparse
import hashlib
import json
from pathlib import Path
import resource
import shutil
import struct

from sc64_carrier_release_physical import slot
from sc64_p10_tape_save import join_timing
from sc64_p8_physical_save import SLOT_SIZE
from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import (
    KEY_A, KEY_B, KEY_DOWN, KEY_L, KEY_RIGHT, KEY_START, PhysicalGame,
)
from sc64_t9_arm_replay import COMPARED, load_trace
from sc64_terran_controller_replay import _find_raw_actor, _snapshot, _verify_terminal, validate_trace
from sc64_z7_normal_replay import input_hashes, require_unchanged, timing_delta, timing_snapshot

INNER_TAG, OUTER_TAG = 0x72435442, 0x76435442
T9_DIALOGUE = {1: (1, 4), 47969: (47251, 9)}
T10_FIELDS = ("frame", "screen", "won", "lost", "mission", "faults", "phase",
              "objective_flags", "race_completed", "hostile_buildings",
              "ion_cannon_slot", "ion_cannon_alive", "hero_slot", "hero_raw",
              "hero_alive", "minerals", "gas")


def load_t10_trace(path):
    reference = validate_trace(json.loads(path.read_text()), 256)
    if reference["mission"] != 13:
        raise ValueError("T10 requires a Hammer Falls controller trace")
    launch = reference["launch_tick"]
    stop = reference["stop_tick"] - launch
    rows = (reference["start"], *reference["checkpoints"], reference["final"])
    observations = {}
    for row in rows:
        tick = row["source_host_tick"]
        if any(type(row.get(name)) is not int for name in T10_FIELDS):
            raise ValueError(f"T10 observation at {tick} is missing a required integer field")
        previous = observations.get(tick)
        if previous and any(previous[name] != row[name] for name in T10_FIELDS):
            raise ValueError(f"T10 observations disagree at {tick}")
        observations[tick] = row
    expected_ticks = {0, 120, 1000, 2000, 3000, 3446}
    if not expected_ticks <= observations.keys() or stop != 3446:
        raise ValueError("T10 trace is missing a checkpoint from the qualified route")
    transitions = [(tick - launch + 1, held) for tick, held in reference["inputs"] if tick >= launch]
    if transitions and transitions[0][0] == 1:
        raise ValueError("T10 trace must start with neutral input")
    return dict(transitions=transitions, observations=observations, final=reference["final"], stop=stop)


def main():
    if not __debug__:
        raise RuntimeError("Replay validation requires Python assertions")
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("trace", "rom", "elf", "runner", "out"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--chapter", type=int, choices=(9, 10), default=9)
    parser.add_argument("--save-tick", type=int, required=True,
                        help="neutral completed gameplay tick for pause, save and fresh HOME load")
    args = parser.parse_args()
    resource.setrlimit(resource.RLIMIT_FSIZE, (16 << 20, 16 << 20))
    files = (args.trace, args.rom, args.elf, args.runner, Path(__file__))
    pins = input_hashes(files)
    t10 = args.chapter == 10
    mission = args.chapter + 3
    compared = T10_FIELDS if t10 else COMPARED
    dialogue_cuts = {1: (1, 4)} if t10 else T9_DIALOGUE
    briefing_pages = 14 if t10 else 9
    plan = load_t10_trace(args.trace) if t10 else load_trace(args.trace)
    plan["mission"] = mission
    stop, cut = plan["stop"], args.save_tick
    assert plan["final"]["kind"] == "won" and stop == (3446 if t10 else 47984)
    transitions = [(0, 0)] + [(tick - 1, held) for tick, held in plan["transitions"]]
    if not 1 < cut < stop or cut in dialogue_cuts or next(
            held for index, held in reversed(transitions) if index < cut) != 0:
        parser.error("--save-tick must be a neutral live gameplay tick")
    # This observer keeps only three 32 KiB SRAM images and small JSON
    # receipts. Retain the existing long-run disk guard for T9.
    disk_reserve = 256 << 20 if t10 else 4 << 30
    if shutil.disk_usage(args.out.parent).free < disk_reserve:
        raise RuntimeError(f"replay requires {disk_reserve >> 20} MiB free")
    _, save_layout = dwarf_struct_layout(args.elf, "RaceSave")
    args.out.mkdir()
    pause_path, victory_path = (args.out / name for name in ("pause.sav", "victory.sav"))

    def open_game(name, **save):
        directory = args.out / name
        directory.mkdir()
        (directory / "headless.log").symlink_to("/dev/null")
        return PhysicalGame(args.runner, args.rom, args.elf, directory, **save)

    g = open_game("guest", save_output=pause_path)

    def n(name):
        assert 0 < g.sizes[name] <= 4
        return int.from_bytes(g.rsp.read(g.sym[name], g.sizes[name]), "little",
                              signed=name == "selected_building")

    def modal():
        return struct.unpack("<4H", g.rsp.read(0x06013fe0, 8))

    def selected_units():
        layout = g.unit_layout
        width = layout.widths["selected"]
        units = g.rsp.read(g.sym["u"], g.sizes["u"])
        return sum(
            int.from_bytes(units[i:i + width], "little") != 0
            for i in range(layout["selected"], len(units), g.unit_stride))

    def observe():
        state = _snapshot(g.rsp, g.sym, g.sizes, args.elf, mission)
        if t10:
            hero = _find_raw_actor(state["units"], 19)
            cannon = _find_raw_actor(state["buildings"], 127)
            state["hero_slot"] = hero["slot"] if hero else -1
            state["hero_raw"] = hero["raw_type"] if hero else -1
            state["ion_cannon_slot"] = cannon["slot"] if cannon else -1
        else:
            state["selected_building"] = n("selected_building")
            state["selection"] = selected_units()
        return state

    def persisted():
        world = g.state()
        world.pop("screen")
        return dict(world=world, mission=g.rsp.read(g.sym["race_late"], g.sizes["race_late"]),
                    controls={name: n(name) for name in
                              ("cx", "cy", "menu_last")})

    def journal(image, bank, frame, won):
        payload = slot(image, bank, OUTER_TAG, bank + 1)
        assert len(payload) >= save_layout["state"] + 80
        assert struct.unpack_from("<I", payload, save_layout["version"])[0] == INNER_TAG
        words = struct.unpack_from("<20I", payload, save_layout["state"])
        assert (words[0], words[1], words[2], words[7]) == (0, 0, mission, frame)
        assert (words[3], words[18], words[19]) == (won << mission, won, 0)
        return dict(sequence=bank + 1, frame=frame, payload_bytes=len(payload),
                    tag=f"{OUTER_TAG:#010x}", sha256=hashlib.sha256(image).hexdigest())

    def compare(tick):
        expected = plan["observations"][tick]
        observed = observe()
        diff = {k: (expected[k], observed[k]) for k in compared if expected[k] != observed[k]}
        if diff:
            (args.out / "divergence.json").write_text(json.dumps(
                dict(tick=tick, expected=expected, differences=diff), indent=2) + "\n")
            raise ValueError(f"checkpoint {tick} diverged: {diff}")
        checkpoints.append(dict(tick=tick, **{k: observed[k] for k in compared}))
        print("Matched", tick, expected["kind"], observed["frame"], flush=True)
        return observed

    def reveal_briefing():
        g.tap(KEY_A)
        for _ in range(4):
            g.tap(KEY_RIGHT)
        if t10:
            g.tap(KEY_DOWN)
        g.tap(KEY_A)
        assert (n("screen"), g.front_field("page"), g.front_field("mission")) == (0, 3, mission)
        for page in range(briefing_pages):
            if page:
                g.tap(KEY_A)
            if not g.front_field("briefing_revealed"):
                g.tap(KEY_A)
            assert (g.front_field("briefing_page"), g.front_field("briefing_pages"),
                    g.front_field("briefing_revealed")) == (page, briefing_pages, 1)
            briefing.append(page)
        g.step(KEY_START)
        assert (n("screen"), n("frame"), n("race_selected_mission")) == (1, 0, mission)

    def acknowledge(tick):
        frame, limit = dialogue_cuts[tick]
        for _ in range(limit):
            count, page, armed, row = modal()
            assert 0 < count <= 13 and not armed and row < 256 and n("frame") == frame
            held = g.rsp.held
            key = KEY_START if held & KEY_A else KEY_A
            g.step(held | key)
            assert modal() == (count, page, key, row) and n("frame") == frame
            g.step(held)
            assert n("frame") == frame
            dialogue.append(dict(tick=tick, frame=frame, row=row, page=page))
        assert modal()[0] == 0

    briefing, dialogue, checkpoints = [], [], []
    try:
        assert n("screen") == n("frame") == g.front_field("page") == 0
        before = timing_snapshot(g.rsp, g.sym, g.sizes)
        reveal_briefing()
        compare(0)
        offset = g.rsp.completed
        edges = [(t + offset, mask) for t, mask in transitions]
        for tick in sorted(set(plan["observations"]) | set(dialogue_cuts) | {cut}):
            if not tick:
                continue
            if shutil.disk_usage(args.out).free < disk_reserve:
                raise RuntimeError(f"replay requires {disk_reserve >> 20} MiB free")
            g.rsp.play_to(tick + offset, edges)
            if tick in dialogue_cuts:
                acknowledge(tick)
                offset = g.rsp.completed - tick
                edges = [(t + offset, mask) for t, mask in transitions if t >= tick]
            else:
                assert modal()[0] == 0
            if tick in plan["observations"]:
                observed = compare(tick)
            if tick == cut:
                assert n("screen") == 1 and n("mode") == n("menu_open") == g.rsp.held == 0
                assert n("selected_building") == -1 and selected_units() == 0
                saved = persisted()
                old_camera = [n("camx"), n("camy")]
                g.step(KEY_START)
                g.step(0)
                g.step(KEY_B)
                g.step(0)
                assert n("screen") == 2 and n("pause_notice") & 0xc0 == 0x40
                assert persisted() == saved and n("selected_building") == -1 and selected_units() == 0
                g.rsp.export_sram()
                pause_image = pause_path.read_bytes()
                pause = journal(pause_image, 0, n("frame"), 0)
                assert pause_image[SLOT_SIZE:] == b"\xff" * SLOT_SIZE
                (args.out / "pause.json").write_text(json.dumps(pause, indent=2) + "\n")
                prefix = timing_delta(before, timing_snapshot(g.rsp, g.sym, g.sizes), g.rsp.completed)
                g.close()
                g = None
                g = open_game("fresh", save_input=pause_path, save_output=victory_path)
                before = timing_snapshot(g.rsp, g.sym, g.sizes)
                assert n("screen") == n("frame") == g.front_field("page") == 0
                g.step(KEY_L)
                assert n("screen") == 1 and n("race_selected_mission") == mission
                assert n("selected_building") == -1 and selected_units() == 0
                loaded = persisted()
                assert loaded == saved, ("pause reload differs",
                                         [k for k in loaded if loaded[k] != saved[k]],
                                         loaded["controls"], saved["controls"])
                pause["camera_before"] = old_camera
                pause["camera_after"] = [n("camx"), n("camy")]
                (args.out / "pause.json").write_text(json.dumps(pause, indent=2) + "\n")
                g.rsp.set_keys(0)
                offset = g.rsp.completed - tick
                edges = [(t + offset, mask) for t, mask in transitions if t >= tick]
                print("Fresh load", tick, n("frame"), flush=True)
        _verify_terminal(plan, observed)
        assert len(briefing) == briefing_pages
        assert len(dialogue) == sum(count for _, count in dialogue_cuts.values())
        assert len(checkpoints) == len(plan["observations"])
        timing = join_timing(prefix, timing_delta(before, timing_snapshot(g.rsp, g.sym, g.sizes), g.rsp.completed))
        final_state = persisted()
        victory_selection = n("selected_building")
        g.rsp.export_sram()
        victory_image = victory_path.read_bytes()
        assert pause_path.read_bytes() == pause_image
        assert victory_image[:SLOT_SIZE] == pause_image[:SLOT_SIZE]
        victory = journal(victory_image, 1, n("frame"), 1)
        g.close()
        g = None
        readback = args.out / "victory-readback.sav"
        g = open_game("result", save_input=victory_path, save_output=readback)
        assert n("screen") == n("frame") == g.front_field("page") == 0
        g.step(KEY_L)
        assert (n("screen"), n("won"), n("lost"), n("race_completed")) == (3, 1, 0, 1 << mission)
        loaded = persisted()
        assert loaded == final_state, ("result reload differs", [k for k in loaded if loaded[k] != final_state[k]])
        # Normal Terran journals restore the cursor and the remembered menu
        # action. The active selection and command mode reset on a fresh load.
        assert n("selected_building") == -1 and n("mode") == n("menu_open") == 0 and selected_units() == 0
        assert n("race_impact_count") == 0
        g.step(0, 8)
        assert n("screen") == 3 and persisted() == loaded and n("selected_building") == -1
        g.rsp.export_sram()
        assert readback.read_bytes() == victory_path.read_bytes() == victory_image
        require_unchanged(files, pins)
        result = dict(gameplay_calls=stop, cut=cut, briefing=briefing, dialogue=dialogue,
                      checkpoints=checkpoints, pause=pause, victory=victory,
                      victory_selection=victory_selection, timing=timing, sha256=pins)
        (args.out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
        print("Verified victory, fresh load and result journal", victory["frame"], flush=True)
    finally:
        if g is not None:
            g.close()


if __name__ == "__main__":
    main()
