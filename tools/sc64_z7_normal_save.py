# SPDX-License-Identifier: GPL-3.0-or-later
"""Ordinary pause-save and fresh-title continuation for Z7 input tapes."""
import json
import struct

import sc64_z7_arm_replay as z7

import sc64_p8_normal_replay as p8

START, B, L = 8, 2, 512
# Canonical Z7 Egg profile only: SC64_RACE_SAVE_GENERATION=36 in game.c.
# The tag is a profile constraint, not a value inferred from DWARF.
CANONICAL_Z7_EGG_TAG = 0x53435a54


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


def resume_inputs(transitions, cut, load_updates=2):
    """Load from title and map the remaining one-based host inputs.

    Physical input reaches the restored state after one title update. The next
    update must simultaneously release L and execute host cut+1. Scheduler
    transport retains its historical two-update convention pending its own
    runtime qualification.
    """
    if load_updates not in (1, 2):
        raise ValueError('unsupported load update count')
    previous = 0
    for tick, held in transitions:
        if tick > cut: break
        previous = held
    if load_updates == 1:
        if previous: raise ValueError('physical load requires neutral cut')
        following = [(tick-cut, held) for tick,held in transitions if tick > cut]
        if following and following[0][0] == 1:
            if following[0][1] & L:
                raise ValueError('first physical resume input cannot hold L')
            return [(0,L)] + following
        return [(0,L),(1,0)] + following
    inputs = [(0, L), (1, 0)]
    if previous: inputs.append((2, previous))
    inputs.extend((2 + tick - 1 - cut, held) for tick, held in transitions if tick > cut)
    if len(inputs) > 3 and inputs[2][0] == inputs[3][0]: del inputs[2]
    return inputs


MODEL_FIELDS = ('started', 'outcome', 'switch10', 'ai_script_mask', 'ai_plan_ticks',
                'ai_plan_phase', 'fault', 'action_cursors', 'action_ticks')


def validate_journal(data, layout, frame):
    """Validate the fresh Z7 journal; common/gba.c has no header checksum."""
    size, fields = layout
    if type(size) is not int or not 0 < size <= 16368:
        raise ValueError('invalid ELF RaceSave size')
    if fields.widths.get('version') != 4 or fields.arrays.get('state') != (32, 4, True):
        raise ValueError('invalid ELF RaceSave version or state layout')
    version, state = fields['version'], fields['state']
    if version < 0 or version + 4 > size or state < 0 or state + 128 > size:
        raise ValueError('ELF RaceSave fields exceed payload')
    if len(data) != 32768:
        raise ValueError('SRAM image must be exactly 32768 bytes')
    # Refuse an ambiguous newer (or partially written) slot, even if slot 0 is valid.
    if data[16384:] != b'\xff' * 16384:
        raise ValueError('second SRAM slot is not erased')
    # Fresh title starts with blank SRAM, so a successful first save is slot 0, sequence 1.
    tag, length, checksum, sequence = struct.unpack_from('<4I', data)
    if (tag, length, sequence) != (CANONICAL_Z7_EGG_TAG, size, 1):
        raise ValueError('invalid fresh Z7 journal header')
    payload = data[16:16+size]
    h = 2166136261
    for byte in payload:
        h = ((h ^ byte) * 16777619) & 0xffffffff
    if h != checksum:
        raise ValueError('Z7 journal payload checksum differs')
    if struct.unpack_from('<I', payload, version)[0] != tag:
        raise ValueError('Z7 payload version differs from tag')
    mode, mission, saved_frame = (struct.unpack_from('<i', payload, state + 4*i)[0]
                                   for i in (0, 2, 7))
    if (mode, mission, saved_frame) != (0, z7.MISSION, frame):
        raise ValueError('Z7 payload mission or frame differs')
    return {'slot': 0, 'tag': tag, 'payload_bytes': size, 'sequence': sequence,
            'checksum': checksum, 'frame': saved_frame}


def saved_summary(rsp, sym, sizes, elf, cut, snapshot):
    observed = snapshot(rsp, sym, sizes)
    return {'cut': cut, 'mission': int.from_bytes(rsp.read(sym['race_selected_mission'], 4), 'little'),
            'world': p8.persisted_summary(rsp, sym, sizes, elf),
            'model': {key: observed[key] for key in MODEL_FIELDS},
            'observation': {key: observed[key] for key in z7.SCALARS + z7.LISTS}}


def verify_cut_summary(sample, reference, cut, screen):
    """Bind all retained cut observations to the saved or restored sample."""
    if not isinstance(sample, dict) or sample.get('cut') != cut or sample.get('mission') != z7.MISSION:
        raise ValueError('saved summary has wrong cut or mission')
    observed = sample.get('observation')
    world = sample.get('world')
    model = sample.get('model')
    if not isinstance(observed,dict) or not isinstance(world,dict) or not isinstance(model,dict):
        raise ValueError('saved summary is incomplete')
    for key in z7.SCALARS + z7.LISTS:
        expected = screen if key == 'screen' else reference[key]
        if observed.get(key) != expected:
            raise RuntimeError(f'saved cut observation differs: {key}')
    if world.get('frame') != reference['frame'] or world.get('screen') != screen:
        raise RuntimeError('saved world frame or screen differs from cut')
    for key in MODEL_FIELDS:
        if model.get(key) != reference[key]:
            raise RuntimeError(f'saved Z7 model differs from cut: {key}')
    for world_key, observed_keys in (('units', ('side_units','enemy_units')),
                                     ('buildings', ('side_buildings','enemy_buildings'))):
        count=world.get(world_key)
        if type(count) is not int or any(count < reference[key] for key in observed_keys):
            raise RuntimeError(f'saved world {world_key} count differs from cut')


def verify_provenance(sample, provenance):
    if sample.get('provenance') != provenance:
        raise ValueError('saved summary provenance differs from replay inputs or seed')


def verify_restored(saved, restored, cut, mission):
    if not isinstance(saved, dict) or saved.get('cut') != cut or saved.get('mission') != mission:
        raise ValueError('saved summary has wrong cut or mission')
    if not isinstance(restored, dict) or restored.get('cut') != cut or restored.get('mission') != mission:
        raise RuntimeError('restored mission differs')
    if saved.get('world', {}).get('screen') != 2 or restored.get('world', {}).get('screen') != 1:
        raise RuntimeError('save/load did not transition pause to play')
    p8.verify_restored(saved['world'], restored['world'])
    for sample in (saved, restored):
        model = sample.get('model')
        if not isinstance(model, dict) or set(model) != set(MODEL_FIELDS):
            raise ValueError('invalid Z7 model summary')
        for field in MODEL_FIELDS:
            value = model[field]
            if field in ('action_cursors', 'action_ticks'):
                if not isinstance(value, list) or len(value) != 8 or any(type(n) is not int for n in value):
                    raise ValueError('invalid Z7 action array')
            elif type(value) is not int: raise ValueError('invalid Z7 model scalar')
    if restored['model'] != saved['model']:
        raise RuntimeError('restored Z7 model differs from saved sample')


def read_summary(path):
    return json.loads(path.read_text())
