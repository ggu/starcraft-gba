#!/usr/bin/env python3
"""Convert StarCraft 64 sound effects and music into a private GBA audio bank.

The user-supplied ROM and every converted sample stay under .local/. Output is
a blob of losslessly compressed 8-bit signed PCM streams, a small index header
and a manifest; none may be committed or distributed.

Source format (verified against the USA retail image):
  * BOLT 014/000 is a 1144-entry table, indexed like PC sfxdata ids: a count,
    1144 big-endian blob offsets, a zero word and 1144 byte lengths. The blob
    is the unlisted tail of the BOLT archive. Every sound starts with eight
    zero bytes (initial decoder state) followed by mono IMA ADPCM, high nibble
    first, at 11025 Hz.
  * BOLT 015/000-00F are music streams of the same codec, each with a 12-byte
    header: format byte 2, 0x10, big-endian rate 11025, big-endian data length
    and four zero bytes.
  * The advisor refusals are sfxdata ids 146-154: "not enough minerals", then
    "not enough vespene gas", then "supply limit", each for Zerg, Terran and
    Protoss in that order (identified by transcribing the decoded PCM).
  * The resident N64 code selects music resource 0x1500+track. Track 0 is the
    menu theme; the in-game themes alternate per player race (Terran 4/5,
    Zerg 8/9, Protoss 12/13); the mission-end stingers are 2/3, 6/7 and
    10/11, chosen by the result flag.

Output format ("SC64 audio codec format 2", decoded by common/audio_codec.h):
  * Signed 8-bit mono PCM at 10512 Hz. Each SFX clip is zero-padded to a
    multiple of 8 samples; each music track to whole 176-sample frames.
  * Every clip and track is one independent stream at a 4-byte-aligned blob
    offset, followed after the last stream by a SAMPLES_PER_FRAME + 32 byte
    zero guard. The header records byte offsets and SAMPLE COUNTS (clips) or
    176-sample frame counts (music).
  * A stream is a bit string stored LSB-first in little-endian halfwords and
    zero-padded to a multiple of 4 bytes: a 2-bit predictor P (0: x1,
    1: 2*x1 - x2, 2: (3*x1 - x2) >> 1; x1 and x2 start at 0), then blocks of
    8 samples. Each block has a width code relative to the previous width
    (0: same; 1,0: one less; 1,1,0: one more; 1,1,1 + 4 bits: explicit 0-8)
    followed by eight k-bit two's complement residuals r, where
    x = (s8)(pred + r). The encoder picks per stream the allowed predictor
    with the fewest bits (by default only the linear P 0 and 1, which decode
    cheapest on the ARM; --predictors 012 also tries P 2), the minimal k per
    block and the shortest width code. Decoders accept all three predictors.
  * The manifest records the format, encoded and PCM sizes, and the sha256 of
    every stream's decoded PCM in blob order. --verify-bank rechecks a bank
    with the reference decoder; --encode-pcm and --decode-stream expose the
    codec for ROM-free cross checks.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.sc64_construction_sprites import (  # noqa: E402
    animation_instructions, image_animation_starts, le_array, load_sprites, script_index,
)

UPSTREAM = ROOT / "references/sc64-maps/extract_sc64_maps.py"
ROM_DEFAULT = ROOT / ".local/sc64/rom/StarCraft64-USA.z64"
OUT_DEFAULT = ROOT / ".local/sc64/audio/generated"
EXPECTED_ROM_SHA1 = "a36d91b5aac7bb95df0d7de5f9814f5270e25c2b"
SOUND_TABLE = "014/000"
SOUND_COUNT = 1144
SOUND_BLOB_ROM_OFFSET = 0x1A516B0
SOUND_BLOB_LENGTH = 0x5603C2
SOURCE_RATE = 11025
# 280896 CPU cycles per 59.73 Hz frame / 176 samples = exactly 1596 cycles,
# so one mix buffer is consumed per video frame without drift.
OUTPUT_RATE = 10512
SAMPLES_PER_FRAME = 176
UNIT_COUNT = 228
VOICED_UNITS = 106
FLINGY_COUNT = 209
SPRITE_COUNT = 517
IMAGE_COUNT = 999
WEAPON_COUNT = 130
MAX_SET = 4
MUSIC_TRACKS = 16
# Advisor refusal sfxdata ids in the game's race order (Terran, Zerg, Protoss),
# each minerals, gas, supply. Transcribed from the decoded clips:
#   147 "Not enough minerals"   150 "Insufficient vespene gas"
#   153 "Additional supply depots required"
#   146 "We require more minerals"   149 "We require more vespene gas"
#   152 "Spawn more Overlords"
#   148 "You have not enough minerals"   151 "You require more vespene gas"
#   154 "You must construct additional pylons"
ADVISOR_LINES = ("minerals", "gas", "supply")
ADVISOR_SOUNDS = ((147, 150, 153), (146, 149, 152), (148, 151, 154))
# Roles come from the resident N64 music selector (see the module comment).
MUSIC_ROLES = {
    0: "menu", 1: "unused-1", 2: "terran-result-a", 3: "terran-result-b",
    4: "terran-1", 5: "terran-2", 6: "zerg-result-a", 7: "zerg-result-b",
    8: "zerg-1", 9: "zerg-2", 10: "protoss-result-a", 11: "protoss-result-b",
    12: "protoss-1", 13: "protoss-2", 14: "unused-14", 15: "unused-15",
}
MUSIC_DEFAULT = (0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)
# DMA sound reads ahead up to one frame plus the 16-byte FIFO after the last
# mixed frame; a zero guard after the last stream keeps any overshoot silent.
GUARD_BYTES = SAMPLES_PER_FRAME + 32
SFX_GAIN = 1.0
MUSIC_GAIN = 0.85

IMA_STEPS = (
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767,
)
IMA_INDEX = (-1, -1, -1, -1, 2, 4, 6, 8) * 2
PLAYSND, PLAYSNDRAND, PLAYSNDBTWN, ATTACKMELEE = 24, 25, 26, 28
SPRITE_OPS = {15, 16, 17, 19, 20, 21}
IMAGE_OPS = {8, 9, 10, 11, 13, 14}
ATTACK_ANIMATIONS = ("GndAttkInit", "AirAttkInit", "GndAttkRpt", "AirAttkRpt")


def ima_decode(data: bytes) -> list[int]:
    """Decode high-nibble-first IMA ADPCM from a zero predictor and index."""
    predictor = 0
    index = 0
    out = []
    append = out.append
    for byte in data:
        for nibble in (byte >> 4, byte & 15):
            step = IMA_STEPS[index]
            diff = step >> 3
            if nibble & 4:
                diff += step
            if nibble & 2:
                diff += step >> 1
            if nibble & 1:
                diff += step >> 2
            predictor = predictor - diff if nibble & 8 else predictor + diff
            if predictor > 32767:
                predictor = 32767
            elif predictor < -32768:
                predictor = -32768
            index += IMA_INDEX[nibble]
            index = 0 if index < 0 else 88 if index > 88 else index
            append(predictor)
    return out


def resample_to_s8(samples: list[int], gain: float) -> bytes:
    """Linearly resample 11025 Hz PCM to the GBA mix rate as signed 8-bit."""
    if not samples:
        return b""
    count = (len(samples) * OUTPUT_RATE) // SOURCE_RATE
    out = bytearray(count)
    last = len(samples) - 1
    scale = gain / 256.0
    for i in range(count):
        position = i * SOURCE_RATE / OUTPUT_RATE
        left = int(position)
        frac = position - left
        right = left + 1 if left < last else last
        value = (samples[left] * (1.0 - frac) + samples[right] * frac) * scale
        value = int(value + (0.5 if value >= 0 else -0.5))
        value = 127 if value > 127 else -128 if value < -128 else value
        out[i] = value & 0xFF
    return bytes(out)


def parse_sound_table(table: bytes) -> tuple[list[int], list[int]]:
    words = struct.unpack(f">{len(table) // 4}I", table)
    if len(words) != 2 * SOUND_COUNT + 2 or words[0] != SOUND_COUNT or words[SOUND_COUNT + 1]:
        raise ValueError("unexpected StarCraft 64 sound table layout")
    offsets = list(words[1:SOUND_COUNT + 1])
    lengths = list(words[SOUND_COUNT + 2:])
    for offset, length in zip(offsets, lengths):
        if offset + length > SOUND_BLOB_LENGTH or (length and length < 8):
            raise ValueError("sound table entry outside the sound blob")
    return offsets, lengths


def sound_ids(instructions: Iterable[tuple[int, list]]) -> list[int]:
    found = []
    for opcode, args in instructions:
        if opcode == PLAYSND:
            found.append(args[0])
        elif opcode in (PLAYSNDRAND, ATTACKMELEE):
            found.extend(args[-1])
        elif opcode == PLAYSNDBTWN:
            found.extend(range(args[0], args[1] + 1))
    return found


# --- SC64 audio codec format 2 -------------------------------------------

CODEC_FORMAT = 2
CODEC_BLOCK = 8
CODEC_PREDICTORS = (0, 1, 2)
# P 2 costs about 50% more Thumb instructions per decoded sample on the GBA
# than P 0/1 and saves only about 70 KB on the real bank, so it is opt-in.
DEFAULT_PREDICTORS = (0, 1)


def _residual_width(byte: int) -> int:
    """Minimal two's complement width (0-8) holding a wrapped residual byte."""
    value = byte - 256 if byte & 0x80 else byte
    if value == 0:
        return 0
    width = 1
    while not -(1 << (width - 1)) <= value < (1 << (width - 1)):
        width += 1
    return width


_FLIP_SIGN = bytes(b ^ 0x80 for b in range(256))
# A block's width is the maximum of its residual widths; written as the mask
# (1 << k) - 1 that maximum is a bitwise OR, which big-int lanes do at C speed.
_WIDTH_MASK = bytes((1 << _residual_width(b)) - 1 for b in range(256))
_MASK_WIDTH = bytes(((m + 1).bit_length() - 1) if m & (m + 1) == 0 else 0 for m in range(256))
# Shortest valid block header (value, bits; LSB first) from previous width p to k.
_HEADER = tuple(tuple((0, 1) if k == p else (1, 2) if k == p - 1 else (3, 3) if k == p + 1
                      else (7 | k << 3, 7) for k in range(9)) for p in range(9))
_HEADER_BITS = bytes(_HEADER[i >> 4][i & 15][1] if i >> 4 < 9 and i & 15 < 9 else 0
                     for i in range(256))
# Per width k: block width byte -> residual mask for blocks of exactly that width.
_SELECT = tuple(bytes((1 << k) - 1 if w == k else 0 for w in range(256)) for k in range(9))


def _repeat(pattern: bytes, count: int) -> int:
    return int.from_bytes(pattern * count, "little")


def _spread(per_block: bytes) -> bytes:
    """Repeat each per-block byte over that block's eight sample bytes."""
    out = bytearray(len(per_block) * CODEC_BLOCK)
    for lane in range(CODEC_BLOCK):
        out[lane::CODEC_BLOCK] = per_block
    return bytes(out)


def _residuals(pcm: bytes, predictor: int) -> bytes:
    """Wrapped residual bytes (x - pred) & 255 for one predictor.

    Samples are biased to u = x + 128 and held in 16-bit big-int lanes. Every
    lane expression below stays within 0..65535, so whole-stream integer
    arithmetic never carries or borrows between lanes.
    """
    n = len(pcm)
    lanes = bytearray(2 * (n + 2))
    lanes[0] = lanes[2] = 0x80  # x2 = x1 = 0 before the first sample
    lanes[4::2] = pcm.translate(_FLIP_SIGN)
    value = int.from_bytes(lanes, "little")
    full = (1 << (16 * n)) - 1
    one = _repeat(b"\x01\x00", n)
    u, u1, u2 = value >> 32, (value >> 16) & full, value & full
    if predictor == 0:    # x - x1 = u - u1
        r = u + (one << 8) - u1
    elif predictor == 1:  # x - 2*x1 + x2 = u - 2*u1 + u2
        r = u + u2 + (one << 9) - (u1 << 1)
    elif predictor == 2:  # pred = floor((3*u1 - u2 + 256) / 2) - 256
        t = u1 * 3 + (one << 8) - u2
        r = u + one * 640 - ((t >> 1) & _repeat(b"\xff\x7f", n))
    else:
        raise ValueError(f"unknown predictor {predictor}")
    return r.to_bytes(2 * n, "little")[0::2]


def _block_widths(residuals: bytes) -> bytes:
    """Minimal width k of every 8-residual block, one byte per block."""
    if not residuals:
        return b""
    masks = int.from_bytes(residuals.translate(_WIDTH_MASK), "little")
    masks |= masks >> 8
    masks |= masks >> 16
    masks |= masks >> 32
    return masks.to_bytes(len(residuals), "little")[0::CODEC_BLOCK].translate(_MASK_WIDTH)


def _stream_bits(widths: bytes) -> int:
    """Exact bit length of a stream with these block widths."""
    if not widths:
        return 2
    previous = int.from_bytes(b"\0" + widths[:-1], "little")
    pairs = (previous << 4 | int.from_bytes(widths, "little")).to_bytes(len(widths), "little")
    return 2 + sum(pairs.translate(_HEADER_BITS)) + CODEC_BLOCK * sum(widths)


def _block_payloads(residuals: bytes, widths: bytes) -> tuple[int, ...]:
    """Each block's eight k-bit residual fields packed LSB first."""
    blocks = len(widths)
    words = int.from_bytes(residuals, "little")
    payload = 0
    for k in sorted(set(widths) - {0}):
        w = words & int.from_bytes(_spread(widths.translate(_SELECT[k])), "little")
        if k < 8:
            # Compress the k low bits of each byte lane: pairs, quads, octets.
            w = (w & _repeat(b"\xff\x00", 4 * blocks)) | ((w & _repeat(b"\x00\xff", 4 * blocks)) >> (8 - k))
            w = (w & _repeat(b"\xff\xff\x00\x00", 2 * blocks)) | (
                (w & _repeat(b"\x00\x00\xff\xff", 2 * blocks)) >> (16 - 2 * k))
            w = (w & _repeat(b"\xff" * 4 + b"\x00" * 4, blocks)) | (
                (w & _repeat(b"\x00" * 4 + b"\xff" * 4, blocks)) >> (32 - 4 * k))
        payload |= w
    return struct.unpack(f"<{blocks}Q", payload.to_bytes(len(residuals), "little"))


def _pack(predictor: int, residuals: bytes, widths: bytes) -> tuple[bytes, int]:
    out = bytearray()
    acc, bits, previous = predictor, 2, 0
    for k, payload in zip(widths, _block_payloads(residuals, widths)):
        code, length = _HEADER[previous][k]
        acc |= (code | payload << length) << bits
        bits += length + CODEC_BLOCK * k
        previous = k
        if bits >= 512:  # Flush whole 64-byte chunks; a block adds at most 71 bits.
            out += (acc & ((1 << 512) - 1)).to_bytes(64, "little")
            acc >>= 512
            bits -= 512
    total = 8 * len(out) + bits
    size = 2 * ((total + 15) // 16)
    out += acc.to_bytes(size - len(out), "little")
    out += bytes(-len(out) % 4)
    return bytes(out), total


def _check_predictors(predictors) -> tuple[int, ...]:
    chosen = tuple(sorted(set(predictors)))
    if not chosen or any(p not in CODEC_PREDICTORS for p in chosen):
        raise ValueError(f"predictors must be a non-empty subset of {CODEC_PREDICTORS}")
    return chosen


def encode_stream(pcm: bytes, predictors=DEFAULT_PREDICTORS) -> tuple[bytes, int]:
    """Encode signed 8-bit PCM (a multiple of 8 samples) as one format-2 stream.

    Returns the stream bytes, zero-padded to a multiple of 4, and the chosen
    predictor: the allowed one with the fewest bits, the lowest on ties.
    """
    pcm = bytes(pcm)
    if len(pcm) % CODEC_BLOCK:
        raise ValueError(f"stream length {len(pcm)} is not a multiple of {CODEC_BLOCK} samples")
    best = None
    for predictor in _check_predictors(predictors):
        residuals = _residuals(pcm, predictor)
        widths = _block_widths(residuals)
        bits = _stream_bits(widths)
        if best is None or bits < best[0]:
            best = (bits, predictor, residuals, widths)
    bits, predictor, residuals, widths = best
    stream, packed = _pack(predictor, residuals, widths)
    if packed != bits:
        raise AssertionError("stream bit count disagrees with the size estimate")
    return stream, predictor


class _BitReader:
    """Reference reader: bit i is bit i % 16 of little-endian halfword i // 16."""

    def __init__(self, stream: bytes):
        if len(stream) % 2:
            raise ValueError("stream is not whole halfwords")
        self.halfwords = struct.unpack(f"<{len(stream) // 2}H", stream)
        self.position = 0

    def read(self, count: int) -> int:
        """Read a count-bit field, least significant bit first."""
        if self.position + count > 16 * len(self.halfwords):
            raise ValueError("stream ends inside a code")
        value = 0
        for bit in range(count):
            word = self.halfwords[(self.position + bit) // 16]
            value |= ((word >> ((self.position + bit) % 16)) & 1) << bit
        self.position += count
        return value


def _decode(stream: bytes, samples: int) -> tuple[bytes, int]:
    """Decode samples (a multiple of 8) and return the PCM and bits consumed."""
    if samples < 0 or samples % CODEC_BLOCK:
        raise ValueError(f"sample count {samples} is not a multiple of {CODEC_BLOCK}")
    reader = _BitReader(stream)
    predictor = reader.read(2)
    if predictor not in CODEC_PREDICTORS:
        raise ValueError(f"reserved predictor {predictor}")
    out = bytearray()
    x1 = x2 = 0
    k = 0
    for _ in range(samples // CODEC_BLOCK):
        if reader.read(1) == 0:
            pass
        elif reader.read(1) == 0:
            if k == 0:
                raise ValueError("width code decrements below 0")
            k -= 1
        elif reader.read(1) == 0:
            if k == 8:
                raise ValueError("width code increments above 8")
            k += 1
        else:
            k = reader.read(4)
            if k > 8:
                raise ValueError(f"reserved explicit width {k}")
        for _ in range(CODEC_BLOCK):
            residual = reader.read(k)
            if k and residual >= 1 << (k - 1):
                residual -= 1 << k
            if predictor == 0:
                prediction = x1
            elif predictor == 1:
                prediction = 2 * x1 - x2
            else:
                prediction = (3 * x1 - x2) >> 1  # Python >> floors like an arithmetic shift.
            x = ((prediction + residual + 128) & 255) - 128
            out.append(x & 255)
            x2, x1 = x1, x
    return bytes(out), reader.position


def decode_stream(stream: bytes, samples: int) -> bytes:
    """Reference decoder: samples (a multiple of 8) of signed 8-bit PCM."""
    return _decode(bytes(stream), samples)[0]


def pad_to_block(pcm: bytes) -> bytes:
    return bytes(pcm) + bytes(-len(pcm) % CODEC_BLOCK)


class Scripts:
    """Bounded IScript sound queries over authenticated DAT tables."""

    def __init__(self, units: bytes, flingy: bytes, sprites: bytes, images: bytes,
                 weapons: bytes, iscript: bytes):
        self.unit_flingy = le_array(units, UNIT_COUNT, 1, "units.dat flingy")
        self.subunit = le_array(units[UNIT_COUNT:], UNIT_COUNT, 2, "units.dat subunit")
        self.ground_weapon = le_array(units[5892:], UNIT_COUNT, 1, "units.dat ground weapon")
        self.air_weapon = le_array(units[6348:], UNIT_COUNT, 1, "units.dat air weapon")
        self.ready = le_array(units[9312:], VOICED_UNITS, 2, "units.dat ready")
        self.what = (le_array(units[9524:], UNIT_COUNT, 2, "units.dat what start"),
                     le_array(units[9980:], UNIT_COUNT, 2, "units.dat what end"))
        self.yes = (le_array(units[10860:], VOICED_UNITS, 2, "units.dat yes start"),
                    le_array(units[11072:], VOICED_UNITS, 2, "units.dat yes end"))
        self.flingy_sprite = le_array(flingy, FLINGY_COUNT, 2, "flingy.dat sprite")
        self.sprite_image = le_array(sprites, SPRITE_COUNT, 2, "sprites.dat image")
        self.image_script = le_array(images[IMAGE_COUNT * 10:], IMAGE_COUNT, 4, "images.dat iscript")
        self.weapon_flingy = le_array(weapons[WEAPON_COUNT * 2:], WEAPON_COUNT, 4, "weapons.dat flingy")
        self.payload = iscript
        self.index = script_index(iscript)

    def unit_image(self, unit: int) -> int:
        return self.sprite_image[self.flingy_sprite[self.unit_flingy[unit]]]

    def animation_sounds(self, image: int, names: tuple[str, ...], depth: int = 1) -> list[int]:
        offset = self.index.get(self.image_script[image]) if image < IMAGE_COUNT else None
        if offset is None:
            return []
        starts = image_animation_starts(self.payload, offset)
        found: list[int] = []
        for name in names:
            if name not in starts:
                continue
            program = animation_instructions(self.payload, starts[name])
            found.extend(sound_ids(program))
            if depth <= 0:
                continue
            # Explosions and remnants are separate sprites/images in IScript.
            for opcode, args in program:
                child = None
                if opcode in SPRITE_OPS and args and args[0] < SPRITE_COUNT:
                    child = self.sprite_image[args[0]]
                elif opcode in IMAGE_OPS and args and args[0] < IMAGE_COUNT:
                    child = args[0]
                if child is not None and child != image:
                    found.extend(self.animation_sounds(child, ("Init",), depth - 1))
        return found

    def unit_sounds(self, unit: int) -> dict[str, list[int]]:
        image = self.unit_image(unit)
        images = [image]
        if self.subunit[unit] < UNIT_COUNT:
            images.append(self.unit_image(self.subunit[unit]))
        attack: list[int] = []
        for candidate in images:
            attack.extend(self.animation_sounds(candidate, ATTACK_ANIMATIONS, 0))
        for weapon in (self.ground_weapon[unit], self.air_weapon[unit]):
            if weapon < WEAPON_COUNT and not attack:
                bullet = self.sprite_image[self.flingy_sprite[self.weapon_flingy[weapon]]]
                attack.extend(self.animation_sounds(bullet, ("Init",), 0))
        sounds = {
            "what": list(range(self.what[0][unit], self.what[1][unit] + 1)) if self.what[0][unit] else [],
            "attack": attack,
            "death": self.animation_sounds(image, ("Death",)),
            "built": self.animation_sounds(image, ("Built", "WarpIn"), 0),
        }
        if unit < VOICED_UNITS:
            sounds["ready"] = [self.ready[unit]] if self.ready[unit] else []
            sounds["yes"] = list(range(self.yes[0][unit], self.yes[1][unit] + 1)) if self.yes[0][unit] else []
        else:
            sounds["ready"], sounds["yes"] = [], []
        for key, values in sounds.items():
            unique = []
            for value in values:
                if 0 < value < SOUND_COUNT and value not in unique:
                    unique.append(value)
            sounds[key] = unique[:MAX_SET]
        return sounds


KINDS = ("ready", "what", "yes", "attack", "death", "built")


def c_array(name: str, ctype: str, values: list[int], width: int = 16) -> str:
    rows = [", ".join(str(v) for v in values[i:i + width]) for i in range(0, len(values), width)]
    return f"static const {ctype} {name}[{len(values)}] __attribute__((unused)) = {{\n    " + ",\n    ".join(rows or ["0"]) + "\n};\n"


def encode_bank(clips: list[bytes], tracks: dict[int, bytes], verify: bool = True,
                predictors=DEFAULT_PREDICTORS) -> dict:
    """Lay out the format-2 blob: SFX clip streams in order, then music by track.

    Clips are zero-padded to whole 8-sample blocks and tracks to whole
    176-sample frames. Every stream starts 4-byte aligned; the zero guard
    follows the last one. With verify, every stream is decoded again with the
    reference decoder before it is accepted. Only the given predictors are tried.
    """
    predictors = _check_predictors(predictors)
    out = bytearray()
    pcm_hash = hashlib.sha256()
    chosen = [0] * len(CODEC_PREDICTORS)

    def add_stream(pcm: bytes) -> int:
        stream, predictor = encode_stream(pcm, predictors)
        if verify and decode_stream(stream, len(pcm)) != pcm:
            raise AssertionError(f"stream at blob offset {len(out)} fails to round-trip")
        while len(out) % 4:
            out.append(0)
        offset = len(out)
        out.extend(stream)
        pcm_hash.update(pcm)
        chosen[predictor] += 1
        return offset

    sample_offset: list[int] = []
    sample_length: list[int] = []
    for clip in clips:
        pcm = pad_to_block(clip)
        sample_offset.append(add_stream(pcm))
        sample_length.append(len(pcm))
    sfx_encoded_bytes = len(out)
    music_offset = [0] * MUSIC_TRACKS
    music_frames = [0] * MUSIC_TRACKS
    for track in sorted(tracks):
        pcm = tracks[track]
        frames = (len(pcm) + SAMPLES_PER_FRAME - 1) // SAMPLES_PER_FRAME
        music_offset[track] = add_stream(pcm + bytes(frames * SAMPLES_PER_FRAME - len(pcm)))
        music_frames[track] = frames
    encoded_bytes = len(out)
    out += bytes(GUARD_BYTES)
    sfx_pcm_bytes = sum(sample_length)
    music_pcm_bytes = SAMPLES_PER_FRAME * sum(music_frames)
    pcm_bytes = sfx_pcm_bytes + music_pcm_bytes
    return {
        "blob": out,
        "sample_offset": sample_offset, "sample_length": sample_length,
        "music_offset": music_offset, "music_frames": music_frames,
        "pcm_sha256": pcm_hash.hexdigest(),
        "sizes": {
            "sfx_pcm_bytes": sfx_pcm_bytes, "sfx_encoded_bytes": sfx_encoded_bytes,
            "music_pcm_bytes": music_pcm_bytes,
            "music_encoded_bytes": encoded_bytes - sfx_encoded_bytes,
            "pcm_bytes": pcm_bytes, "encoded_bytes": encoded_bytes,
            "saved_bytes": pcm_bytes - encoded_bytes,
            "saved_percent": round(100 * (pcm_bytes - encoded_bytes) / max(pcm_bytes, 1), 2),
            "predictors": "".join(str(p) for p in predictors),
            "stream_predictors": {str(p): n for p, n in enumerate(chosen)},
            "guard_bytes": GUARD_BYTES,
            "blob_bytes": len(out),
        },
    }


def generate(rom_path: Path, out_dir: Path, music: tuple[int, ...],
             upstream: Path = UPSTREAM, verify: bool = True,
             predictors=DEFAULT_PREDICTORS) -> dict:
    extractor = load_sprites(ROOT / "tools/sc64_sprites.py").load_extractor(upstream)
    rom = extractor.load_rom(str(rom_path))
    rom_sha = hashlib.sha1(rom).hexdigest()
    if rom_sha != EXPECTED_ROM_SHA1:
        raise ValueError(f"ROM is not the authenticated USA retail image ({rom_sha})")
    archive = extractor.BoltArchive(rom)
    entries = {entry.path: entry for entry in archive.entries()}

    def read(path: str) -> bytes:
        if path not in entries:
            raise ValueError(f"missing BOLT entry {path}")
        return archive.read(entries[path])

    offsets, lengths = parse_sound_table(read(SOUND_TABLE))
    blob = rom[SOUND_BLOB_ROM_OFFSET:SOUND_BLOB_ROM_OFFSET + SOUND_BLOB_LENGTH]
    if len(blob) != SOUND_BLOB_LENGTH or any(rom[SOUND_BLOB_ROM_OFFSET + SOUND_BLOB_LENGTH:]):
        raise ValueError("sound blob does not end the archive")
    for offset, length in zip(offsets, lengths):
        if length and blob[offset:offset + 8] != bytes(8):
            raise ValueError(f"sound at blob offset {offset:#x} lacks its zero decoder state")

    scripts = Scripts(read("00C/001"), read("00C/002"), read("00C/003"), read("00C/000"),
                      read("00C/007"), read("012/002"))
    unit_sets = [scripts.unit_sounds(unit) for unit in range(UNIT_COUNT)]
    selected: list[int] = []
    for sets in unit_sets:
        for kind in KINDS:
            for sound in sets[kind]:
                if lengths[sound] > 8 and sound not in selected:
                    selected.append(sound)
    advisor = [sound for race in ADVISOR_SOUNDS for sound in race]
    for sound in advisor:
        if lengths[sound] <= 8:
            raise ValueError(f"advisor sound {sound} is empty")
        if sound not in selected:
            selected.append(sound)
    selected.sort()

    clips: list[bytes] = []
    converted: dict[tuple[int, int], int] = {}
    bank_index: dict[int, int] = {}
    for sound in selected:
        key = (offsets[sound], lengths[sound])
        if key not in converted:
            converted[key] = len(clips)
            clips.append(resample_to_s8(ima_decode(blob[key[0] + 8:key[0] + key[1]]), SFX_GAIN))
        bank_index[sound] = converted[key]

    tracks: dict[int, bytes] = {}
    for track in music:
        stream = read(f"015/{track:03X}")
        if stream[:4] != b"\x02\x10\x2b\x11" or struct.unpack(">I", stream[4:8])[0] + 12 != len(stream):
            raise ValueError(f"unexpected music stream header for track {track}")
        tracks[track] = resample_to_s8(ima_decode(stream[12:]), MUSIC_GAIN)
    bank = encode_bank(clips, tracks, verify, predictors)
    out = bank["blob"]
    sample_offset, sample_length = bank["sample_offset"], bank["sample_length"]
    music_offset, music_frames = bank["music_offset"], bank["music_frames"]

    unit_first: list[int] = []
    unit_count: list[int] = []
    flat: list[int] = []
    for sets in unit_sets:
        for kind in KINDS:
            entries_for_kind = [bank_index[s] for s in sets[kind] if s in bank_index]
            unit_first.append(len(flat))
            unit_count.append(len(entries_for_kind))
            flat.extend(entries_for_kind)

    out_dir.mkdir(parents=True, exist_ok=True)
    blob_path = out_dir / "sc64_audio_generated.bin"
    blob_path.write_bytes(out)
    header = [
        "/* Generated by tools/sc64_audio_pack.py from a user-supplied ROM. */",
        "/* Private derived data: do not commit or distribute. */",
        "#ifndef SC64_AUDIO_GENERATED_H",
        "#define SC64_AUDIO_GENERATED_H",
        f"#define SC64_AUDIO_FORMAT {CODEC_FORMAT}",
        f"#define SC64_AUDIO_RATE {OUTPUT_RATE}",
        f"#define SC64_AUDIO_SAMPLES_PER_FRAME {SAMPLES_PER_FRAME}",
        f"#define SC64_AUDIO_SAMPLE_COUNT {len(sample_offset)}",
        f"#define SC64_AUDIO_UNIT_COUNT {UNIT_COUNT}",
        f"#define SC64_AUDIO_KIND_COUNT {len(KINDS)}",
        f"#define SC64_AUDIO_MUSIC_TRACKS {MUSIC_TRACKS}",
        f"#define SC64_AUDIO_BLOB_BYTES {len(out)}u",
        "enum { " + ", ".join(f"SC64_AUDIO_{k.upper()}" for k in KINDS) + " };",
        "/* Byte offset of each clip's stream; clip length in samples (a multiple of 8). */",
        c_array("sc64_audio_sample_offset", "u32", sample_offset, 8),
        c_array("sc64_audio_sample_length", "u32", sample_length, 8),
        "/* Per raw unit id, per kind: first entry and count in sc64_audio_unit_sound. */",
        c_array("sc64_audio_unit_first", "u16", unit_first),
        c_array("sc64_audio_unit_count", "u8", unit_count, 24),
        c_array("sc64_audio_unit_sound", "u16", flat or [0]),
        "/* Advisor refusal clips per player race (Terran, Zerg, Protoss): minerals, gas, supply. */",
        f"#define SC64_AUDIO_ADVISOR_LINES {len(ADVISOR_LINES)}",
        c_array("sc64_audio_advisor", "u16", [bank_index[s] for s in advisor], len(ADVISOR_LINES)),
        "/* Byte offset of each imported track's stream. */",
        c_array("sc64_audio_music_offset", "u32", music_offset, 8),
        "/* Track lengths in 176-sample frames; zero when not imported. */",
        c_array("sc64_audio_music_frames", "u32", music_frames, 8),
        "#endif",
        "",
    ]
    header_path = out_dir / "sc64_audio_generated.h"
    header_path.write_text("\n".join(header), encoding="ascii")
    report = {
        "format": CODEC_FORMAT,
        "rom_sha1": rom_sha,
        "source": {"sound_table": SOUND_TABLE, "sound_ids": SOUND_COUNT,
                   "unique_source_sounds": len(set(zip(offsets, lengths))),
                   "codec": "IMA ADPCM, high nibble first, mono", "rate_hz": SOURCE_RATE},
        "output": {"rate_hz": OUTPUT_RATE, "format": "signed 8-bit PCM",
                   "codec": f"SC64 audio codec format {CODEC_FORMAT} (lossless)",
                   "samples": len(sample_offset),
                   "music_tracks": {MUSIC_ROLES[t]: music_frames[t] for t in music},
                   **bank["sizes"],
                   "blob_sha256": hashlib.sha256(out).hexdigest()},
        "pcm_sha256": bank["pcm_sha256"],
        "verified": verify,
        "units_with_sounds": sum(1 for s in unit_sets if any(s[k] for k in KINDS)),
        "advisor": {race: {line: {"sound_id": sound, "sample": bank_index[sound],
                                  "samples": sample_length[bank_index[sound]]}
                           for line, sound in zip(ADVISOR_LINES, sounds)}
                    for race, sounds in zip(("terran", "zerg", "protoss"), ADVISOR_SOUNDS)},
    }
    (out_dir / "sc64_audio_manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def _header_array(text: str, name: str) -> list[int]:
    match = re.search(rf"\b{name}\[(\d+)\][^=]*=\s*\{{([^}}]*)\}}", text)
    if not match:
        raise ValueError(f"generated header lacks {name}")
    values = [int(v) for v in match.group(2).replace(",", " ").split()]
    if len(values) != int(match.group(1)):
        raise ValueError(f"generated header array {name} has the wrong length")
    return values


def _header_define(text: str, name: str) -> int:
    match = re.search(rf"^#define {name} (\d+)u?$", text, re.MULTILINE)
    if not match:
        raise ValueError(f"generated header lacks {name}")
    return int(match.group(1))


def verify_bank(bank_dir: Path) -> dict:
    """Decode every stream of a generated bank and check it against its manifest."""
    header = (bank_dir / "sc64_audio_generated.h").read_text(encoding="ascii")
    blob = (bank_dir / "sc64_audio_generated.bin").read_bytes()
    manifest = json.loads((bank_dir / "sc64_audio_manifest.json").read_text())
    problems = []
    if not re.search(rf"^#define SC64_AUDIO_FORMAT {CODEC_FORMAT}$", header, re.MULTILINE) \
            or manifest.get("format") != CODEC_FORMAT:
        return {"ok": False, "problems": [f"bank is not codec format {CODEC_FORMAT}"]}
    if _header_define(header, "SC64_AUDIO_BLOB_BYTES") != len(blob):
        problems.append("SC64_AUDIO_BLOB_BYTES does not match the blob size")
    if manifest.get("output", {}).get("blob_sha256") != hashlib.sha256(blob).hexdigest():
        problems.append("blob_sha256 does not match the blob")
    frame = _header_define(header, "SC64_AUDIO_SAMPLES_PER_FRAME")
    # Blob order: SFX clips in index order, then imported music tracks.
    streams = list(zip(_header_array(header, "sc64_audio_sample_offset"),
                       _header_array(header, "sc64_audio_sample_length")))
    streams += sorted((offset, frames * frame) for offset, frames in zip(
        _header_array(header, "sc64_audio_music_offset"),
        _header_array(header, "sc64_audio_music_frames")) if frames)
    end = len(blob) - GUARD_BYTES
    if any(blob[end:]):
        problems.append("the trailing guard is not zero")
    pcm_hash = hashlib.sha256()
    pcm_bytes = 0
    for index, (offset, samples) in enumerate(streams):
        limit = streams[index + 1][0] if index + 1 < len(streams) else end
        if offset % 4 or samples % CODEC_BLOCK or not offset < limit <= end:
            problems.append(f"stream {index} at {offset} is misplaced or misaligned")
            continue
        try:
            pcm, bits = _decode(blob[offset:limit], samples)
        except ValueError as error:
            problems.append(f"stream {index} at {offset}: {error}")
            continue
        if any(blob[offset + 2 * ((bits + 15) // 16):limit]):
            problems.append(f"stream {index} at {offset} has nonzero padding")
        pcm_hash.update(pcm)
        pcm_bytes += len(pcm)
    if pcm_hash.hexdigest() != manifest.get("pcm_sha256"):
        problems.append("decoded PCM does not match pcm_sha256")
    return {"streams": len(streams), "pcm_bytes": pcm_bytes, "blob_bytes": len(blob),
            "pcm_sha256": pcm_hash.hexdigest(), "ok": not problems, "problems": problems}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--rom", type=Path, default=ROM_DEFAULT)
    parser.add_argument("--out", type=Path, default=None,
                        help=f"bank directory (default {OUT_DEFAULT.relative_to(ROOT)}), "
                             "or the output file of --encode-pcm/--decode-stream")
    parser.add_argument("--music", default=",".join(str(t) for t in MUSIC_DEFAULT),
                        help="comma-separated music tracks to import, or 'none'")
    parser.add_argument("--predictors", default="".join(str(p) for p in DEFAULT_PREDICTORS),
                        help="predictors the encoder may choose per stream, a non-empty subset "
                             "of '012' (default %(default)s; '012' compresses most)")
    parser.add_argument("--no-verify", action="store_true",
                        help="skip decoding every stream with the reference decoder")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--encode-pcm", type=Path, metavar="IN",
                      help="encode raw signed 8-bit PCM as one stream to --out")
    mode.add_argument("--decode-stream", type=Path, metavar="IN",
                      help="decode one stream of --samples samples to raw PCM at --out")
    mode.add_argument("--verify-bank", type=Path, metavar="DIR",
                      help="decode every stream of a bank and check its manifest")
    parser.add_argument("--samples", type=int, help="sample count for --decode-stream")
    args = parser.parse_args(argv)
    if not args.predictors or any(c not in "012" for c in args.predictors):
        parser.error("--predictors must be a non-empty subset of '012'")
    predictors = tuple(sorted({int(c) for c in args.predictors}))
    if args.encode_pcm or args.decode_stream:
        if args.out is None:
            parser.error("--out is required with --encode-pcm and --decode-stream")
    if args.encode_pcm:
        pcm = pad_to_block(args.encode_pcm.read_bytes())
        stream, predictor = encode_stream(pcm, predictors)
        if not args.no_verify and decode_stream(stream, len(pcm)) != pcm:
            raise AssertionError("encoded stream fails to round-trip")
        args.out.write_bytes(stream)
        print(json.dumps({"samples": len(pcm), "predictor": predictor, "bytes": len(stream)}))
        return 0
    if args.decode_stream:
        if args.samples is None or args.samples < 0:
            parser.error("--decode-stream needs --samples N")
        # Whole blocks are decoded; a count that is not a multiple of 8 is truncated.
        blocks = (args.samples + CODEC_BLOCK - 1) // CODEC_BLOCK
        pcm = decode_stream(args.decode_stream.read_bytes(), blocks * CODEC_BLOCK)
        args.out.write_bytes(pcm[:args.samples])
        return 0
    if args.verify_bank:
        result = verify_bank(args.verify_bank)
        print(json.dumps(result, indent=2))
        return 0 if result["ok"] else 1
    music = () if args.music == "none" else tuple(sorted({int(t) for t in args.music.split(",")}))
    if any(t < 0 or t >= MUSIC_TRACKS for t in music):
        parser.error("music tracks are 0-15")
    report = generate(args.rom, args.out or OUT_DEFAULT, music, verify=not args.no_verify,
                      predictors=predictors)
    print(json.dumps(report["output"], indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
