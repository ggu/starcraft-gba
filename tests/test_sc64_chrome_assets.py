#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic checks for the private SC64 menu-chrome image packer."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_chrome_assets', ROOT / 'tools' / 'sc64_chrome_assets.py')
chrome = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(chrome)


class ChromeAssetTests(unittest.TestCase):
    def test_c_array_reader_handles_attributes_comments_and_hex(self):
        source = '''static const u8 pixels[4] __attribute__((aligned(4))) = {
            0, 0x20, /* data */ 255, 32 // tail
        };'''
        self.assertEqual(chrome.c_array_values(source, 'pixels', 4), [0, 32, 255, 32])

    def test_c_array_reader_rejects_missing_or_wrong_size(self):
        with self.assertRaises(ValueError):
            chrome.c_array_values('static const u8 other[1]={0};', 'pixels')
        with self.assertRaises(ValueError):
            chrome.c_array_values('static const u8 pixels[1]={0};', 'pixels', 2)

    def test_rgb555_expansion_and_reserved_index_validation(self):
        self.assertEqual(chrome.rgb555_to_rgb(0x7FFF), (255, 255, 255))
        self.assertEqual(chrome.rgb555_to_rgb(0), (0, 0, 0))
        self.assertEqual(chrome.indexed_rgb(bytes([32, 255]), [0] * 223 + [0x7FFF]),
                         bytes((0, 0, 0, 255, 255, 255)))
        with self.assertRaises(ValueError):
            chrome.indexed_rgb(bytes([31]), [0] * 224)

    def test_nearest_resize_keeps_each_source_pixel_intact(self):
        source = bytes((255, 0, 0, 255, 0, 255, 0, 0))
        self.assertEqual(chrome.resize_nearest(source, 2, 1, 4, 1),
                         bytes((255, 0, 0, 255, 255, 0, 0, 255,
                                0, 255, 0, 0, 0, 255, 0, 0)))

    def test_keyed_composite_preserves_green_and_blends_source_glass(self):
        bg = bytearray((10, 20, 30) * (chrome.WIDTH * chrome.HEIGHT))
        source = bytes((0, 255, 0, 255, 40, 80, 40, 255))
        chrome.over_rgb(bg, source, (2, 1), (0, 0, 2, 1),
                        fill_color=(40, 80, 40), fill_mix=128)
        self.assertEqual(bg[:3], bytes((10, 20, 30)))
        self.assertEqual(bg[3:6], bytes((25, 50, 35)))

    def test_keyed_composite_can_remove_source_panel_fill_separately_from_matte(self):
        bg = bytearray((11, 16, 23) * 2)
        source = bytes((41, 82, 41, 255, 255, 206, 8, 255))
        chrome.over_rgb(bg, source, (2, 1), (0, 0, 2, 1),
                        extra_keys=((41, 82, 41),))
        self.assertEqual(bg[:3], bytes((11, 16, 23)))
        self.assertEqual(bg[3:6], bytes((255, 206, 8)))

    def test_keyed_sprite_fills_clear_area_with_panel_color(self):
        source = bytes((0, 255, 0, 255, 200, 20, 10, 255))
        sprite = chrome.keyed_sprite(source, (2, 1), (2, 1), (7, 8, 9))
        self.assertEqual(sprite, bytes((7, 8, 9, 200, 20, 10)))

    def test_nine_slice_preserves_source_bevel_rails_and_selector(self):
        width, height = 368, 131
        frame = bytearray(bytes((0, 255, 0, 255)) * (width * height))

        def paint(rect, color):
            x, y, w, h = rect
            for row in range(y, y + h):
                for col in range(x, x + w):
                    i = (row * width + col) * 4
                    frame[i:i + 4] = bytes((*color, 255))

        paint((80, 13, 270, 12), (70, 20, 100))
        paint((80, 119, 270, 12), (180, 30, 20))
        paint((60, 20, 17, 91), (20, 50, 120))
        paint((351, 20, 17, 91), (210, 160, 20))
        selector = bytes((0, 255, 0, 255)) * (208 * 18)
        result = chrome.nine_slice_button(frame, (width, height), selector,
                                          (208, 18), (24, 20), (8, 8, 8))
        self.assertEqual(result[:3], bytes((70, 20, 100)))
        self.assertEqual(result[((19 * 24) * 3):((19 * 24) * 3 + 3)],
                         bytes((180, 30, 20)))
        center = (8 * 24 + 12) * 3
        self.assertEqual(result[center:center + 3], bytes((8, 8, 8)))

    def test_rail_nineslice_keeps_source_arrow_end_caps(self):
        sw, sh, cap, tw, th = 32, 4, 4, 48, 8
        source = bytearray(bytes((0, 255, 0, 255)) * (sw * sh))
        for y in range(sh):
            for x in range(sw):
                color = (220, 160, 20, 255) if x < cap else (70, 40, 90, 255)
                i = (y * sw + x) * 4
                source[i:i + 4] = bytes(color)
        result = chrome.rail_nineslice(bytes(source), (sw, sh), (tw, th), cap=cap)
        for y in range(th):
            sy = min(sh - 1, int((y + .5) * sh / th))
            for x in range(cap):
                self.assertEqual(result[(y * tw + x) * 3:(y * tw + x) * 3 + 3],
                                 source[(sy * sw + x) * 4:(sy * sw + x) * 4 + 3])
                self.assertEqual(result[(y * tw + tw - cap + x) * 3:
                                        (y * tw + tw - cap + x) * 3 + 3],
                                 source[(sy * sw + sw - cap + x) * 4:
                                        (sy * sw + sw - cap + x) * 4 + 3])

    def test_world_panel_has_transparent_glass_and_palette_indices(self):
        world_palette = [0] * 256
        world_palette[32] = 0x001F  # full red in the terrain bank's RGB555 order
        rgba = bytes((255, 0, 0, 255,
                      0, 255, 0, 255,
                      255, 0, 0, 255,
                      0, 0, 0, 255))
        strengths, palette_indices = chrome.world_panel_map(
            rgba, (2, 2), (2, 2), world_palette)
        self.assertEqual(strengths, bytes((1, 0, 1, 0)))
        self.assertEqual(palette_indices, [32])
        with self.assertRaises(ValueError):
            chrome.world_panel_map(rgba, (2, 2), (2, 2), [0] * 31)

    def test_world_panel_nineslice_keeps_arbitrary_center_clear(self):
        sw, sh = chrome.NOTICE_SIZE
        source = bytearray(sw * sh)
        for y in range(sh):
            for x in range(sw):
                if (x < chrome.WORLD_BORDER_X or x >= sw - chrome.WORLD_BORDER_X or
                        y < chrome.WORLD_BORDER_Y or y >= sh - chrome.WORLD_BORDER_Y):
                    source[y * sw + x] = 1
        tw, th = 192, 132
        rendered = chrome.world_panel_nineslice(source, (sw, sh), (tw, th))
        self.assertTrue(any(rendered))
        for y in range(chrome.WORLD_BORDER_Y, th - chrome.WORLD_BORDER_Y):
            for x in range(chrome.WORLD_BORDER_X, tw - chrome.WORLD_BORDER_X):
                self.assertEqual(rendered[y * tw + x], 0)

    def test_world_panel_nineslice_rejects_zero_interior_span(self):
        sw, sh = chrome.NOTICE_SIZE
        source = bytes(sw * sh)
        with self.assertRaises(ValueError):
            chrome.world_panel_nineslice(source, (sw, sh), (13, 132))
        with self.assertRaises(ValueError):
            chrome.world_panel_nineslice(source, (sw, sh), (192, 11))

    def test_header_requires_four_screens_and_reserved_palette_indices(self):
        palette = [(0, 0, 0)] * chrome.PALETTE_SIZE
        backgrounds = {name: bytes((0, 0, 0)) * (chrome.WIDTH * chrome.HEIGHT)
                       for name in ('home', 'zerg', 'protoss', 'terran')}
        header = chrome.build_header(backgrounds, {}, palette)
        self.assertIn('sc64_chrome_menu_palette[224]', header)
        self.assertIn('sc64_chrome_menu_home[38400]', header)
        self.assertIn('__attribute__((aligned(4)))', header)
        with self.assertRaises(ValueError):
            chrome.build_header({'home': backgrounds['home']}, {}, palette)


if __name__ == '__main__':
    unittest.main()
