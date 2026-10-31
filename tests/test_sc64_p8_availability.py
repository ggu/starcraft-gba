# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
spec=importlib.util.spec_from_file_location('p8_availability',ROOT/'tools/sc64_p8_availability.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)

class P8AvailabilityTest(unittest.TestCase):
    def test_authentication_and_effective_bits(self):
        payload=bytearray([1]*5700)
        # One player-specific denial and one global denial through selectors.
        payload[7*228+71]=0
        payload[12*228+228+7*228+71]=0
        payload[12*228+170]=0
        payload[12*228+228+7*228+170]=1
        chk=b'PUNI'+struct.pack('<I',len(payload))+payload
        manifest={'schema':1,'mission_id':'P8','source_map_path':'008/025',
            'source_rom_sha1':'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b',
            'source_chk_sha256':hashlib.sha256(chk).hexdigest(),'source_chk_bytes':len(chk),
            'extractor':'references/sc64-maps/extract_sc64_maps.py',
            'extractor_commit':'09b57f787a247107d1de82b8c5eab2734667ea1d'}
        with patch.object(m,'CHK',manifest['source_chk_sha256']),patch.object(m,'PUNI',hashlib.sha256(payload).hexdigest()):
            extracted=m.extract(chk,manifest);bits=m.effective_bits(extracted)
            get=lambda p,u:(bits[(p*228+u)//8]>>((p*228+u)%8))&1
            self.assertEqual((get(7,71),get(7,170),get(0,71)),(0,0,1))
            self.assertIn('SC64_P8_AVAILABILITY_BIT_BYTES 342u',m.render(extracted))
            with self.assertRaises(ValueError):m.extract(chk[:-1],manifest)
            with self.assertRaises(ValueError):m.extract(chk,{**manifest,'mission_id':'P7'})
            corrupt=bytearray(chk);corrupt[-1]^=1
            with self.assertRaises(ValueError):m.extract(bytes(corrupt),manifest)
    def test_duplicate_puni_rejected(self):
        payload=bytes([1]*5700);record=b'PUNI'+struct.pack('<I',5700)+payload;chk=record+record
        manifest={'schema':1,'mission_id':'P8','source_map_path':'008/025','source_rom_sha1':'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b','source_chk_sha256':hashlib.sha256(chk).hexdigest(),'source_chk_bytes':len(chk),'extractor':'references/sc64-maps/extract_sc64_maps.py','extractor_commit':'09b57f787a247107d1de82b8c5eab2734667ea1d'}
        with patch.object(m,'CHK',manifest['source_chk_sha256']),patch.object(m,'PUNI',hashlib.sha256(payload).hexdigest()),self.assertRaises(ValueError):m.extract(chk,manifest)
if __name__=='__main__':unittest.main()
