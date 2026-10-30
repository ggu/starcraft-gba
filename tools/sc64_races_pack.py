#!/usr/bin/env python3
"""Generate private terrain and placement tables for imported campaign maps.

All extracted game data and generated headers stay under ignored .local/.
The header is derived from a user-supplied verified StarCraft 64 USA ROM and
the pinned sc64-maps extractor; it must not be committed or distributed.
"""
from __future__ import annotations

from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
BASE = ROOT / '.local/sc64/backwater/generated'
OUTPUT = ROOT / '.local/sc64/races/generated/sc64_races_generated.h'
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
T6_CHK_SHA256 = 'f07f21939bb8dbeeb6e3933fc632a04ff13bc1617ac5f8ae224c0ff978623f35'
UPSTREAM_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
MISSIONS = [
    ('Z1', '008/014', 'Among the Ruins'),
    ('Z2', '008/015', 'Egression'),
    ('P1', '008/01E', 'First Strike'),
    ('P2', '008/01F', 'Into the Flames'),
    # Keep these after the four Races missions: save slots and current builds
    # depend on the original indices remaining Z1/Z2/P1/P2 = 0..3.
    ('T1', '008/00A', 'Wasteland'),
    ('T2', '008/00B', 'Backwater Station'),
    # T3 is emitted into the private header, but its data and map count are
    # compiled only by unified builds that explicitly enable stage 3.
    ('T3', '008/00C', 'Desperate Alliance'),
    # Z3 has its fixed global catalog ID 14. Unified builds keep catalog
    # slots 7..13 explicitly unavailable rather than compacting this index.
    ('Z3', '008/016', 'The New Dominion'),
    # P3 has fixed global catalog ID 22 after the reserved Zerg chapter slots.
    ('P3', '008/020', 'Higher Ground'),
    # T4 occupies stable campaign ID 7, ahead of the later Z3 IDs.
    ('T4', '008/00D', 'The Jacobs Installation'),
    # T5's descriptor occupies stable global ID 8.
    ('T5', '008/00E', 'The Antigan Revolt'),
    # T6 is import-only at stable global ID 9 until runtime qualification.
    ('T6', '008/00F', 'The Norad II'),
    # T7-T10 source maps are retained/imported only behind the late Terran
    # data gate. They do not become campaign-playable through this table.
    ('T7', '008/010', 'The Trump Card'),
    ('T8', '008/011', 'The Big Push'),
    ('T9', '008/012', 'New Gettysburg'),
    ('T10', '008/013', 'The Hammer Falls'),
    # Z4-Z10 and P4-P10 are explicit data imports. Their own *_DATA_ENABLED
    # gates stage source-backed map data without enabling catalog/playable flags.
    ('Z4', '008/017', 'Agent of the Swarm'),
    ('Z5', '008/018', 'Assimilation'),
    ('Z6', '008/019', 'The Dark Templar'),
    ('Z7', '008/01A', 'The Culling'),
    ('Z8', '008/01B', 'The Dark Templar'),
    ('Z9', '008/01C', 'The Invasion of Aiur'),
    ('Z10', '008/01D', 'Untitled Scenario'),
    ('P4', '008/021', 'The Hunt for Tassadar'),
    ('P5', '008/022', 'Choosing Sides'),
    ('P6', '008/023', 'Protoss 06'),
    ('P7', '008/024', 'Homeland'),
    ('P8', '008/025', 'The Trial of Tassadar'),
    ('P9', '008/026', 'Shadow Hunters'),
    ('P10', '008/027', 'Untitled Scenario'),
]
TERRAN_LATE_CHK_SHA256 = {
    '008/010': '0883a5241140199415650934bcb3c034b0944cb5be19c477a94a95aa70a2a1f7',
    '008/011': '7c765fc6b8ccfd2ee63c321499dce934a25a81267aa6f8822149c0313facc3a2',
    '008/012': 'afa086b680f37f1de4d467893351dc6bea5abb475121ff59da55c632de33f6f3',
    '008/013': 'd1bcc9759b948e95ad565537ed3c29eb980cd325935aa25480dc0ad4a8298b6f',
}
RACE_EXTRA_CHK_SHA256 = {
    '008/017': '2cb8e516f07651801c1d6f519017d494af87c479a0ba8df9e33af34b14ec685a',
    '008/018': '070d8e0233f57ee478571cadbb77c25dc13f4c71ad2acabeecc4088adddd5466',
    '008/019': 'cad7296dbb2c6f965510a80abfa028eca34b2be1f4657e774b536ec43edaa878',
    '008/01A': '3e827076bee7558d743928e0f3e0fba534b86da0cafc0bb84a959debe4ad5b13',
    '008/01B': 'ff6d0177a6a76fc68ef98f0962661857853ea11c50de9060b90ff934826cfd8e',
    '008/01C': '0eff2a2e44c55c0f5de9aa0c9123121f16cb870e1d326ae3a0fb3dba6bd8e5f7',
    '008/01D': '1a8f0e1de2ea9353e9561fb4b31da951852b32292f5963be416dc026c2862e17',
    '008/021': '40f30ccedf87a3b9c88308dc3092611ceff10185ddaf32471104902042a93ecd',
    '008/022': '5cb5bc0da8ba7b73515a53dd36150a0a2f32b4482faa6ca00898e652ce820d4c',
    '008/023': 'd9bc1f180d945bc0d75e5eac561a91daf291821e97171d761de16d29f792cbec',
    '008/024': '7c566d8b5e9d851be407dc1ac5438bebf32a8ff1d66a3d1b784cffd66f19e3a7',
    '008/025': 'e2257c48a119bfde3da075e08628b312857e3aad9222c47678fc29d57c578cd9',
    '008/026': 'eecccb0220c7456cacccf8b84f9bd82535f5cdce9192ba7d88986ea2867ba601',
    '008/027': '59d0adcb12d7be73082a069d900acf119643cd3c0c754f6cfe80b2613a35a99f',
}
BUILDING_RAW_IDS = {
    106, 109, 110, 111, 112, 113, 114, 122, 124, 125, 130, 131, 132,
    133, 135, 137, 139, 141, 142, 143, 144, 146, 149, 150, 151, 154, 155,
    156, 157, 160, 162, 164, 166, 167,
}
# Source StarCraft UNIT building range used for campaign pool sizing, including
# structures this runtime profile does not model. Raw 150 is
# B_MATURE_CHRYSALIS; raw 216 is U_CHRYSALIS.
SOURCE_BUILDING_RAW_IDS = set(range(106, 176))
SOURCE_BUILDING_RAW_IDS.update({194, 195, 196})
SOURCE_UNIT_RAW_IDS = set(range(0, 106)) | {216, 218}
RUNTIME_UNIT_RAW_IDS_ZERG = {
    0, 2, 3, 5, 7, 8, 11, 19, 30, 32, 34, 37, 38, 41, 42, 43, 45,
    47, 53, 64, 65, 66, 70, 77, 78, 79, 83, 216,
}
RUNTIME_UNIT_RAW_IDS_PROTOSS = RUNTIME_UNIT_RAW_IDS_ZERG - {11}
RUNTIME_BUILDING_RAW_IDS = BUILDING_RAW_IDS | {194}
# Capabilities supplied by the opt-in Z7 profile's combined ZP and Z6 actor
# gates. Keep these separate from the canonical baseline sets above: raw 39,
# raw 44, and buildings 138/140 are not available in early race profiles.
RUNTIME_UNIT_RAW_IDS_Z7 = RUNTIME_UNIT_RAW_IDS_ZERG | {39, 44}
RUNTIME_BUILDING_RAW_IDS_Z7 = RUNTIME_BUILDING_RAW_IDS | {138, 140}
RESOURCE_RAW_IDS = {176, 177, 178, 188}
RETAINED_SOURCE_RECORDS = {
    b'UNIT': 'UNIT.bin', b'DIM ': 'DIM.bin', b'ERA ': 'ERA.bin',
    b'MTXM': 'MTXM.bin', b'MRGN': 'MRGN.bin', b'DD2 ': 'DD2.bin',
    b'THG2': 'THG2.bin', b'TRIG': 'TRIG.bin', b'FORC': 'FORC.bin',
    b'OWNR': 'OWNR.bin', b'SIDE': 'SIDE.bin', b'UPRP': 'UPRP.bin',
    b'UPUS': 'UPUS.bin',
}
# Retain the old name for existing T4-specific callers and tests.
T4_REQUIRED_SOURCE_RECORDS = RETAINED_SOURCE_RECORDS


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if not spec or not spec.loader:
        raise RuntimeError(f'Cannot load {path}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def c_array(name: str, ctype: str, values, width: int = 24) -> str:
    values = list(values)
    rows = [','.join(str(v) for v in values[i:i + width])
            for i in range(0, len(values), width)]
    return f'static const {ctype} {name}[{len(values)}] = {{\n' + ',\n'.join(rows) + '\n};\n'


def emit_map_count() -> str:
    """Emit compile-time map count for the stable direct-global-ID table."""
    lines = []
    # Descending high-water checks preserve direct IDs and make a data flag
    # reserve all intervening unavailable slots without changing playability.
    gates = [('P', n, 23 + n - 4) for n in range(10, 3, -1)]
    gates += [('Z', n, 15 + n - 4) for n in range(10, 3, -1)]
    for i, (prefix, chapter, global_id) in enumerate(gates):
        directive = '#if' if i == 0 else '#elif'
        lines.append(
            f'{directive} defined(SC64_UNIFIED) && defined(SC64_{prefix}{chapter}_DATA_ENABLED)\n'
            f'#define SC64_RACE_MAP_COUNT {global_id + 1}\n')
    lines.append('#elif defined(SC64_UNIFIED) && defined(SC64_P3_ENABLED)\n#define SC64_RACE_MAP_COUNT 23\n')
    lines.extend([
        '#elif defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n#define SC64_RACE_MAP_COUNT 15\n',
        '#elif defined(SC64_UNIFIED) && defined(SC64_TERRAN_LATE_ENABLED)\n#define SC64_RACE_MAP_COUNT 14\n',
        '#elif defined(SC64_UNIFIED) && defined(SC64_T6_ENABLED)\n#define SC64_RACE_MAP_COUNT 10\n',
        '#elif defined(SC64_UNIFIED) && defined(SC64_T5_ENABLED)\n#define SC64_RACE_MAP_COUNT 9\n',
        '#elif defined(SC64_UNIFIED) && defined(SC64_T4_ENABLED)\n#define SC64_RACE_MAP_COUNT 8\n',
        '#elif defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)\n#define SC64_RACE_MAP_COUNT 7\n',
        '#elif defined(SC64_UNIFIED)\n#define SC64_RACE_MAP_COUNT 6\n',
        '#else\n#define SC64_RACE_MAP_COUNT 4\n#endif\n',
    ])
    return ''.join(lines)


def campaign_guard_for_map(map_index: int) -> str:
    """Return the profile guard used around one map's data and descriptor."""
    if map_index in (9, 10, 11):
        return 'defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 0'
    if map_index < 2 or map_index == 7 or 16 <= map_index <= 22:
        race = 1
    elif map_index < 4 or map_index == 8 or 23 <= map_index <= 29:
        race = 2
    else:
        race = 0
    return f'!defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE == {race}'


def emit_descriptor_table(descriptors: list[str]) -> str:
    """Emit stable direct-ID descriptor slots, with disabled IDs zeroed.

    ``descriptors`` uses packer order from ``MISSIONS``. Each source-order map
    is placed into its stable direct global-ID slot; disabled intermediate IDs
    become zero descriptors. Per-descriptor race guards stay inside each token.
    """
    if len(descriptors) not in (16, len(MISSIONS)):
        raise ValueError(f'expected 16 legacy descriptors or {len(MISSIONS)} full-campaign descriptors')
    if len(descriptors) == 16:
        descriptors = descriptors + [f'    {{{global_id_for_map(i)}}}, /* staged */\n'
                                      for i in range(16, len(MISSIONS))]

    ids = [global_id_for_map(i) for i in range(len(MISSIONS))]
    by_id = {global_id: index for index, global_id in enumerate(ids)}
    output = ['static const Sc64RaceMap sc64_race_maps[SC64_RACE_MAP_COUNT] = {\n',
              *descriptors[:4], '#ifdef SC64_UNIFIED\n', *descriptors[4:6],
              '#endif /* SC64_UNIFIED */\n']
    for global_id in range(6, 30):
        higher = [feature_condition(index) for index, mapped_id in enumerate(ids)
                  if mapped_id >= global_id and index >= 6]
        if not higher:
            continue
        output.append(f'#if defined(SC64_UNIFIED) && ({" || ".join(higher)})\n')
        index = by_id.get(global_id)
        if index is None:
            output.append(f'    {{0}}, /* global ID {global_id} unavailable */\n')
        else:
            output.append(f'#if {feature_condition(index)}\n')
            output.append(descriptors[index])
            output.append(f'#else\n    {{0}}, /* global ID {global_id} unavailable */\n#endif\n')
        output.append('#endif\n')
    output.extend(['};\n'])
    return ''.join(output)


def global_id_for_map(map_index: int) -> int:
    """Map source-order index to its stable campaign catalog ID."""
    if map_index < 6:
        return map_index
    if map_index == 6:
        return 6
    if map_index == 7:
        return 14
    if map_index == 8:
        return 22
    if map_index == 9:
        return 7
    if map_index == 10:
        return 8
    if map_index == 11:
        return 9
    if 12 <= map_index <= 15:
        return map_index - 2
    if 16 <= map_index <= 22:
        return map_index - 1
    if 23 <= map_index <= 29:
        return map_index
    raise ValueError(f'unknown race map index {map_index}')


def feature_condition(map_index: int) -> str:
    """Compile-time gate that controls whether this map contributes a slot."""
    if map_index < 4:
        return '1'
    if map_index in (4, 5):
        return 'defined(SC64_UNIFIED)'
    if map_index == 6:
        return 'defined(SC64_T3_ENABLED)'
    if map_index == 7:
        return 'defined(SC64_Z3_ENABLED)'
    if map_index == 8:
        return 'defined(SC64_P3_ENABLED)'
    if map_index == 9:
        return 'defined(SC64_T4_ENABLED)'
    if map_index == 10:
        return 'defined(SC64_T5_ENABLED)'
    if map_index == 11:
        return 'defined(SC64_T6_ENABLED)'
    if 12 <= map_index <= 15:
        return 'defined(SC64_TERRAN_LATE_ENABLED)'
    if 16 <= map_index <= 22:
        return f'defined(SC64_Z{map_index - 12}_DATA_ENABLED)'
    if 23 <= map_index <= 29:
        return f'defined(SC64_P{map_index - 19}_DATA_ENABLED)'
    raise ValueError(f'unknown race map index {map_index}')


def nearest(color: int, palette: list[int], candidates: list[int]) -> int:
    def dist(i: int):
        p = palette[i]
        return sum((((color >> s) & 31) - ((p >> s) & 31)) ** 2 for s in (0, 5, 10))
    return min(candidates, key=lambda i: (dist(i), i))


def retain_source_records(chk: bytes, records_dir: Path, *, map_path: str,
                          rom_sha1: str = ROM_SHA1,
                          extractor_commit: str = UPSTREAM_COMMIT,
                          retain_puni: bool = False) -> dict:
    """Write exact framed campaign CHK records and return hash/size metadata.

    This intentionally stores only the bounded source records required for
    scenario and trigger auditing. The input is a CHK byte string returned
    by the pinned BOLT extractor; no Blizzard records are added to tracked
    project files.
    """
    retained_tags = dict(RETAINED_SOURCE_RECORDS)
    # PUNI is retained only for T5; keep the existing T4 and earlier record
    # sets and manifests byte-for-byte stable.
    if retain_puni:
        retained_tags[b'PUNI'] = 'PUNI.bin'
    found: dict[bytes, list[bytes]] = {tag: [] for tag in retained_tags}
    offset = 0
    while offset < len(chk):
        if len(chk) - offset < 8:
            raise ValueError('campaign CHK has a truncated section header')
        tag = chk[offset:offset + 4]
        size = struct.unpack_from('<I', chk, offset + 4)[0]
        end = offset + 8 + size
        if end > len(chk):
            raise ValueError(f'campaign CHK section {tag!r} exceeds the CHK bounds')
        if tag in found:
            found[tag].append(chk[offset:end])
        offset = end
    for tag, records in found.items():
        if len(records) != 1:
            raise ValueError(f'campaign CHK requires exactly one {tag.decode("ascii")} section; found {len(records)}')

    records_dir.mkdir(parents=True, exist_ok=True)
    metadata = {}
    for tag, filename in retained_tags.items():
        record = found[tag][0]
        payload_size = struct.unpack_from('<I', record, 4)[0]
        if tag == b'PUNI' and payload_size != 5700:
            raise ValueError(f'T5 PUNI payload must be 5700 bytes; found {payload_size}')
        path = records_dir / filename
        # Keep every retained record in its exact framed CHK representation.
        path.write_bytes(record)
        metadata[tag.decode('ascii').strip()] = {
            'file': filename,
            'record_bytes': len(record),
            'payload_bytes': payload_size,
            'record_sha256': hashlib.sha256(record).hexdigest(),
            'payload_sha256': hashlib.sha256(record[8:]).hexdigest(),
        }
    chk_sha256 = hashlib.sha256(chk).hexdigest()
    manifest = {
        'schema': 1,
        'source_map_path': map_path,
        'source_rom_sha1': rom_sha1,
        'source_chk_sha256': chk_sha256,
        'extractor': 'references/sc64-maps/extract_sc64_maps.py',
        'extractor_commit': extractor_commit,
        'records': metadata,
    }
    manifest_path = records_dir.parent / 'manifest.json'
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n', encoding='ascii')
    return {'manifest': str(manifest_path), 'source_chk_sha256': chk_sha256,
            'records': metadata}


def retain_t4_source_records(chk: bytes, records_dir: Path, *,
                             rom_sha1: str = ROM_SHA1,
                             extractor_commit: str = UPSTREAM_COMMIT) -> dict:
    """Backward-compatible T4 wrapper around the shared record retainer."""
    return retain_source_records(chk, records_dir, map_path='008/00D',
                                 rom_sha1=rom_sha1,
                                 extractor_commit=extractor_commit)


def retain_complete_map_source(chk: bytes, source_dir: Path, *, map_path: str,
                               mission_id: str,
                               rom_sha1: str = ROM_SHA1,
                               extractor_commit: str = UPSTREAM_COMMIT) -> dict:
    """Retain the complete bounded CHK for source auditing, byte-for-byte."""
    if not chk or len(chk) > 1024 * 1024:
        raise ValueError(f'{mission_id} source CHK size is outside the 1 MiB bound')
    sections = []
    offset = 0
    while offset < len(chk):
        if len(chk) - offset < 8:
            raise ValueError(f'{mission_id} CHK has a truncated section header')
        tag = chk[offset:offset + 4]
        size = struct.unpack_from('<I', chk, offset + 4)[0]
        end = offset + 8 + size
        if end > len(chk):
            raise ValueError(f'{mission_id} CHK section {tag!r} exceeds the bounds')
        sections.append({'tag': tag.decode('ascii', errors='replace'),
                          'record_bytes': end - offset,
                          'payload_bytes': size,
                          'record_sha256': hashlib.sha256(chk[offset:end]).hexdigest()})
        if len(sections) > 128:
            raise ValueError(f'{mission_id} CHK exceeds 128 section records')
        offset = end
    unit_sections = [s for s in sections if s['tag'] == 'UNIT']
    if len(unit_sections) != 1:
        raise ValueError(f'{mission_id} source requires exactly one UNIT section')
    source_dir.mkdir(parents=True, exist_ok=True)
    chk_path = source_dir / 'source.chk'
    chk_path.write_bytes(chk)
    manifest = {
        'schema': 1, 'mission_id': mission_id, 'source_map_path': map_path,
        'source_rom_sha1': rom_sha1, 'source_chk_bytes': len(chk),
        'source_chk_sha256': hashlib.sha256(chk).hexdigest(),
        'source_chk_file': chk_path.name,
        'extractor': 'references/sc64-maps/extract_sc64_maps.py',
        'extractor_commit': extractor_commit, 'sections': sections,
    }
    manifest_path = source_dir / 'manifest.json'
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n', encoding='ascii')
    return {'directory': str(source_dir), 'source_chk_file': str(chk_path),
            'manifest': str(manifest_path),
            'source_chk_sha256': manifest['source_chk_sha256'],
            'source_chk_bytes': len(chk), 'section_count': len(sections)}


def verify_extractor_source(path: Path | None = None) -> None:
    """Refuse a dirty or substituted copy of the pinned extractor script."""
    path = path or ROOT / 'references/sc64-maps/extract_sc64_maps.py'
    try:
        expected = subprocess.run(
            ['git', '-C', str(path.parent), 'show',
             f'{UPSTREAM_COMMIT}:{path.name}'], check=True, capture_output=True).stdout
    except subprocess.CalledProcessError as exc:
        raise ValueError('cannot verify pinned BOLT extractor source bytes') from exc
    if path.read_bytes() != expected:
        raise ValueError('pinned BOLT extractor source differs from its committed bytes')


def source_actor_audit(rows: list[dict], race: int,
                       support_profile: str = 'legacy') -> dict:
    """Classify source placements and report the selected profile's mapping gaps.

    ``legacy`` preserves the original common-profile capability census.
    ``z7`` adds only the actor/building bindings enabled by the Z7 profile's
    ZP and Z6 gates; it does not imply those bindings exist in earlier ROMs.
    """
    if support_profile not in ('legacy', 'z7'):
        raise ValueError(f'unknown runtime support profile: {support_profile}')
    if support_profile == 'z7' and race != 1:
        raise ValueError('the Z7 support profile is only valid for Zerg')
    counts = Counter(row['source_type'] for row in rows)
    categories = Counter()
    unsupported = Counter()
    if support_profile == 'z7':
        units_supported = RUNTIME_UNIT_RAW_IDS_Z7
        buildings_supported = RUNTIME_BUILDING_RAW_IDS_Z7
    else:
        units_supported = (RUNTIME_UNIT_RAW_IDS_ZERG if race == 1
                           else RUNTIME_UNIT_RAW_IDS_PROTOSS)
        buildings_supported = RUNTIME_BUILDING_RAW_IDS
    for raw_type, count in counts.items():
        if raw_type == 214:
            categories['start_markers'] += count
        elif raw_type in (176, 177, 178):
            categories['mineral_fields'] += count
        elif raw_type == 188:
            categories['geysers'] += count
        elif raw_type in SOURCE_BUILDING_RAW_IDS:
            categories['building_slots'] += count
            if raw_type not in buildings_supported:
                unsupported[raw_type] += count
        elif raw_type in SOURCE_UNIT_RAW_IDS:
            categories['unit_slots'] += count
            if raw_type not in units_supported:
                unsupported[raw_type] += count
        else:
            categories['unclassified_actor_slots'] += count
            unsupported[raw_type] += count
    return {
        'source_pool_demand': {
            'unit_slots': categories['unit_slots'],
            'unit_slots_upper_bound_including_unclassified': (
                categories['unit_slots'] + categories['unclassified_actor_slots']),
            'building_slots': categories['building_slots'],
            'building_slots_upper_bound_including_unclassified': (
                categories['building_slots'] + categories['unclassified_actor_slots']),
            'mineral_fields': categories['mineral_fields'],
            'geysers': categories['geysers'],
            'start_markers_excluded_from_actor_pools': categories['start_markers'],
            'unclassified_actor_slots': categories['unclassified_actor_slots'],
        },
        'source_raw_type_counts': {str(raw): count for raw, count in sorted(counts.items())},
        'unsupported_or_unclassified_raw_types': [
            {'raw_type': raw, 'count': count,
             'source_class': ('building' if raw in SOURCE_BUILDING_RAW_IDS else
                              'unit' if raw in SOURCE_UNIT_RAW_IDS else 'unclassified')}
            for raw, count in sorted(unsupported.items())],
    }


def source_placement_category(raw_type: int, map_index: int,
                              mission_id: str | None = None) -> str:
    """Classify a source UNIT row for its authored per-map census only.

    Special raw-195 handling is intentionally map-scoped: T9 retains its
    unclassified report, T10 retains its building report, and staged Z5's
    seven Terran Beacons count as building placements. This does not alter
    runtime mappings or pooled actor imports.
    """
    if raw_type == 214:
        return 'start_marker'
    if raw_type in (176, 177, 178):
        return 'mineral'
    if raw_type == 188:
        return 'geyser'
    if map_index == 9 and raw_type == 195:
        return 'unclassified'
    if (raw_type in BUILDING_RAW_IDS or
            (raw_type == 195 and (map_index == 10 or mission_id == 'Z5'))):
        return 'building'
    if map_index == 11 and raw_type in (126, 138):
        return 'building'
    return 'unit'


def pack(rom_path: Path = ROM, base: Path = BASE, output: Path = OUTPUT, include_cover: bool = False) -> dict:
    verify_extractor_source()
    importer = load('sc64_races_import', ROOT / 'tools/sc64_import.py')
    assets = load('sc64_races_assets', ROOT / 'tools/sc64_assets.py')
    x = importer.upstream_module(importer.UPSTREAM)
    rom = x.load_rom(str(rom_path))
    if hashlib.sha1(rom).hexdigest() != ROM_SHA1:
        raise ValueError('ROM does not match the verified StarCraft 64 USA image')
    archive = x.BoltArchive(rom)
    entries = {entry.path: entry for entry in archive.entries()}
    def read(path: str) -> bytes:
        try:
            return archive.read(entries[path])
        except KeyError as exc:
            raise ValueError(f'Missing BOLT terrain dependency {path}') from exc

    backwater_palette = json.loads((base / 'palette.json').read_text())['gba_rgb555']
    remap_data = (base / 'palette-remap.bin').read_bytes()
    if len(backwater_palette) != 256 or len(remap_data) != 256:
        raise ValueError('Prepare Backwater generated palette/remap before packing race maps')
    shared_indices = sorted({i for i in remap_data if i >= 32})
    if not shared_indices:
        raise ValueError('Backwater sprite palette profile has no world colors')

    maps = list(x.find_maps(archive))
    lines = [
        '/* Private generated data. Do not commit or distribute. */\n',
        '/* Source: references/sc64-maps/extract_sc64_maps.py @ ' + UPSTREAM_COMMIT + ' */\n',
        '#ifndef SC64_RACES_GENERATED_H\n#define SC64_RACES_GENERATED_H\n',
        '#include "sc64_types.h"\n',
        emit_map_count(),
        'typedef struct {\n'
        '    unsigned short width, height;\n'
        '    unsigned char tileset_id, human_slot;\n'
        '    const unsigned char *owner_modes, *owner_races;\n'
        '    const Sc64ImportedUnit *units; unsigned short unit_count;\n'
        '    const unsigned char (*tile_pixels)[256]; unsigned short tile_count;\n'
        '    const unsigned short *tile_lookup; const unsigned char *blocked;\n'
        '    const char *title; const char *mission_id;\n'
        '} Sc64RaceMap;\n',
    ]
    reports = []
    descriptors = []
    for map_index, (mission_id, path, _title) in enumerate(MISSIONS):
        selected = next(((i, chk) for i, chk in maps if i.bolt_path == path), None)
        if selected is None:
            raise ValueError(f'Mission {path} is absent from the verified ROM')
        info, chk = selected
        sections = importer.sections(chk, x)
        owners = sections.get(b'OWNR', b'')
        sides = sections.get(b'SIDE', b'')
        human_slots = [slot for slot, owner in enumerate(owners) if owner == 6]
        if not human_slots or len(sides) < len(owners):
            raise ValueError(f'{path} has no human OWNR/SIDE slot')
        if len(human_slots) != 1 and map_index < 16:
            raise ValueError(f'{path} has no unique human OWNR/SIDE slot')
        # New map imports are data-only. When their original OWNR has several
        # human slots, use the first only as a bounded descriptor placeholder;
        # preserve and report the complete OWNR/SIDE source records separately.
        human_slot = human_slots[0]
        owner_modes = [2 if owner == 11 else 3 if owner == 3 else
                       0 if owner == 6 else 1 if owner in (1, 5, 7) else 2
                       for owner in owners[:8]]
        # CHK SIDE uses Terran=1, Zerg=0, Protoss=2; normalize into the
        # compact runtime enum (R_TER=0, R_ZERG=1, R_PROTOSS=2).
        source_sides = list(sides[:8])
        owner_races = [0 if side == 1 else 1 if side == 0 else 2 if side == 2 else 3
                       for side in source_sides]
        force_data = sections.get(b'FORC', b'')
        force_assignments = list(force_data[:8]) if len(force_data) >= 8 else []
        raw = sections.get(b'MTXM', b'')
        if len(raw) != info.width * info.height * 2:
            raise ValueError(f'{path} MTXM dimensions do not match')
        tile_ids = list(struct.unpack(f'<{len(raw) // 2}H', raw))
        tileset = info.tileset & 7
        # BOLT/00B stores eight consecutive 15-entry graphics groups, one per
        # StarCraft tileset. Each group's first five files are VR4/VX4/VF4/CV5/WPE.
        asset_base = tileset * 0x0f
        sources = {label: read(f'00B/{asset_base + index:03X}') for label, index in
                   [('vr4', 0), ('vx4', 1), ('vf4', 2), ('cv5', 3), ('palette', 4)]}
        decoded = {t: assets.decode_tile(t, sources['cv5'], sources['vx4'],
                                         sources['vf4'], sources['vr4']) for t in set(tile_ids)}
        source_palette = assets.palette_rgb555(sources['palette'])
        # Terrain pixels and sprites share the existing Backwater palette.
        # Quantize every source terrain color offline; runtime rendering then
        # uses the same fixed palette with no per-map sprite recoloring.
        palette_remap = bytes(nearest(color, backwater_palette, shared_indices)
                              for color in source_palette)
        tiles: list[bytes] = []
        lookup: list[int] = []
        blocked: list[int] = []
        dedup: dict[bytes, int] = {}
        for tile_id in tile_ids:
            pixels, walk = decoded[tile_id]
            packed = pixels.translate(palette_remap)
            if packed not in dedup:
                dedup[packed] = len(tiles)
                tiles.append(packed)
            lookup.append(dedup[packed])
            blocked.append(int((walk & 0x0660) != 0x0660) | (assets.cv5_cover(tile_id, sources['cv5']) << 1 if include_cover else 0))
        units = [u for u in importer.unit_rows(sections.get(b'UNIT', b'')) if u['source_type'] != 214]
        name = mission_id.lower()
        # The race-only binary and tests must not acquire the much larger
        # Terran campaign payload. Unified builds opt into both map datasets.
        if map_index == 4:
            lines.append('#ifdef SC64_UNIFIED\n')
        elif map_index == 6:
            lines.append('#if defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)\n')
        elif map_index == 7:
            lines.append('#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n')
        elif map_index == 8:
            lines.append('#if defined(SC64_UNIFIED) && defined(SC64_P3_ENABLED)\n')
        elif map_index == 9:
            lines.append('#if defined(SC64_UNIFIED) && defined(SC64_T4_ENABLED)\n')
        elif map_index == 10:
            lines.append('#if defined(SC64_UNIFIED) && defined(SC64_T5_ENABLED)\n')
        elif map_index == 11:
            lines.append('#if defined(SC64_UNIFIED) && defined(SC64_T6_ENABLED)\n')
        elif map_index >= 16:
            lines.append(f'#if defined(SC64_UNIFIED) && defined(SC64_{mission_id}_DATA_ENABLED)\n')
        elif 12 <= map_index <= 15:
            lines.append('#if defined(SC64_UNIFIED) && defined(SC64_TERRAN_LATE_ENABLED)\n')
        campaign_guard = campaign_guard_for_map(map_index)
        lines.append(f'#if {campaign_guard}\n')
        unit_name = f'sc64_race_{name}_units'
        lines.append(f'static const Sc64ImportedUnit {unit_name}[{len(units)}] = {{\n')
        for unit in units:
            if unit['flags'] > 0xffff or unit['resources'] > 0xffff:
                raise ValueError(f'{path} unit fields exceed runtime schema')
            lines.append(f"    {{{unit['x_source_px']//2}u,{unit['y_source_px']//2}u,{unit['source_type']}u,{unit['owner_raw']}u,{unit['hp_percent']}u,{unit['flags']}u,{unit['resources']}u}},\n")
        lines.append('};\n')
        lines.append(c_array(f'sc64_race_{name}_owner_modes', 'unsigned char', owner_modes, 8))
        lines.append(c_array(f'sc64_race_{name}_owner_races', 'unsigned char', owner_races, 8))
        lines.append(c_array(f'sc64_race_{name}_lookup', 'unsigned short', lookup))
        lines.append(c_array(f'sc64_race_{name}_blocked', 'unsigned char', blocked))
        lines.append(f'static const unsigned char sc64_race_{name}_tiles[{len(tiles)}][256] __attribute__((aligned(4))) = {{\n')
        lines.extend('{' + ','.join(map(str, tile)) + '},\n' for tile in tiles)
        lines.append('};\n')
        lines.append('#endif /* selected campaign assets */\n')
        if map_index == 5:
            lines.append('#endif /* SC64_UNIFIED */\n')
        elif map_index == 6:
            lines.append('#endif /* SC64_UNIFIED && SC64_T3_ENABLED */\n')
        elif map_index == 7:
            lines.append('#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n')
        elif map_index == 8:
            lines.append('#endif /* SC64_UNIFIED && SC64_P3_ENABLED */\n')
        elif map_index == 9:
            lines.append('#endif /* SC64_UNIFIED && SC64_T4_ENABLED */\n')
        elif map_index == 10:
            lines.append('#endif /* SC64_UNIFIED && SC64_T5_ENABLED */\n')
        elif map_index == 11:
            lines.append('#endif /* SC64_UNIFIED && SC64_T6_ENABLED */\n')
        elif map_index >= 16:
            lines.append(f'#endif /* SC64_UNIFIED && SC64_{mission_id}_DATA_ENABLED */\n')
        elif 12 <= map_index <= 15:
            lines.append('#endif /* SC64_UNIFIED && SC64_TERRAN_LATE_ENABLED */\n')
        descriptors.append(f'#if {campaign_guard}\n'
            f'    {{{info.width}u,{info.height}u,{tileset}u,{human_slot}u,sc64_race_{name}_owner_modes,sc64_race_{name}_owner_races,{unit_name},{len(units)}u,sc64_race_{name}_tiles,{len(tiles)}u,sc64_race_{name}_lookup,sc64_race_{name}_blocked,"{info.name}","{mission_id}"}},\n'
            '#else\n    {0}, /* campaign omitted from this cartridge */\n#endif\n')
        triggers = importer.trigger_rows(sections.get(b'TRIG', b''), sections.get(b'STR ', b''), x)
        objectives = [a.get('text') for tr in triggers for a in tr['actions'] if a['opcode'] == 12 and a.get('text')]
        reports.append({
            'mission_id': mission_id, 'path': path, 'title': info.name,
            'dimensions': [info.width, info.height], 'tileset_id': tileset,
            'human_slot': human_slot, 'human_side': sides[human_slot],
            'human_slot_candidates': human_slots,
            'human_slot_is_representative_only': len(human_slots) != 1,
            'owner_modes': owner_modes,
            'owner_races': owner_races,
            'force_assignments_raw': force_assignments,
            'source_sides': source_sides,
            'chk_sha1': hashlib.sha1(chk).hexdigest(), 'unit_records': len(units),
            'trigger_records': len(triggers), 'objective_text': objectives,
            'unique_source_tiles': len(decoded), 'unique_runtime_tiles': len(tiles),
            'cells': len(tile_ids), 'blocked_cells': sum(v & 1 for v in blocked),
            'terrain_assets_sha256': {k: hashlib.sha256(v).hexdigest() for k, v in sources.items()},
        })
        if map_index in (0, 1, 2, 3, 7, 8) or map_index >= 16:
            source_units = importer.unit_rows(sections.get(b'UNIT', b''))
            source_starts = sum(u['source_type'] == 214 for u in source_units)
            if len(units) != len(source_units) - source_starts:
                raise ValueError(f'{mission_id} importer did not retain every non-start UNIT row')
            race = 1 if mission_id.startswith('Z') else 2
            reports[-1].update(source_actor_audit(
                source_units, race,
                support_profile='z7' if mission_id == 'Z7' else 'legacy'))
            reports[-1]['source_unit_records'] = len(source_units)
            reports[-1]['runtime_unit_records'] = len(units)
            reports[-1]['runtime_unit_rows_are_all_non_start_source_rows'] = True
            reports[-1]['global_mission_id'] = global_id_for_map(map_index)
            if map_index >= 16:
                chk_hash = hashlib.sha256(chk).hexdigest()
                if chk_hash != RACE_EXTRA_CHK_SHA256[path]:
                    raise ValueError(f'{mission_id} CHK hash does not match its audited source')
                reports[-1]['global_mission_id'] = global_id_for_map(map_index)
                reports[-1]['campaign_data_gate'] = f'SC64_{mission_id}_DATA_ENABLED'
                reports[-1]['runtime_or_catalog_enabled'] = False
                reports[-1]['source_chk_sha256'] = chk_hash
                reports[-1]['source_record_retention'] = retain_complete_map_source(
                    chk, output.parent / 'source_maps' / mission_id.lower(),
                    map_path=path, mission_id=mission_id)
        if map_index >= 4:
            source_units = importer.unit_rows(sections.get(b'UNIT', b''))
            source_categories = Counter(
                source_placement_category(u['source_type'], map_index, mission_id)
                for u in source_units
            )
            reports[-1]['source_unit_records'] = len(source_units)
            reports[-1]['runtime_unit_records'] = len(units)
            reports[-1]['excluded_start_markers'] = source_categories['start_marker']
            reports[-1]['source_categories'] = {
                'unit_placements': source_categories['unit'],
                'building_placements': source_categories['building'],
                'mineral_fields': source_categories['mineral'],
                'geysers': source_categories['geyser'],
                'unclassified_source_placements': source_categories['unclassified'],
            }
            if map_index >= 16:
                demand = reports[-1]['source_pool_demand']
                reports[-1]['source_categories'] = {
                    'unit_placements': demand['unit_slots'],
                    'building_placements': demand['building_slots'],
                    'mineral_fields': demand['mineral_fields'],
                    'geysers': demand['geysers'],
                    'start_markers': demand['start_markers_excluded_from_actor_pools'],
                    'unclassified_source_placements': demand['unclassified_actor_slots'],
                }
            reports[-1]['runtime_unit_locations'] = [
                {'source_type': u['source_type'], 'owner_raw': u['owner_raw'],
                 'x': u['x_source_px'] // 2, 'y': u['y_source_px'] // 2,
                 'hp_percent': u['hp_percent'], 'flags': u['flags'],
                 'resources': u['resources']}
                for u in units
            ]
            reports[-1]['rescuable_units'] = [
                {'source_type': u['source_type'], 'owner_raw': u['owner_raw'],
                 'x': u['x_source_px'] // 2, 'y': u['y_source_px'] // 2}
                for u in units if u['owner_raw'] == 3
            ]
            if map_index == 7:
                expected = {'unit_placements': 58, 'building_placements': 33,
                            'mineral_fields': 28, 'geysers': 3}
                category_keys = {
                    'unit_placements': 'unit', 'building_placements': 'building',
                    'mineral_fields': 'mineral', 'geysers': 'geyser',
                }
                actual = {key: source_categories[source_key]
                          for key, source_key in category_keys.items()}
                if actual != expected or source_categories['start_marker'] != 2:
                    raise ValueError(f'Z3 source UNIT roster changed: {actual}, '
                                     f"start markers={source_categories['start_marker']}")
                if len(units) != sum(expected.values()):
                    raise ValueError('Z3 importer skipped or duplicated non-start UNIT rows')
                if len(force_assignments) != 8 or force_assignments[3] != 0 or force_assignments[6] != 1:
                    raise ValueError('Z3 human/enemy force assignment no longer matches audited CHK')
                reports[-1]['global_mission_id'] = 14
                reports[-1]['force_assignment_summary'] = {
                    'human_owner_slot_3': force_assignments[3],
                    'enemy_owner_slot_6': force_assignments[6],
                    'interpretation': 'Force 1 and Force 2 respectively; raw FORCE section bytes are preserved above',
                }
                reports[-1]['runtime_unit_rows_are_all_non_start_source_rows'] = True
            if map_index == 8:
                expected = {'unit_placements': 96, 'building_placements': 42,
                            'mineral_fields': 48, 'geysers': 3}
                category_keys = {
                    'unit_placements': 'unit', 'building_placements': 'building',
                    'mineral_fields': 'mineral', 'geysers': 'geyser',
                }
                actual = {key: source_categories[source_key]
                          for key, source_key in category_keys.items()}
                if actual != expected or source_categories['start_marker'] != 3:
                    raise ValueError(f'P3 source UNIT roster changed: {actual}, '
                                     f"start markers={source_categories['start_marker']}")
                if len(units) != 189:
                    raise ValueError('P3 importer skipped or duplicated non-start UNIT rows')
                if len(force_assignments) < 8 or force_assignments[:8] != [1, 3, 3, 3, 3, 3, 2, 0]:
                    raise ValueError('P3 source force assignments no longer match audited CHK')
                raw_owners = Counter((u['source_type'], u['owner_raw']) for u in units)
                if raw_owners[(47, 6)] != 8 or raw_owners[(133, 0)] != 1:
                    raise ValueError('P3 Scourge/Hive source owner roster changed')
                if raw_owners[(137, 0)] != 1 or raw_owners[(137, 6)] != 1:
                    raise ValueError('P3 Greater Spire source owner roster changed')
                reports[-1]['global_mission_id'] = 22
                reports[-1]['raw_type_counts_by_owner_slot'] = {
                    str(owner): {str(raw): count for (raw, slot), count in raw_owners.items()
                                 if slot == owner}
                    for owner in sorted({slot for _, slot in raw_owners})
                }
                reports[-1]['force_owner_policy'] = {
                    'human_source_slot_7': 'SIDE (owner 0)',
                    'force_2_source_slot_0': 'ENEMY (owner 1)',
                    'force_3_source_slot_6': 'distinct hostile owner 4; RESCUABLE owner 3 remains reserved',
                    'source_slot_and_force_assignment_are_preserved_in_source_unit_records': True,
                    'runtime_note': 'descriptor owner_modes intentionally classifies both computer slots as enemies; P3 runtime must map by source owner slot and treat Force 2/3 as allied to each other and hostile to the human',
                }
                reports[-1]['trigger_source_facts'] = {
                    'human_slot_7_start_resources': {'minerals': 300, 'gas': 100},
                    'force_2_slot_0_start_resources': {'combined': 5000},
                    'force_3_slot_6_start_resources': {'combined': 8000},
                    'human_any_units_loss_condition': {'comparison': 'at_most', 'amount': 0, 'type': 'Any Unit'},
                    'foes_buildings_at_most_zero_victory': {
                        'wait_ms': 2000, 'transmissions': 2, 'post_transmission_wait_ms': 3000,
                    },
                    'raw_trigger_records_preserved_in_audit': True,
                    'runtime_trigger_behavior_claimed': False,
                }
                reports[-1]['runtime_unit_rows_are_all_non_start_source_rows'] = True
            if map_index == 9:
                expected_raw_types = {0: 27, 1: 1, 2: 2, 3: 1, 7: 6, 15: 31,
                                      20: 1, 32: 8, 37: 5, 89: 2, 95: 3, 195: 8}
                source_types = Counter(u['source_type'] for u in units)
                if (info.width, info.height, tileset, human_slot) != (128, 128, 2, 0):
                    raise ValueError('T4 dimensions, Installation tileset, or human slot changed')
                if len(source_units) != 102 or len(units) != 95 or source_categories['start_marker'] != 7:
                    raise ValueError('T4 importer skipped or duplicated UNIT/start-marker rows')
                if dict(source_types) != expected_raw_types:
                    raise ValueError(f'T4 source UNIT type histogram changed: {dict(source_types)}')
                if force_assignments[:8] != [0, 2, 2, 1, 3, 3, 3, 2]:
                    raise ValueError('T4 source force assignments no longer match audited CHK')
                if source_categories['unclassified'] != 8 or source_categories['building'] != 0:
                    raise ValueError('T4 unknown raw195 rows must remain unclassified, not remapped')
                reports[-1]['global_mission_id'] = 7
                reports[-1]['raw_type_counts_by_owner_slot'] = {
                    str(owner): {str(raw): count for (raw, slot), count in
                                 Counter((u['source_type'], u['owner_raw'])
                                         for u in units).items() if slot == owner}
                    for owner in sorted({u['owner_raw'] for u in units})
                }
                reports[-1]['unclassified_source_types'] = {'195': 8}
                reports[-1]['t4_import_only'] = True
                reports[-1]['runtime_or_catalog_enabled'] = False
                reports[-1]['source_record_retention'] = retain_t4_source_records(
                    chk, output.parent / 't4' / 'records')
                reports[-1]['runtime_unit_rows_are_all_non_start_source_rows'] = True
            if map_index == 10:
                expected = {'unit_placements': 79, 'building_placements': 48,
                            'mineral_fields': 41, 'geysers': 0}
                actual = {'unit_placements': source_categories['unit'],
                          'building_placements': source_categories['building'],
                          'mineral_fields': source_categories['mineral'],
                          'geysers': source_categories['geyser']}
                if (info.width, info.height, tileset, human_slot) != (96, 96, 0, 0):
                    raise ValueError('T5 dimensions, Badlands tileset, or human slot changed')
                if len(source_units) != 172 or len(units) != 168 or source_categories['start_marker'] != 4:
                    raise ValueError('T5 importer skipped or duplicated UNIT/start-marker rows')
                if actual != expected:
                    raise ValueError(f'T5 source UNIT categories changed: {actual}')
                if force_assignments[:8] != [0, 3, 2, 2, 3, 3, 1, 3]:
                    raise ValueError('T5 source force assignments no longer match audited CHK')
                if list(owners[:8]) != [6, 0, 3, 7, 0, 0, 5, 0]:
                    raise ValueError('T5 source owner assignments no longer match audited CHK')
                reports[-1]['global_mission_id'] = 8
                reports[-1]['raw_type_counts_by_owner_slot'] = {
                    str(owner): {str(raw): count for (raw, slot), count in
                                 Counter((u['source_type'], u['owner_raw'])
                                         for u in units).items() if slot == owner}
                    for owner in sorted({u['owner_raw'] for u in units})
                }
                reports[-1]['runtime_unit_rows_are_all_non_start_source_rows'] = True
                reports[-1]['t5_import_only'] = True
                reports[-1]['runtime_or_catalog_enabled'] = False
                reports[-1]['source_record_retention'] = retain_source_records(
                    chk, output.parent / 't5' / 'records', map_path=path,
                    retain_puni=True)
                availability_packer = load(
                    'sc64_t5_availability_source', ROOT / 'tools/sc64_t5_availability.py')
                reports[-1]['availability_table'] = availability_packer.generate(
                    output.parent / 't5' / 'records',
                    output.parent / 't5' / 'manifest.json',
                    output.parent / 'sc64_t5_availability_generated.h')
                scenario_packer = load(
                    'sc64_t5_scenario_source', ROOT / 'tools/sc64_t4_scenario_pack.py')
                scenario = scenario_packer.parse_source(
                    output.parent / 't5' / 'records',
                    output.parent / 't5' / 'manifest.json', profile='t5')
                reports[-1]['scenario_source_counts'] = {
                    'locations': len(scenario['locations']),
                    'triggers': len(scenario['triggers']),
                    'doodads': len(scenario['doodads']),
                    'properties': len(scenario['properties']),
                    'active_conditions': scenario['active_condition_count'],
                    'active_actions': scenario['active_action_count'],
                }
            if 12 <= map_index <= 15:
                profile = f't{map_index - 5}'
                chk_hash = hashlib.sha256(chk).hexdigest()
                if chk_hash != TERRAN_LATE_CHK_SHA256[path]:
                    raise ValueError(f'{profile.upper()} CHK hash does not match the audited source')
                reports[-1]['global_mission_id'] = map_index - 2
                reports[-1]['source_chk_sha256'] = chk_hash
                reports[-1]['late_terran_import_only'] = True
                reports[-1]['runtime_or_catalog_enabled'] = False
                reports[-1]['source_record_retention'] = retain_source_records(
                    chk, output.parent / profile / 'records', map_path=path,
                    retain_puni=True)
                availability_packer = load(
                    'sc64_late_availability_source', ROOT / 'tools/sc64_t5_availability.py')
                reports[-1]['availability_table'] = availability_packer.generate(
                    output.parent / profile / 'records',
                    output.parent / profile / 'manifest.json',
                    output.parent / f'sc64_{profile}_availability_generated.h',
                    profile=profile)
                scenario_packer = load(
                    'sc64_late_scenario_source', ROOT / 'tools/sc64_t4_scenario_pack.py')
                reports[-1]['scenario_table'] = scenario_packer.pack(
                    output.parent / profile / 'records',
                    output.parent / profile / 'manifest.json',
                    output.parent / f'sc64_{profile}_scenario_generated.h',
                    profile=profile)
                late_scenario = scenario_packer.parse_source(
                    output.parent / profile / 'records',
                    output.parent / profile / 'manifest.json', profile=profile)
                reports[-1]['scenario_source_counts'] = {
                    'locations': len(late_scenario['locations']),
                    'triggers': len(late_scenario['triggers']),
                    'doodads': len(late_scenario['doodads']),
                    'properties': len(late_scenario['properties']),
                    'active_conditions': late_scenario['active_condition_count'],
                    'active_actions': late_scenario['active_action_count'],
                }
            if map_index == 11:
                if hashlib.sha256(chk).hexdigest() != T6_CHK_SHA256:
                    raise ValueError('T6 CHK hash does not match the audited 008/00F source')
                expected = {'unit_placements': 105, 'building_placements': 73,
                            'mineral_fields': 35, 'geysers': 3}
                actual = {'unit_placements': source_categories['unit'],
                          'building_placements': source_categories['building'],
                          'mineral_fields': source_categories['mineral'],
                          'geysers': source_categories['geyser']}
                if (info.width, info.height, tileset, human_slot) != (96, 96, 0, 0):
                    raise ValueError('T6 dimensions, Badlands tileset, or human slot changed')
                if len(source_units) != 221 or len(units) != 216 or source_categories['start_marker'] != 5:
                    raise ValueError('T6 importer skipped or duplicated UNIT/start-marker rows')
                if actual != expected:
                    raise ValueError(f'T6 source UNIT categories changed: {actual}')
                if force_assignments[:8] != [0, 3, 1, 3, 2, 3, 1, 3]:
                    raise ValueError('T6 source force assignments no longer match audited CHK')
                if list(owners[:8]) != [6, 5, 3, 0, 5, 0, 7, 0]:
                    raise ValueError('T6 source owner assignments no longer match audited CHK')
                reports[-1]['global_mission_id'] = 9
                reports[-1]['raw_type_counts_by_owner_slot'] = {
                    str(owner): {str(raw): count for (raw, slot), count in
                                 Counter((u['source_type'], u['owner_raw'])
                                         for u in units).items() if slot == owner}
                    for owner in sorted({u['owner_raw'] for u in units})
                }
                reports[-1]['runtime_unit_rows_are_all_non_start_source_rows'] = True
                reports[-1]['source_actor_identity_notes'] = {
                    'raw_126': 'Special_Crashed_Norad_II; source building classification',
                    'raw_138': 'Queens_Nest; source building classification',
                    'raw_195': 'Terran Beacon; source UNIT category but runtime building pool classification',
                    'runtime_pool_counts_if_all_source_types_map': {
                        'ordinary_units': 104, 'buildings': 74,
                    },
                }
                reports[-1]['t6_import_only'] = True
                reports[-1]['runtime_or_catalog_enabled'] = False
                reports[-1]['source_record_retention'] = retain_source_records(
                    chk, output.parent / 't6' / 'records', map_path=path,
                    retain_puni=True)
                scenario_packer = load(
                    'sc64_t6_scenario_source', ROOT / 'tools/sc64_t4_scenario_pack.py')
                scenario = scenario_packer.parse_source(
                    output.parent / 't6' / 'records',
                    output.parent / 't6' / 'manifest.json', profile='t6')
                reports[-1]['scenario_source_counts'] = {
                    'locations': len(scenario['locations']),
                    'triggers': len(scenario['triggers']),
                    'doodads': len(scenario['doodads']),
                    'properties': len(scenario['properties']),
                    'active_conditions': scenario['active_condition_count'],
                    'active_actions': scenario['active_action_count'],
                }
    lines.extend([emit_descriptor_table(descriptors), '#endif\n'])
    content = ''.join(lines)
    output.parent.mkdir(parents=True, exist_ok=True)
    if include_cover: content='#define SC64_RACES_COVER_DATA 1\n'+content
    output.write_text(content, encoding='ascii')
    race_campaigns = {}
    for race_name, prefix in [('zerg', 'Z'), ('protoss', 'P')]:
        race_maps = [row for row in reports
                     if row['mission_id'].startswith(prefix) and 'source_pool_demand' in row]
        demand_fields = ('unit_slots', 'unit_slots_upper_bound_including_unclassified',
                         'building_slots', 'building_slots_upper_bound_including_unclassified',
                         'mineral_fields', 'geysers',
                         'start_markers_excluded_from_actor_pools', 'unclassified_actor_slots')
        maximum = {}
        for field in demand_fields:
            peak = max((row['source_pool_demand'][field] for row in race_maps), default=0)
            maximum[field] = {
                'slots_or_records': peak,
                'missions': [row['mission_id'] for row in race_maps
                             if row['source_pool_demand'][field] == peak],
            }
        race_campaigns[race_name] = {
            'source_map_count': len(race_maps),
            'maximum_single_mission_pool_demand': maximum,
            'aggregate_staged_records_across_maps': sum(row['source_unit_records']
                                                         for row in race_maps),
            'maximum_dimensions': {
                'width': max((row['dimensions'][0] for row in race_maps), default=0),
                'height': max((row['dimensions'][1] for row in race_maps), default=0),
            },
        }
    report = {
        'schema': 1,
        'source': {'rom_sha1': ROM_SHA1, 'extractor': 'references/sc64-maps/extract_sc64_maps.py', 'extractor_commit': UPSTREAM_COMMIT},
        'maps': [{'index': index, **row} for index, row in enumerate(reports)],
        'race_campaign_source_pool_audit': race_campaigns,
        'source_pool_classification': {
            'buildings': 'source StarCraft structure IDs in SOURCE_BUILDING_RAW_IDS, including raw 150 B_MATURE_CHRYSALIS',
            'units': 'non-resource/non-start source UNIT rows not classified as buildings',
            'raw_216': 'unit U_CHRYSALIS, never a building',
            'start_marker_raw_214': 'reported separately and excluded from unit/building pool demand',
            'unclassified_actor_slots': 'source actor types with no recognized unit/building category; must be resolved before runtime use',
        },
        'header': str(output), 'header_sha256': hashlib.sha256(content.encode('ascii')).hexdigest(),
        'palette_policy': 'all terrain colors are quantized offline to nearest used world color in the fixed Backwater palette; source indices remap directly to shared palette indices 32..255, preserving sprites without runtime recoloring',
        'collision_policy': 'VF4 walk bit 0; central four minitiles required walkable; this is a coarse 16px GBA pathfinding approximation',
    }
    output.with_name('sc64_races_pack_report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROM)
    parser.add_argument('--backwater-generated', type=Path, default=BASE)
    parser.add_argument('--out', type=Path, default=OUTPUT)
    parser.add_argument('--cover', action='store_true')
    args = parser.parse_args()
    try:
        print(json.dumps(pack(args.rom, args.backwater_generated, args.out, args.cover), indent=2))
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f'sc64_races_pack: {exc}', file=sys.stderr)
        raise SystemExit(1)
