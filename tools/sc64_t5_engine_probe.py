#!/usr/bin/env python3
"""Observe the internal T5 auto-entry ARM build under neutral input.

This is a bounded engine/performance probe, not a normal-menu controller win.
Only controller scheduler metadata is written through the debugger.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import time

import sc64_menu_smoke as menu
import sc64_races_profile as races
import sc64_split_profile as profile
import sc64_split_smoke as split
from emulator_smoke import ROOT, capture, symbol_sizes, symbols


def run(rom: Path, elf: Path, ticks: int, output: Path) -> dict:
    if not 1 <= ticks <= 0xffffffff:
        raise ValueError("ticks must fit a positive 32-bit scheduler length")
    sym, sizes = symbols(elf), symbol_sizes(elf)
    needed = {"race_t5", "game_update", "demo_replay_checkpoint",
              "demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
              "demo_schedule_tick", "demo_tick_buckets"}
    if needed - sym.keys():
        raise ValueError(f"T5 probe ELF is missing symbols: {sorted(needed - sym.keys())}")
    state_size, fields = races.dwarf_struct(elf, "Sc64T5RuntimeState")
    player_size, player_fields = races.dwarf_struct(elf, "Sc64T4TriggerState")
    if sizes["race_t5"] != state_size or state_size != 1340 or player_size != 144:
        raise ValueError("unsupported T5 probe state layout")
    bank = races.race_bank_profile(elf, sizes)
    campaign_layout = races.dwarf_struct(elf, "Sc64RaceCampaignState")
    out = output / f"t5-internal-{time.time_ns()}"
    out.mkdir(parents=True)
    proc = sock = log = None
    observations = []

    def snapshot(rsp, tick: int, cycles: int, interval: int) -> dict:
        state = profile.observe(rsp, sym, sizes, bank, campaign_layout)
        raw = rsp.read(sym["race_t5"], state_size)
        players = []
        for slot in range(8):
            start = fields["player"] + slot * player_size
            players.append({name: raw[start + player_fields[name]]
                            for name in ("fault", "fault_trigger", "fault_slot", "outcome")})
        if any(p["fault"] for p in players):
            raise RuntimeError(f"T5 trigger fault at tick {tick}: {players}")
        if (state["screen"], state["race_mode"], state["race_selected_mission"]) != (1, 0, 8):
            raise RuntimeError(f"T5 probe left live play at tick {tick}: {state['screen']}")
        result = {"tick": tick, "interval_ticks": interval, "interval_cycles": cycles,
                  "interval_effective_fps": round(profile.CPU_HZ * interval / cycles, 2)
                      if cycles else None,
                  "players": players, "t5_state_sha256": hashlib.sha256(raw).hexdigest(),
                  **state}
        result["capture"] = capture(rsp, out / f"tick-{tick:06d}.png")
        return result

    try:
        proc, sock, rsp, log = split.start_emulator("t5-internal", rom, out, sym)
        rsp.write(sym["demo_schedule_events"], struct.pack("<2I", 0, 0))
        rsp.write(sym["demo_schedule_length"], struct.pack("<I", ticks))
        rsp.write(sym["demo_schedule_count"], struct.pack("<I", 1))
        for opcode, name in (("z1", "game_update"), ("Z1", "demo_replay_checkpoint")):
            if rsp.cmd(f"{opcode},{sym[name] & ~1:x},2") != "OK":
                raise RuntimeError(f"failed to change breakpoint for {name}")
        rsp.tick()
        if profile.u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("scheduler failed to stop at tick zero")
        last_tick = total_cycles = 0
        last_clock = profile.hardware_clock(rsp)
        observations.append(snapshot(rsp, 0, 0, 0))
        while last_tick < ticks:
            rsp.tick()
            tick = profile.u32(rsp, sym["demo_schedule_tick"])
            if tick != min(ticks, last_tick + 1000):
                raise RuntimeError(f"unexpected scheduler checkpoint {tick}")
            now = profile.hardware_clock(rsp)
            cycles = (now - last_clock) & 0xffffffff
            observations.append(snapshot(rsp, tick, cycles, tick - last_tick))
            total_cycles += cycles
            print(f"T5 internal tick {tick}/{ticks}: {observations[-1]['interval_effective_fps']} fps", flush=True)
            last_tick, last_clock = tick, now
        result = {"entry": "internal-auto-entry", "controller_completion": False,
                  "requested_ticks": ticks,
                  "mean_live_play_effective_fps": round(profile.CPU_HZ * ticks / total_cycles, 2),
                  "rom": str(rom), "elf": str(elf),
                  "rom_sha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
                  "elf_sha256": hashlib.sha256(elf.read_bytes()).hexdigest(),
                  "observations": observations}
        (out / "profile.json").write_text(json.dumps(result, indent=2) + "\n")
        print(out / "profile.json", flush=True)
        return result
    finally:
        if proc is not None:
            menu.stop_emulator(proc, sock, log)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=ROOT / "build/StarCraft-T5-Engine-Probe.gba")
    parser.add_argument("--elf", type=Path, default=ROOT / "build/starcraft-t5-engine-probe.elf")
    parser.add_argument("--ticks", type=int, default=3000)
    parser.add_argument("--out", type=Path, default=ROOT / "build/emulator/sc64-t5-engine-probe")
    args = parser.parse_args()
    run(args.rom.resolve(), args.elf.resolve(), args.ticks, args.out.resolve())
