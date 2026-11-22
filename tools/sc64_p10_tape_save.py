# SPDX-License-Identifier: GPL-3.0-or-later
"""Input alignment and timing for normal P10 cartridge pause-save replays."""
from sc64_skirmish_sram_continuation import KEY_B, KEY_START


def save_window(plan, stop):
    record = plan['save']
    if plan['profile'] != 'PROTOSS_P10_FLAGS' or record is None:
        raise ValueError('fresh SRAM requires the normal P10 tape with a pause-save record')
    first = record['first_call']
    end = record['resume_call']
    transitions = dict(plan['transitions'])
    expected = (KEY_START, 0, KEY_B, 0, KEY_START, 0)
    if (not 0 < first < end <= stop or end != first + len(expected) or
            tuple(transitions.get(first + i) for i in range(len(expected))) != expected or
            record['screen'] != 1 or record['menu_last'] != 0):
        raise ValueError('save record is not a neutral ordinary pause, save and resume')
    return first, first + 4


def join_timing(prefix, tail):
    if prefix is None or tail is None:
        return None
    updates = prefix['updates'] + tail['updates']
    cycles = prefix['guest_cycles'] + tail['guest_cycles']
    return dict(tail, updates=updates, guest_cycles=cycles, mean_cycles=cycles / updates,
                effective_main_loop_hz=16777216 * updates / cycles,
                peak_since_boot=max(prefix['peak_since_boot'], tail['peak_since_boot']),
                vblank_buckets=[a + b for a, b in zip(prefix['vblank_buckets'], tail['vblank_buckets'])])
