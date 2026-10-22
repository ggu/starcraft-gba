"""ROM byte-order and archive validation using synthetic data only."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

spec = importlib.util.spec_from_file_location('sc64_workspace', Path(__file__).resolve().parents[1] / 'tools/sc64_workspace.py')
workspace = importlib.util.module_from_spec(spec)
spec.loader.exec_module(workspace)


class RomTests(unittest.TestCase):
    def test_byte_orders_and_rejection(self):
        data = bytes.fromhex('80371240') + bytes(range(12))
        v64 = bytearray(16)
        v64[0::2], v64[1::2] = data[1::2], data[0::2]
        n64 = bytearray(16)
        for i in range(4): n64[i::4] = data[3-i::4]
        with patch.object(workspace, 'SIZE', 16), patch.object(workspace, 'USA_SHA1', hashlib.sha1(data).hexdigest()):
            for source in (data, bytes(v64), bytes(n64)):
                self.assertEqual(workspace.normalize(source), data)
            with self.assertRaises(ValueError): workspace.normalize(data[:-1])
            with self.assertRaises(ValueError): workspace.normalize(data[:-1] + b'Z')

    def test_archive_requires_one_matching_size_rom(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'ambiguous.zip'
            with zipfile.ZipFile(path, 'w') as z:
                z.writestr('a.z64', b'A' * 16)
                z.writestr('b.z64', b'B' * 16)
            with patch.object(workspace, 'SIZE', 16), self.assertRaises(ValueError):
                workspace.import_rom(path)


if __name__ == '__main__': unittest.main()
