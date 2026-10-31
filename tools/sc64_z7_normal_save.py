# SPDX-License-Identifier: GPL-3.0-or-later
"""Ordinary pause-save and fresh-title continuation for Z7 input tapes."""
import json

import sc64_p8_normal_replay as p8

START, B, L = 8, 2, 512


def validate_cut(plan, cut):
    if type(cut) is not int or not 0 < cut < plan['stop'] or cut not in plan['observations']:
        raise ValueError('save cut must be a nonterminal host observation')
    row = plan['observations'][cut]
    if row['screen'] != 1 or row['won'] or row['lost'] or row['fault']:
        raise ValueError('save cut must be fault-free play')
    held = 0
    for tick, value in plan['transitions']:
        if tick > cut:
            break
        held = value
    if held:
        raise ValueError('save cut requires neutral held keys')


def save_inputs(cut):
    # Scheduler ticks are zero based; the checkpoint at cut is after cut updates.
    return [(cut, START), (cut + 1, 0), (cut + 2, B), (cut + 3, 0)]


def resume_inputs(transitions, cut):
    """Load on title, release L, then replay transitions after the cut."""
    previous = 0
    for tick, held in transitions:
        if tick > cut:
            break
        previous = held
    inputs = [(0, L), (1, 0)]
    if previous:
        inputs.append((2, previous))
    inputs.extend((2 + tick - 1 - cut, held) for tick, held in transitions if tick > cut)
    # A transition on the first resumed update supersedes re-establishment.
    if len(inputs) > 3 and inputs[2][0] == inputs[3][0]:
        del inputs[2]
    return inputs


def saved_summary(rsp, sym, sizes, elf, cut):
    return {'cut': cut, 'mission': int.from_bytes(rsp.read(sym['race_selected_mission'], 4), 'little'),
            'world': p8.persisted_summary(rsp, sym, sizes, elf)}


def verify_restored(saved, restored, cut, mission):
    if not isinstance(saved, dict) or saved.get('cut') != cut or saved.get('mission') != mission:
        raise ValueError('saved summary has wrong cut or mission')
    if not isinstance(restored, dict) or restored.get('cut') != cut or restored.get('mission') != mission:
        raise RuntimeError('restored mission differs')
    if saved.get('world', {}).get('screen') != 2 or restored.get('world', {}).get('screen') != 1:
        raise RuntimeError('save/load did not transition pause to play')
    p8.verify_restored(saved['world'], restored['world'])


def read_summary(path):
    return json.loads(path.read_text())
