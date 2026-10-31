#!/usr/bin/env python3
"""Fail-closed validation of AI progression, resume and recovery records."""
from __future__ import annotations

import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import sc64_skirmish_ai_matrix as ai


def owner(o, race, **kw):
    row = {"owner": o, "race": race, "minerals": 50, "gas": 0, "supply": 8, "cap": 18,
           "workers": 4, "mining": 4, "gas_workers": 0, "idle_workers": 0, "building_workers": 0,
           "army": 0, "army_supply": 0, "air_army": 0, "anti_air": 0, "depots": 1, "refineries": 0,
           "buildings": 1, "complete": 1, "queued": 0,
           "unit_raw": {str(ai.WORKERS[race]): 4}, "building_raw": {"106" if race == 0 else "131" if race == 1 else "154": [1, 0]}}
    row.update(kw)
    return row


def snapshot(kind, frame, owners, won=0, lost=0):
    return json.dumps({"kind": kind, "frame": frame, "won": won, "lost": lost,
                       "max_units": 192, "max_buildings": 96, "owners": owners})


def lines(frames=(1800, 3600), final=3601, human=0, bots=(1,)):
    owners = [owner(0, human)] + [owner(o, r) for o, r in zip(ai.BOT_OWNERS, bots)]
    out = [snapshot("tick", f, owners) for f in frames]
    out.append(snapshot("final", final, owners))
    return out


class RunValidation(unittest.TestCase):
    def test_valid_run(self):
        records = ai.validate_run(lines(), 0, [1], 1800, 3600)
        self.assertEqual([r["frame"] for r in records], [1800, 3600, 3601])

    def test_rejects_malformed_json(self):
        bad = lines()
        bad[1] = bad[1][:-3]
        with self.assertRaisesRegex(ai.RecordError, "malformed"):
            ai.validate_run(bad, 0, [1], 1800, 3600)

    def test_rejects_duplicate_and_missing_snapshots(self):
        with self.assertRaisesRegex(ai.RecordError, "duplicate|out-of-order"):
            ai.validate_run(lines(frames=(1800, 1800, 3600)), 0, [1], 1800, 3600)
        with self.assertRaisesRegex(ai.RecordError, "missing or misaligned"):
            ai.validate_run(lines(frames=(1800, 5400), final=5401), 0, [1], 1800, 5400)
        with self.assertRaisesRegex(ai.RecordError, "missing before the final"):
            ai.validate_run(lines(frames=(1800,), final=5401), 0, [1], 1800, 5400)

    def test_rejects_missing_or_extra_final(self):
        no_final = lines()[:-1]
        with self.assertRaisesRegex(ai.RecordError, "missing final"):
            ai.validate_run(no_final, 0, [1], 1800, 3600)
        extra = lines() + [lines()[-1]]
        with self.assertRaisesRegex(ai.RecordError, "after final"):
            ai.validate_run(extra, 0, [1], 1800, 3600)

    def test_rejects_early_nonterminal_end(self):
        with self.assertRaisesRegex(ai.RecordError, "nonterminal"):
            ai.validate_run(lines(), 0, [1], 1800, 36000)

    def test_rejects_wrong_lobby_and_bad_owner_rows(self):
        with self.assertRaisesRegex(ai.RecordError, "owner list"):
            ai.validate_run(lines(), 0, [1, 2], 1800, 3600)
        with self.assertRaisesRegex(ai.RecordError, "owner 1/race 1"):
            ai.validate_run(lines(), 0, [2], 1800, 3600)
        for field, value, message in (("workers", -1, "integer"), ("mining", 9, "worker order"),
                                      ("air_army", 1, "subtotal"), ("buildings", 2, "building_raw total"),
                                      ("unit_raw", {"7": 0}, "integer"), ("unit_raw", {"07": 4}, "raw ID"),
                                      ("building_raw", {"106": [1]}, r"\[complete, pending\]"),
                                      ("bogus", 1, "unknown fields")):
            bad = [json.loads(t) for t in lines()]
            bad[0]["owners"][1][field] = value
            with self.assertRaisesRegex(ai.RecordError, message, msg=field):
                ai.validate_run([json.dumps(r) for r in bad], 0, [1], 1800, 3600)
        bad = [json.loads(t) for t in lines()]
        del bad[0]["owners"][1]["gas"]
        with self.assertRaisesRegex(ai.RecordError, "lacks"):
            ai.validate_run([json.dumps(r) for r in bad], 0, [1], 1800, 3600)

    def test_progression_criteria(self):
        bot = owner(1, 0, workers=12, mining=9, gas_workers=3, army=6, army_supply=14,
                    unit_raw={"7": 12, "0": 4, "5": 2},
                    building_raw={"106": [1, 0], "111": [1, 0], "113": [1, 0], "120": [1, 0]}, buildings=4, complete=4)
        recs = [json.loads(snapshot("tick", f, [owner(0, 1), bot])) for f in (9000, 10800)]
        recs = [{**r, "owners": [ai.validate_owner(r["owners"][0], 0, 1), ai.validate_owner(r["owners"][1], 1, 0)]} for r in recs]
        prog = ai.progression(recs)
        self.assertTrue(prog[1]["ok"], prog[1])
        weak = copy.deepcopy(recs)
        weak[1]["owners"][1]["unit_raw"] = {7: 12, 0: 4}
        weak[0]["owners"][1]["unit_raw"] = {7: 12, 0: 4}
        self.assertFalse(ai.progression(weak)[1]["ok"])
        self.assertTrue(ai.progression(weak, "economy")[1]["ok"])
        early = [recs[0]]
        self.assertIsNone(ai.progression(early)[1]["ok"])


class ResumeAndRecovery(unittest.TestCase):
    def resume(self, **kw):
        row = {"kind": "resume", "saved_frame": 15000, "reference_frame": 16800, "resumed_frame": 16800,
               "saved": "0000abcd", "restored": "0000abcd", "reference": "1234abcd", "resumed": "1234abcd", "match": 1}
        row.update(kw)
        return [json.dumps(row)]

    def test_valid_resume(self):
        self.assertEqual(ai.validate_resume(self.resume())["match"], 1)

    def test_resume_rejects_missing_duplicate_and_inconsistent(self):
        with self.assertRaisesRegex(ai.RecordError, "exactly one"):
            ai.validate_resume([])
        with self.assertRaisesRegex(ai.RecordError, "exactly one"):
            ai.validate_resume(self.resume() * 2)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_resume(self.resume(resumed="1234abce"))
        with self.assertRaisesRegex(ai.RecordError, "hex digest"):
            ai.validate_resume(self.resume(saved="xyz"))
        with self.assertRaisesRegex(ai.RecordError, "integer"):
            ai.validate_resume(self.resume(saved_frame="15000"))
        self.assertEqual(ai.validate_resume(self.resume(resumed="1234abce", match=0))["match"], 0)

    def test_evaluate_resume_exit_must_agree(self):
        case = ("resume", "terran", 1, 0, "zpt", 15000)
        with self.assertRaisesRegex(ai.RecordError, "disagree"):
            ai.evaluate(case, 6, "\n".join(self.resume()), 1800)
        self.assertTrue(ai.evaluate(case, 0, "\n".join(self.resume()), 1800)["ok"])

    def test_recovery_records(self):
        row = json.dumps({"kind": "recovery", "scenario": "depot-destroyed", "ok": 1})
        case = ("recovery", "zerg", 0, 2, "z", 3)
        self.assertTrue(ai.evaluate(case, 0, row, 1800)["ok"])
        with self.assertRaisesRegex(ai.RecordError, "disagree"):
            ai.evaluate(case, 7, row, 1800)
        with self.assertRaisesRegex(ai.RecordError, "does not match"):
            ai.evaluate(("recovery", "zerg", 0, 2, "z", 2), 0, row, 1800)
        with self.assertRaisesRegex(ai.RecordError, "exactly one"):
            ai.evaluate(case, 0, row + "\n" + row, 1800)

    def test_unsaved_match_is_classified_not_counted(self):
        case = ("resume", "terran", 0, 0, "zpt", 9000)
        ended = json.dumps({"kind": "ended-before-save", "frame": 8000})
        with self.assertRaisesRegex(ai.RecordError, "no save: ended-before-save"):
            ai.evaluate(case, 4, ended, 1800)
        with self.assertRaisesRegex(ai.RecordError, "no save: no-save-point"):
            ai.evaluate(case, 9, "", 1800)
        # A resume record cannot pass with a nonzero exit, nor without a save.
        with self.assertRaisesRegex(ai.RecordError, "exactly one"):
            ai.evaluate(case, 0, ended, 1800)

    def morph(self, scenario, **kw):
        base = {"kind": "morph-hp", "scenario": ai.MORPH_SCENARIOS[scenario], "start_hp": 400,
                "during_hp": 370, "during_max": 400, "final_hp": 270, "final_max": 300, "final_raw": 146,
                "alive": 1, "charged": 50, "completed_at": 581, "reference": "00000000",
                "resumed": "00000000", "ok": 1}
        base.update(kw)
        return [json.dumps(base)]

    def test_morph_hp_records(self):
        ok = self.morph(0, during_hp=400, final_hp=300)
        self.assertTrue(ai.evaluate(("morph-hp", "zerg", 1, 1, "t", 0), 0, "\n".join(ok), 1800)["ok"])
        # Deducting at start (final 300 - 30 = 270 from a 370 snapshot is
        # right; 170 is the old early-deduction value) must be rejected.
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_morph_hp(self.morph(0, final_hp=170), 0)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_morph_hp(self.morph(1, during_hp=150, final_hp=50), 1)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_morph_hp(self.morph(2, start_hp=50, during_hp=1, final_hp=1), 2)
        dies = self.morph(3, start_hp=50, during_hp=0, final_hp=0, final_max=400, final_raw=143,
                          alive=0, completed_at=-1)
        self.assertEqual(ai.validate_morph_hp(dies, 3)["alive"], 0)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_morph_hp(self.morph(3, start_hp=50, during_hp=0, alive=1), 3)
        saved = self.morph(4, reference="1234abcd", resumed="1234abcd")
        self.assertEqual(ai.validate_morph_hp(saved, 4)["ok"], 1)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_morph_hp(self.morph(4, reference="1234abcd", resumed="1234abce"), 4)
        with self.assertRaisesRegex(ai.RecordError, "does not match"):
            ai.validate_morph_hp(saved, 5)
        with self.assertRaisesRegex(ai.RecordError, "exactly one"):
            ai.validate_morph_hp(saved * 2, 4)
        with self.assertRaisesRegex(ai.RecordError, "disagree"):
            ai.evaluate(("morph-hp", "zerg", 1, 1, "t", 4), 8, "\n".join(saved), 1800)

    def addon(self, scenario, **kw):
        base = {"kind": "addon", "scenario": ai.ADDON_SCENARIOS[scenario], "factories": [7, 8],
                "blocked": 1, "parent": 8, "reserved": [50, 50], "started_on": 1, "charged": [50, 50], "ok": 1}
        if scenario == 1:
            base.update(blocked=2, parent=-1, reserved=[150, 0], started_on=-1, charged=[0, 0])
        base.update(kw)
        return [json.dumps(base)]

    def test_addon_records(self):
        self.assertEqual(ai.validate_addon(self.addon(0), 0)["parent"], 8)
        self.assertEqual(ai.validate_addon(self.addon(1), 1)["parent"], -1)
        # Reserving the addon's price with no feasible parent is the defect.
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_addon(self.addon(1, reserved=[50, 50]), 1)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_addon(self.addon(0, parent=7), 0)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_addon(self.addon(0, started_on=0), 0)
        with self.assertRaisesRegex(ai.RecordError, "pair"):
            ai.validate_addon(self.addon(0, reserved=[50]), 0)
        with self.assertRaisesRegex(ai.RecordError, "disagree"):
            ai.evaluate(("addon", "terran", 1, 2, "t", 0), 9, "\n".join(self.addon(0)), 1800)
    def builder(self, scenario, **kw):
        base = {"kind": "builder", "scenario": ai.BUILDER_SCENARIOS[scenario], "start": 1968,
                "timer_at_save": 1, "event": 2079, "reference_event": 2079, "arrived": 1, "released": 0,
                "replanned": -1, "stall_passes": 40, "reference": "03050aa7", "resumed": "03050aa7", "ok": 1}
        if scenario:
            base.update(timer_at_save=20 if scenario == 2 else -1, event=3936, arrived=0, released=1,
                        replanned=3937, reference_event=3936 if scenario == 2 else -1)
            if scenario == 1:
                base.update(reference="00000000", resumed="00000000")
        base.update(kw)
        return [json.dumps(base)]

    def test_builder_records(self):
        for s in range(3):
            self.assertEqual(ai.validate_builder(self.builder(s), s)["ok"], 1)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_builder(self.builder(0, resumed="03050aa8"), 0)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_builder(self.builder(0, timer_at_save=0), 0)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_builder(self.builder(1, replanned=-1), 1)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_builder(self.builder(1, event=1968 + 43 * 48), 1)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_builder(self.builder(2, event=3984), 2)
        with self.assertRaisesRegex(ai.RecordError, "stall_passes"):
            ai.validate_builder(self.builder(1, stall_passes=30), 1)
        with self.assertRaisesRegex(ai.RecordError, "disagree"):
            ai.evaluate(("builder", "terran", 1, 2, "t", 1), 11, "\n".join(self.builder(1)), 1800)

    def workers(self, scenario, **kw):
        base = {"kind": "workers", "scenario": ai.WORKER_SCENARIOS[scenario], "slot": 6, "idle_at": 901,
                "back_at": 912, "old_patch": 9, "new_patch": 9, "order": 4, "ok": 1}
        if scenario == 1:
            base.update(back_at=-1, order=0)
        if scenario == 2:
            base.update(new_patch=17)
        base.update(kw)
        return [json.dumps(base)]

    def test_worker_records(self):
        for s in range(4):
            self.assertEqual(ai.validate_workers(self.workers(s), s)["ok"], 1)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_workers(self.workers(0, back_at=-1), 0)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_workers(self.workers(0, back_at=901 + 113), 0)
        # The planner must never take over the human's stopped worker.
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_workers(self.workers(1, back_at=950, order=4), 1)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_workers(self.workers(2, new_patch=9), 2)

    def commit(self, scenario, **kw):
        base = {"kind": "commit", "scenario": ai.COMMIT_SCENARIOS[scenario], "kept": 3, "army_supply": 6,
                "committed": 3, "commit_frame": 10800, "reference": "00000000", "resumed": "00000000", "ok": 1}
        if scenario == 3:
            base.update(committed=0, commit_frame=-1)
        base.update(kw)
        return [json.dumps(base)]

    def test_commit_records(self):
        self.assertEqual(ai.validate_commit(self.commit(0), 0)["committed"], 3)
        self.assertEqual(ai.validate_commit(self.commit(3), 3)["commit_frame"], -1)
        # A small army staying home under a forced commit is the defect.
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_commit(self.commit(0, committed=0, commit_frame=-1), 0)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_commit(self.commit(1, army_supply=14), 1)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_commit(self.commit(3, committed=3, commit_frame=10800), 3)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_commit(self.commit(2), 2)
        self.assertEqual(ai.validate_commit(self.commit(2, reference="1234abcd", resumed="1234abcd"), 2)["ok"], 1)
        with self.assertRaisesRegex(ai.RecordError, "does not match"):
            ai.validate_commit(self.commit(0), 1)
        with self.assertRaisesRegex(ai.RecordError, "disagree"):
            ai.evaluate(("commit", "terran", 1, 2, "t", 0), 10, "\n".join(self.commit(0)), 1800)

    def test_case_list_is_complete_and_unique(self):
        cases = ai.cases(36000)
        self.assertEqual(len(cases), len(set(cases)))
        self.assertEqual(sum(c[0] == "resume-morph" for c in cases), 2 * len(ai.PROFILES))
        self.assertFalse(any(c[0] == "resume-morph" for c in ai.cases(36000, "economy")))
        self.assertEqual(sum(c[0] == "morph-hp" for c in cases), 6 * len(ai.PROFILES))
        self.assertEqual(sum(c[0] == "addon" for c in cases), 2 * len(ai.PROFILES))
        self.assertEqual(sum(c[0] == "commit" for c in cases), 4 * len(ai.PROFILES))
        self.assertEqual(sum(c[0] == "builder" for c in cases), 3 * len(ai.PROFILES))
        self.assertEqual(sum(c[0] == "workers" for c in cases), 4 * len(ai.PROFILES))
        runs = [c for c in cases if c[0] == "run"]
        self.assertEqual({(c[1], c[2], c[3], len(c[4])) for c in runs if len(set(c[4])) > 1 or len(c[4]) == 1},
                         {(p, m, h, n) for p in ai.PROFILES for m in (0, 1) for h in range(3) for n in (1, 2, 3)})


if __name__ == "__main__":
    unittest.main()
