#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check T1, T4 and T6–T10 on a normal Terran ROM using physical buttons."""
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
from sc64_races_profile import campaign_state, dwarf_struct, dwarf_struct_layout, race_bank_profile
from sc64_skirmish_sram_continuation import (
    KEY_A, KEY_B, KEY_DOWN, KEY_L, KEY_RIGHT, KEY_START, PhysicalGame,
)
import sc64_t1_normal_replay as t1
import sc64_t4_controller_replay as t4
from sc64_t9_arm_replay import COMPARED, load_trace
from sc64_terran_controller_replay import (
    _compare_trace_row, _find_raw_actor, _snapshot, _verify_terminal, validate_trace,
)
from sc64_z7_normal_replay import input_hashes, require_unchanged, timing_delta, timing_snapshot

INNER_TAG, OUTER_TAG = 0x72435442, 0x76435442
T1_FIELDS = ("frame", "minerals", "marines", "scv", "zerg", "raynor_alive", "raynor_owner",
             "raynor_slot", "won", "lost", "screen", "phase", "objective_flags")
T4_FIELDS = t4.TRACE_STATE_FIELDS + ("actor_slot", "race_mode", "race_selected_mission")
T7_DIALOGUE = {1: (1, 2), 682: (661, 1), 4189: (3931, 1)}
T8_DIALOGUE = {1404: (1321, 1)}
T8_LAST_ASSAULT = 143122
T9_DIALOGUE = {1: (1, 4), 47969: (47251, 9)}
T6_FIELDS = (
    "frame", "screen", "won", "lost", "mission", "faults", "campaign_phase",
    "objective_stage", "t6_outcome", "objective_flags", "race_completed",
    "switch2", "rescue_mask", "raynor", "raynor_alive", "raynors_at_goal",
    "norad", "norad_alive", "side_units", "enemy_units", "side_buildings",
    "enemy_buildings", "minerals", "gas", "cc_slot", "cc_x", "cc_y",
    "refinery_slot", "factory_slot", "starport_slot", "tower_slot",
    "dropships", "dropships_at_goal", "cargo_loaded", "raw19_alive", "raw126_alive",
)
T7_FIELDS = (
    "frame", "screen", "won", "lost", "mission", "faults", "phase",
    "objective_flags", "race_completed", "kerrigan", "kerrigan_alive",
    "emitter_slot", "emitter_alive", "emitter_owner", "emitter_raw_owner",
    "emitter_x", "emitter_y", "emitter_carrier", "carrier_alive", "carrier_x",
    "carrier_y", "carrier_hp", "carrier_order", "carrier_tx", "carrier_ty",
    "carrier_path_retry", "nearest_enemy", "nearest_enemy_raw", "nearest_enemy_x",
    "nearest_enemy_y", "nearest_enemy_dist2", "emitter_at_goal", "side_units",
    "enemy_units", "side_buildings", "enemy_buildings", "minerals", "gas",
)
T10_FIELDS = ("frame", "screen", "won", "lost", "mission", "faults", "phase",
              "objective_flags", "race_completed", "hostile_buildings",
              "ion_cannon_slot", "ion_cannon_alive", "hero_slot", "hero_raw",
              "hero_alive", "minerals", "gas")
T8_FIELDS = tuple(name for name in T10_FIELDS if name not in (
    "ion_cannon_slot", "ion_cannon_alive", "hero_slot", "hero_raw",
))


def load_late_trace(path, chapter, fields, stop, expected_ticks):
    reference = validate_trace(json.loads(path.read_text()), 256)
    if reference["mission"] != chapter + 3:
        raise ValueError(f"T{chapter} requires a matching campaign controller trace")
    launch = reference["launch_tick"]
    if reference["stop_tick"] - launch != stop:
        raise ValueError(f"T{chapter} controller trace ends at the wrong tick")
    rows = (reference["start"], *reference["checkpoints"], reference["final"])
    observations, labels = {}, {}
    for row in rows:
        tick = row["source_host_tick"]
        if (any(type(row.get(name)) is not int for name in fields) or
                chapter == 7 and type(row.get("tracked_carrier")) is not int):
            raise ValueError(f"T{chapter} observation at {tick} is missing a required integer field")
        previous = observations.get(tick)
        if previous and any(previous[name] != row[name] for name in fields):
            raise ValueError(f"T{chapter} observations disagree at {tick}")
        observations[tick] = row
        labels.setdefault(tick, []).append(row["kind"])
    expected_rows = {6: 42, 7: 15, 8: 272}
    if not expected_ticks <= observations.keys() or len(rows) != expected_rows.get(chapter, len(rows)):
        raise ValueError(f"T{chapter} trace is missing a checkpoint from the qualified route")
    transitions = [(tick - launch + 1, held) for tick, held in reference["inputs"] if tick >= launch]
    if transitions and transitions[0][0] == 1:
        raise ValueError(f"T{chapter} trace must start with neutral input")
    return dict(transitions=transitions, observations=observations, labels=labels,
                final=reference["final"], stop=stop)


def load_t1_trace(path, stop):
    total, events, points, final = t1.validate(json.loads(path.read_text()))
    launch = 5
    opening = [(0, KEY_A), (1, 0), (2, KEY_A), (3, 0), (4, KEY_START)]
    keys = [(row["tick"], row["held"]) for row in events]
    if keys[:launch] != opening or total - launch != stop:
        raise ValueError("T1 requires the qualifying campaign-menu route")
    observations = {row["tick"] - launch: row for row in points}
    observations[0] = dict(kind="start", frame=0, raynor_alive=1, raynor_owner=2)
    observations[stop] = final = dict(final, kind="won", raynor_alive=1, raynor_owner=0)
    transitions = [(tick - launch + 1, held) for tick, held in keys[launch:]]
    pause = t1.find_roundtrip(events) - launch
    # Replace the source tape's save, restart and in-process load with a
    # pause save and a HOME load. Keep its following neutral release.
    resume = pause + len(t1.ROUND_TRIP) - 1
    if any(pause < tick <= resume for tick in observations):
        raise ValueError("T1 save transition overlaps a tape checkpoint")
    return dict(transitions=transitions, observations=observations, final=final, stop=stop,
                tape_pause_tick=pause, tape_resume_tick=resume)


def load_t4_trace(path, stop):
    reference = t4.validate_trace(json.loads(path.read_text()), 256)
    launch = reference["start_tick"]
    if reference["entry"] != "campaign-menu" or reference["stop_tick"] - launch != stop:
        raise ValueError("T4 requires the qualifying campaign-menu route")
    rows = (reference["start"], *reference["periodic"], reference["final"])
    observations = {row["host_tick"] - launch: row for row in rows}
    if len(rows) != 10 or len(observations) != len(rows):
        raise ValueError("T4 trace is missing a checkpoint from the qualified route")
    transitions = [(tick - launch + 1, held) for tick, held in reference["inputs"] if tick >= launch]
    return dict(transitions=transitions, observations=observations,
                final=reference["final"], stop=stop)


def observe_t7_carrier(game, state, last_slot):
    # The carrier can die before the objective trigger. Follow the slot first
    # witnessed carrying the emitter, then read its dead row at the result.
    carrier = state["emitter_carrier"]
    slot = carrier if carrier >= 0 else last_slot
    names = ("alive", "x", "y", "hp", "order", "tx", "ty", "path_retry")
    values = dict.fromkeys(names, -1)
    if slot >= 0:
        assert slot < game.sizes["u"] // game.unit_stride
        row = game.rsp.read(game.sym["u"] + slot * game.unit_stride, game.unit_stride)
        for name in names:
            offset, width = game.unit_layout[name], game.unit_layout.widths[name]
            assert 0 <= offset <= game.unit_stride - width and width in (1, 2, 4)
            values[name] = int.from_bytes(row[offset:offset + width], "little",
                                          signed=game.unit_layout.signed[name])
    state.update({"carrier_" + name: value for name, value in values.items()})
    state["carrier_alive"] = int(values["alive"] > 0)
    enemies = (q for q in state["units"] if q["owner"] in (1, 4)) if slot >= 0 else ()
    x, y = values["x"], values["y"]

    def distance(q):
        return (q["x"] - x) ** 2 + (q["y"] - y) ** 2

    enemy = min(enemies, key=distance, default=None)
    state.update(nearest_enemy=enemy["slot"] if enemy else -1,
                 nearest_enemy_raw=enemy["raw_type"] if enemy else -1,
                 nearest_enemy_x=enemy["x"] if enemy else -1,
                 nearest_enemy_y=enemy["y"] if enemy else -1,
                 nearest_enemy_dist2=distance(enemy) if enemy else 0x7fffffff)
    return slot


def main():
    if not __debug__:
        raise RuntimeError("Replay validation requires Python assertions")
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("trace", "rom", "elf", "runner", "out"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--chapter", type=int, choices=(1, 4, 6, 7, 8, 9, 10), default=9)
    parser.add_argument("--save-tick", type=int, required=True,
                        help="neutral completed gameplay tick for pause, save and fresh HOME load")
    args = parser.parse_args()
    resource.setrlimit(resource.RLIMIT_FSIZE, (16 << 20, 16 << 20))
    files = (args.trace, args.rom, args.elf, args.runner, Path(__file__))
    if args.chapter == 1:
        files += (Path(t1.__file__), Path(__file__).with_name("sc64_t1_trace_export.py"))
    elif args.chapter == 4:
        files += (Path(t4.__file__),)
    elif args.chapter in (6, 7):
        files += (Path(__file__).with_name("sc64_terran_controller_replay.py"),)
    pins = input_hashes(files)
    chapter = args.chapter
    mission = chapter + 3
    mission_symbol = {1: "race_campaign", 4: "race_t4", 6: "race_t6"}.get(chapter, "race_late")
    briefing_pages, dialogue_cuts, stop, compared = {
        1: (9, {}, 10144, T1_FIELDS),
        4: (8, {}, 8943, T4_FIELDS),
        6: (15, {}, 23849, T6_FIELDS),
        7: (17, T7_DIALOGUE, 4189, T7_FIELDS),
        8: (9, T8_DIALOGUE, 143260, T8_FIELDS),
        9: (9, T9_DIALOGUE, 47984, COMPARED),
        10: (14, {1: (1, 4)}, 3446, T10_FIELDS),
    }[chapter]
    qualified_ticks = {
        6: {0, 680, 1792, 3765, 9056, 11559, 19659, 20578, 23846, 23849},
        7: {0, 1, 2, 347, 1000, 1264, 2000, 2200, 2334, 3000, 4000, 4189},
        8: {0, 120, 332, 507, 1645, 5795, 143260, *range(1000, 143260, 1000)},
        10: {0, 120, 1000, 2000, 3000, 3446},
    }
    if chapter == 1:
        plan = load_t1_trace(args.trace, stop)
    elif chapter == 4:
        plan = load_t4_trace(args.trace, stop)
    elif chapter == 9:
        plan = load_trace(args.trace)
    else:
        plan = load_late_trace(args.trace, chapter, compared, stop, qualified_ticks[chapter])
    plan["mission"] = mission
    cut = args.save_tick
    if chapter == 1 and cut != plan["tape_pause_tick"]:
        parser.error("--save-tick must replace the T1 tape's existing save")
    resume_tick = plan.get("tape_resume_tick", cut)
    assert plan["final"]["kind"] == "won" and plan["stop"] == stop
    transitions = [(tick - 1, held) for tick, held in plan["transitions"]]
    if not transitions or transitions[0][0] != 0:
        transitions.insert(0, (0, 0))
    if not 1 < cut < stop or cut in dialogue_cuts or next(
            held for index, held in reversed(transitions) if index < cut) != 0:
        parser.error("--save-tick must be a neutral live gameplay tick")
    next_held = dict(transitions).get(resume_tick, 0)
    if next_held & KEY_L and next_held & KEY_START:
        parser.error("--save-tick cannot precede a joint L and Start press")
    # Each route keeps only three 32 KiB SRAM images and small JSON receipts.
    disk_reserve = (48 if chapter == 1 else 256) << 20
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
    last_carrier_slot = -1
    if chapter == 1:
        t1_types = t1.read_types(args.elf)
        campaign_layout = dwarf_struct(args.elf, "Sc64RaceCampaignState")
    elif chapter == 4:
        t4_layouts = (race_bank_profile(args.elf, g.sizes),
                      *(dwarf_struct(args.elf, name) for name in
                        ("Sc64RaceCampaignState", "Sc64T4RuntimeState", "Sc64T4TriggerState")))

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
        return [slot for slot, i in enumerate(range(layout["selected"], len(units), g.unit_stride))
                if int.from_bytes(units[i:i + width], "little") != 0]

    def observe():
        nonlocal last_carrier_slot
        if chapter == 1:
            state = t1.checkpoint(g, t1_types, 0)
            campaign = campaign_state(g.rsp, g.sym[mission_symbol], *campaign_layout)
            assert (n("race_mode"), n("race_selected_mission"), campaign["mission"]) == (0, mission, mission)
            assert campaign["elapsed_frames"] == state["frame"]
            return dict(state, screen=n("screen"), raynor_slot=campaign["rescue_unit"],
                        phase=campaign["phase"], objective_flags=campaign["objective_flags"])
        if chapter == 4:
            return t4.t4_snapshot(g.rsp, g.sym, g.sizes, *t4_layouts)
        state = _snapshot(g.rsp, g.sym, g.sizes, args.elf, mission)
        if chapter == 7:
            last_carrier_slot = observe_t7_carrier(g, state, last_carrier_slot)
        elif chapter == 10:
            hero = _find_raw_actor(state["units"], 19)
            cannon = _find_raw_actor(state["buildings"], 127)
            state["hero_slot"] = hero["slot"] if hero else -1
            state["hero_raw"] = hero["raw_type"] if hero else -1
            state["ion_cannon_slot"] = cannon["slot"] if cannon else -1
        elif chapter == 9:
            state["selected_building"] = n("selected_building")
            state["selection"] = len(selected_units())
        return state

    def persisted():
        world = g.state()
        world.pop("screen")
        controls = {name: n(name) for name in ("cx", "cy", "menu_last")}
        controls["selected_units"] = selected_units()
        return dict(world=world, mission=g.rsp.read(g.sym[mission_symbol], g.sizes[mission_symbol]),
                    controls=controls)

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
        if chapter == 4:
            assert (observed["race_mode"], observed["race_selected_mission"]) == (0, mission)
            t4.compare_observation(expected, observed, f"T4 tick {tick}")
        elif chapter == 6:
            _compare_trace_row(expected, observed)
            observed["campaign_phase"] = observed["phase"]
        else:
            diff = {k: (expected[k], observed[k]) for k in compared if
                    (chapter != 1 or k in expected) and expected[k] != observed[k]}
            if diff:
                (args.out / "divergence.json").write_text(json.dumps(
                    dict(tick=tick, expected=expected, differences=diff), indent=2) + "\n")
                raise ValueError(f"checkpoint {tick} diverged: {diff}")
        point = dict(tick=tick, **{k: observed[k] for k in compared})
        if chapter == 7:
            assert expected["tracked_carrier"] == last_carrier_slot
        if chapter in (6, 7, 8):
            point["labels"] = plan["labels"][tick]
        checkpoints.append(point)
        print("Matched", tick, expected.get("kind", "periodic"), observed["frame"], flush=True)
        return observed

    def reveal_briefing():
        g.tap(KEY_A)
        for _ in range((chapter - 1) // 2):
            g.tap(KEY_RIGHT)
        if chapter % 2 == 0:
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

    def acknowledge(tick, frame, limit, allow_fewer=False):
        for _ in range(limit):
            count, page, armed, row = modal()
            if allow_fewer and count == 0:
                break
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
    # T8's six victory pages are triggered once the last hostile building
    # falls. Inspect each update after the last observed assault order so
    # added physical acknowledgements cannot swallow a tape transition.
    # The retained T1, T4 and T6 tapes omit physical dialogue boundaries.
    dialogue_scan = ({1: range(1, stop + 1), 4: range(1, stop + 1), 6: range(1, stop + 1),
                      8: range(T8_LAST_ASSAULT + 1, stop + 1)}.get(chapter, ()))
    dialogue_count = {1: 8, 4: 11, 6: 13, 8: 7}.get(chapter, sum(count for _, count in dialogue_cuts.values()))
    try:
        assert n("screen") == n("frame") == g.front_field("page") == 0
        before = timing_snapshot(g.rsp, g.sym, g.sizes)
        reveal_briefing()
        compare(0)
        offset = g.rsp.completed
        edges = [(t + offset, mask) for t, mask in transitions]
        for tick in sorted(set(plan["observations"]) | set(dialogue_cuts) | {cut} | set(dialogue_scan)):
            if not tick or cut < tick <= resume_tick:
                continue
            if shutil.disk_usage(args.out).free < disk_reserve:
                raise RuntimeError(f"replay requires {disk_reserve >> 20} MiB free")
            g.rsp.play_to(tick + offset, edges)
            if tick in dialogue_cuts:
                acknowledge(tick, *dialogue_cuts[tick])
                offset = g.rsp.completed - tick
                edges = [(t + offset, mask) for t, mask in transitions if t >= tick]
            elif tick in dialogue_scan and modal()[0]:
                acknowledge(tick, n("frame"), dialogue_count - len(dialogue), allow_fewer=True)
                offset = g.rsp.completed - tick
                edges = [(t + offset, mask) for t, mask in transitions if t >= tick]
            else:
                assert modal()[0] == 0
            if tick in plan["observations"]:
                observed = compare(tick)
            if tick == cut:
                assert n("screen") == 1 and n("mode") == n("menu_open") == g.rsp.held == 0
                assert n("selected_building") == -1
                if chapter not in (1, 4, 6):
                    assert not selected_units()
                if chapter == 1:
                    assert observe()["raynor_owner"] == 0
                saved = persisted()
                old_camera = [n("camx"), n("camy")]
                g.step(KEY_START)
                g.step(0)
                g.step(KEY_B)
                g.step(0)
                assert n("screen") == 2 and n("pause_notice") & 0xc0 == 0x40
                assert persisted() == saved and n("selected_building") == -1
                g.rsp.export_sram()
                pause_image = pause_path.read_bytes()
                pause = journal(pause_image, 0, n("frame"), 0)
                if chapter in (1, 4, 6):
                    pause["selected_units"] = saved["controls"]["selected_units"]
                if chapter == 1:
                    pause["tape_resume_tick"] = resume_tick
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
                assert n("selected_building") == -1
                loaded = persisted()
                assert loaded == saved, ("pause reload differs",
                                         [k for k in loaded if loaded[k] != saved[k]],
                                         loaded["controls"], saved["controls"])
                if next_held & KEY_L:
                    # HOME Continue used L. Release it while paused so the
                    # next tape sample can press L without advancing gameplay.
                    g.step(KEY_START)
                    assert n("screen") == 2 and persisted() == saved
                    g.step(0)
                    g.step(KEY_START)
                    assert n("screen") == 1 and persisted() == saved
                    pause["release_home_l_calls"] = 3
                pause["camera_before"] = old_camera
                pause["camera_after"] = [n("camx"), n("camy")]
                (args.out / "pause.json").write_text(json.dumps(pause, indent=2) + "\n")
                g.rsp.set_keys(0)
                offset = g.rsp.completed - resume_tick
                edges = [(t + offset, mask) for t, mask in transitions if t >= resume_tick]
                print("Fresh load", tick, n("frame"), flush=True)
        if chapter == 1:
            assert (observed["screen"], observed["phase"], n("race_completed")) == (3, 3, 1 << mission)
            assert observed["objective_flags"] & 1 and observed["raynor_slot"] == checkpoints[0]["raynor_slot"]
        elif chapter == 4:
            t4.verify_final(plan["final"], observed)
        else:
            _verify_terminal(plan, observed)
        assert len(briefing) == briefing_pages
        assert len(dialogue) == dialogue_count
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
        # Unit selection is serialized with the actors. Building selection
        # and command mode reset on a fresh load of the normal Terran ROM.
        assert n("selected_building") == -1 and n("mode") == n("menu_open") == 0
        assert n("race_impact_count") == 0
        g.step(0, 8)
        assert n("screen") == 3 and persisted() == loaded and n("selected_building") == -1
        g.rsp.export_sram()
        assert readback.read_bytes() == victory_path.read_bytes() == victory_image
        require_unchanged(files, pins)
        result = dict(gameplay_calls=stop, cut=cut, briefing=briefing, dialogue=dialogue,
                      checkpoints=checkpoints, pause=pause, victory=victory,
                      victory_selection=victory_selection,
                      victory_selected_units=final_state["controls"]["selected_units"],
                      timing=timing, sha256=pins)
        (args.out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
        print("Verified victory, fresh load and result journal", victory["frame"], flush=True)
    finally:
        if g is not None:
            g.close()


if __name__ == "__main__":
    main()
