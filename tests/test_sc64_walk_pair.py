# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import sc64_walk_pair as pair

class WalkPairTest(unittest.TestCase):
    def test_embedded_hash_and_untrusted_markers(self):
        facing=b'known facing pixels'
        digest=hashlib.sha256(facing).hexdigest()
        marker=('#define SC64_UNIT_WALK_FACING_SHA256 "'+digest+'"\n').encode()
        self.assertTrue(pair.verify(marker+b'walk',facing))
        self.assertFalse(pair.verify(marker+b'walk',facing+b'changed'))
        self.assertFalse(pair.verify(marker+marker+b'walk',facing))
        self.assertFalse(pair.verify(b'#define SC64_UNIT_WALK_FACING_SHA256 "bad"\n',facing))
        self.assertFalse(pair.verify(b'unmarked',facing))
    def test_legacy_requires_both_exact_hashes(self):
        walk=b'legacy walk';facing=b'legacy facing'
        hashes=(hashlib.sha256(walk).hexdigest(),hashlib.sha256(facing).hexdigest())
        with patch.object(pair,'LEGACY_PAIRS',frozenset({hashes})):
            self.assertTrue(pair.verify(walk,facing))
            self.assertFalse(pair.verify(walk+b'x',facing))
            self.assertFalse(pair.verify(walk,facing+b'x'))
if __name__=='__main__':unittest.main()
