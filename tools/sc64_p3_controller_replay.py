#!/usr/bin/env python3
"""Replay the private Higher Ground (P3) controller-only win trace on ARM.

Only the normal controller input queue and its scheduler metadata are written.
Mission state is observed through mGBA symbols/DWARF at bounded checkpoints.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
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
from emulator_smoke import ROOT, capture, symbol_sizes, symbols

MISSION_P3 = 22
SCREEN_TITLE, SCREEN_PLAY, SCREEN_RESULT = 0, 1, 3
DEFAULT_TRACE = ROOT / ".local/sc64/p3-controller-garrison.json"
DEFAULT_OUTPUT = ROOT / "build/emulator/sc64-p3-controller-replay"
EXPECTED_FINAL = {
    "host_tick": 152107, "frame": 151088, "screen": SCREEN_RESULT,
    "won": 1, "lost": 0, "p3_outcome": 1, "p3_sequence": 5,
    "p3_fired": 31, "action_ticks": 0, "side_units": 14,
    "side_buildings": 11, "force2_units": 7, "force2_buildings": 0,
    "force3_units": 0, "force3_buildings": 0, "minerals": 44,
    "gas": 1048, "supply": 54, "supply_cap": 98, "gateways": 1,
}


def validate_trace(trace: object, capacity: int, *, windowed: bool = False) -> dict:
    """Strictly validate the frozen P3 controller trace before any emulator work."""
    if type(capacity) is not int or capacity < 1:
        raise ValueError("controller queue capacity must be a positive integer")
    if not isinstance(trace, dict) or type(trace.get("schema")) is not int or trace["schema"] != 1:
        raise ValueError("trace schema must be an object with schema=1")
    if type(trace.get("mission")) is not int or trace["mission"] != MISSION_P3:
        raise ValueError("trace mission must be P3 global ID 22")
    if not isinstance(trace.get("policy"), str) or not trace["policy"]:
        raise ValueError("trace policy must be a nonempty string")
    rows = trace.get("events")
    if not isinstance(rows, list) or not rows:
        raise ValueError("trace events must be a nonempty array")
    inputs: list[tuple[int, int]] = []
    previous_tick, previous_held = 0, 0
    start = final = None
    last_observation_tick = 0
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise ValueError(f"trace event {index} must be an object")
        kind = row.get("kind")
        if kind == "input":
            tick, held, pressed = row.get("host_tick"), row.get("held"), row.get("pressed")
            if type(tick) is not int or not 1 <= tick <= 0xFFFFFFFF or tick <= previous_tick:
                raise ValueError(f"input event {index} has invalid or unordered host_tick")
            if type(held) is not int or held < 0 or held & ~scheduler.KEY_MASK:
                raise ValueError(f"input event {index} has held bits outside 0x3ff")
            if held == previous_held:
                raise ValueError(f"input event {index} is not a held-state transition")
            if type(pressed) is not int or pressed != (held & ~previous_held):
                raise ValueError(f"input event {index} pressed does not match held transition")
            inputs.append((tick - 1, held))
            previous_tick, previous_held = tick, held
        elif kind == "start":
            if start is not None:
                raise ValueError("trace must contain exactly one start event")
            start = row
        elif kind == "periodic":
            tick = row.get("host_tick")
            if type(tick) is not int or tick <= last_observation_tick or tick > 0xFFFFFFFF:
                raise ValueError(f"periodic event {index} has invalid or unordered host_tick")
            last_observation_tick = tick
        elif kind == "won":
            if final is not None:
                raise ValueError("trace must contain exactly one won event")
            final = row
        else:
            raise ValueError(f"trace event {index} has unsupported kind {kind!r}")
    if not inputs or inputs[0][0] != 0:
        raise ValueError("first controller transition must be host_tick=1")
    if not windowed and len(inputs) > capacity:
        raise ValueError(f"controller queue exceeds ELF capacity ({len(inputs)}/{capacity})")
    if start is None or start.get("host_tick") != 7 or start.get("frame") != 0 or start.get("screen") != SCREEN_PLAY:
        raise ValueError("trace must record P3 launch at host_tick=7, frame=0, screen=PLAY")
    if start.get("p3_outcome") != 0 or start.get("won") != 0 or start.get("lost") != 0:
        raise ValueError("trace start must be a live, unfinished P3 mission")
    if final is None or rows[-1] is not final:
        raise ValueError("trace must end with its final won event")
    stop_tick = final.get("host_tick")
    if type(stop_tick) is not int or not 1 <= stop_tick <= 0xFFFFFFFF or stop_tick <= previous_tick:
        raise ValueError("final won event has invalid host_tick or precedes controller input")
    if last_observation_tick > stop_tick:
        raise ValueError("periodic observation occurs after final result")
    for key, expected in EXPECTED_FINAL.items():
        if type(final.get(key)) is not int or final[key] != expected:
            raise ValueError(f"final event {key} does not match the verified P3 win trace")
    if final.get("elapsed") != final["frame"]:
        raise ValueError("final P3 elapsed frame count does not match frame")
    if final.get("side_army") != 13 or final.get("p3_fired") < 1:
        raise ValueError("final trace is missing expected player force or P3 trigger state")
    if windowed:
        last_page = (stop_tick - 1) // scheduler.CHECKPOINT_TICKS
        peak = max((sum(1 for tick, _ in inputs if tick // scheduler.CHECKPOINT_TICKS == page)
                    for page in range(last_page + 1)), default=0)
        if peak > capacity:
            raise ValueError(f"controller event page exceeds ELF capacity ({peak}/{capacity})")
    else:
        peak = len(inputs)
    return {"inputs": inputs, "input_count": len(inputs), "stop_tick": stop_tick,
            "start": start, "final": final, "scheduler_mode": "windowed" if windowed else "full",
            "peak_page_transitions": peak}


def p3_snapshot(rsp, sym: dict[str, int], sizes: dict[str, int], bank: dict,
                campaign_layout: tuple[int, dict[str, int]], p3_layout: tuple[int, dict[str, int]]) -> dict:
    state = profile.observe(rsp, sym, sizes, bank, campaign_layout=campaign_layout,
                            p3_layout=p3_layout)
    units = state["units"]["by_owner"]
    buildings = state["buildings"]["by_owner"]
    state["side_units"] = units.get("0", 0)
    state["force2_units"] = units.get("1", 0)
    state["force3_units"] = units.get("4", 0)
    state["side_buildings"] = buildings.get("0", 0)
    state["force2_buildings"] = buildings.get("1", 0)
    state["force3_buildings"] = buildings.get("4", 0)
    state["race_minerals"] = list(struct.unpack("<5i", rsp.read(sym["race_minerals"], 20)))
    state["race_gas"] = list(struct.unpack("<5i", rsp.read(sym["race_gas"], 20)))
    state["supply"] = races.read_u32(rsp, sym["supply_used"])
    state["supply_cap"] = races.read_u32(rsp, sym["supply_cap"])
    state["race_completed"] = races.read_u32(rsp, sym["race_completed"])
    state["race_mode"] = races.read_u32(rsp, sym["race_mode"])
    state["race_selected_mission"] = races.read_u32(rsp, sym["race_selected_mission"])
    state["gateways"] = sum(1 for building in state["buildings"]["entities"]
                            if building["owner"] == 0 and building["type"] == 14)
    return state


def verify_final(trace_final: dict, state: dict) -> None:
    fields = ("screen", "frame", "won", "lost", "side_units", "side_buildings",
              "force2_units", "force2_buildings", "force3_units", "force3_buildings", "minerals", "gas")
    expected = {"screen": SCREEN_RESULT, "frame": trace_final["frame"], "won": 1, "lost": 0,
                **{key: trace_final[key] for key in fields[4:]}}
    expected.update({"race_mode": 0, "race_selected_mission": MISSION_P3,
                     "supply": trace_final["supply"], "supply_cap": trace_final["supply_cap"]})
    for key, value in expected.items():
        if state.get(key) != value:
            raise RuntimeError(f"final {key} mismatch: expected {value}, observed {state.get(key)}")
    # The legacy controller trace stores gateway presence (side_building() >= 0),
    # while p3_snapshot reports the full live gateway count for diagnostics.
    if bool(state.get("gateways", 0)) != bool(trace_final["gateways"]):
        raise RuntimeError("final gateways presence mismatch: "
                           f"expected {bool(trace_final['gateways'])}, observed count {state.get('gateways')}")
    campaign = state.get("race_campaign", {})
    for key, value in (("mission", MISSION_P3), ("elapsed_frames", trace_final["frame"]), ("phase", 3)):
        if campaign.get(key) != value:
            raise RuntimeError(f"final campaign.{key} mismatch: expected {value}, got {campaign.get(key)}")
    if not (campaign.get("objective_flags", 0) & 1):
        raise RuntimeError("final campaign primary objective flag is absent")
    model = state.get("race_p3", {})
    for key, value in (("outcome", 1), ("sequence", 5), ("fired", trace_final["p3_fired"]),
                       ("transmission", 0), ("reserved", 0), ("action_ticks", 0)):
        if model.get(key) != value:
            raise RuntimeError(f"final race_p3.{key} mismatch: expected {value}, got {model.get(key)}")
    if not (state.get("race_completed", 0) & (1 << MISSION_P3)):
        raise RuntimeError("P3 completion bit is absent")
def run(trace_path: Path, output: Path, rom_path: Path | None = None,
        elf_path: Path | None = None, ticks: int | None = None,
        save_at_end: bool = False) -> dict:
    trace = json.loads(trace_path.read_text(encoding="utf-8"))
    rom = rom_path or ROOT / "build/StarCraft-Protoss-P3-GBA.gba"
    elf = elf_path or ROOT / "build/starcraft-protoss-p3.elf"
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError(f"P3 ROM/ELF missing ({rom}, {elf})")
    sym, sizes = symbols(elf), symbol_sizes(elf)
    mode, capacity = scheduler.schedule_mode(sym, sizes)
    plan = validate_trace(trace, capacity, windowed=mode == "windowed")
    requested, prefix = scheduler.run_length(plan, ticks, save_at_end)
    pages = scheduler.make_schedule_pages(plan, requested, capacity) if mode == "windowed" else None
    if save_at_end and not mode == "windowed":
        raise ValueError("--save-at-end requires a windowed controller scheduler")
    campaign_meta = split.read_campaign_metadata(rom, elf, "protoss")
    if MISSION_P3 not in campaign_meta["enabled_mission_ids"]:
        raise RuntimeError("P3 ELF catalog does not enable Higher Ground global mission ID 22")
    bank = races.race_bank_profile(elf, sizes)
    campaign_layout = races.dwarf_struct(elf, "Sc64RaceCampaignState")
    p3_layout = races.dwarf_struct(elf, "Sc64P3State")
    required = ("game_update", "demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
                "demo_schedule_tick", "demo_replay_checkpoint", "screen", "frame", "won", "lost",
                "race_mode", "race_selected_mission", "race_completed", "race_campaign", "race_p3",
                "race_p3_action_ticks", "race_minerals", "race_gas", "supply_used", "supply_cap", "u", "b")
    required += ("race_front", "demo_inject_keys")
    if mode == "windowed":
        required += ("demo_schedule_active", "demo_schedule_index")
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError("P3 ELF is missing symbols: " + ", ".join(missing))
    run_dir = output / f"p3-controller-{time.time_ns()}"
    run_dir.mkdir(parents=True, exist_ok=False)
    rom_hash = hashlib.sha256(rom.read_bytes()).hexdigest()
    elf_hash = hashlib.sha256(elf.read_bytes()).hexdigest()
    trace_hash = hashlib.sha256(trace_path.read_bytes()).hexdigest()
    history_path = run_dir / "checkpoints.jsonl"
    split.FRONT_FIELDS = split.frontend_offsets(elf)
    scheduler.check_debugger_port_free()
    proc = sock = rsp = log = None
    try:
        proc, sock, rsp, log = split.start_emulator(run_dir.name, rom, run_dir, sym)
        initial = {"screen": profile.u32(rsp, sym["screen"]), "frame": profile.u32(rsp, sym["frame"]),
                   "page": profile.u32(rsp, sym["race_front"] + split.FRONT_FIELDS["page"])}
        if initial != {"screen": SCREEN_TITLE, "frame": 0, "page": split.FRONT_HOME}:
            raise RuntimeError(f"fresh boot advanced away from HOME: {initial}")

        def write_page(events: list[tuple[int, int]]) -> None:
            if len(events) > capacity:
                raise RuntimeError(f"controller page exceeds ELF capacity ({len(events)}/{capacity})")
            blob = b"".join(struct.pack("<2I", tick, held) for tick, held in events)
            for offset in range(0, len(blob), 256):
                rsp.write(sym["demo_schedule_events"] + offset, blob[offset:offset + 256])
            rsp.write(sym["demo_schedule_count"], struct.pack("<I", len(events)))

        if mode == "windowed":
            assert pages is not None
            write_page(pages[0]["events"])
            rsp.write(sym["demo_schedule_index"], struct.pack("<I", 0))
            rsp.write(sym["demo_schedule_length"], struct.pack("<I", requested))
            rsp.write(sym["demo_schedule_active"], struct.pack("<I", 1))
        else:
            blob = b"".join(struct.pack("<2I", tick, held) for tick, held in plan["inputs"])
            for offset in range(0, len(blob), 256):
                rsp.write(sym["demo_schedule_events"] + offset, blob[offset:offset + 256])
            rsp.write(sym["demo_schedule_count"], struct.pack("<I", len(plan["inputs"])))
            rsp.write(sym["demo_schedule_length"], struct.pack("<I", requested))
        if rsp.cmd(f"z1,{sym['game_update'] & ~1:x},2") != "OK" or rsp.cmd(
                f"Z1,{sym['demo_replay_checkpoint'] & ~1:x},2") != "OK":
            raise RuntimeError("failed to install controller replay observer")
        rsp.tick()
        if profile.u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("scheduler failed to stop before first controller input")

        observations = []

        def record_checkpoint(checkpoint: dict) -> None:
            observations.append(checkpoint)
            # Durable bounded history: at most one compact record per observer
            # interval, plus tick zero. Keep it independently readable if final
            # verification or report generation fails.
            with history_path.open("a", encoding="utf-8") as stream:
                stream.write(json.dumps(checkpoint, separators=(",", ":")) + "\n")
                stream.flush()
                os.fsync(stream.fileno())
        previous_state = p3_snapshot(rsp, sym, sizes, bank, campaign_layout, p3_layout)
        previous_clock = profile.hardware_clock(rsp)
        previous_routes = {key: profile.u32(rsp, sym[key]) for key in
                           ("sc64_route_hits", "sc64_route_builds") if key in sym}
        counted_frames = counted_cycles = 0
        record_checkpoint({"tick": 0, "scheduler_tick": 0, "host_tick": 0,
                             "elapsed_hardware_cycles": 0, "frame_delta": 0,
                             "counted_live_play_interval": False, "effective_fps": None,
                             "profile_cycles_last_tick": scheduler.profile_counters(rsp, sym)[0],
                             "route_counters": previous_routes,
                             "route_interval": {key: 0 for key in previous_routes},
                             **previous_state})
        capture(rsp, run_dir / "home.png")
        profile.write_progress(
            run_dir / "progress.json", profile="protoss-p3", rom=rom, elf=elf,
            rom_sha256=rom_hash, elf_sha256=elf_hash,
            configuration={"mission": MISSION_P3, "trace": str(trace_path), "trace_sha256": trace_hash,
                           "input_transitions": plan["input_count"], "scheduler_mode": mode,
                           "scheduler_capacity": capacity, "peak_page_transitions": plan["peak_page_transitions"],
                           "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested,
                           "prefix": prefix}, requested_ticks=requested,
            checkpoint_interval=scheduler.CHECKPOINT_TICKS, latest_checkpoint=observations[0])
        tick = 0
        while tick < requested:
            rsp.tick()
            completed = profile.u32(rsp, sym["demo_schedule_tick"])
            expected = min(requested, tick + scheduler.CHECKPOINT_TICKS)
            if completed != expected:
                raise RuntimeError(f"expected observer tick {expected}, got {completed}")
            if mode == "windowed":
                assert pages is not None
                page = pages[(completed - 1) // scheduler.CHECKPOINT_TICKS]
                cursor, count = profile.u32(rsp, sym["demo_schedule_index"]), profile.u32(rsp, sym["demo_schedule_count"])
                if cursor != count or count != len(page["events"]):
                    raise RuntimeError(f"scheduler page incomplete at {completed}: {cursor}/{count}")
                if completed < requested:
                    write_page(pages[completed // scheduler.CHECKPOINT_TICKS]["events"])
            now = profile.hardware_clock(rsp)
            elapsed = (now - previous_clock) & 0xFFFFFFFF
            state = p3_snapshot(rsp, sym, sizes, bank, campaign_layout, p3_layout)
            frame_delta = (state["frame"] - previous_state["frame"]) & 0xFFFFFFFF
            counted = previous_state["screen"] == SCREEN_PLAY and state["screen"] == SCREEN_PLAY and frame_delta > 0
            fps = round(profile.CPU_HZ * frame_delta / max(1, elapsed), 2) if counted else None
            if counted:
                counted_frames += frame_delta
                counted_cycles += elapsed
            cycle_values, route_values = scheduler.profile_counters(rsp, sym)
            route_delta = scheduler.route_counter_deltas(route_values, previous_routes)
            checkpoint = {"tick": completed, "scheduler_tick": completed, "host_tick": completed,
                          "elapsed_hardware_cycles": elapsed, "frame_delta": frame_delta,
                          "counted_live_play_interval": counted, "effective_fps": fps,
                          "profile_cycles_last_tick": cycle_values, "route_counters": route_values,
                          "route_interval": route_delta, **state}
            if completed in (1000, 10000, 30000, 50000, requested):
                checkpoint["capture"] = capture(rsp, run_dir / f"checkpoint-{completed:06d}.png")
            record_checkpoint(checkpoint)
            profile.write_progress(
                run_dir / "progress.json", profile="protoss-p3", rom=rom, elf=elf,
                rom_sha256=rom_hash, elf_sha256=elf_hash,
                configuration={"mission": MISSION_P3, "trace": str(trace_path),
                               "trace_sha256": trace_hash, "input_transitions": plan["input_count"],
                               "scheduler_mode": mode, "scheduler_capacity": capacity,
                               "peak_page_transitions": plan["peak_page_transitions"],
                               "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested,
                               "prefix": prefix},
                requested_ticks=requested, checkpoint_interval=scheduler.CHECKPOINT_TICKS,
                latest_checkpoint=checkpoint)
            print(f"P3 tick={completed}/{requested} frame={state['frame']} screen={state['screen']} "
                  f"fps={fps if fps is not None else 'n/a'}", flush=True)
            previous_state, previous_clock, previous_routes = state, now, route_values
            tick = completed
        final = observations[-1]
        report = {"profile": "protoss-p3", "status": "prefix" if prefix else "verification_pending", "mission": MISSION_P3,
                  "rom": str(rom), "elf": str(elf), "trace": str(trace_path),
                  "rom_sha256": rom_hash, "elf_sha256": elf_hash, "trace_sha256": trace_hash,
                  "input_transitions": plan["input_count"], "scheduler_mode": mode,
                  "scheduler_capacity": capacity, "peak_page_transitions": plan["peak_page_transitions"],
                  "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested,
                  "observed_ticks": final["tick"], "prefix": prefix,
                  "counted_live_play_frames": counted_frames,
                  "mean_live_play_effective_fps": round(profile.CPU_HZ * counted_frames / max(1, counted_cycles), 2),
                  "timer_method": "GBA chained hardware timers at 1000-tick and exact-final observers",
                  "fps_exclusions": "Initial HOME and result intervals are excluded; only positive frame deltas between PLAY checkpoints count.",
                  "final": final, "checkpoints": observations,
                  "checkpoint_history": str(history_path),
                  "captures": {"home": str(run_dir / "home.png"),
                               "final": str(run_dir / f"checkpoint-{requested:06d}.png")},
                  "verified_win": False}
        path = run_dir / "profile.json"

        def save_report() -> None:
            temp_path = path.with_suffix(".json.tmp")
            with temp_path.open("w", encoding="utf-8") as stream:
                json.dump(report, stream, indent=2)
                stream.write("\n")
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temp_path, path)

        # Persist the complete observations before any final contract checks.
        save_report()
        if not prefix:
            try:
                verify_final(plan["final"], final)
            except Exception as exc:
                report.update({"status": "verification_failed", "verification_failed": str(exc),
                               "verified_win": False})
                save_report()
                profile.write_progress(
                    run_dir / "progress.json", profile="protoss-p3", rom=rom, elf=elf,
                    rom_sha256=rom_hash, elf_sha256=elf_hash,
                    configuration={"mission": MISSION_P3, "trace": str(trace_path),
                                   "trace_sha256": trace_hash, "input_transitions": plan["input_count"],
                                   "scheduler_mode": mode, "scheduler_capacity": capacity,
                                   "peak_page_transitions": plan["peak_page_transitions"],
                                   "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested,
                                   "prefix": prefix},
                    requested_ticks=requested, checkpoint_interval=scheduler.CHECKPOINT_TICKS,
                    latest_checkpoint={"tick": final["tick"], **final},
                    status="verification_failed", final_report=str(path))
                raise
            report.update({"status": "won", "verified_win": True})
        if save_at_end:
            if final["screen"] != SCREEN_PLAY:
                raise RuntimeError("--save-at-end requires a prefix ending during live play")
            rsp.write(sym["demo_schedule_active"], struct.pack("<I", 0))
            rsp.write(sym["demo_schedule_count"], struct.pack("<I", 0))
            saved = profile.save_live_checkpoint(rsp, sym, tick=final["tick"])
            saved["state"] = p3_snapshot(rsp, sym, sizes, bank,
                                          campaign_layout, p3_layout)
            menu.stop_emulator(proc, sock, log)
            proc = sock = log = None
            save_dir = run_dir / "saves" / run_dir.name
            saves = list(save_dir.glob("*.sav"))
            if len(saves) != 1:
                raise RuntimeError(f"checkpoint did not flush exactly one SRAM file: {saves}")
            saved_path = profile.checked_sram(saves[0])
            saved.update({"path": str(saved_path),
                          "sha256": hashlib.sha256(saved_path.read_bytes()).hexdigest()})
            report["save"] = saved
        save_report()
        profile.write_progress(
            run_dir / "progress.json", profile="protoss-p3", rom=rom, elf=elf,
            rom_sha256=rom_hash, elf_sha256=elf_hash,
            configuration={"mission": MISSION_P3, "trace": str(trace_path),
                           "trace_sha256": trace_hash, "input_transitions": plan["input_count"],
                           "scheduler_mode": mode, "scheduler_capacity": capacity,
                           "peak_page_transitions": plan["peak_page_transitions"],
                           "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested,
                           "prefix": prefix},
            requested_ticks=requested, checkpoint_interval=scheduler.CHECKPOINT_TICKS,
            latest_checkpoint={"tick": final["tick"], **final},
            status="prefix" if prefix else "finished", final_report=str(path))
        print(json.dumps({"status": report["status"], "observed_ticks": final["tick"],
                          "scheduler_mode": mode, "input_transitions": plan["input_count"],
                          "report": str(path)}, indent=2), flush=True)
        return report
    finally:
        if proc is not None:
            try:
                menu.stop_emulator(proc, sock, log)
            except Exception:
                proc.terminate()
                try:
                    proc.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()
                if log is not None and not log.closed:
                    log.close()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, default=DEFAULT_TRACE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--rom", type=Path, help="P3 ROM override")
    parser.add_argument("--elf", type=Path, help="P3 ELF override")
    parser.add_argument("--ticks", type=int, help="replay a positive trace prefix")
    parser.add_argument("--save-at-end", action="store_true", help="pause/save with normal controls at prefix end")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        run(args.trace, args.output, args.rom, args.elf, args.ticks, args.save_at_end)
    except (OSError, ValueError, RuntimeError, KeyError, json.JSONDecodeError,
            subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        print(f"sc64_p3_controller_replay: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
