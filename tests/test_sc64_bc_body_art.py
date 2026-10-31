import unittest
from tools.sc64_bc_body_art import NAMES, extract_declarations, render_header


class BcBodyArtTests(unittest.TestCase):
    def test_extracts_only_required_declarations_in_order(self):
        text = '\n'.join(f'static const unsigned char {name}[] = {{0}};' for name in reversed(NAMES))
        rows = extract_declarations(text)
        self.assertEqual(len(rows), 4)
        self.assertTrue(all(name in row for name, row in zip(NAMES, rows)))
        self.assertIn('SC64_BC_BODY_ART_FORMAT_VERSION 1u', render_header(rows))

    def test_missing_or_duplicate_frame_fails_closed(self):
        text = '\n'.join(f'static const unsigned char {name}[] = {{0}};' for name in NAMES)
        with self.assertRaises(ValueError):
            extract_declarations(text.replace(NAMES[0], 'wrong'))
        with self.assertRaises(ValueError):
            extract_declarations(text + '\n' + text.splitlines()[0])


if __name__ == '__main__':
    unittest.main()
