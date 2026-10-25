#!/usr/bin/env python3
"""Convert StarCraft 64 sound effects and music into a private GBA audio bank.

The user-supplied ROM and every converted sample stay under .local/. Output is
an 8-bit signed PCM blob plus a small index header; neither may be committed
or distributed.

Source format (verified against the USA retail image):
  * BOLT 014/000 is a 1144-entry table, indexed like PC sfxdata ids: a count,
    1144 big-endian blob offsets, a zero word and 1144 byte lengths. The blob
    is the unlisted tail of the BOLT archive. Every sound starts with eight
    zero bytes (initial decoder state) followed by mono IMA ADPCM, high nibble
    first, at 11025 Hz.
  * BOLT 015/000-00F are music streams of the same codec, each with a 12-byte
    header: format byte 2, 0x10, big-endian rate 11025, big-endian data length
    and four zero bytes.
  * The resident N64 code selects music resource 0x1500+track. Track 0 is the
    menu theme; the in-game themes alternate per player race (Terran 4/5,
    Zerg 8/9, Protoss 12/13); the mission-end stingers are 2/3, 6/7 and
    10/11, chosen by the result flag.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.sc64_construction_sprites import (  # noqa: E402
    ANIMATIONS, animation_instructions, decode_instruction, image_animation_starts,
    le_array, load_sprites, script_index,
)

UPSTREAM = ROOT / "references/sc64-maps/extract_sc64_maps.py"
ROM_DEFAULT = ROOT / ".local/sc64/rom/StarCraft64-USA.z64"
OUT_DEFAULT = ROOT / ".local/sc64/audio/generated"
EXPECTED_ROM_SHA1 = "a36d91b5aac7bb95df0d7de5f9814f5270e25c2b"
SOUND_TABLE = "014/000"
SOUND_TABLE_SHA256 = None  # checked structurally below; the ROM hash pins it
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
# Roles come from the resident N64 music selector (see the module comment).
MUSIC_ROLES = {
    0: "menu", 1: "unused-1", 2: "terran-result-a", 3: "terran-result-b",
    4: "terran-1", 5: "terran-2", 6: "zerg-result-a", 7: "zerg-result-b",
    8: "zerg-1", 9: "zerg-2", 10: "protoss-result-a", 11: "protoss-result-b",
    12: "protoss-1", 13: "protoss-2", 14: "unused-14", 15: "unused-15",
}
MUSIC_DEFAULT = (0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)
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


def generate(rom_path: Path, out_dir: Path, music: tuple[int, ...],
             upstream: Path = UPSTREAM) -> dict:
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
    selected.sort()

    out = bytearray()
    sample_offset: list[int] = []
    sample_length: list[int] = []
    converted: dict[tuple[int, int], int] = {}
    bank_index: dict[int, int] = {}
    for sound in selected:
        key = (offsets[sound], lengths[sound])
        if key not in converted:
            pcm = resample_to_s8(ima_decode(blob[key[0] + 8:key[0] + key[1]]), SFX_GAIN)
            # The mixer reads two samples per ROM halfword.
            pcm += bytes(len(pcm) & 1)
            converted[key] = len(sample_offset)
            sample_offset.append(len(out))
            sample_length.append(len(pcm))
            out += pcm
        bank_index[sound] = converted[key]

    music_offset = [0] * MUSIC_TRACKS
    music_frames = [0] * MUSIC_TRACKS
    for track in music:
        stream = read(f"015/{track:03X}")
        if stream[:4] != b"\x02\x10\x2b\x11" or struct.unpack(">I", stream[4:8])[0] + 12 != len(stream):
            raise ValueError(f"unexpected music stream header for track {track}")
        pcm = resample_to_s8(ima_decode(stream[12:]), MUSIC_GAIN)
        frames = (len(pcm) + SAMPLES_PER_FRAME - 1) // SAMPLES_PER_FRAME
        while len(out) % 4:
            out.append(0)
        music_offset[track] = len(out)
        music_frames[track] = frames
        out += pcm + bytes(frames * SAMPLES_PER_FRAME - len(pcm))
    # DMA sound reads ahead up to one frame plus the 16-byte FIFO after the
    # last mixed frame; a zero guard keeps any overshoot silent.
    out += bytes(SAMPLES_PER_FRAME + 32)

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
        f"#define SC64_AUDIO_RATE {OUTPUT_RATE}",
        f"#define SC64_AUDIO_SAMPLES_PER_FRAME {SAMPLES_PER_FRAME}",
        f"#define SC64_AUDIO_SAMPLE_COUNT {len(sample_offset)}",
        f"#define SC64_AUDIO_UNIT_COUNT {UNIT_COUNT}",
        f"#define SC64_AUDIO_KIND_COUNT {len(KINDS)}",
        f"#define SC64_AUDIO_MUSIC_TRACKS {MUSIC_TRACKS}",
        f"#define SC64_AUDIO_BLOB_BYTES {len(out)}u",
        "enum { " + ", ".join(f"SC64_AUDIO_{k.upper()}" for k in KINDS) + " };",
        c_array("sc64_audio_sample_offset", "u32", sample_offset, 8),
        c_array("sc64_audio_sample_length", "u32", sample_length, 8),
        "/* Per raw unit id, per kind: first entry and count in sc64_audio_unit_sound. */",
        c_array("sc64_audio_unit_first", "u16", unit_first),
        c_array("sc64_audio_unit_count", "u8", unit_count, 24),
        c_array("sc64_audio_unit_sound", "u16", flat or [0]),
        c_array("sc64_audio_music_offset", "u32", music_offset, 8),
        "/* Track lengths in 176-sample frames; zero when not imported. */",
        c_array("sc64_audio_music_frames", "u32", music_frames, 8),
        "#endif",
        "",
    ]
    header_path = out_dir / "sc64_audio_generated.h"
    header_path.write_text("\n".join(header), encoding="ascii")
    report = {
        "rom_sha1": rom_sha,
        "source": {"sound_table": SOUND_TABLE, "sound_ids": SOUND_COUNT,
                   "unique_source_sounds": len(set(zip(offsets, lengths))),
                   "codec": "IMA ADPCM, high nibble first, mono", "rate_hz": SOURCE_RATE},
        "output": {"rate_hz": OUTPUT_RATE, "format": "signed 8-bit PCM",
                   "samples": len(sample_offset), "sfx_bytes": sum(sample_length),
                   "music_tracks": {MUSIC_ROLES[t]: music_frames[t] for t in music},
                   "blob_bytes": len(out),
                   "blob_sha256": hashlib.sha256(out).hexdigest()},
        "units_with_sounds": sum(1 for s in unit_sets if any(s[k] for k in KINDS)),
    }
    (out_dir / "sc64_audio_manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--rom", type=Path, default=ROM_DEFAULT)
    parser.add_argument("--out", type=Path, default=OUT_DEFAULT)
    parser.add_argument("--music", default=",".join(str(t) for t in MUSIC_DEFAULT),
                        help="comma-separated music tracks to import, or 'none'")
    args = parser.parse_args(argv)
    music = () if args.music == "none" else tuple(sorted({int(t) for t in args.music.split(",")}))
    if any(t < 0 or t >= MUSIC_TRACKS for t in music):
        parser.error("music tracks are 0-15")
    report = generate(args.rom, args.out, music)
    print(json.dumps(report["output"], indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
