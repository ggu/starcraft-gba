#!/usr/bin/env python3
"""Validate SC64 drag-box selection in the ARM ROM using controller input only.

The debugger is stopped at ``game_update`` entry. The script writes only the
demo controller injection word; all game state is observed through symbols.
It intentionally does not start mGBA when imported.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import socket
import struct
import subprocess
import time
import sys

TOOLS_DIR = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))
from emulator_smoke import MGBA, ROOT, RSP, capture, symbols


KEY_A = 1
KEY_B = 2
KEY_RIGHT = 16
KEY_LEFT = 32
KEY_UP = 64
KEY_DOWN = 128
KEY_R = 256
KEY_L = 512

MAX_UNITS = 48
UNIT_WORDS = 22
UNIT_BYTES = UNIT_WORDS * 4
SCREEN_W, SCREEN_H = 240, 160
VRAM_PAGE0, VRAM_PAGE1 = 0x06000000, 0x0600A000
HIGHLIGHT_INDEX = 31


def read_int(rsp: RSP, addr: int) -> int:
    return struct.unpack("<i", rsp.read(addr, 4))[0]


def read_units(rsp: RSP, addr: int) -> list[dict[str, int]]:
    data = rsp.read(addr, MAX_UNITS * UNIT_BYTES)
    result = []
    for index in range(MAX_UNITS):
        values = struct.unpack_from("<22i", data, index * UNIT_BYTES)
        result.append({
            "index": index, "x": values[0], "y": values[1],
            "owner": values[4], "type": values[5], "alive": values[6],
            "selected": values[7],
        })
    return result


class SelectionRun:
    def __init__(self, rsp: RSP, sym: dict[str, int]):
        self.rsp, self.sym = rsp, sym
        self.held = 0
        self.ticks = 0

    def set_keys(self, keys: int) -> None:
        """Load keys for the next update; current update may consume old keys."""
        self.rsp.write(self.sym["demo_inject_keys"], struct.pack("<H", keys))
        self.rsp.tick(1)
        self.held = keys
        self.ticks += 1

    def advance(self, count: int = 1) -> None:
        self.rsp.tick(count)
        self.ticks += count

    def press(self, keys: int) -> None:
        self.set_keys(keys)
        self.advance(1)

    def release(self) -> None:
        self.set_keys(0)
        self.advance(1)

    def cursor(self) -> tuple[int, int]:
        return read_int(self.rsp, self.sym["cx"]), read_int(self.rsp, self.sym["cy"])

    def held_units(self) -> list[dict[str, int]]:
        return read_units(self.rsp, self.sym["u"])

    def selected(self) -> list[int]:
        return [u["index"] for u in self.held_units()
                if u["alive"] and u["owner"] == 0 and u["selected"]]


def framebuffer_indices(rsp: RSP) -> bytes:
    dispcnt = int.from_bytes(rsp.read(0x04000000, 2), "little")
    page = VRAM_PAGE1 if dispcnt & 0x10 else VRAM_PAGE0
    return rsp.read(page, SCREEN_W * SCREEN_H)


def parity_floor(value: int, residue: int, step: int) -> int:
    return value - ((value - residue) % step)


def parity_ceil(value: int, residue: int, step: int) -> int:
    return value + ((residue - value) % step)


def directional_keys(x: int, y: int, tx: int, ty: int, fast: bool) -> int:
    keys = KEY_R if fast else 0
    if x < tx:
        keys |= KEY_RIGHT
    elif x > tx:
        keys |= KEY_LEFT
    if y < ty:
        keys |= KEY_DOWN
    elif y > ty:
        keys |= KEY_UP
    return keys


def drive_to(run: SelectionRun, tx: int, ty: int, *, drag: bool,
             fast: bool = True, timeout: int = 1000) -> None:
    """Move the cursor to a reachable even-step coordinate, re-reading each tick."""
    base = (KEY_L if drag else 0) | (KEY_R if fast else 0)
    for _ in range(timeout):
        x, y = run.cursor()
        if (x, y) == (tx, ty):
            return
        keys = directional_keys(x, y, tx, ty, fast)
        directions = keys & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN)
        # Pulse from/to a nondirectional base. At the update-entry breakpoint,
        # set_keys first executes the prior input; this sequence makes that
        # frame neutral and applies exactly one directional step in the middle.
        run.set_keys(base | directions)
        run.set_keys(base)
    raise RuntimeError(f"cursor did not reach target {(tx, ty)}; now at {run.cursor()}")


def require_symbols(sym: dict[str, int]) -> None:
    names = ("demo_inject_keys", "game_update", "screen", "cx", "cy", "camx", "camy",
             "box_state", "box_anchor_x", "box_anchor_y", "box_end_x", "box_end_y", "u")
    missing = [name for name in names if name not in sym]
    if missing:
        raise RuntimeError(f"ARM ELF is missing required symbols: {', '.join(missing)}")


def friendly_bounds(units: list[dict[str, int]], *, marine_only: bool = True) -> tuple[int, int, int, int]:
    eligible = [u for u in units if u["alive"] and u["owner"] == 0 and
                (not marine_only or u["type"] == 1)]
    if not eligible:
        raise RuntimeError("no living opening friendly Marines found")
    return (min(u["x"] for u in eligible), min(u["y"] for u in eligible),
            max(u["x"] for u in eligible), max(u["y"] for u in eligible))


def in_bounds(unit: dict[str, int], bounds: tuple[int, int, int, int]) -> bool:
    left, top, right, bottom = bounds
    return left <= unit["x"] <= right and top <= unit["y"] <= bottom


def validate(rom: pathlib.Path, elf: pathlib.Path, out: pathlib.Path) -> dict:
    if not rom.is_file() or not elf.is_file():
        raise FileNotFoundError(f"ROM/ELF not found: {rom}, {elf}")
    sym = symbols(elf)
    require_symbols(sym)
    out.mkdir(parents=True, exist_ok=True)
    save_dir = out / "saves"
    save_dir.mkdir(parents=True, exist_ok=True)

    guard = socket.socket()
    try:
        guard.bind(("127.0.0.1", 2345))
    except OSError as exc:
        raise RuntimeError("port 2345 is already in use; leaving the existing debugger untouched") from exc
    finally:
        guard.close()

    log = (out / "mgba.log").open("ab")
    proc = subprocess.Popen([
        str(MGBA), "-g", "-C", "mute=1", "-C", "audioSync=0", "-C", "videoSync=0",
        "-C", f"savegamePath={save_dir}", str(rom),
    ], stdout=log, stderr=log)
    sock = None
    try:
        deadline = time.monotonic() + 12
        while time.monotonic() < deadline:
            try:
                sock = socket.create_connection(("127.0.0.1", 2345), timeout=1)
                break
            except OSError:
                time.sleep(0.1)
        if sock is None:
            raise RuntimeError("mGBA GDB server did not start")
        sock.settimeout(20)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        rsp = RSP(sock)
        rsp.cmd("qSupported")
        rsp.cmd("?")
        update = sym["game_update"] & ~1
        if rsp.cmd(f"Z1,{update:x},2") != "OK":
            raise RuntimeError("failed to install game_update breakpoint")
        rsp.tick(2)
        run = SelectionRun(rsp, sym)

        # Start the campaign by normal A input, then release it on a later frame.
        run.press(KEY_A)
        run.release()
        if read_int(rsp, sym["screen"]) != 1:
            raise RuntimeError("A did not enter the mission play screen")

        initial_units = run.held_units()
        min_x, min_y, max_x, max_y = friendly_bounds(initial_units)
        start_x, start_y = run.cursor()
        # Navigate without R so this preparatory move cannot trigger group
        # selection. Normal cursor steps are 2 pixels; snap box corners to the
        # same parity so they remain exactly reachable.
        residue_x, residue_y = start_x % 2, start_y % 2
        left = max(residue_x, parity_floor(min_x - 4, residue_x, 2))
        top = max(residue_y, parity_floor(min_y - 4, residue_y, 2))
        drive_to(run, left, top, drag=False, fast=False)
        anchor_x, anchor_y = run.cursor()
        # During the box gesture R makes each step four pixels. Align the far
        # corner to the anchor's four-pixel lattice so it cannot oscillate.
        right = min(1023 - ((1023 - anchor_x % 4) % 4),
                    parity_ceil(max_x + 4, anchor_x % 4, 4))
        bottom = min(1023 - ((1023 - anchor_y % 4) % 4),
                     parity_ceil(max_y + 4, anchor_y % 4, 4))
        base = capture(rsp, out / "selection-before.png")
        base_highlights = framebuffer_indices(rsp).count(HIGHLIGHT_INDEX)
        selection_before_drag = run.selected()

        # Press L at the top-left. Use R to verify it accelerates cursor
        # movement without invoking group selection, then navigate to bottom-right.
        run.set_keys(KEY_L | KEY_R)
        run.advance(1)
        anchor = (read_int(rsp, sym["box_anchor_x"]), read_int(rsp, sym["box_anchor_y"]))
        start_camera = (read_int(rsp, sym["camx"]), read_int(rsp, sym["camy"]))
        if run.selected() != selection_before_drag:
            raise RuntimeError("R during the drag changed selection before commit")
        drive_to(run, right, bottom, drag=True)

        # Remove directional input while retaining L for one full update before
        # releasing; this absorbs the breakpoint's one-frame input latency.
        run.set_keys(KEY_L)
        run.advance(1)
        active_end = run.cursor()
        if run.selected() != selection_before_drag:
            raise RuntimeError("R during the drag changed selection before commit")
        active_state = read_int(rsp, sym["box_state"])
        active_anchor = (read_int(rsp, sym["box_anchor_x"]), read_int(rsp, sym["box_anchor_y"]))
        end_camera = (read_int(rsp, sym["camx"]), read_int(rsp, sym["camy"]))
        active_path = out / "selection-active.png"
        active = capture(rsp, active_path)
        active_highlights = framebuffer_indices(rsp).count(HIGHLIGHT_INDEX)
        if active_state != 2:
            raise RuntimeError(f"selection outline did not become active (box_state={active_state})")
        if active_anchor != anchor:
            raise RuntimeError(f"world anchor changed during camera movement: {anchor} -> {active_anchor}")
        if start_camera == end_camera:
            raise RuntimeError("camera did not move during the drag; anchor stability was not exercised")
        if active_highlights <= base_highlights:
            raise RuntimeError("active selection outline did not add visible highlight pixels")

        # Use the world endpoints that the game actually drew and committed.
        anchor = active_anchor
        committed_bounds = (min(anchor[0], active_end[0]), min(anchor[1], active_end[1]),
                            max(anchor[0], active_end[0]), max(anchor[1], active_end[1]))
        run.set_keys(0)
        run.advance(1)
        after = capture(rsp, out / "selection-after.png")
        selected = run.selected()
        post_units = run.held_units()
        expected = sorted(u["index"] for u in post_units
                          if u["alive"] and u["owner"] == 0 and in_bounds(u, committed_bounds))
        if selected != expected:
            raise RuntimeError(f"selected friendly indices {selected} differ from bounds {expected}")
        if not selected:
            raise RuntimeError("box selection committed no living friendly units")
        if selected == selection_before_drag:
            raise RuntimeError("box selection did not change the prior friendly selection")

        # Start a second drag and cancel it with B; prior selected indices must
        # survive the canceled gesture unchanged.
        before_cancel = run.selected()
        run.press(KEY_L)
        run.press(KEY_L | KEY_RIGHT)
        run.advance(2)
        run.press(KEY_L | KEY_B)
        if read_int(rsp, sym["box_state"]) != 3:
            raise RuntimeError("B did not enter selection-box cancellation state")
        run.release()
        after_cancel = run.selected()
        if after_cancel != before_cancel:
            raise RuntimeError(f"B cancel changed selection: {before_cancel} -> {after_cancel}")

        # A stationary L tap must still cycle from a multi-selection to one
        # living friendly unit, with the cursor moved to that unit.
        old_units = run.held_units()
        old_selected = run.selected()
        if len(old_selected) > 1:
            first = min(old_selected)
            candidates = [u["index"] for u in old_units
                          if u["alive"] and u["owner"] == 0 and u["index"] > first]
            candidates += [u["index"] for u in old_units
                           if u["alive"] and u["owner"] == 0 and u["index"] <= first]
            expected_cycle = candidates[0] if candidates else None
        else:
            expected_cycle = None
        run.press(KEY_L)
        run.release()
        after_tap = run.selected()
        if len(after_tap) != 1 or after_tap == old_selected:
            raise RuntimeError(f"stationary L did not cycle selection: {old_selected} -> {after_tap}")
        if expected_cycle is not None and after_tap != [expected_cycle]:
            raise RuntimeError(f"L cycled to {after_tap}, expected {[expected_cycle]}")
        cycled = next(u for u in run.held_units() if u["index"] == after_tap[0])
        cycle_cursor = run.cursor()
        cycle_offset = (cycle_cursor[0] - cycled["x"], cycle_cursor[1] - cycled["y"])
        if abs(cycle_offset[0]) > 8 or abs(cycle_offset[1]) > 8:
            raise RuntimeError(f"L cycle cursor {cycle_cursor} is far from selected unit "
                               f"{(cycled['x'], cycled['y'])}")

        # Double-A: move to a nearby living Marine using ordinary cursor
        # movement (no R/group shortcut), then verify the first tap selects one
        # unit immediately and the second tap expands to the visible same-type
        # group. A remains held for several more updates to catch repeat bugs.
        camera_x = read_int(rsp, sym["camx"])
        camera_y = read_int(rsp, sym["camy"])
        visible_marines = [u for u in run.held_units()
                           if u["alive"] and u["owner"] == 0 and u["type"] == 1
                           and camera_x <= u["x"] < camera_x + SCREEN_W
                           and camera_y <= u["y"] < camera_y + 108]
        if not visible_marines:
            raise RuntimeError("no living friendly Marine is in the current viewport for double-A")
        cursor_before_a = run.cursor()
        target = min(visible_marines,
                     key=lambda u: (u["x"] - cursor_before_a[0]) ** 2 + (u["y"] - cursor_before_a[1]) ** 2)

        def snap_to_parity(value: int, residue: int) -> int:
            low = value - ((value - residue) % 2)
            high = low + 2
            if low < 0:
                return high
            if high > 1023:
                return low
            return low if abs(value - low) <= abs(high - value) else high

        target_x = snap_to_parity(target["x"], cursor_before_a[0] % 2)
        target_y = snap_to_parity(target["y"], cursor_before_a[1] % 2)
        drive_to(run, target_x, target_y, drag=False, fast=False)
        cursor_at_a = run.cursor()
        live_units = run.held_units()
        # Reidentify the unit after the navigation updates; units may move while
        # the cursor travels. Require the nearest friendly hit to be a Marine.
        in_hit_range = [u for u in live_units if u["alive"] and u["owner"] == 0 and
                        (u["x"] - cursor_at_a[0]) ** 2 + (u["y"] - cursor_at_a[1]) ** 2 < 31 * 31]
        if not in_hit_range:
            raise RuntimeError("cursor did not reach a friendly unit for the first A tap")
        expected_single = min(in_hit_range,
                              key=lambda u: (u["x"] - cursor_at_a[0]) ** 2 + (u["y"] - cursor_at_a[1]) ** 2)
        if expected_single["type"] != 1:
            raise RuntimeError(f"nearest friendly at double-A cursor is not a Marine: {expected_single}")

        run.press(KEY_A)
        first_tap_selection = run.selected()
        if first_tap_selection != [expected_single["index"]]:
            raise RuntimeError(f"first A tap did not immediately select Marine {expected_single['index']}: "
                               f"{first_tap_selection}")
        first_tap_cursor = run.cursor()
        first_tap_tick = run.ticks
        run.release()
        cursor_after_release = run.cursor()
        if cursor_after_release != first_tap_cursor:
            raise RuntimeError("cursor drifted between the two A taps")
        group_camera = (read_int(rsp, sym["camx"]), read_int(rsp, sym["camy"]))
        group_units_before = run.held_units()
        expected_group = sorted(u["index"] for u in group_units_before
                                if u["alive"] and u["owner"] == 0 and u["type"] == 1
                                and group_camera[0] <= u["x"] < group_camera[0] + SCREEN_W
                                and group_camera[1] <= u["y"] < group_camera[1] + 108)
        run.press(KEY_A)
        second_tap_tick = run.ticks
        double_a_selection = run.selected()
        if double_a_selection != expected_group:
            raise RuntimeError(f"double-A group {double_a_selection} differs from visible Marines "
                               f"{expected_group}")
        if double_a_selection == first_tap_selection or len(double_a_selection) < 2:
            raise RuntimeError(f"double-A did not expand the single selection: "
                               f"{first_tap_selection} -> {double_a_selection}")
        if any(u["owner"] != 0 or u["type"] != 1 for u in group_units_before
               if u["index"] in double_a_selection):
            raise RuntimeError("double-A selected a unit with a different owner or type")
        run.advance(3)
        held_a_selection = run.selected()
        if held_a_selection != double_a_selection:
            raise RuntimeError(f"holding A changed the double-A group: "
                               f"{double_a_selection} -> {held_a_selection}")
        double_a_path = out / "selection-double-a-group.png"
        double_a_capture = capture(rsp, double_a_path)
        run.release()

        result = {
            "rom_sha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
            "rom": str(rom), "elf": str(elf), "ticks": run.ticks,
            "cluster": {"unit_type": "Marine", "source_bounds": [min_x, min_y, max_x, max_y],
                        "initial_indices": [u["index"] for u in initial_units
                                            if u["alive"] and u["owner"] == 0 and u["type"] == 1]},
            "selection_bounds": list(committed_bounds), "anchor_world": list(anchor),
            "camera_start": list(start_camera), "camera_end": list(end_camera),
            "camera_moved": start_camera != end_camera,
            "selected_friendly_indices": selected,
            "selection_before_drag": selection_before_drag,
            "expected_friendly_indices_from_bounds": expected,
            "box_cancel_selection_before": before_cancel,
            "box_cancel_selection_after": after_cancel,
            "tap_l_selection_before": old_selected,
            "tap_l_selection_after": after_tap,
            "tap_l_expected_index": expected_cycle,
            "tap_l_cursor": list(cycle_cursor),
            "tap_l_selected_unit_position": [cycled["x"], cycled["y"]],
            "tap_l_cursor_offset": list(cycle_offset),
            "double_a": {
                "target_index": expected_single["index"],
                "cursor": list(cursor_at_a),
                "cursor_after_first_tap": list(first_tap_cursor),
                "cursor_after_release": list(cursor_after_release),
                "cursor_drift": [cursor_after_release[0] - first_tap_cursor[0],
                                 cursor_after_release[1] - first_tap_cursor[1]],
                "first_tap_selection": first_tap_selection,
                "tap_tick_delta": second_tap_tick - first_tap_tick,
                "viewport": [group_camera[0], group_camera[1], SCREEN_W, 108],
                "expected_visible_same_type_indices": expected_group,
                "selected_indices": double_a_selection,
                "selected_while_held": held_a_selection,
                "capture_path": str(double_a_path),
                "capture": double_a_capture,
            },
            "active_outline": {"path": str(active_path), "state": active_state,
                               "highlight_pixels_before": base_highlights,
                               "highlight_pixels_active": active_highlights,
                               "capture": active},
            "captures": {"before": str(out / "selection-before.png"),
                         "after": str(out / "selection-after.png"),
                         "after_capture": after, "before_capture": base},
            "checks": {"outline_visible": True, "anchor_stable": True,
                       "camera_scrolled": True, "bounds_match_selection": True,
                       "b_cancel_preserved_selection": True, "tap_l_cycles": True,
                       "double_a_same_type_viewport_group": True,
                       "double_a_preserved_while_held": True},
            "input_write_symbol": hex(sym["demo_inject_keys"]),
        }
        (out / "selection-smoke.json").write_text(json.dumps(result, indent=2) + "\n")
        return result
    finally:
        if sock:
            sock.close()
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        log.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=pathlib.Path, default=ROOT / "build" / "StarCraft64-Wasteland-GBA.gba")
    parser.add_argument("--elf", type=pathlib.Path, default=ROOT / "build" / "starcraft64.elf")
    parser.add_argument("--out", type=pathlib.Path, default=ROOT / "build" / "emulator" / "selection-smoke")
    args = parser.parse_args()
    try:
        result = validate(args.rom, args.elf, args.out)
    except Exception as exc:
        print(f"selection smoke failed: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
