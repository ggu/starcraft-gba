"""Compile a narrow, original IScript frame-stream whitelist.

It emits a timeline descriptor, not pixels,
and does not implement StarCraft's IScript virtual machine. Unsupported control
flow and instructions fail closed. Frame identities can be source-derived;
GBA timing remains an adaptation of source update waits.
"""

# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

from typing import Any

MAX_INSTRUCTIONS = 256
MAX_STEPS = 64
HOLD_LAST = 0xFF
MAX_LAYERS = 8
MAX_DEPTH = 3


def compile_primary_frame_stream(
    data: bytes,
    start: int,
    frame_count: int,
    initial_frame: int = 0,
) -> dict[str, Any]:
    """Compile supported primary-image IScript operations into bounded steps.

    Supported operations are playfram (0), wait/randwait (5/6), goto (7), imgol (8),
    imgul (9), sigorder (36), and end (22). Image-layer operations are recorded
    in ``omitted_layers`` and omitted from output because this is a primary-frame
    sampler. ``sigorder`` values are recorded as metadata only; they have no
    gameplay effect here.
    """
    if not isinstance(data, bytes):
        raise ValueError("script data must be bytes")
    if not isinstance(start, int) or start < 0 or start >= len(data):
        raise ValueError("start offset is outside the script payload")
    if not isinstance(frame_count, int) or frame_count <= 0 or frame_count > 255:
        raise ValueError("frame_count must be in 1..255")
    if not isinstance(initial_frame, int) or not 0 <= initial_frame < frame_count:
        raise ValueError("initial_frame is outside the frame bank")

    pc = start
    current_frame = initial_frame
    steps: list[dict[str, int]] = []
    omitted_layers: list[dict[str, int]] = []
    signals: list[int] = []
    # The number of output waits seen at the first visit identifies the point
    # where the repeated instruction/frame state starts its looping timeline.
    first_state_step: dict[tuple[int, int], int] = {}
    def need(n: int) -> None:
        if pc + n > len(data):
            raise ValueError(f"truncated instruction operand at offset {pc}")

    def append_wait(duration: int) -> None:
        if duration <= 0:
            raise ValueError("wait duration must be positive")
        if len(steps) >= MAX_STEPS:
            raise ValueError(f"timeline exceeds {MAX_STEPS} steps")
        steps.append({"frame": current_frame,
                      "duration_source_updates": duration})

    for _instruction_index in range(MAX_INSTRUCTIONS):
        if pc >= len(data):
            raise ValueError("script falls off the payload without end or goto")
        state = (pc, current_frame)
        if state in first_state_step:
            loop_start = first_state_step[state]
            if len(steps) == loop_start:
                raise ValueError("instantaneous instruction loop has no wait")
            if loop_start >= len(steps):
                raise ValueError("loop has no frame duration")
            return {
                "steps": steps,
                "loop_start": loop_start,
                "frame_count": frame_count,
                "omitted_layers": omitted_layers,
                "signals": signals,
            }
        first_state_step[state] = len(steps)

        opcode = data[pc]
        pc += 1
        if opcode == 0:  # playfram u16
            need(2)
            frame = int.from_bytes(data[pc:pc + 2], "little")
            pc += 2
            if frame >= frame_count:
                raise ValueError(f"playfram index {frame} outside frame bank")
            current_frame = frame
        elif opcode == 5:  # wait u8; N means N+1 source updates
            need(1)
            wait = data[pc]
            pc += 1
            duration = wait + 1
            append_wait(duration)
        elif opcode == 6:  # randwait: bounded visual timing uses its midpoint
            need(2)
            low, high = data[pc], data[pc + 1]
            pc += 2
            append_wait((low + high) // 2 + 1)
        elif opcode == 7:  # goto u16 absolute payload offset
            need(2)
            target = int.from_bytes(data[pc:pc + 2], "little")
            pc += 2
            if target >= len(data):
                raise ValueError(f"goto target {target} outside script payload")
            pc = target
        elif opcode in (8, 9):  # imgol/imgul: image u16, x/y s8
            need(4)
            image_id = int.from_bytes(data[pc:pc + 2], "little")
            x = int.from_bytes(data[pc + 2:pc + 3], "little", signed=True)
            y = int.from_bytes(data[pc + 3:pc + 4], "little", signed=True)
            pc += 4
            omitted_layers.append({
                "opcode": opcode,
                "image_id": image_id,
                "x_offset": x,
                "y_offset": y,
            })
        elif opcode == 36:  # sigorder u8, metadata only
            need(1)
            signals.append(data[pc])
            pc += 1
        elif opcode == 41:  # move speed; no effect on a static construction image
            need(1)
            pc += 1
        elif opcode == 23:  # setflipstate needs a renderer flip timeline
            raise ValueError("setflipstate is not represented in the source timeline")
        elif opcode == 56:  # creategasoverlays: dynamic resource overlay metadata
            need(1)
            pc += 1
        elif opcode == 22:  # end: freeze current frame
            if not steps:
                append_wait(1)
            elif steps[-1]["frame"] != current_frame:
                append_wait(1)
            return {
                "steps": steps,
                "loop_start": HOLD_LAST,
                "frame_count": frame_count,
                "omitted_layers": omitted_layers,
                "signals": signals,
            }
        else:
            raise ValueError(f"unsupported IScript opcode {opcode} at {pc - 1}")

        if len(steps) > MAX_STEPS:
            raise ValueError(f"timeline exceeds {MAX_STEPS} steps")
    raise ValueError(f"script exceeds {MAX_INSTRUCTIONS} instructions")


def compile_source_layers(data: bytes, starts: dict[int, int], root_image: int,
                         decode_instruction, sprite_images: list[int],
                         image_starts: dict[tuple[int, str], int],
                         frame_counts: dict[int, int]) -> list[dict[str, Any]]:
    """Compile source animation plus bounded spawned image/sprite Init layers.

    The result is a visual schedule in source-update ticks. It models the
    image creation opcodes used by the imported building animations, not the
    general IScript VM. Unsupported control flow fails closed.
    """
    over, under = {8, 10, 13}, {9, 11, 14}
    sprite_over, sprite_under = {15, 16, 21}, {17, 19, 20}
    lo_offsets, skip = {13, 14, 19, 21}, {24, 25, 26, 36, 41, 52, 63}
    layers: list[dict[str, Any]] = []

    def walk(image: int, start: int, x: int, y: int, is_under: bool,
             depth: int, layer_start: int):
        if depth > MAX_DEPTH:
            raise ValueError("spawned image nesting exceeds layer depth limit")
        if len(layers) >= MAX_LAYERS:
            raise ValueError("spawned image count exceeds layer limit")
        layer = {"image_id": image, "x_offset": x, "y_offset": y,
                 "under": is_under, "start_source_updates": layer_start,
                 "follows_main_graphic": False, "gas_overlay_requests": [],
                 "program": None}
        layers.append(layer)
        pc, frame, elapsed = start, 0, 0
        steps: list[dict[str, int]] = []
        seen: dict[tuple[int, int], int] = {}
        loop_start = HOLD_LAST
        for _ in range(MAX_INSTRUCTIONS):
            state = (pc, frame)
            if state in seen:
                if seen[state] == len(steps):
                    raise ValueError("instantaneous IScript loop")
                loop_start = seen[state]
                break
            seen[state] = len(steps)
            opcode, args, next_pc, jump = decode_instruction(data, pc)
            pc = next_pc
            if opcode == 0:
                frame = args[0]
            elif opcode in (5, 6):
                duration = args[0] + 1 if opcode == 5 else (args[0] + args[1]) // 2 + 1
                if len(steps) >= MAX_STEPS:
                    raise ValueError(f"timeline exceeds {MAX_STEPS} steps")
                steps.append({"frame": frame, "duration_source_updates": duration})
                elapsed += duration
            elif opcode == 7:
                pc = jump
            elif opcode in over | under:
                child = args[0]
                ox, oy = (0, 0) if opcode in lo_offsets else (args[1], args[2])
                child_start = image_starts.get((child, "Init"))
                if child_start is None:
                    raise ValueError(f"spawned image {child} lacks Init animation")
                walk(child, child_start, x + _signed_byte(ox), y + _signed_byte(oy),
                     opcode in under, depth + 1, layer_start + elapsed)
            elif opcode == 61:
                child = image + 1
                child_start = image_starts.get((child, "Init"))
                if child_start is None:
                    raise ValueError(f"implicit spawned image {child} lacks Init animation")
                walk(child, child_start, x + _signed_byte(args[0]),
                     y + _signed_byte(args[1]), True, depth + 1, layer_start + elapsed)
            elif opcode == 29:  # followmaingraphic: parent frame/flip linkage
                layer["follows_main_graphic"] = True
            elif opcode == 41:  # move speed; static building images do not translate
                pass
            elif opcode == 23:  # setflipstate; fail rather than lose flip semantics
                raise ValueError("setflipstate is not represented in the source timeline")
            elif opcode == 56:  # creategasoverlays uses a signed LOS selector
                layer["gas_overlay_requests"].append({
                    "selector": _signed_byte(args[0]),
                    "start_source_updates": layer_start + elapsed,
                })
            elif opcode in sprite_over | sprite_under:
                sprite = args[0]
                if not 0 <= sprite < len(sprite_images):
                    raise ValueError(f"spawned sprite {sprite} is out of range")
                child = sprite_images[sprite]
                child_start = image_starts.get((child, "Init"))
                if child_start is None:
                    raise ValueError(f"spawned sprite image {child} lacks Init animation")
                ox, oy = (0, 0) if opcode in lo_offsets else (args[1], args[2])
                walk(child, child_start, x + _signed_byte(ox), y + _signed_byte(oy),
                     opcode in sprite_under, depth + 1, layer_start + elapsed)
            elif opcode == 22:
                if not steps:
                    steps.append({"frame": frame, "duration_source_updates": 1})
                break
            elif opcode in skip:
                pass
            else:
                raise ValueError(f"unsupported IScript opcode {opcode} at {pc - 1}")
        else:
            raise ValueError(f"script exceeds {MAX_INSTRUCTIONS} instructions")
        if not steps:
            raise ValueError("script draws no frame")
        frame_count = frame_counts[image]
        if any(step["frame"] >= frame_count for step in steps):
            raise ValueError(f"image {image} animation frame outside frame bank")
        layer["program"] = {"steps": steps, "loop_start": loop_start,
                            "frame_count": frame_count}

    walk(root_image, starts[root_image], 0, 0, False, 0, 0)
    return layers


def _signed_byte(value: int) -> int:
    return value - 256 if value > 127 else value


__all__ = ["compile_primary_frame_stream", "HOLD_LAST", "MAX_INSTRUCTIONS",
           "MAX_STEPS", "compile_source_layers"]
