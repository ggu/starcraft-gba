#!/usr/bin/env python3
"""Replay private T6/T7-T10 controller traces through ordinary Terran menus.

Only bounded controller scheduler pages are written to mGBA. Mission state is
read through symbols; the replay never writes world state. The adapter follows
the authored 30 Hz model and is not a native-engine or hardware-GBA claim.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import time

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import sc64_menu_smoke as menu
import sc64_races_profile as races
import sc64_split_smoke as split
import sc64_split_profile as profile
import sc64_z3_controller_replay as scheduler
import sc64_t5_save_smoke as save_smoke
from emulator_smoke import ROOT, capture, symbol_sizes, symbols

CPU_HZ = 16_777_216
SCREEN_TITLE, SCREEN_PLAY, SCREEN_RESULT = 0, 1, 3
KEY_A, KEY_START, KEY_RIGHT, KEY_DOWN = 1, 8, 16, 128
CHECKPOINT_TICKS = 1000
DEFAULT_TRACE = ROOT / ".local/sc64/t6/controller-final-win-c.json"
DEFAULT_OUTPUT = ROOT / "build/emulator/terran-late-controller-replay"
DEFAULT_ROM = ROOT / "build/StarCraft-Terran-Late-Controller.gba"
DEFAULT_ELF = ROOT / "build/starcraft-terran-late-controller.elf"


def menu_prefix(mission: int) -> tuple[list[tuple[int, int]], int]:
    """Normal unified campaign route; return controller events and deploy tick."""
    if mission == 9:
        page, row = 2, 1
    elif 10 <= mission <= 13:
        chapter = mission - 3
        page, row = (chapter - 1) // 2, (chapter - 1) % 2
    else:
        raise ValueError("T6/T7-T10 replay supports global missions 9..13")
    sequence = [KEY_A]
    sequence.extend([KEY_RIGHT] * page)
    if row:
        sequence.append(KEY_DOWN)
    sequence.extend([KEY_A, KEY_START])  # open briefing, explicitly deploy
    events: list[tuple[int, int]] = []
    tick = 1
    for key in sequence:
        events.extend(((tick - 1, key), (tick, 0)))
        deploy = tick
        tick += 2
    # Final release is not needed to launch and source tick 1 is neutral.
    return events[:-1], deploy


def _entry_for(mission: int) -> str:
    return "t6-probe-entry" if mission == 9 else f"t{mission - 3}-probe-entry"


def validate_trace(trace: object, capacity: int) -> dict:
    """Strictly validate a private T6/late-Terran trace and offset menu entry."""
    if type(capacity) is not int or capacity < 1:
        raise ValueError("controller queue capacity must be positive")
    if not isinstance(trace, dict) or type(trace.get("trace_schema")) is not int or trace["trace_schema"] != 1:
        raise ValueError("trace must use trace_schema=1")
    mission = trace.get("mission")
    if type(mission) is not int or mission not in range(9, 14):
        raise ValueError("trace mission must be global ID 9..13")
    expected_entry = _entry_for(mission)
    if trace.get("entry") != expected_entry:
        raise ValueError(f"mission {mission} trace entry must be {expected_entry!r}")
    if not isinstance(trace.get("policy"), str) or not trace["policy"]:
        raise ValueError("trace policy must be nonempty")
    limit = trace.get("tick_limit")
    if type(limit) is not int or not 1 <= limit <= scheduler.MAX_U32:
        raise ValueError("tick_limit must be a positive 32-bit integer")
    rows = trace.get("events")
    if not isinstance(rows, list) or not rows:
        raise ValueError("trace events must be nonempty")
    prefix, launch_tick = menu_prefix(mission)
    prefix_transitions = [(tick + 1, held) for tick, held in prefix]
    inputs: list[tuple[int, int]] = []
    transitions: list[tuple[int, int]] = []
    checkpoints: list[dict] = []
    start = final = None
    prior_tick, prior_held, prior_event_tick = 0, 0, 0
    for i, row in enumerate(rows):
        if not isinstance(row, dict):
            raise ValueError(f"event {i} must be an object")
        kind, tick = row.get("kind"), row.get("host_tick")
        if type(tick) is not int or not 0 <= tick <= limit or tick < prior_event_tick:
            raise ValueError(f"event {i} has invalid or unordered host_tick")
        prior_event_tick = tick
        if kind == "input":
            if start is None:
                raise ValueError("source controller inputs must follow start")
            if tick < 1 or tick <= prior_tick:
                raise ValueError(f"input event {i} is not strictly ordered")
            held, pressed = row.get("held"), row.get("pressed")
            if type(held) is not int or held < 0 or held & ~scheduler.KEY_MASK:
                raise ValueError(f"input event {i} held mask is invalid")
            if type(pressed) is not int or pressed != (held & ~prior_held):
                raise ValueError(f"input event {i} pressed does not match held transition")
            # Probe mains emit an initial neutral sample because their local
            # trace writer starts with an unset last_keys sentinel. It is not
            # a controller transition and the menu route already ends neutral.
            if not transitions and tick == 1 and held == 0 and pressed == 0:
                prior_tick = tick
                continue
            if held == prior_held:
                raise ValueError(f"input event {i} is not a held-state transition")
            transitions.append((tick, held))
            prior_tick, prior_held = tick, held
        elif kind == "start":
            if start is not None or final is not None:
                raise ValueError("trace requires one start before final")
            start = row
        elif kind in ("won", "lost", "limit"):
            if start is None or final is not None or i != len(rows) - 1:
                raise ValueError("trace must end with one terminal event")
            final = row
        else:
            if start is None or final is not None:
                raise ValueError("checkpoints must follow start and precede final")
            if not isinstance(kind, str) or not kind or kind == "input":
                raise ValueError(f"event {i} has unsupported kind")
            if checkpoints and tick < checkpoints[-1]["host_tick"]:
                raise ValueError("checkpoint ticks must not go backwards")
            if checkpoints and tick == checkpoints[-1]["host_tick"]:
                # Several source-side probes may label the same observation
                # with separate event kinds. They are valid only when their
                # overlapping state fields agree; do not force artificial
                # one-tick gaps into a controller trace.
                prior = checkpoints[-1]
                shared = (set(prior) & set(row)) - {"kind"}
                if any(prior[field] != row[field] for field in shared):
                    raise ValueError("same-tick checkpoints disagree")
            checkpoints.append(row)

    if start is None or final is None:
        raise ValueError("trace requires start and final event")
    # Probe traces start after direct internal game_init(); the normal menu
    # prefix is added here, so it must not be present in the source trace.
    if start["host_tick"] != 0 or start.get("frame") != 0 or start.get("screen") != SCREEN_PLAY:
        raise ValueError("source start must record frame-zero PLAY at host_tick=0")
    if start.get("mission") != mission or start.get("won") != 0 or start.get("lost") != 0:
        raise ValueError("start observation does not match selected live mission")
    for label, observation in [("start", start), *[("checkpoint", c) for c in checkpoints],
                               ("final", final)]:
        for field in ("frame", "screen", "mission", "won", "lost", "faults"):
            if type(observation.get(field)) is not int:
                raise ValueError(f"{label} observation requires integer {field}")
    if final["host_tick"] < prior_tick:
        raise ValueError("terminal event precedes input events or mission launch")
    if final["host_tick"] < launch_tick:
        raise ValueError("terminal event precedes normal-menu launch budget")
    if final["kind"] == "won":
        if final.get("screen") != SCREEN_RESULT or final.get("won") != 1 or final.get("lost") != 0:
            raise ValueError("won trace must end in a won RESULT state")
        if final.get("mission") != mission:
            raise ValueError("won event mission mismatch")
        if final.get("faults", 0) != 0:
            raise ValueError("won event cannot contain mission trigger faults")
        if final.get("campaign_phase", 3) != 3:
            raise ValueError("won event must carry completed campaign phase")
        if "race_completed" in final and not (final["race_completed"] & (1 << mission)):
            raise ValueError("won event lacks this mission's completion bit")
    elif final["kind"] == "lost":
        if final.get("screen") != SCREEN_RESULT or final.get("lost") != 1 or final.get("won") != 0:
            raise ValueError("lost trace must end in a lost RESULT state")
        if final.get("mission") != mission:
            raise ValueError("lost event mission mismatch")
        if final.get("campaign_phase", 4) != 4:
            raise ValueError("lost event must carry failed campaign phase")
    else:
        if final["host_tick"] != limit or final.get("screen") != SCREEN_PLAY:
            raise ValueError("limit event must match declared budget in live PLAY")
    stop = final["host_tick"] + launch_tick
    absolute_inputs = [(tick, held) for tick, held in prefix]
    for tick, held in transitions:
        translated = tick + launch_tick
        if translated <= (absolute_inputs[-1][0] + 1 if absolute_inputs else 0):
            raise ValueError("source input collides with normal-menu route")
        absolute_inputs.append((translated - 1, held))
    translated_start = dict(start, source_host_tick=start["host_tick"], host_tick=launch_tick)
    translated_checkpoints = [dict(r, source_host_tick=r["host_tick"],
                                    host_tick=r["host_tick"] + launch_tick) for r in checkpoints]
    translated_final = dict(final, source_host_tick=final["host_tick"], host_tick=stop)
    return {"mission": mission, "entry": "campaign-menu", "launch_tick": launch_tick,
            "prefix": prefix, "inputs": absolute_inputs, "input_count": len(absolute_inputs),
            "source_input_count": len(transitions) - len(prefix_transitions),
            "source_policy": trace["policy"], "source_tick_limit": limit,
            "start": translated_start, "checkpoints": translated_checkpoints,
            "final": translated_final, "stop_tick": stop}


def make_pages(plan: dict, requested: int, capacity: int) -> list[dict]:
    if type(requested) is not int or requested < 1 or requested > scheduler.MAX_U32:
        raise ValueError("requested ticks must be positive 32-bit integer")
    if type(capacity) is not int or capacity < 1:
        raise ValueError("controller queue capacity must be positive")
    boundaries = {requested, plan["launch_tick"]}
    boundaries.update(range(CHECKPOINT_TICKS, requested, CHECKPOINT_TICKS))
    for row in [*plan["checkpoints"], plan["final"]]:
        if plan["launch_tick"] <= row["host_tick"] < requested:
            boundaries.add(row["host_tick"])
    ordered = sorted(boundaries)
    pages, cursor, start = [], 0, 0
    for end in ordered:
        while start < end:
            if cursor < len(plan["inputs"]) and plan["inputs"][cursor][0] < start:
                raise ValueError("input transition omitted before observer")
            end_cursor = cursor
            while end_cursor < len(plan["inputs"]) and plan["inputs"][end_cursor][0] < end:
                end_cursor += 1
            if end_cursor - cursor <= capacity:
                page_events = plan["inputs"][cursor:end_cursor]
                pages.append({"start_tick": start, "end_tick": end, "events": page_events})
                cursor = end_cursor
                start = end
                continue
            # Split only between transitions. The transition at split_tick is
            # the first event on the next page, preserving absolute ticks and
            # held input state while respecting the ELF's actual queue size.
            split_tick = plan["inputs"][cursor + capacity][0]
            if split_tick <= start or split_tick >= end:
                raise ValueError("cannot split overfull input page at a transition boundary")
            pages.append({"start_tick": start, "end_tick": split_tick,
                          "events": plan["inputs"][cursor:cursor + capacity]})
            cursor += capacity
            start = split_tick
    if any(page["end_tick"] - page["start_tick"] > CHECKPOINT_TICKS for page in pages):
        raise ValueError("observer page exceeds 1000 ticks")
    if requested >= plan["stop_tick"] and cursor != len(plan["inputs"]):
        raise ValueError("input transitions occur after exact stop")
    return pages


def _compare_trace_row(expected: dict, actual: dict) -> None:
    fields = ("frame", "screen", "mission", "won", "lost", "faults", "campaign_phase", "phase",
              "objective_stage", "objective_flags", "race_completed",
              "side_units", "enemy_units", "side_buildings", "enemy_buildings",
              "hostile_buildings", "hero_alive", "ion_cannon_alive",
              "norad_flying_alive", "emitter_at_goal",
              "protoss_force2_buildings", "zerg_force3_buildings",
              "raw16_alive", "zerg_units",
              "minerals", "gas", "raw19_alive", "raynor_alive", "raw126_alive",
              "raynors_at_goal", "dropships_at_goal",
              "norad_alive", "kerrigan_alive", "emitter_alive", "emitter_slot", "emitter_owner",
              "emitter_raw_owner", "emitter_x", "emitter_y", "emitter_carrier", "carrier_x",
              "carrier_y", "carrier_hp", "raynor", "norad", "cc_slot", "cc_x", "cc_y",
              "refinery_slot", "factory_slot", "starport_slot", "tower_slot", "switch2",
              "rescue_mask", "t6_outcome", "cargo_loaded", "dropships")
    aliases = {"objective_stage": "objective_stage", "objective_flags": "objective_flags",
               "race_completed": "race_completed"}
    for field in fields:
        if field not in expected:
            continue
        slot_identity = {"raynor": ("unit_source_slots", 19),
                         "norad": ("building_source_slots", 126),
                         "starport_slot": ("building_source_slots", 114),
                         "tower_slot": ("building_source_slots", 115)}
        if field in slot_identity:
            slot = expected[field]
            if slot < 0:
                continue
            bank, raw = slot_identity[field]
            if slot not in actual.get(bank, {}).get(raw, []):
                raise RuntimeError(f"{field} identity mismatch at host tick {expected.get('host_tick')}: "
                                   f"expected tracked slot {slot} with raw type {raw}")
            continue
        actual_field = aliases.get(field, "phase" if field == "campaign_phase" else field)
        if actual_field not in actual:
            raise RuntimeError(f"observer cannot verify source field {field}")
        if expected[field] != actual[actual_field]:
            raise RuntimeError(f"{field} mismatch at host tick {expected.get('host_tick')}: "
                               f"ARM={actual[actual_field]!r}, source={expected[field]!r}")


def _source_location_count(units: list[dict], raw_type: int,
                           bounds: tuple[int, int, int, int]) -> int:
    """Match the T6 runner's SIDE, non-transported source-location count."""
    left, top, right, bottom = bounds
    return sum(1 for unit in units
               if unit["owner"] == 0 and unit["raw_type"] == raw_type and
               unit["order"] != 8 and
               left <= unit["x"] * 2 < right and top <= unit["y"] * 2 < bottom)


def _verify_terminal(plan: dict, state: dict) -> None:
    final = plan["final"]
    if (state.get("race_mode") != 0 or state.get("mission") != plan["mission"] or
            state.get("elapsed") != state.get("frame")):
        raise RuntimeError("terminal runtime identity/frame clock does not match the selected campaign mission")
    expected = final["kind"]
    outcome = state.get("human_outcome")
    if expected == "won":
        if (state["screen"] != SCREEN_RESULT or state["won"] != 1 or state["lost"] != 0 or
                state.get("phase") != 3 or outcome != 1 or state["faults"] != 0 or
                not (state.get("race_completed", 0) & (1 << plan["mission"]))):
            raise RuntimeError("win lacks matching runtime outcome, completion bit, clocks, or clean triggers")
    elif expected == "lost":
        if (state["screen"] != SCREEN_RESULT or state["lost"] != 1 or state["won"] != 0 or
                state.get("phase") != 4 or outcome != 2 or state["faults"] != 0):
            raise RuntimeError("loss lacks matching runtime outcome or clean triggers")
    elif expected == "limit":
        if (state["screen"] != SCREEN_PLAY or state["won"] or state["lost"] or
                state.get("phase") != 1 or outcome != 0 or state["faults"] != 0):
            raise RuntimeError("limit trace must finish live with no terminal result or trigger fault")


def _write_page(rsp, sym: dict, page: dict, capacity: int) -> None:
    if len(page["events"]) > capacity:
        raise RuntimeError("controller page exceeds actual ELF scheduler capacity")
    blob = b"".join(struct.pack("<2I", tick, held) for tick, held in page["events"])
    for offset in range(0, len(blob), 256):
        rsp.write(sym["demo_schedule_events"] + offset, blob[offset:offset + 256])
    rsp.write(sym["demo_schedule_count"], struct.pack("<I", len(page["events"])))


def _read_u32(rsp, address: int) -> int:
    return int.from_bytes(rsp.read(address, 4), "little")


def _read_i32(rsp, address: int) -> int:
    return int.from_bytes(rsp.read(address, 4), "little", signed=True)


def run(trace_path: Path, output: Path, rom_path: Path | None = None,
        elf_path: Path | None = None, ticks: int | None = None) -> dict:
    trace_bytes = trace_path.read_bytes()
    trace = json.loads(trace_bytes)
    initial_plan = validate_trace(trace, 256)
    rom, elf = rom_path or DEFAULT_ROM, elf_path or DEFAULT_ELF
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError(f"Terran campaign replay ROM/ELF missing ({rom}, {elf})")
    sym, sizes = symbols(elf), symbol_sizes(elf)
    mode, capacity = scheduler.schedule_mode(sym, sizes)
    if mode != "windowed":
        raise RuntimeError("campaign replay requires the paged windowed scheduler")
    plan = validate_trace(trace, capacity)
    requested, is_prefix = scheduler.run_length(plan, ticks)
    if requested < plan["launch_tick"]:
        raise ValueError("prefix is too short to reach normal-menu mission launch")
    pages = make_pages(plan, requested, capacity)
    state_symbol = "race_t6" if plan["mission"] == 9 else "race_late"
    required = ("game_update", "demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
                "demo_schedule_tick", "demo_replay_checkpoint", "demo_schedule_active",
                "demo_schedule_index", "race_front", "race_campaign", state_symbol,
                "race_selected_mission", "race_completed", "race_mode", "mission_objective_stage",
                "mission_unsupported_count", "screen", "frame", "won", "lost", "u", "b",
                "minerals", "gas")
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError("campaign replay ELF is missing symbols: " + ", ".join(missing))
    output.mkdir(parents=True, exist_ok=True)
    run_dir = output / f"terran-mission-{plan['mission']}-{time.time_ns()}"
    run_dir.mkdir(parents=False, exist_ok=False)
    split.FRONT_FIELDS = split.frontend_offsets(elf)
    scheduler.check_debugger_port_free()
    proc = sock = rsp = log = None
    history_path, report_path = run_dir / "checkpoints.jsonl", run_dir / "profile.json"
    observations: list[dict] = []

    def record(value: dict) -> None:
        observations.append(value)
        with history_path.open("a", encoding="utf-8") as stream:
            stream.write(json.dumps(value, separators=(",", ":")) + "\n")
            stream.flush()

    try:
        proc, sock, rsp, log = split.start_emulator(run_dir.name, rom, run_dir, sym)
        home = {"screen": _read_i32(rsp, sym["screen"]), "frame": _read_i32(rsp, sym["frame"]),
                "page": _read_u32(rsp, sym["race_front"] + split.FRONT_FIELDS["page"])}
        if home != {"screen": SCREEN_TITLE, "frame": 0, "page": split.FRONT_HOME}:
            raise RuntimeError(f"fresh replay ROM did not boot at campaign HOME: {home}")
        first = pages[0]
        _write_page(rsp, sym, first, capacity)
        rsp.write(sym["demo_schedule_index"], struct.pack("<I", 0))
        rsp.write(sym["demo_schedule_length"], struct.pack("<I", first["end_tick"]))
        rsp.write(sym["demo_schedule_active"], struct.pack("<I", 1))
        if rsp.cmd(f"z1,{sym['game_update'] & ~1:x},2") != "OK" or rsp.cmd(
                f"Z1,{sym['demo_replay_checkpoint'] & ~1:x},2") != "OK":
            raise RuntimeError("could not install bounded observer breakpoints")
        rsp.tick()
        if _read_u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("windowed scheduler did not pause before tick zero")
        previous = {"screen": _read_i32(rsp, sym["screen"]),
                    "frame": _read_i32(rsp, sym["frame"])}
        clock_before = profile.hardware_clock(rsp)
        record({"tick": 0, "entry": "campaign-menu", "runtime_phase": "HOME",
                "elapsed_hardware_cycles": 0,
                "frame_delta": 0, "counted_live_play_interval": False,
                "effective_fps": None, **previous})
        capture(rsp, run_dir / "home.png")
        counted_frames = counted_cycles = 0
        worst_interval = None
        cycle_names = tuple(getattr(profile, "PROFILE_CYCLE_NAMES", ()))
        pages_by_tick = {p["end_tick"]: p for p in pages}
        for index, page in enumerate(pages):
            tick = page["end_tick"]
            rsp.tick()
            if _read_u32(rsp, sym["demo_schedule_tick"]) != tick:
                raise RuntimeError(f"scheduler observer tick mismatch at {tick}")
            count = _read_u32(rsp, sym["demo_schedule_count"])
            cursor = _read_u32(rsp, sym["demo_schedule_index"])
            if count != len(page["events"]) or cursor != count:
                raise RuntimeError(f"scheduler page incomplete at {tick}: {cursor}/{count}")
            current = _snapshot(rsp, sym, sizes, elf, plan["mission"])
            expected_rows = [r for r in [plan["start"], *plan["checkpoints"], plan["final"]]
                             if r["host_tick"] == tick]
            for row in expected_rows:
                _compare_trace_row(row, current)
            now_clock = profile.hardware_clock(rsp)
            cycles = (now_clock - clock_before) & scheduler.MAX_U32
            frame_delta = (current["frame"] - previous["frame"]) & scheduler.MAX_U32
            counted = previous["screen"] == SCREEN_PLAY and current["screen"] == SCREEN_PLAY and frame_delta > 0
            fps = round(CPU_HZ * frame_delta / max(1, cycles), 2) if counted else None
            if counted:
                counted_frames += frame_delta
                counted_cycles += cycles
                interval = {"tick": tick, "frame_delta": frame_delta,
                            "elapsed_hardware_cycles": cycles, "effective_fps": fps}
                if worst_interval is None or fps < worst_interval["effective_fps"]:
                    worst_interval = interval
            value = {"tick": tick, "entry": "campaign-menu", "elapsed_hardware_cycles": cycles,
                     "frame_delta": frame_delta, "counted_live_play_interval": counted,
                     "effective_fps": fps,
                     "profile_cycles_last_tick": {name: _read_u32(rsp, sym[name])
                                                   for name in cycle_names if name in sym},
                     "demo_tick_buckets": (list(struct.unpack("<8I", rsp.read(sym["demo_tick_buckets"], 32)))
                                            if "demo_tick_buckets" in sym else None),
                     **current}
            record(value)
            if tick == requested:
                rsp.write(sym["demo_schedule_active"], struct.pack("<I", 0))
                rsp.write(sym["demo_schedule_count"], struct.pack("<I", 0))
            elif index + 1 < len(pages):
                following = pages[index + 1]
                _write_page(rsp, sym, following, capacity)
                rsp.write(sym["demo_schedule_length"], struct.pack("<I", following["end_tick"]))
            previous, clock_before = current, now_clock
        final_state = observations[-1]
        status = "prefix" if is_prefix else plan["final"]["kind"]
        verified = False
        if not is_prefix:
            _verify_terminal(plan, final_state)
            verified = status == "won"
        report = {"profile": f"terran-{('t6' if plan['mission']==9 else 't'+str(plan['mission']-3))}-campaign-menu",
                  "entry": "campaign-menu", "mission": plan["mission"], "status": status,
                  "rom": str(rom), "elf": str(elf),
                  "rom_sha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
                  "elf_sha256": hashlib.sha256(elf.read_bytes()).hexdigest(),
                  "trace_sha256": hashlib.sha256(trace_bytes).hexdigest(),
                  "input_transitions": plan["input_count"],
                  "source_input_transitions": plan["source_input_count"],
                  "peak_page_transitions": max((len(p["events"]) for p in pages), default=0),
                  "requested_ticks": requested, "observed_ticks": final_state["tick"],
                  "prefix": is_prefix, "counted_live_play_frames": counted_frames,
                  "counted_live_play_hardware_cycles": counted_cycles,
                  "mean_live_play_effective_fps": round(CPU_HZ * counted_frames / max(1, counted_cycles), 2),
                  "worst_live_play_interval": worst_interval,
                  "fps_method": "chained GBA timer cycles divided by PLAY-frame deltas; menu/result excluded",
                  "completed": verified, "verified_win": verified,
                  "checkpoint_history": str(history_path), "final": final_state}
        tmp = report_path.with_suffix(".tmp")
        tmp.write_text(json.dumps(report, indent=2) + "\n")
        tmp.replace(report_path)
        return report
    finally:
        if proc is not None:
            try:
                menu.stop_emulator(proc, sock, log)
            except Exception:
                proc.terminate()
                proc.wait(timeout=3)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, default=DEFAULT_TRACE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--rom", type=Path, default=DEFAULT_ROM)
    parser.add_argument("--elf", type=Path, default=DEFAULT_ELF)
    parser.add_argument("--ticks", type=int, help="run a bounded prefix")
    parser.add_argument("--validate-only", action="store_true",
                        help="validate and print the replay plan without starting mGBA")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        trace = json.loads(args.trace.read_text())
        plan = validate_trace(trace, capacity=256)
        requested, prefix_run = scheduler.run_length(plan, args.ticks)
        pages = make_pages(plan, requested, 256)
        if args.validate_only:
            print(json.dumps({"profile": "terran-late-campaign-menu", "mission": plan["mission"],
                              "entry": plan["entry"], "launch_tick": plan["launch_tick"],
                              "stop_tick": plan["stop_tick"], "requested_ticks": requested,
                              "prefix": prefix_run, "pages": len(pages),
                              "peak_page_transitions": max(map(lambda p: len(p["events"]), pages), default=0),
                              "trace_sha256": hashlib.sha256(args.trace.read_bytes()).hexdigest()}, indent=2))
            return 0
        result = run(args.trace, args.output, args.rom, args.elf, args.ticks)
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f"terran replay plan: {exc}", file=sys.stderr)
        return 2

def _runtime_rows(rsp, sym: dict, sizes: dict, elf: Path) -> tuple[list[dict], list[dict]]:
    unit_size, unit_fields = races.dwarf_struct(elf, "Unit")
    building_size, building_fields = races.dwarf_struct(elf, "Building")
    if "u" not in sizes or "b" not in sizes or sizes["u"] % unit_size or sizes["b"] % building_size:
        raise RuntimeError("entity bank sizes do not agree with Unit/Building DWARF")
    unit_count, building_count = sizes["u"] // unit_size, sizes["b"] // building_size
    if unit_size == 56:
        units = save_smoke.compact_live_unit_rows(rsp, sym["u"], unit_fields, unit_count, unit_size)
    else:
        raw = rsp.read(sym["u"], sizes["u"])
        needed = ("x", "y", "hp", "owner", "type", "alive", "order", "target", "raw_type", "raw_owner")
        if any(name not in unit_fields for name in needed):
            raise RuntimeError("Unit DWARF lacks fields needed for actor verification")
        units = []
        for slot in range(unit_count):
            row = raw[slot * unit_size:(slot + 1) * unit_size]
            units.append({name: int.from_bytes(row[unit_fields[name]:unit_fields[name] + 4],
                                               "little", signed=True) for name in needed})
    raw_b = rsp.read(sym["b"], sizes["b"])
    need_b = ("owner", "type", "alive", "raw_type", "raw_owner",
              "under_construction", "x", "y")
    if any(name not in building_fields for name in need_b):
        raise RuntimeError("Building DWARF lacks fields needed for actor verification")
    buildings = []
    for slot in range(building_count):
        row = raw_b[slot * building_size:(slot + 1) * building_size]
        buildings.append({name: int.from_bytes(row[building_fields[name]:building_fields[name] + 4],
                                               "little", signed=True) for name in need_b})
    return units, buildings


def _snapshot(rsp, sym: dict, sizes: dict, elf: Path, mission: int) -> dict:
    late = mission >= 10
    units, buildings = _runtime_rows(rsp, sym, sizes, elf)
    live_units = [dict(row, slot=i) for i, row in enumerate(units) if row["alive"]]
    live_buildings = [dict(row, slot=i) for i, row in enumerate(buildings) if row["alive"]]
    unit_size, unit_fields = races.dwarf_struct(elf, "Unit")
    building_size, building_fields = races.dwarf_struct(elf, "Building")
    if unit_size == 56:
        by_owner = {}
        for row in live_units: by_owner[row["owner"]] = by_owner.get(row["owner"], 0) + 1
    else:
        by_owner = {}
        for row in live_units: by_owner[row["owner"]] = by_owner.get(row["owner"], 0) + 1
    b_by_owner = {}
    for row in live_buildings: b_by_owner[row["owner"]] = b_by_owner.get(row["owner"], 0) + 1
    state = {name: _read_i32(rsp, sym[name]) for name in
             ("screen", "frame", "won", "lost", "race_mode", "race_selected_mission", "race_completed")
             if name in sym}
    campaign_size, campaign_fields = races.dwarf_struct(elf, "Sc64RaceCampaignState")
    campaign = races.campaign_state(rsp, sym["race_campaign"], campaign_size, campaign_fields)
    state.update({"mission": state.get("race_selected_mission", -1),
                  "side_units": by_owner.get(0, 0), "enemy_units": by_owner.get(1, 0),
                  "side_buildings": b_by_owner.get(0, 0), "enemy_buildings": b_by_owner.get(1, 0),
                  "hostile_buildings": b_by_owner.get(1, 0) + b_by_owner.get(4, 0),
                  "protoss_force2_buildings": _count_owner_raw_buildings(live_buildings, 1, 1),
                  "zerg_force3_buildings": _count_owner_raw_buildings(live_buildings, 4, 3),
                  "zerg_units": by_owner.get(4, 0),
                  "minerals": _read_i32(rsp, sym["minerals"]), "gas": _read_i32(rsp, sym["gas"]),
                  "faults": _read_i32(rsp, sym["mission_unsupported_count"]),
                  "elapsed": campaign.get("elapsed_frames", -1),
                  "objective_flags": campaign.get("objective_flags", 0),
                  "phase": campaign.get("phase", 0),
                  "units": live_units, "buildings": live_buildings})
    if not late:
        layout = races.dwarf_struct(elf, "Sc64T6MissionState")[1]
        trig_layout = races.dwarf_struct(elf, "Sc64T4TriggerState")[1]
        base = sym["race_t6"]
        trigger_base = base + layout["triggers"]
        player0 = trigger_base
        faults = 0
        elapsed = []
        for owner in range(8):
            po = trigger_base + owner * 144
            faults += int.from_bytes(rsp.read(po + trig_layout["fault"], 1), "little")
            elapsed.append(_read_u32(rsp, po + trig_layout["elapsed_ticks"]))
        state["faults"] = max(state["faults"], faults)
        state["human_outcome"] = int.from_bytes(rsp.read(player0 + trig_layout["outcome"], 1), "little")
        state["all_trigger_clocks"] = elapsed
        state["objective_stage"] = _read_i32(rsp, sym.get("mission_objective_stage", 0))
        runtime_layout = races.dwarf_struct(elf, "Sc64T6RuntimeState")[1]
        switches = base + layout["triggers"] + runtime_layout["switches"]
        state["switch2"] = (rsp.read(switches, 1)[0] >> 2) & 1
        state["rescue_mask"] = int.from_bytes(rsp.read(base + layout["rescue_mask"], 1), "little")
        state["t6_outcome"] = state["human_outcome"]
    else:
        layout = races.dwarf_struct(elf, "Sc64TerranLateMissionState")[1]
        rt = races.dwarf_struct(elf, "Sc64TerranLateRuntimeState")[1]
        trig_layout = races.dwarf_struct(elf, "Sc64T4TriggerState")[1]
        base = sym["race_late"]
        trig = base + layout["triggers"]
        force_fields = []
        for owner in range(8):
            po = trig + owner * 144
            force_fields.append(_read_u32(rsp, po + trig_layout["elapsed_ticks"]))
            state["faults"] = max(state["faults"], int.from_bytes(rsp.read(po + trig_layout["fault"], 1), "little"))
        state["all_trigger_clocks"] = force_fields
        human_slot = _late_human_trigger_slot(mission)
        state["human_outcome"] = int.from_bytes(
            rsp.read(trig + human_slot * 144 + trig_layout["outcome"], 1), "little")
        switches = trig + rt["switches"]
        state["switch2"] = (rsp.read(switches, 1)[0] >> 2) & 1
        state["objective_stage"] = _read_i32(rsp, sym.get("mission_objective_stage", 0))
        state["emitter_unit"] = int.from_bytes(rsp.read(base + layout["emitter_unit"], 2), "little")
        state["emitter_carrier"] = int.from_bytes(rsp.read(base + layout["emitter_carrier"], 2), "little")
        if state["emitter_unit"] == 0xffff: state["emitter_unit"] = -1
        if state["emitter_carrier"] == 0xffff: state["emitter_carrier"] = -1
        if 0 <= state["emitter_carrier"] < len(units) and units[state["emitter_carrier"]]["alive"]:
            carrier = units[state["emitter_carrier"]]
            state["carrier_x"], state["carrier_y"], state["carrier_hp"] = (
                carrier["x"], carrier["y"], carrier["hp"])
        else:
            state["carrier_x"], state["carrier_y"], state["carrier_hp"] = -1, -1, -1
    raw_unit = lambda raw, owner=-1: _find_raw_actor(live_units, raw, owner)
    raw_building = lambda raw, owner=-1: _find_raw_actor(live_buildings, raw, owner)
    state["raw19_alive"] = int(raw_unit(19) is not None)
    state["raw16_alive"] = int(raw_unit(16) is not None)
    # The preplaced Raynor source actor is rescuable before the mission
    # converts it to SIDE ownership, so its trace identity is any-owner.
    raynor = raw_unit(19)
    state["raynor_alive"] = int(raynor is not None)
    norad = raw_building(126)
    norad_flying = raw_unit(29)
    state["norad_flying_alive"] = int(norad_flying is not None)
    # The source T8 trace calls its surviving Norad II hero explicitly. Raw
    # 29 is the flying Norad II unit; raw 126 is the distinct crashed building.
    if mission == 11:
        state["hero_alive"] = int(norad_flying is not None)
    elif mission == 10:
        state["hero_alive"] = int(raw_unit(16) is not None)
    elif mission == 13:
        state["hero_alive"] = int(raw_unit(19) is not None)
    ion_cannon = raw_building(127)
    state["ion_cannon_alive"] = int(ion_cannon is not None)
    state["raynor"] = raynor["slot"] if raynor else -1
    state["raw126_alive"] = int(norad is not None)
    state["norad_alive"] = state["raw126_alive"]
    state["norad"] = norad["slot"] if norad else -1
    # Like Raynor, preplaced Kerrigan can remain source-rescuable until the
    # mission's capture path transfers her to the human player.
    kerrigan, emitter = raw_unit(16), raw_unit(217, 0)
    state["kerrigan_alive"] = int(kerrigan is not None)
    state["kerrigan"] = kerrigan["slot"] if kerrigan else -1
    state["emitter_alive"] = int(emitter is not None)
    state["emitter_slot"] = emitter["slot"] if emitter else -1
    if late:
        state["emitter_slot"] = state["emitter_unit"] if 0 <= state["emitter_unit"] < len(units) else -1
        if state["emitter_slot"] >= 0:
            item = units[state["emitter_slot"]]
            state["emitter_owner"], state["emitter_raw_owner"] = item["owner"], item["raw_owner"]
            state["emitter_x"], state["emitter_y"] = item["x"], item["y"]
        else:
            state["emitter_owner"] = state["emitter_raw_owner"] = -1
            state["emitter_x"] = state["emitter_y"] = -1
        if mission == 10:
            bounds = _source_location_bounds(rsp, elf, sym, "sc64_t7_scenario", 1)
            if state["emitter_slot"] >= 0:
                goal_actor = units[state["emitter_slot"]]
                state["emitter_at_goal"] = int(
                    goal_actor["alive"] and goal_actor["raw_type"] == 217 and
                    _point_in_source_location(goal_actor["x"], goal_actor["y"], bounds))
            else:
                state["emitter_at_goal"] = 0
    elif emitter:
        state["emitter_owner"] = emitter["owner"]
        state["emitter_raw_owner"] = emitter["raw_owner"]
        state["emitter_x"], state["emitter_y"] = emitter["x"], emitter["y"]
    state["dropships"] = sum(x["raw_type"] == 11 and x["owner"] == 0 for x in live_units)
    state["cargo_loaded"] = sum(x["order"] == 8 for x in live_units)
    for name, raw in (("cc_slot", 106), ("refinery_slot", 110), ("factory_slot", 113),
                      ("starport_slot", 114), ("tower_slot", 115)):
        actor = raw_building(raw, 0)
        state[name] = actor["slot"] if actor else -1
        if name == "cc_slot":
            state["cc_x"], state["cc_y"] = ((actor["x"], actor["y"]) if actor else (-1, -1))
    state["unit_source_slots"] = {}
    for actor in live_units:
        if actor["raw_type"] >= 0:
            state["unit_source_slots"].setdefault(actor["raw_type"], []).append(actor["slot"])
    state["building_source_slots"] = {}
    for actor in live_buildings:
        if actor["raw_type"] >= 0:
            state["building_source_slots"].setdefault(actor["raw_type"], []).append(actor["slot"])
    if mission == 9:
        # The T6 harness counts living, non-transported actors by source MRGN
        # location 2 using half-scale runtime coordinates. Read the same
        # numeric rectangle from the linked scenario object in this ELF.
        scenario_addr = sym.get("sc64_t6_scenario")
        if scenario_addr is None:
            raise RuntimeError("T6 ELF lacks its linked source-location table")
        _, scenario_fields = races.dwarf_struct(elf, "Sc64T4Scenario")
        location_ptr = int.from_bytes(rsp.read(scenario_addr + scenario_fields["locations"], 4), "little")
        location_count = int.from_bytes(rsp.read(scenario_addr + scenario_fields["location_count"], 2), "little")
        location_size, location_fields = races.dwarf_struct(elf, "Sc64T4Location")
        if location_count < 2 or location_ptr == 0:
            raise RuntimeError("T6 ELF has no source location 2")
        location = rsp.read(location_ptr + location_size, location_size)
        left, top, right, bottom = (int.from_bytes(
            location[location_fields[name]:location_fields[name] + 4], "little")
            for name in ("left", "top", "right", "bottom"))
        bounds = (left, top, right, bottom)
        state["raynors_at_goal"] = _source_location_count(live_units, 19, bounds)
        state["dropships_at_goal"] = _source_location_count(live_units, 11, bounds)
    clocks = state["all_trigger_clocks"]
    if state["elapsed"] != state["frame"] or any(value > state["elapsed"] for value in clocks) or (
            state["screen"] == SCREEN_PLAY and any(value != state["elapsed"] for value in clocks)):
        raise RuntimeError("campaign/frame/source-player trigger clocks diverged")
    return state


def _late_human_trigger_slot(mission: int) -> int:
    """Source human controller slot for the retained late Terran setups.

    T7-T9 assign controller 6 to source player 0; T10 assigns it to player 1.
    This is read from the generated source setup tables rather than inferred
    from the maximum outcome among all eight independently ticking players.
    """
    slots = {10: 0, 11: 0, 12: 0, 13: 1}
    try:
        return slots[mission]
    except KeyError as exc:
        raise ValueError(f"no late human trigger slot for mission {mission}") from exc


def _find_raw_actor(rows: list[dict], raw_type: int, owner: int = -1):
    return next((row for row in rows if row["raw_type"] == raw_type and
                 (owner < 0 or row["owner"] == owner)), None)


def _count_owner_raw_buildings(rows: list[dict], owner: int, raw_owner: int) -> int:
    """Count live buildings only when both runtime and source owner match."""
    return sum(1 for row in rows if row["owner"] == owner and row["raw_owner"] == raw_owner)


def _source_location_bounds(rsp, elf: Path, sym: dict, scenario_symbol: str,
                            location_id: int) -> tuple[int, int, int, int]:
    """Read a one-based source MRGN rectangle from the linked scenario table."""
    scenario_addr = sym.get(scenario_symbol)
    if scenario_addr is None or type(location_id) is not int or location_id < 1:
        raise RuntimeError("linked source scenario/location is unavailable")
    _, scenario_fields = races.dwarf_struct(elf, "Sc64T4Scenario")
    location_ptr = int.from_bytes(rsp.read(scenario_addr + scenario_fields["locations"], 4), "little")
    location_count = int.from_bytes(rsp.read(scenario_addr + scenario_fields["location_count"], 2), "little")
    location_size, location_fields = races.dwarf_struct(elf, "Sc64T4Location")
    if location_id > location_count or location_ptr == 0:
        raise RuntimeError(f"source scenario has no location {location_id}")
    raw = rsp.read(location_ptr + (location_id - 1) * location_size, location_size)
    return tuple(int.from_bytes(raw[location_fields[name]:location_fields[name] + 4], "little")
                 for name in ("left", "top", "right", "bottom"))


def _point_in_source_location(x: int, y: int,
                              bounds: tuple[int, int, int, int]) -> bool:
    """Use the T7 runner's half-scale, left/top-inclusive point test."""
    left, top, right, bottom = bounds
    return left <= x * 2 < right and top <= y * 2 < bottom


if __name__ == "__main__":
    raise SystemExit(main())
