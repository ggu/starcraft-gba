import importlib.util
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_menu_assets', ROOT / 'tools/sc64_menu_assets.py')
assets = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(assets)


class Sc64MenuAssetTests(unittest.TestCase):
    def test_crop_rgb_copies_exact_rectangle_and_rejects_out_of_bounds(self):
        source = bytes(v for y in range(3) for x in range(4) for v in (x, y, 99))
        crop = assets.crop_rgb(source, 4, 3, (1, 1, 2, 2))
        self.assertEqual(crop, bytes((1, 1, 99, 2, 1, 99,
                                      1, 2, 99, 2, 2, 99)))
        with self.assertRaises(ValueError):
            assets.crop_rgb(source, 4, 3, (3, 1, 2, 2))

    def test_resize_preserves_solid_rgb_and_expected_dimensions(self):
        source = bytes((23, 71, 149)) * (8 * 6)
        resized = assets.resize_crop(source, 8, 6)
        self.assertEqual(len(resized), assets.WIDTH * assets.HEIGHT * 3)
        self.assertEqual(set(resized), {23, 71, 149})

    def test_compose_keys_black_matte_and_shades_menu_column(self):
        backdrop = bytes((120, 100, 80)) * (assets.WIDTH * assets.HEIGHT)
        rgba = bytearray((0, 0, 0, 255) * (4 * 4))
        rgba[(0 * 4 + 2) * 4:(0 * 4 + 2) * 4 + 4] = bytes((240, 30, 20, 255))
        composed = assets.compose_scene(backdrop, bytes(rgba), (4, 4))
        self.assertEqual(composed[:3], backdrop[:3])  # keyed black shows backdrop
        portrait_pixel = (24 * assets.WIDTH + 56) * 3
        self.assertNotEqual(composed[portrait_pixel:portrait_pixel + 3], backdrop[:3])
        right = (80 * assets.WIDTH + 220) * 3
        self.assertLess(composed[right], backdrop[right])

    def test_nearest_keeps_indices_inside_menu_palette(self):
        colors = [(0, 0, 0), (255, 0, 0), (0, 255, 0)]
        indices = assets.nearest(bytes((0, 0, 0, 250, 0, 0, 0, 250, 0)), colors)
        self.assertEqual(indices, bytes((32, 33, 34)))

    def test_header_compiles_with_exact_arrays_and_nonuniform_indices(self):
        compiler = shutil.which('clang') or shutil.which('cc')
        if not compiler:
            self.skipTest('No C compiler is available')
        size = assets.WIDTH * assets.HEIGHT
        frames = [bytes(32 + ((i + n) % 224) for i in range(size)) for n in range(4)]
        palette = [0] * 32 + [i & 0x7FFF for i in range(224)]
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            (tmp_path / 'sc64_menu_generated.h').write_text(assets.build_header(frames, palette))
            fixture = tmp_path / 'check.c'
            fixture.write_text(r'''#include "sc64_menu_generated.h"
_Static_assert(sizeof(sc64_menu_palette) / sizeof(sc64_menu_palette[0]) == 224, "palette size");
_Static_assert(sizeof(sc64_menu_home) == 38400, "home pixel count");
_Static_assert(sizeof(sc64_menu_zerg) == 38400, "zerg pixel count");
_Static_assert(sizeof(sc64_menu_protoss) == 38400, "protoss pixel count");
_Static_assert(sizeof(sc64_menu_terran) == 38400, "terran pixel count");
int main(void) {
    const u8 *frames[] = {sc64_menu_home, sc64_menu_zerg, sc64_menu_protoss, sc64_menu_terran};
    for (unsigned n = 0; n < 4; ++n) {
        int different = 0;
        for (unsigned i = 0; i < 38400; ++i) {
            if (frames[n][i] < 32 || frames[n][i] > 255) return 1;
            if (frames[n][i] != frames[n][0]) different = 1;
        }
        if (!different) return 2;
    }
    return 0;
}
''')
            binary = tmp_path / 'check'
            subprocess.run([compiler, '-std=c11', '-Werror', '-I', str(ROOT / 'common'),
                            str(fixture), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    unittest.main()
