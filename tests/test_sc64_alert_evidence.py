#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The advisor alerts that may play for each race are the ones with N64 evidence.

tools/sc64_audio_pack.py decides which clips enter the bank; starcraft/game_audio.h
decides which alerts an in-game race may request. The two must agree, and only
the alerts matched in captured N64 audio (every Terran line, Zerg research) may
be present.
"""
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import sc64_audio_pack as pack

# Matched in captured N64 audio at the trigger (docs/DEVELOPMENT.md, Advisor alerts).
HEARD = {
    'terran': set(pack.ALERT_LINES),
    'zerg': {'research'},
    'protoss': set(),
}
# game_audio.h line names, in enum order, against the pack tool's names.
LINE_NAMES = {
    'BASE': 'base_attack', 'FORCES': 'forces_attack', 'RESEARCH': 'research',
    'UPGRADE': 'upgrade', 'ENERGY': 'energy', 'NO_BUILD': 'no_build',
    'NO_LAND': 'no_land', 'BUILT': 'built',
}


def header_lines():
    text = (ROOT / 'starcraft/game_audio.h').read_text()
    names = re.findall(r'GAME_AUDIO_ALERT_([A-Z_]+)', text.split('GAME_AUDIO_ALERT_KINDS')[0])
    return [LINE_NAMES[n] for n in names if n in LINE_NAMES]


def header_masks():
    text = (ROOT / 'starcraft/game_audio.h').read_text()
    body = re.search(r'game_audio_heard\[3\]\s*=\s*\{([^}]*)\}', text).group(1)
    lines = header_lines()
    masks = []
    for item in (part.strip() for part in body.split(',')):
        if item == '0':
            masks.append(set())
        elif item.startswith('0x'):
            value = int(item.rstrip('u'), 16)
            masks.append({line for i, line in enumerate(lines) if value >> i & 1})
        else:
            shifts = re.findall(r'GAME_AUDIO_ALERT_([A-Z_]+)', item)
            masks.append({LINE_NAMES[s] for s in shifts})
    return masks


class AlertEvidence(unittest.TestCase):
    def test_line_order_matches_the_observer(self):
        self.assertEqual(header_lines(), list(pack.ALERT_LINES))

    def test_bank_has_only_evidenced_clips(self):
        for race, sounds in zip(('terran', 'zerg', 'protoss'), pack.ALERT_SOUNDS):
            present = {line for line, sound in zip(pack.ALERT_LINES, sounds) if sound is not None}
            self.assertEqual(present, HEARD[race], race)

    def test_game_may_request_only_banked_alerts(self):
        masks = header_masks()
        self.assertEqual(len(masks), 3)
        for race, mask, sounds in zip(('terran', 'zerg', 'protoss'), masks, pack.ALERT_SOUNDS):
            banked = {line for line, sound in zip(pack.ALERT_LINES, sounds) if sound is not None}
            self.assertEqual(mask, banked, race)

    def test_guessed_clips_are_not_imported(self):
        used = {s for race in pack.ALERT_SOUNDS for s in race if s is not None}
        for guessed in (116, 119, 130, 155, 118, 121, 124, 132, 157):
            self.assertNotIn(guessed, used)


if __name__ == '__main__':
    unittest.main()
