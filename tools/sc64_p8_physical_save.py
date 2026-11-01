# SPDX-License-Identifier: GPL-3.0-or-later
"""Fail-closed parser for the neutral P8 Transport1 physical replay cut.

This deliberately supports only a fresh single-slot save with no Yamato shots,
no pending impacts, and a zero campaign-transport trailer. It is not a general
RaceSave validator. Layout is supplied from the exact paired ELF's DWARF.
"""
import hashlib
import struct

BASE_TAG = 0x7343505c
TRANSPORT_TAG = 0x7743505c
SLOT_SIZE = 16384
SRAM_SIZE = 2 * SLOT_SIZE
SHOT_BYTES = 20
SHOT_LIMIT = 2
TRAILER_BYTES = 16
P8_MISSION = 27


def validate_neutral_journal(data, layout, expected_frame, *, medium='physical'):
    """Validate a fresh neutral cut and return the exact observed journal facts.

    ``physical`` requires the untouched second flash slot to be erased; ``host``
    admits the host harness's separately initialized all-zero second slot only.
    The caller must bind expected_frame to an observed save action, rather than
    equating it to a later post-update trace frame.
    """
    if medium not in ('physical', 'host'):
        raise ValueError('unknown SRAM medium')
    if type(expected_frame) is not int or expected_frame < 0:
        raise ValueError('invalid expected saved frame')
    if len(data) != SRAM_SIZE:
        raise ValueError('SRAM image must be exactly 32768 bytes')
    erased = b'\xff' if medium == 'physical' else b'\x00'
    if data[SLOT_SIZE:] != erased * SLOT_SIZE:
        raise ValueError('second SRAM slot is not pristine for selected medium')
    size, fields = layout
    if type(size) is not int or size != 16332:
        raise ValueError('unexpected P8 RaceSave layout size')
    if fields.widths.get('version') != 4 or fields.arrays.get('state') != (32, 4, True):
        raise ValueError('invalid RaceSave version or state layout')
    version, state = fields['version'], fields['state']
    base = size - SHOT_BYTES * SHOT_LIMIT
    if min(version, state) < 0 or version + 4 > base or state + 128 > base:
        raise ValueError('RaceSave fields exceed fixed base')
    tag, length, checksum, sequence = struct.unpack_from('<4I', data)
    if (tag, length, sequence) != (TRANSPORT_TAG, base + TRAILER_BYTES, 1):
        raise ValueError('unsupported P8 fresh neutral journal envelope')
    payload = data[16:16 + length]
    h = 2166136261
    for byte in payload:
        h = ((h ^ byte) * 16777619) & 0xffffffff
    if h != checksum:
        raise ValueError('P8 journal checksum differs')
    if struct.unpack_from('<I', payload, version)[0] != BASE_TAG:
        raise ValueError('P8 inner version differs')
    words = struct.unpack_from('<32I', payload, state)
    if (words[0], words[2], words[7]) != (0, P8_MISSION, expected_frame):
        raise ValueError('P8 mode, mission, or saved frame differs')
    if words[27] & 0xffff0000 != 0x42450000:
        raise ValueError('P8 battlecruiser energy marker differs')
    bc = words[28]
    if bc & ~31 != 0x42440000 or bc & 7 > 4 or bc & 24:
        raise ValueError('P8 shot marker invalid or active Yamato shots unsupported')
    # No variable shots and length exactly base+trailer proves no impact bytes.
    if payload[base:] != bytes(TRAILER_BYTES):
        raise ValueError('nonzero campaign transport trailer unsupported')
    return {'slot': 0, 'tag': tag, 'inner_version': BASE_TAG,
            'payload_bytes': length, 'sequence': sequence, 'checksum': checksum,
            'frame': expected_frame, 'yamato_shots': 0, 'impact_bytes': 0,
            'medium': medium, 'sha256': hashlib.sha256(data).hexdigest()}
