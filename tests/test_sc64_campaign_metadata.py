import struct
import unittest

from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))

from sc64_campaign_metadata import parse_catalog_blob


class CampaignMetadataTests(unittest.TestCase):
    offsets = {'id': 0, 'faction': 4, 'chapter': 8, 'playable': 12, 'title': 16}

    @classmethod
    def make_records(cls, records):
        blob = bytearray()
        for mission_id, faction, chapter, playable in records:
            blob.extend(struct.pack('<iiiii', mission_id, faction, chapter, playable, 0x08012345))
        return bytes(blob)

    def test_filters_and_sorts_by_campaign_chapter(self):
        data = self.make_records([
            (5, 0, 2, 1), (3, 2, 1, 1), (6, 0, 3, 1),
            (1, 1, 2, 1), (4, 0, 1, 1), (7, 0, 4, 0),
        ])
        terran = parse_catalog_blob(data, 20, self.offsets, 'terran')
        self.assertEqual(terran['record_count'], 6)
        self.assertEqual(terran['enabled_mission_ids'], [4, 5, 6])
        self.assertEqual([item['chapter'] for item in terran['enabled_missions']], [1, 2, 3])
        self.assertEqual(parse_catalog_blob(data, 20, self.offsets, 'zerg')['enabled_mission_ids'], [1])
        self.assertEqual(parse_catalog_blob(data, 20, self.offsets, 'protoss')['enabled_mission_ids'], [3])
        self.assertEqual(parse_catalog_blob(data, 20, self.offsets, 'unified')['enabled_mission_ids'],
                         [1, 3, 4, 5, 6])

    def test_rejects_duplicate_ids_and_bad_layouts(self):
        duplicate = self.make_records([(2, 0, 1, 1), (2, 0, 2, 1)])
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            parse_catalog_blob(duplicate, 20, self.offsets, 'terran')
        with self.assertRaisesRegex(ValueError, 'whole'):
            parse_catalog_blob(b'123', 20, self.offsets, 'terran')
        with self.assertRaisesRegex(ValueError, 'missing'):
            parse_catalog_blob(self.make_records([(1, 0, 1, 1)]), 20,
                               {'id': 0, 'faction': 4, 'chapter': 8}, 'terran')


if __name__ == '__main__':
    unittest.main()
