"""Asset-free guards for the command-card icon importer."""
import struct
import unittest

from tools.sc64_command_icons import (ORDER_STRINGS, TECH_ARGUMENTS, check_buttons, pack,
                                      shade, stat_string)


def button(icon, argument, string, position=1):
    return struct.pack('>HHIIHHHHI', position, icon, 0x8006D448, 0x800412C4, 0, argument, string, 0, 0)


def all_buttons():
    rows = [button(icon, 0, string) for icon, string in ORDER_STRINGS.items()]
    rows += [button(icon, tech, 400) for tech, icon in TECH_ARGUMENTS.items()]
    return b''.join(rows)


class CommandIconTests(unittest.TestCase):
    def test_shade_ramp_puts_low_values_brightest(self):
        self.assertEqual([shade(v) for v in (1, 5, 6, 9, 10, 15)], [3, 3, 2, 2, 1, 1])

    def test_pack_crops_to_the_opaque_pixels_two_bits_each(self):
        width = 40
        pixels = bytearray(width * width)
        alpha = bytearray(width * width)
        for x, value in ((3, 1), (4, 7), (5, 12), (8, 2)):
            pixels[2 * width + x] = value
            alpha[2 * width + x] = 1
        w, h, data = pack({'width': width, 'pixels': bytes(pixels), 'alpha': bytes(alpha)})
        self.assertEqual((w, h), (6, 1))
        self.assertEqual(data, bytes([3 | 2 << 2 | 1 << 4, 3 << 2]))

    def test_pack_rejects_icons_wider_than_a_card_cell(self):
        width = 40
        alpha = bytearray(width * width)
        alpha[0] = alpha[21] = 1
        with self.assertRaisesRegex(ValueError, 'exceeds'):
            pack({'width': width, 'pixels': bytes(width * width), 'alpha': bytes(alpha)})

    def test_button_sets_must_confirm_every_order_and_spell(self):
        counts = check_buttons(all_buttons())
        self.assertTrue(all(counts['orders'].values()) and all(counts['techs'].values()))
        missing_attack = b''.join(button(icon, 0, 999 if icon == 230 else string)
                                  for icon, string in ORDER_STRINGS.items())
        missing_attack += b''.join(button(icon, tech, 400) for tech, icon in TECH_ARGUMENTS.items())
        with self.assertRaisesRegex(ValueError, '230'):
            check_buttons(missing_attack)

    def test_stat_strings_are_one_based(self):
        table = struct.pack('<HHH', 2, 6, 9) + b'ab\0cd\0'
        self.assertEqual((stat_string(table, 1), stat_string(table, 2)), ('ab', 'cd'))
        with self.assertRaises(ValueError):
            stat_string(table, 3)


if __name__ == '__main__':
    unittest.main()
