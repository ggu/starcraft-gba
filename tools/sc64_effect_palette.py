#!/usr/bin/env python3
"""Project source light-table output into the palette installed by this game."""
# SPDX-License-Identifier: GPL-3.0-or-later
from __future__ import annotations

import struct
from collections.abc import Sequence

try:
    from .sc64_construction_sprites import _actual_gba_palette
except ImportError:
    from sc64_construction_sprites import _actual_gba_palette


def project_light_table(source_rgba5551: bytes, source_table: bytes,
                        gba_rgb555: Sequence[int]) -> bytes:
    """Keep native strength rows, quantizing through all displayed colors.

    The current framebuffer stores GBA palette indices after UI overrides and
    the LCD curve, so inverse projection must use those actual colors. Source
    remap preimages are insufficient. Ties use the lowest index deterministically.
    """
    if len(source_rgba5551) != 512 or not source_table or len(source_table) % 256:
        raise ValueError("source light table must have complete 256-color rows")
    rows = len(source_table) // 256
    if rows > 255 or len(gba_rgb555) != 256:
        raise ValueError("unsupported light table or GBA palette size")
    if any(type(value) is not int or not 0 <= value <= 65535 for value in gba_rgb555):
        raise ValueError("GBA palette values must fit 16 bits")
    source = [((value >> 11) & 31, (value >> 6) & 31, (value >> 1) & 31)
              for value in struct.unpack(">256H", source_rgba5551)]
    actual, _ = _actual_gba_palette(gba_rgb555)

    def distance(a, b):
        return sum((a[i] - b[i]) ** 2 for i in range(3))

    inverse = [min(range(256), key=lambda i: (distance(color, source[i]), i))
               for color in actual]
    output = [min(range(256), key=lambda i: (distance(color, actual[i]), i))
              for color in source]
    return bytes(output[source_table[row * 256 + inverse[destination]]]
                 for row in range(rows) for destination in range(256))
