import sys
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import sc64_z8_program_pack as pack


def nydus(record, linked, x, y):
    return {"source_type": 134, "record_index": record, "linked_record_index": linked,
            "relation": 1024, "owner_raw": 3, "x_source_px": x, "y_source_px": y}


def descriptor():
    """A tiny authored descriptor; structure validation is mocked here."""
    return {
        "descriptor_sha256": pack.sc64_z8_pack.EXPECTED_DESCRIPTOR_SHA256,
        "puni": [0] * pack.PUNI_LENGTH,
        "owners": [0, 5, 5, 6, 0, 7, 0, 0, 0, 0, 0, 0],
        "forces": [3, 1, 2, 0, 3, 3, 3, 3],
        "dimensions": {"width": 128, "height": 128},
        "triggers": [{"first_condition": 0, "condition_count": 1, "first_action": 0,
                      "action_count": 1, "execution_flags": 0, "initial_cursor": 0,
                      "enabled_players": [0] * 18 + [1] + [0] * 8}],
        "conditions": [{"location": 0, "player": 13, "amount": 0, "unit": 229, "mask": 0,
                        "comparison": 1, "opcode": 2, "resource": 0, "flags": 16, "index": 0}],
        "actions": [{"location": 0, "string_id": 0, "wav_id": 0, "time": 0, "group1": 0,
                     "group2": 0, "unit": 0, "opcode": 2, "modifier": 0, "flags": 0, "index": 0}],
        "locations": [{"left": 0, "top": 0, "right": 64, "bottom": 32, "flags": 0}],
        "unit_records": [nydus(192, 194, 800, 384), nydus(194, 192, 3264, 512),
                         {"source_type": 51, "record_index": 7, "linked_record_index": 0}],
        "nydus_pairs": [[192, 194]],
    }


class Z8ProgramPackTest(unittest.TestCase):
    def render(self, d):
        with mock.patch.object(pack.sc64_z8_pack, "validate_structure") as validate:
            text = pack.render(d)
        validate.assert_called_once_with(d)
        return text

    def test_setup_masks_active_owners_and_one_based_forces(self):
        active, forces = pack.setup(descriptor())
        self.assertEqual(active, 0b1110)
        self.assertEqual(forces, [4, 2, 3, 1, 4, 4, 4, 4])

    def test_renders_scheduler_rows_and_nydus_spawn_centres(self):
        text = self.render(descriptor())
        self.assertIn("#define SC64_Z8_PROGRAM_ACTIVE_MASK 14u", text)
        self.assertIn("#define SC64_Z8_PROGRAM_MAP_PIXEL_WIDTH 4096u", text)
        self.assertIn("{0u,1u,0u,1u,0u,0u,{" + "0u," * 18 + "1u," + "0u," * 7 + "0u}}", text)
        self.assertIn("{0u,13u,0u,229u,0u,1u,2u,0u,16u,0u}", text)
        self.assertIn("{192u,194u,134u,1024u,3u,0u,400,192}", text)
        self.assertIn("{194u,192u,134u,1024u,3u,0u,1632,256}", text)
        self.assertIn("Do not commit or distribute", text)

    def test_permission_overrides_globals_and_boundaries(self):
        puni = [0] * pack.PUNI_LENGTH
        global_base = pack.PUNI_CELLS
        default_base = global_base + pack.PUNI_UNITS
        # Global allow, explicit deny, explicit allow, last slot/raw boundary.
        puni[global_base + 143] = 1
        puni[default_base + 3 * 228 + 143] = 1
        puni[3 * 228 + 144] = 0
        puni[global_base + 144] = 1
        puni[3 * 228 + 146] = 1
        puni[11 * 228 + 227] = 1
        bits = pack.permission_bitmap(puni)
        def allowed(owner, raw):
            bit = owner * 228 + raw
            return (bits[bit // 8] >> (bit % 8)) & 1
        self.assertEqual(len(bits), 342)
        self.assertEqual([allowed(3, n) for n in (143, 144, 146)], [1, 0, 1])
        self.assertEqual(allowed(11, 227), 1)
        self.assertEqual(allowed(0, 0), 0)
        d = descriptor()
        d["puni"] = puni
        text = self.render(d)
        self.assertIn("sc64_z8_permission_table", text)
        self.assertIn(pack.sc64_z8_pack.EXPECTED_DESCRIPTOR_SHA256, text)

    def test_rejects_bad_puni_and_fingerprint(self):
        for values in ([0] * 5699, [0] * 5701, [0] * 5699 + [2],
                       [0] * 5699 + [True], [0] * 5699 + [-1]):
            with self.subTest(length=len(values), last=values[-1]):
                with self.assertRaises(ValueError):
                    pack.permission_bitmap(values)
        d = descriptor()
        d["descriptor_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "authenticated"):
            self.render(d)

    def test_rejects_unpaired_or_odd_canals(self):
        d = descriptor()
        d["unit_records"][1]["linked_record_index"] = 7
        with self.assertRaisesRegex(ValueError, "reciprocal"):
            self.render(d)
        d = descriptor()
        d["unit_records"][0]["x_source_px"] = 801
        with self.assertRaisesRegex(ValueError, "engine pixel"):
            self.render(d)
        d = descriptor()
        d["nydus_pairs"] = [[192, 195]]
        with self.assertRaisesRegex(ValueError, "pair list"):
            self.render(d)


if __name__ == "__main__":
    unittest.main()
