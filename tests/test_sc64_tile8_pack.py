"""Synthetic checks for the 8x8 terrain packer; no imported data required."""
import importlib.util
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[1] / 'tools/sc64_tile8_pack.py'
spec = importlib.util.spec_from_file_location('sc64_tile8_pack', TOOL)
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)

HEADER = """#define SC64_RACE_MAP_COUNT 1
typedef struct {
    unsigned short width, height;
    unsigned char tileset_id, human_slot;
    const unsigned char *owner_modes, *owner_races;
    const void *units; unsigned short unit_count;
    const unsigned char (*tile_pixels)[256]; unsigned short tile_count;
    const unsigned short *tile_lookup; const unsigned char *blocked;
    const char *title; const char *mission_id;
} Sc64RaceMap;
#if !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE == 2
static const unsigned short sc64_race_p9_lookup[%(cells)d] = {
%(lookup)s
};
static const unsigned char sc64_race_p9_tiles[2][256] __attribute__((aligned(4))) = {
{%(t0)s},
{%(t1)s},
};
#endif
static const Sc64RaceMap sc64_race_maps[SC64_RACE_MAP_COUNT] = {
    {16u,16u,4u,7u,0,0,0,0u,sc64_race_p9_tiles,2u,sc64_race_p9_lookup,0,"P9","P9"},
};
"""


def tile(fn):
    return [fn(x, y) for y in range(16) for x in range(16)]


class Tile8PackTests(unittest.TestCase):
    def test_flip_flags_reproduce_source(self):
        sub = bytes(range(64))
        for stored, flags in pack.variants(sub):
            rows = [stored[i * 8:i * 8 + 8] for i in range(8)]
            if flags & (1 << 14):
                rows = [r[::-1] for r in rows]
            if flags & (1 << 15):
                rows = rows[::-1]
            self.assertEqual(b''.join(rows), sub)

    def run_pack(self, t0, t1):
        lookup = ','.join(str((i // 16 + i) & 1) for i in range(256))
        text = HEADER % {'cells': 256, 'lookup': lookup,
                         't0': ','.join(map(str, t0)), 't1': ','.join(map(str, t1))}
        with tempfile.TemporaryDirectory() as directory:
            source, out = Path(directory) / 'in.h', Path(directory) / 'out.h'
            source.write_text(text)
            subprocess.run([sys.executable, str(TOOL), '--input', str(source), '--output', str(out)],
                           check=True, capture_output=True)
            return out.read_text()

    def test_dedupes_flips_and_appends_map_fields(self):
        mirrored = tile(lambda x, y: 32 + (x if x < 8 else 15 - x) + (y % 8))
        flat = tile(lambda x, y: 40)
        out = self.run_pack(mirrored, flat)
        pool = re.search(r'sc64_tile8_pool_0\[(\d+)\]\[64\]', out)
        self.assertIsNotNone(pool)
        self.assertEqual(int(pool.group(1)), 2)  # one mirrored quadrant, one flat tile
        self.assertIn('sc64_race_p9_tiles[1][256]', out)
        self.assertIn(',sc64_race_p9_tile8_meta,sc64_tile8_pool_0}', out)
        self.assertIn('#define SC64_TILE8_DATA 1', out)

    def test_alias_index_keeps_map_on_mode4(self):
        out = self.run_pack(tile(lambda x, y: 255), tile(lambda x, y: 40))
        self.assertIn('"P9","P9",0,0}', out)


if __name__ == '__main__':
    unittest.main()
