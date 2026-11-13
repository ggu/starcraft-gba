# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from sc64_early_protoss_home_replay import load_plan


class EarlyProtossPlanTest(unittest.TestCase):
    def make_plan(self, mission):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        tape, raw, host = [Path(temp.name) / n for n in ("tape", "raw", "host")]
        masks = [1, 0, 1, 0, 8, 0] if mission == 2 else [1, 0, 128, 0, 1, 0, 8, 0]
        rows = [dict(kind="metadata", schema=1, mission=mission)]
        held = 0
        for tick, mask in enumerate(masks):
            rows.append(
                dict(kind="input", call_index=tick, held=mask, pressed=mask & ~held)
            )
            held = mask
        rows.append(dict(kind="checkpoint", label="final", call_index=20))
        tape.write_text("\n".join(map(json.dumps, rows)))
        raw.write_text(
            json.dumps(
                dict(
                    total_ticks=20,
                    event_overflow=0,
                    mission=mission,
                    expected=dict(
                        mission=mission,
                        race_completed=1 << mission,
                        screen=3,
                        phase=3,
                        won=1,
                        lost=0,
                        frame=15,
                    ),
                    checkpoints=[],
                    events=[dict(tick=i, held=v) for i, v in enumerate(masks)],
                )
            )
        )
        host.write_text(
            f"PX CHECK won tick=20 frame=15 screen=3 won=1 lost=0 mission={mission} "
            "faults=0 side_units=1 enemy_units=0 enemy_buildings=0 minerals=0 gas=0\n"
        )
        return tape, raw, host

    def test_both_splice_after_start_press(self):
        for mission, splice in ((2, 5), (3, 7)):
            plan = load_plan(mission, *self.make_plan(mission))
            self.assertEqual(plan[-1], splice)
            self.assertEqual([plan[2][splice - 1], plan[2][splice]], [8, 0])

    def test_rejects_mismatched_receipt(self):
        tape, raw, host = self.make_plan(2)
        host.write_text(host.read_text().replace("frame=15", "frame=16"))
        with self.assertRaises(ValueError):
            load_plan(2, tape, raw, host)

    def test_rejects_divergent_tape_representation(self):
        tape, raw, host = self.make_plan(3)
        data = json.loads(raw.read_text())
        data["events"][2]["held"] = 16
        raw.write_text(json.dumps(data))
        with self.assertRaises(ValueError):
            load_plan(3, tape, raw, host)
