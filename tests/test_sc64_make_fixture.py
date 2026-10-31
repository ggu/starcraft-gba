# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic make fixtures retain literal modules, including nested modules."""
from pathlib import Path
import tempfile
import unittest

from tests.make_fixture import copy_makefile_with_includes


class MakeFixtureTest(unittest.TestCase):
    def test_nested_literal_includes_and_dynamic_omission(self):
        with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
            source, fixture = Path(a), Path(b)
            (source / 'config').mkdir()
            (source / 'tests').mkdir()
            (source / 'Makefile').write_text('include config/Makefile\ninclude $(OPTIONAL_INPUT)\n')
            (source / 'config/Makefile').write_text('include tests/Makefile # comment\n')
            (source / 'tests/Makefile').write_text('VALUE := yes\n')
            copy_makefile_with_includes(source, fixture)
            self.assertEqual((fixture / 'Makefile').read_bytes(), (source / 'Makefile').read_bytes())
            self.assertTrue((fixture / 'config/Makefile').is_symlink())
            self.assertTrue((fixture / 'tests/Makefile').is_symlink())
            self.assertEqual((fixture / 'tests/Makefile').read_text(), 'VALUE := yes\n')

    def test_missing_literal_include_fails(self):
        with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
            source = Path(a)
            (source / 'Makefile').write_text('include absent/Makefile\n')
            with self.assertRaises(FileNotFoundError):
                copy_makefile_with_includes(source, Path(b))


if __name__ == '__main__':
    unittest.main()
