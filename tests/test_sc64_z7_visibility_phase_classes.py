"""Independent cell-distance oracle for the optional Z7 phase labels."""
import pathlib
import re
import unittest


class VisibilityPhaseClasses(unittest.TestCase):
    def test_all_phases_and_radii(self):
        source = (pathlib.Path(__file__).resolve().parents[1] /
                  'starcraft/sc64_z7_visibility_phase_classes.h').read_text()
        rows = re.findall(r'\{ /\* radius (\d+), \d+ classes \*/(.*?)\}',
                          source, re.S)
        self.assertEqual([int(r) for r, _ in rows], list(range(64, 177, 16)))
        for radius_text, data in rows:
            radius = int(radius_text)
            labels = [int(v) for v in re.findall(r'\d+', data)]
            self.assertEqual(len(labels), 256)
            label_to_cells, cells_to_label = {}, {}
            for phase, label in enumerate(labels):
                x, y = phase % 16, phase // 16
                # Direct squared-distance test: independent of the row-span
                # formula used to construct the production table.
                cells = frozenset((col, row)
                    for row in range(-12, 13) for col in range(-12, 13)
                    if (col * 16 + 8 - x) ** 2 +
                       (row * 16 + 8 - y) ** 2 <= radius ** 2)
                if label in label_to_cells:
                    self.assertEqual(label_to_cells[label], cells)
                if cells in cells_to_label:
                    self.assertEqual(cells_to_label[cells], label)
                label_to_cells[label] = cells
                cells_to_label[cells] = label


if __name__ == '__main__':
    unittest.main()
