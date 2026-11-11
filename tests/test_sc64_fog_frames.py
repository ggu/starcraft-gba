"""Soft-fog pixel properties on real rendered frames.

Each race product is built twice from the same sources (with and without
-DSC64_SOFT_FOG; tests/sc64_fog_frames.c, rules in mk/fog/Makefile), run on the
same scenarios and compared frame by frame. The properties asserted:

1. Unknown cells stay opaque: every pixel inside an unknown cell is identical
   with the feature on and off (black, apart from anything drawn over it
   identically by both builds).
2. Visible terrain is unchanged: a pixel in a visible cell whose eight
   neighbours are all visible is bit-identical with the feature on and off.
3. Only explored-but-not-visible cells and the edges beside them change: every
   differing pixel lies in an explored cell, or in a visible cell with at least
   one non-visible neighbour. The feature must also change something.

The check needs the ignored private inputs the race products are built from;
without them it skips, so a checkout that only has the original-art game is
unaffected. Set SC64_FOG_FRAMES_OUT to a directory to keep the raw dumps and
a stats.json of per-frame counts (the dumps contain imported art: keep them
under .local/, never commit them).

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""
import json
import os
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCENARIOS = ("c0", "c1", "m0")
PRODUCTS = ("terran", "zerg", "protoss")
WORLD_TOP, WORLD_BOTTOM = 12, 120
KEEP = os.environ.get("SC64_FOG_FRAMES_OUT")
MAKE_ARGS = ["-o", ".local/sc64/p4/generated/sc64_p4_scenario_generated.h"]
EXTRA = {"zerg": ["Z8_DESCRIPTOR=.local/build/z9-inputs/z8-descriptor.json"]}


def parse(path):
    data = Path(path).read_bytes()
    assert data[:4] == b"FOGF"
    count = struct.unpack_from("<I", data, 4)[0]
    pos, frames = 8, []
    for _ in range(count):
        camx, camy, cols, rows = struct.unpack_from("<iiII", data, pos)
        pos += 16
        pixels = data[pos:pos + 240 * 160]
        pos += 240 * 160
        state = data[pos:pos + cols * rows]
        pos += cols * rows
        palette = struct.unpack_from("<256H", data, pos)
        pos += 512
        frames.append({"camx": camx, "camy": camy, "cols": cols, "rows": rows,
                       "pixels": pixels, "state": state, "palette": palette})
    assert pos == len(data)
    return frames


def cell_class(frame, col, row):
    if 0 <= col < frame["cols"] and 0 <= row < frame["rows"]:
        return frame["state"][col + row * frame["cols"]]
    return 0


def compare(on, off):
    """Return (stats, violations) for one pair of frames."""
    stats = {"unknown_px": 0, "explored_px": 0, "visible_px": 0, "visible_interior_px": 0,
             "changed_px": 0, "changed_in_explored": 0, "changed_in_visible_edge": 0,
             "changed_in_unknown": 0, "changed_in_visible_interior": 0,
             "unknown_px_not_black": 0}  # the latter: cursor/sprites, identical in both builds
    violations = []
    assert on["camx"] == off["camx"] and on["camy"] == off["camy"]
    assert on["state"] == off["state"], "simulation differs between the builds"
    assert on["palette"] == off["palette"], "palette differs between the builds"
    for y in range(WORLD_TOP, WORLD_BOTTOM):
        row = (y - WORLD_TOP + on["camy"]) // 16
        for x in range(240):
            col = (x + on["camx"]) // 16
            i = y * 240 + x
            klass = cell_class(on, col, row)
            interior = klass == 2 and all(
                cell_class(on, col + dx, row + dy) == 2
                for dy in (-1, 0, 1) for dx in (-1, 0, 1))
            if klass == 0:
                stats["unknown_px"] += 1
                if on["pixels"][i] != 0:
                    stats["unknown_px_not_black"] += 1
            elif klass == 1:
                stats["explored_px"] += 1
            else:
                stats["visible_px"] += 1
                stats["visible_interior_px"] += interior
            if on["pixels"][i] == off["pixels"][i]:
                continue
            stats["changed_px"] += 1
            if klass == 0:
                stats["changed_in_unknown"] += 1
                violations.append(("unknown pixel changed", x, y))
            elif klass == 1:
                stats["changed_in_explored"] += 1
            elif interior:
                stats["changed_in_visible_interior"] += 1
                violations.append(("visible interior pixel changed", x, y))
            else:
                stats["changed_in_visible_edge"] += 1
    return stats, violations


class SoftFogFrameTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.out = Path(KEEP) if KEEP else Path(tempfile.mkdtemp(prefix="fog-frames-"))
        cls.out.mkdir(parents=True, exist_ok=True)
        cls.stats = {}

    @classmethod
    def tearDownClass(cls):
        if KEEP:
            (cls.out / "stats.json").write_text(json.dumps(cls.stats, indent=1, sort_keys=True))

    def build(self, product, variant):
        target = f"build/sc64-fog-frames-{product}-{variant}"
        result = subprocess.run(
            ["make", "-s", *MAKE_ARGS, *EXTRA.get(product, []), target],
            cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode:
            if "No rule to make target" in result.stdout or "No such file" in result.stdout:
                self.skipTest(f"private inputs for the {product} product are absent")
            self.fail(f"{target} failed to build:\n{result.stdout[-3000:]}")
        return ROOT / target

    def run_scenario(self, binary, product, variant, scenario):
        path = self.out / f"{product}-{scenario}-{variant}.bin"
        result = subprocess.run([str(binary), str(path), scenario], cwd=ROOT, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode == 3:
            self.skipTest(f"{product} {scenario}: scenario unavailable in this build")
        self.assertEqual(result.returncode, 0, result.stdout[-2000:])
        state = [line for line in result.stdout.splitlines() if line.startswith("STATE ")]
        self.assertEqual(len(state), 1, result.stdout[-2000:])
        return parse(path), state[0]

    def check(self, product, scenario):
        on_bin = self.build(product, "on")
        off_bin = self.build(product, "off")
        on, on_state = self.run_scenario(on_bin, product, "on", scenario)
        off, off_state = self.run_scenario(off_bin, product, "off", scenario)
        # 600 updates with a render after each leave the same simulation.
        self.assertEqual(on_state, off_state)
        self.assertEqual(len(on), len(off))
        total_changed = 0
        for k, (a, b) in enumerate(zip(on, off)):
            stats, violations = compare(a, b)
            self.stats[f"{product}-{scenario}-{k}"] = stats
            self.assertEqual(violations[:5], [], f"{product} {scenario} frame {k}")
            total_changed += stats["changed_px"]
        self.assertGreater(total_changed, 0, "the feature changed no pixel")


def _add(product, scenario):
    def test(self):
        self.check(product, scenario)
    setattr(SoftFogFrameTests, f"test_{product}_{scenario}", test)


for _product in PRODUCTS:
    for _scenario in SCENARIOS:
        _add(_product, _scenario)

if __name__ == "__main__":
    unittest.main()
