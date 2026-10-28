# SPDX-License-Identifier: GPL-3.0-or-later
import importlib.util
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_briefing_assets', ROOT / 'tools/sc64_briefing_assets.py')
assets = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(assets)


class Sc64BriefingAssetTests(unittest.TestCase):
    def test_variable_width_font_wrap_preserves_words_and_pixel_bounds(self):
        advances = {ch: 3 for ch in range(32, 127)}
        advances[ord('W')] = 9
        text = 'WW WW thin letters WW small'
        pages = assets.wrap_text(text, rows=2, advances=advances, pixel_width=24)
        self.assertEqual(' '.join(' '.join(pages).split()), text)
        for page in pages:
            self.assertLessEqual(len(page.split('\n')), 2)
            for line in page.split('\n'):
                self.assertLessEqual(sum(advances[ord(ch)] for ch in line), 24)
        with self.assertRaisesRegex(ValueError, 'pixel width'):
            assets.wrap_text('WWW', advances=advances, pixel_width=24)

    def test_catalog_ids_map_to_explicit_n64_briefing_order(self):
        self.assertEqual(len(assets.MISSION_BRIEFING_INDEX), 30)
        self.assertEqual(assets.MISSION_BRIEFING_INDEX[:6],
                         (0x00C, 0x00D, 0x016, 0x017, 0x002, 0x003))
        self.assertEqual(assets.MISSION_BRIEFING_INDEX[-8:],
                         tuple(range(0x018, 0x020)))

    def test_crop_and_resizers_preserve_data_and_pixel_edges(self):
        source = bytes(v for y in range(3) for x in range(4) for v in (x, y, 99))
        self.assertEqual(assets.crop_rgb(source, 4, 3, (1, 1, 2, 2)),
                         bytes((1, 1, 99, 2, 1, 99, 1, 2, 99, 2, 2, 99)))
        with self.assertRaises(ValueError):
            assets.crop_rgb(source, 4, 3, (3, 1, 2, 2))

        solid = bytes((23, 71, 149)) * (8 * 6)
        self.assertEqual(set(assets.resize_crop(solid, 8, 6, 4, 3)),
                         {23, 71, 149})
        checker = bytes(channel for i in range(4 * 3)
                        for channel in ((255, 0, 0) if i % 2 else (0, 0, 0)))
        nearest = assets.resize_nearest(checker, 4, 3, 4, 3)
        self.assertEqual(nearest, checker)

    def test_retiled_bevel_keeps_rails_outside_clear_content_rectangle(self):
        green, red, blue, yellow, white = (0, 255, 0), (255, 0, 0), (0, 0, 255), (255, 255, 0), (255, 255, 255)
        pixels = bytearray(bytes(green) * (8 * 8))
        for y in range(2):
            for x in range(8):
                pixels[(y * 8 + x) * 3:(y * 8 + x + 1) * 3] = bytes(red)
                pixels[((6 + y) * 8 + x) * 3:((6 + y) * 8 + x + 1) * 3] = bytes(blue)
        for y in range(2, 6):
            for x in range(2):
                pixels[(y * 8 + x) * 3:(y * 8 + x + 1) * 3] = bytes(yellow)
            for x in range(6, 8):
                pixels[(y * 8 + x) * 3:(y * 8 + x + 1) * 3] = bytes(white)
        frame = assets.retile_bevel(bytes(pixels), (8, 8), (14, 12),
                                    ((0, 0, 8, 2), (0, 6, 8, 2),
                                     (0, 2, 2, 4), (6, 2, 2, 4)),
                                    (2, 2, 2, 2))

        def pixel(x, y):
            offset = (y * 14 + x) * 3
            return tuple(frame[offset:offset + 3])

        self.assertEqual(pixel(7, 0), red)
        self.assertEqual(pixel(7, 11), blue)
        self.assertEqual(pixel(0, 5), yellow)
        self.assertEqual(pixel(13, 5), white)
        self.assertEqual(pixel(7, 5), green)

    def test_compose_room_keys_green_and_blends_original_panel_color(self):
        room_color = (40, 80, 120)
        room = bytes(room_color) * (2 * 2)
        key_frame = bytes((0, 255, 0)) * (2 * 2)
        base = assets.compose_room(room, (2, 2), [])
        keyed = assets.compose_room(room, (2, 2), [(key_frame, (2, 2), (0, 0, 240, 160))])
        self.assertEqual(keyed, base)

        panel = (41, 82, 41)
        panel_frame = bytes(panel) * (2 * 2)
        mixed = assets.compose_room(room, (2, 2), [(panel_frame, (2, 2), (0, 0, 240, 160))])
        expected = tuple((panel[c] * 48 + room_color[c] * 207) // 255 for c in range(3))
        self.assertEqual(mixed[:3], bytes(expected))

    def test_portrait_resize_keeps_transparency_mask(self):
        rgba = bytes((200, 10, 20, 255, 0, 0, 0, 0))
        rgb, mask = assets.resize_portrait(rgba, 2, 1, 2, 1)
        self.assertEqual(rgb, bytes((200, 10, 20, 0, 0, 0)))
        self.assertEqual(mask, bytes((1, 0)))

    def test_wrap_and_briefing_pages_stay_inside_runtime_limits(self):
        text = 'one two three four five six seven eight nine ten'
        wrapped = assets.wrap_text(text, columns=10, rows=2)
        self.assertEqual(wrapped, ['one two\nthree four', 'five six\nseven', 'eight nine\nten'])
        self.assertEqual(' '.join(' '.join(page.split()) for page in wrapped),
                         ' '.join(text.split()))
        self.assertTrue(all(len(line) <= 10 for page in wrapped for line in page.split('\n')))
        self.assertTrue(all(len(page.split('\n')) <= 2 for page in wrapped))
        with self.assertRaises(ValueError):
            assets.wrap_text('unbreakableword', columns=5)
        with self.assertRaises(ValueError):
            assets.wrap_text('unsupported \u03bb')

        long_body = ' '.join(f'word{i:03d},' for i in range(120))
        briefing = SimpleNamespace(
            bolt_path='007/001',
            objectives=['Capture the point.', 'Protect the squad.'],
            transmissions=[
                SimpleNamespace(kind='TEXT', speaker='Advisor', portrait=0,
                                body=[long_body]),
                SimpleNamespace(kind='TEXTC', speaker='End of Briefing', portrait=None,
                                body=['End of Briefing']),
            ],
        )
        pages = assets.briefing_pages(briefing)
        dialogue_pages = [page for page in pages if page[0] == 'Advisor']
        self.assertGreater(len(dialogue_pages), 1)
        self.assertTrue(all(page[2] == 0 for page in dialogue_pages))
        self.assertTrue(all(len(page[1].split('\n')) <= assets.TEXT_ROWS
                            and all(len(line) <= assets.TEXT_COLUMNS for line in page[1].split('\n'))
                            for page in dialogue_pages))
        self.assertEqual(' '.join(' '.join(page[1].split()) for page in dialogue_pages),
                         ' '.join(long_body.split()))
        self.assertEqual(pages[0][0], 'Advisor')
        self.assertEqual(pages[-1], ('OBJECTIVES', 'Capture the point.\nProtect the squad.', -1))

    def test_generated_header_has_runtime_contract_and_compiles(self):
        compiler = shutil.which('clang') or shutil.which('cc')
        if not compiler:
            self.skipTest('No C compiler is available')
        background = bytes((32 + i % 224) for i in range(assets.WIDTH * assets.HEIGHT))
        portraits = [bytes([0 if n == 0 else 32 + n]) *
                     (assets.PORTRAIT_WIDTH * assets.PORTRAIT_HEIGHT)
                     for n in range(assets.PORTRAIT_COUNT)]
        palette = [i & 0x7fff for i in range(assets.PALETTE_SIZE)]
        pages = [[('OBJECTIVES', 'Hold position.', -1), ('Advisor', 'All units ready.', 0)]
                 for _ in assets.MISSION_BRIEFING_INDEX]
        header = assets.build_header(background, portraits, palette, pages)
        with tempfile.TemporaryDirectory() as tmp:
            temp = Path(tmp)
            (temp / 'sc64_briefing_generated.h').write_text(header)
            source = temp / 'check.c'
            source.write_text(r'''#include "sc64_briefing_generated.h"
_Static_assert(sizeof(sc64_briefing_palette) / sizeof(sc64_briefing_palette[0]) == 224, "palette");
_Static_assert(sizeof(sc64_briefing_background) == 240 * 160, "background");
_Static_assert(sizeof(sc64_briefing_portraits) == 23 * 40 * 38, "portraits");
_Static_assert(__alignof__(sc64_briefing_background) >= 4, "DMA background alignment");
_Static_assert(__alignof__(sc64_briefing_portraits) >= 4, "portrait alignment");
int main(void) {
    const Sc64Briefing *b = sc64_briefing_get(29);
    if (!b || b->page_count != 2 || b->pages[1].portrait != 0) return 1;
    if (sc64_briefing_get(-1) || sc64_briefing_get(30)) return 2;
    return sc64_briefing_background[0] < 32 || sc64_briefing_portraits[0][0] != 0;
}
''')
            binary = temp / 'check'
            subprocess.run([compiler, '-std=c11', '-Werror', '-I', str(ROOT / 'common'),
                            '-I', str(ROOT / 'starcraft'), '-I', str(temp),
                            str(source), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    unittest.main()
