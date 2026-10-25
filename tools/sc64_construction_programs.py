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


def compile_primary_frame_stream(
    data: bytes,
    start: int,
    frame_count: int,
    initial_frame: int = 0,
) -> dict[str, Any]:
    """Compile supported primary-image IScript operations into bounded steps.

    Supported operations are playfram (0), wait (5), goto (7), imgol (8),
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


__all__ = ["compile_primary_frame_stream", "HOLD_LAST", "MAX_INSTRUCTIONS",
           "MAX_STEPS"]
