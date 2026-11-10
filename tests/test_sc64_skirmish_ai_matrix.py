#!/usr/bin/env python3
"""Fail-closed validation of AI progression, resume and recovery records."""
from __future__ import annotations

import contextlib
import copy
import io
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import sc64_skirmish_ai_matrix as ai


HALL = {0: 106, 1: 131, 2: 154}


def owner(o, race, **kw):
    row = {"owner": o, "race": race, "minerals": 50, "gas": 0, "supply": 8, "cap": 18,
           "workers": 4, "mining": 4, "gas_workers": 0, "idle_workers": 0, "building_workers": 0,
           "army": 0, "army_supply": 0, "air_army": 0, "anti_air": 0, "depots": 1, "refineries": 0,
           "buildings": 1, "complete": 1, "queued": 0, "trainable": 1,
           "unit_raw": {str(ai.WORKERS[race]): 4}, "building_raw": {str(HALL[race]): [1, 0]},
           "first_made": {str(ai.WORKERS[race]): 0, str(HALL[race]): 0}, "first_complete": {str(HALL[race]): 0}}
    row.update(kw)
    return row


def audit_row(o, start=50, **kw):
    row = {"owner": o, "start": start, "spent": 0, "spent_gas": 0, "income": 0, "vanished": 0, "low": 0,
           "high": 0, "first_bad": -1, "supply_bad": 0, "supply_over": 0, "model_errors": 0, "made_n": {}}
    row.update(kw)
    return row


def snapshot(kind, frame, owners, won=0, lost=0, ore_left=1000, audit=None):
    record = {"kind": kind, "frame": frame, "won": won, "lost": lost, "max_units": 192, "max_buildings": 96,
              "ore_left": ore_left, "owners": owners}
    if kind == "final":
        record["audit"] = audit if audit is not None else {
            "ore_start": 1000, "owners": [audit_row(r["owner"], r["minerals"]) for r in owners]}
    return json.dumps(record)


def lines(frames=(1800, 3600), final=3601, human=0, bots=(1,)):
    owners = [owner(0, human)] + [owner(o, r) for o, r in zip(ai.BOT_OWNERS, bots)]
    out = [snapshot("tick", f, owners) for f in frames]
    out.append(snapshot("final", final, owners))
    return out


def run_records(count=12, human=0, bots=(1,), end=0, edit=None):
    """Validated records of a match of count snapshots; edit(records) may change them."""
    frames = tuple(1800 * k for k in range(1, count + 1))
    recs = ai.validate_run(lines(frames, frames[-1] + 1, human, bots), human, list(bots), 1800, 3600)
    for r in recs:
        r["won"], r["lost"] = (1, 0) if end == 1 else (0, 1) if end == 2 else (0, 0)
    if edit:
        edit(recs)
    return recs


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


def developed(o, race=0, **kw):
    """An owner that has met every development criterion, with prerequisites in order."""
    rows = {
        0: dict(unit_raw={"7": 12, "0": 6, "2": 4}, building_raw={"106": [1, 0], "109": [2, 0], "111": [1, 0], "113": [1, 0], "114": [1, 0]},
                first_made={"7": 0, "0": 120, "2": 400, "106": 0, "109": 5, "111": 10, "113": 20, "114": 30},
                first_complete={"106": 0, "109": 8, "111": 15, "113": 25, "114": 35}, buildings=6, complete=6),
        1: dict(unit_raw={"41": 12, "37": 6, "38": 4}, building_raw={"131": [1, 0], "142": [1, 0], "135": [1, 0], "132": [1, 0]},
                first_made={"41": 0, "37": 120, "38": 400, "131": 0, "142": 10, "135": 20, "132": 30},
                first_complete={"131": 0, "142": 15, "135": 25, "132": 35}, buildings=4, complete=4),
        2: dict(unit_raw={"64": 12, "65": 6, "66": 4}, building_raw={"154": [1, 0], "156": [2, 0], "160": [1, 0], "164": [1, 0], "167": [1, 0]},
                first_made={"64": 0, "65": 120, "66": 400, "154": 0, "156": 5, "160": 10, "164": 20, "167": 30},
                first_complete={"154": 0, "156": 8, "160": 15, "164": 25, "167": 35}, buildings=6, complete=6),
    }[race]
    return owner(o, race, workers=12, mining=9, gas_workers=3, army=10, army_supply=20, **{**rows, **kw})


def match(count=12, human=0, bots=(0,), end=1, edit=None, ore_left=1000, audit=None):
    """Validated records of a developed match; edit(records) may change them."""
    frames = tuple(1800 * k for k in range(1, count + 1))
    owners = [developed(0, human)] + [developed(o, r) for o, r in zip(ai.BOT_OWNERS, bots)]
    text = [snapshot("tick", f, owners, ore_left=ore_left) for f in frames]
    text.append(snapshot("final", frames[-1] + 1, owners, won=int(end == 1), lost=int(end == 2),
                         ore_left=ore_left, audit=audit))
    recs = ai.validate_run(text, human, list(bots), 1800, 3600)
    if edit:
        edit(recs)
    return recs


class MatchCriteria(unittest.TestCase):
    def failing(self, records, **kw):
        result = ai.match_checks(records, **kw)
        return sorted(k for k, v in result["checks"].items() if not v), result

    def test_a_developed_match_passes_every_criterion(self):
        failing, result = self.failing(match())
        self.assertEqual(failing, [])
        self.assertTrue(result["ok"])
        self.assertEqual(result["outcome"], "won")
        self.assertEqual(self.failing(match(end=2))[1]["outcome"], "lost")

    def test_outcome_reaching_the_cap_fails_unless_nothing_can_act(self):
        records = match(end=0)
        self.assertEqual(ai.outcome(records), "stuck")
        self.assertEqual(self.failing(records)[0], ["outcome"])
        self.assertFalse(ai.match_checks(records, expect_draw=True)["ok"])

        def dead(recs):
            for rec in recs:
                for row in rec["owners"]:
                    row.update(army=0, trainable=0, mining=0, idle_workers=0, building_workers=0)
        records = match(end=0, edit=dead)
        self.assertEqual(ai.outcome(records), "draw: exhausted")
        # A dead world is not a stall. It keeps its own outcome name, so a
        # report never shows it as a victory or a defeat.
        failing, result = self.failing(records)
        self.assertNotIn("outcome", failing)
        self.assertEqual(result["outcome"], "draw: exhausted")
        self.assertTrue(self.failing(records, expect_draw=True)[1]["checks"]["outcome"])
        # An expected draw that resolves is reported, not passed.
        self.assertFalse(self.failing(match(end=1), expect_draw=True)[1]["checks"]["outcome"])

    def test_dead_world_needs_every_condition_for_the_whole_tail(self):
        def tail(**row):
            def edit(recs):
                for rec in recs:
                    for r in rec["owners"]:
                        r.update(army=0, trainable=0, mining=0, idle_workers=0, building_workers=0)
                for k, v in row.items():
                    recs[-1]["owners"][0][k] = v
            return match(end=0, edit=edit)
        self.assertTrue(ai.dead_world(tail()))
        for field, value in (("army", 1), ("trainable", 1), ("mining", 1), ("idle_workers", 1), ("building_workers", 1),
                             ("minerals", 77)):
            self.assertFalse(ai.dead_world(tail(**{field: value})), field)
        # A worker on gas cannot earn minerals; a bank that moved is not dead.
        self.assertTrue(ai.dead_world(tail(gas_workers=1)))
        self.assertFalse(ai.dead_world(match(end=0, count=2)))

    def test_prerequisites_by_event_and_by_snapshot(self):
        self.assertEqual(ai.prerequisite_violations(match()), [])
        # A Factory made before the Barracks stood.
        late = match(edit=lambda recs: [r["owners"][1]["first_complete"].__setitem__(111, 99) or
                                        r["owners"][1]["first_made"].__setitem__(111, 10) for r in recs])
        self.assertTrue(any("kind 113 made at 20 without 111 complete" in v for v in ai.prerequisite_violations(late)))
        self.assertIn("prerequisites", self.failing(late)[0])
        # A Siege Tank with no Machine Shop ever.
        tank = match(edit=lambda recs: [r["owners"][1]["unit_raw"].__setitem__(5, 1) or
                                        r["owners"][1]["first_made"].__setitem__(5, 500) for r in recs])
        self.assertTrue(any("kind 5 stands" in v and "without 120" in v for v in ai.prerequisite_violations(tank)))
        # Present but never recorded as made: the audit cannot vouch for it.
        ghost = match(edit=lambda recs: [r["owners"][1]["unit_raw"].__setitem__(3, 1) for r in recs])
        self.assertTrue(any("never made" in v for v in ai.prerequisite_violations(ghost)))

    def test_prerequisite_aliases_and_protoss_power(self):
        def zerg(edit):
            def go(recs):
                for r in recs:
                    row = r["owners"][1]
                    row["building_raw"][141] = (1, 0)
                    row["first_made"][141] = 500
                    row["buildings"] += 1; row["complete"] += 1
                    edit(row)
            return match(human=1, bots=(1,), edit=go)
        # A Spire needs a completed Lair; a Hive stands in for it, a morphing Lair does not.
        self.assertEqual(ai.prerequisite_violations(zerg(lambda row: row["first_complete"].__setitem__(141, 600))), [])
        self.assertEqual(ai.prerequisite_violations(zerg(lambda row: (row["first_complete"].pop(132),
                         row["first_complete"].__setitem__(133, 40), row["first_made"].__setitem__(133, 40)))), [])
        bad = zerg(lambda row: (row["first_complete"].__setitem__(132, 900), row["first_complete"].__setitem__(141, 950)))
        self.assertTrue(any("kind 141 made at 500 without 132" in v for v in ai.prerequisite_violations(bad)))

        def protoss(edit):
            def go(recs):
                for r in recs:
                    edit(r["owners"][1])
            return match(human=2, bots=(2,), edit=go)
        self.assertEqual(ai.prerequisite_violations(protoss(lambda row: None)), [])
        # Protoss structures and units of powered producers also need a completed Pylon.
        unpowered = protoss(lambda row: (row["first_complete"].pop(156), row["building_raw"].pop(156),
                                         row.__setitem__("buildings", 4), row.__setitem__("complete", 4)))
        self.assertTrue(any("without 156" in v for v in ai.prerequisite_violations(unpowered)))

    def test_audit_balance_bound_and_world_inequality(self):
        self.assertEqual(ai.audit_violations(match()[-1]), [])

        def audit(**per_owner):
            return {"ore_start": 1000, "owners": [audit_row(0, 50, **per_owner), audit_row(1, 50)]}

        def violations(ore_left=1000, **per_owner):
            return ai.audit_violations(match(audit=audit(**per_owner), ore_left=ore_left)[-1])
        # Mining 100 against 150 of cargo and spending 100 leaves a bank of 50: sound.
        self.assertEqual(violations(spent=100, income=100, vanished=150, ore_left=850), [])
        # The books must balance against the bank the owner ended with.
        self.assertTrue(any("does not equal its bank" in v for v in violations(spent=100)))
        self.assertTrue(any("does not equal its bank" in v for v in violations(income=10, vanished=10, ore_left=990)))
        # Income beyond the cargo that left workers is money from nowhere; a negative one
        # is a charge nothing explains.
        text = violations(income=60, spent=60, vanished=10, high=1, first_bad=900, ore_left=940)
        self.assertTrue(any("outside [0, cargo 10]" in v for v in text), text)
        self.assertTrue(any("1 updates with money no cargo explains, first at frame 900" in v for v in text), text)
        self.assertTrue(any("outside [0, cargo 0]" in v for v in violations(income=-10)))
        for name, message in (("low", "charge no creation explains"), ("model_errors", "could not read"),
                              ("supply_bad", "admissions beyond the supply cap")):
            self.assertTrue(any(message in v for v in violations(**{name: 2})), name)
        # World inequality: nobody spends more than the start plus what the fields gave up.
        self.assertTrue(any("world spent" in v for v in violations(spent=500, income=500, vanished=500)))
        self.assertEqual(violations(spent=500, income=500, vanished=500, ore_left=400), [])
        # Cargo cannot exceed what left the fields.
        self.assertTrue(any("exceeds ore removed" in v for v in violations(vanished=900, ore_left=900)))

    def test_idle_and_float_streaks_have_margin_and_excuses(self):
        def idle(n, ore=True, depots=1, last=None):
            def edit(recs):
                for rec in recs[:n]:
                    rec["owners"][1]["idle_workers"] = 2
                    if not ore:
                        rec["ore_left"] = 0
                    rec["owners"][1]["depots"] = depots
            return match(count=14, edit=edit)
        self.assertEqual(ai.economy_streaks(idle(ai.IDLE_LIMIT - 1))[1]["idle"], ai.IDLE_LIMIT - 1)
        self.assertEqual(self.failing(idle(ai.IDLE_LIMIT - 1))[0], [])
        self.assertEqual(self.failing(idle(ai.IDLE_LIMIT))[0], ["idle"])
        # Nothing to mine, or no depot to return to: not a lasting idle economy.
        self.assertEqual(self.failing(idle(ai.IDLE_LIMIT, ore=False))[0], [])
        self.assertEqual(self.failing(idle(ai.IDLE_LIMIT, depots=0))[0], [])

        def pile(n, units=None, queued=0, buildings=None):
            def edit(recs):
                for rec in recs[:n]:
                    row = rec["owners"][0]
                    row["minerals"] = ai.FLOAT_MINERALS
                    if units:
                        row["unit_raw"] = {7: units}
                    row["queued"] = queued
                    if buildings is not None:
                        row["buildings"] = buildings
            return match(count=20, edit=edit)
        self.assertEqual(self.failing(pile(ai.FLOAT_LIMIT - 1))[0], [])
        self.assertEqual(self.failing(pile(ai.FLOAT_LIMIT))[0], ["float"])
        # An owner at its unit share (56 with two players; queued products count) cannot spend.
        self.assertEqual(self.failing(pile(ai.FLOAT_LIMIT, units=55))[0], [])
        self.assertEqual(self.failing(pile(ai.FLOAT_LIMIT, units=40, queued=15))[0], [])
        self.assertEqual(self.failing(pile(ai.FLOAT_LIMIT, units=30, queued=15))[0], ["float"])
        # A bank left behind by an owner with no structures is not a pile it could spend.
        self.assertEqual(self.failing(pile(ai.FLOAT_LIMIT, buildings=0))[0], [])
        # A break in the streak restarts it.
        def broken(recs):
            for i, rec in enumerate(recs[:-1]):
                rec["owners"][0]["minerals"] = ai.FLOAT_MINERALS if i % 10 != 9 else 0
        self.assertEqual(self.failing(match(count=20, edit=broken))[0], [])

    def test_development_judges_the_human_owner_of_a_mirror_match_too(self):
        def weak_human(recs):
            for rec in recs:
                rec["owners"][0].update(unit_raw={7: 12}, army=0, army_supply=0)
        failing, result = self.failing(match(edit=weak_human))
        self.assertEqual(failing, ["development"])
        self.assertEqual(result["failed_owners"], [0])
        # The old passive-human criteria leave the human out.
        self.assertTrue(ai.progression(match(edit=weak_human))[1]["ok"])

    def test_audit_control_must_be_flagged_exactly(self):
        case = ("audit-control", "terran", 0, 0, "z", 0)

        def run(**human):
            recs = [json.loads(t) for t in lines(frames=(1800, 3600), final=3601, human=0, bots=(1,))]
            recs[-1]["audit"]["owners"][0].update(human)
            return "\n".join(json.dumps(r) for r in recs)

        def go(text):
            # lines() ends at 3601; the control run's tick limit is its own.
            return ai.audit_control_detected(ai.validate_run(text.splitlines(), 0, [1], 1800, 3600))
        flagged = dict(high=1, first_bad=ai.AUDIT_CONTROL_FRAME, income=100, vanished=60, spent=100)
        # The balance does not matter to the control: only that the update was flagged.
        self.assertTrue(go(run(**flagged)))
        self.assertFalse(go(run()))                                             # undetected
        self.assertFalse(go(run(**{**flagged, "first_bad": 3600})))            # flagged elsewhere
        self.assertFalse(go(run(**{**flagged, "high": 2})))
        self.assertFalse(go(run(**{**flagged, "low": 1})))
        self.assertFalse(go(run(**{**flagged, "model_errors": 1})))
        recs = [json.loads(t) for t in run(**flagged).splitlines()]
        recs[-1]["audit"]["owners"][1].update(high=1, first_bad=5)
        self.assertFalse(go("\n".join(json.dumps(r) for r in recs)))          # the wrong owner flagged too
        with self.assertRaisesRegex(ai.RecordError, "harness exit"):
            ai.evaluate(case, 3, run(**flagged), 1800)

    def test_passive_human_runs_also_fail_on_an_unpaid_creation(self):
        case = ("run", "terran", 0, 0, "z", 3600)
        good = "\n".join(lines(human=0, bots=(1,)))
        result = ai.evaluate(case, 0, good, 1800)
        self.assertEqual((result["ok"], result["audit"]), (True, []))
        recs = [json.loads(t) for t in lines(human=0, bots=(1,))]
        recs[-1]["audit"]["owners"][1].update(high=1, first_bad=2000)
        bad = ai.evaluate(case, 0, "\n".join(json.dumps(r) for r in recs), 1800)
        self.assertFalse(bad["ok"])
        self.assertTrue(any("money no cargo explains" in v for v in bad["audit"]))

    def test_final_record_may_repeat_the_last_snapshot_frame(self):
        owners = [owner(0, 0), owner(1, 1)]
        same = [snapshot("tick", 1800, owners), snapshot("tick", 3600, owners), snapshot("final", 3600, owners)]
        self.assertEqual([r["frame"] for r in ai.validate_run(same, 0, [1], 1800, 3600)], [1800, 3600, 3600])
        changed = [snapshot("tick", 1800, owners), snapshot("tick", 3600, owners),
                   snapshot("final", 3600, owners, ore_left=5)]
        with self.assertRaisesRegex(ai.RecordError, "differs from the snapshot"):
            ai.validate_run(changed, 0, [1], 1800, 3600)
        with self.assertRaisesRegex(ai.RecordError, "duplicate or out-of-order"):
            ai.validate_run([snapshot("tick", 1800, owners), snapshot("tick", 1800, owners),
                             snapshot("final", 1801, owners)], 0, [1], 1800, 3600)

    def test_new_record_fields_are_strict(self):
        def edited(change):
            recs = [json.loads(t) for t in lines()]
            change(recs)
            return [json.dumps(r) for r in recs]

        def run(text):
            return ai.validate_run(text, 0, [1], 1800, 3600)
        with self.assertRaisesRegex(ai.RecordError, "ore_left"):
            run(edited(lambda r: r[0].pop("ore_left")))
        with self.assertRaisesRegex(ai.RecordError, "lacks"):
            run(edited(lambda r: r[0]["owners"][0].pop("trainable")))
        with self.assertRaisesRegex(ai.RecordError, "trainable"):
            run(edited(lambda r: r[0]["owners"][0].update(trainable=2)))
        with self.assertRaisesRegex(ai.RecordError, "lacks"):
            run(edited(lambda r: r[0]["owners"][0].pop("first_made")))
        with self.assertRaisesRegex(ai.RecordError, "raw ID"):
            run(edited(lambda r: r[0]["owners"][0]["first_made"].update({"012": 0})))
        with self.assertRaisesRegex(ai.RecordError, "integer"):
            run(edited(lambda r: r[0]["owners"][0]["first_made"].update({"7": 99999})))
        with self.assertRaisesRegex(ai.RecordError, "completed before it was made"):
            run(edited(lambda r: r[0]["owners"][0]["first_complete"].update({"111": 5})))
        with self.assertRaisesRegex(ai.RecordError, "changed after it was recorded"):
            run(edited(lambda r: r[1]["owners"][0]["first_made"].update({"7": 5})))
        with self.assertRaisesRegex(ai.RecordError, "changed after it was recorded"):
            run(edited(lambda r: r[1]["owners"][0]["first_complete"].pop("106")))
        with self.assertRaisesRegex(ai.RecordError, "audit"):
            run(edited(lambda r: r[-1].pop("audit")))
        with self.assertRaisesRegex(ai.RecordError, "audit outside"):
            run(edited(lambda r: r[0].update(audit=r[-1]["audit"])))
        with self.assertRaisesRegex(ai.RecordError, "audit owner list"):
            run(edited(lambda r: r[-1]["audit"]["owners"].pop()))
        with self.assertRaisesRegex(ai.RecordError, "audit owner 3"):
            run(edited(lambda r: r[-1]["audit"]["owners"][1].update(owner=3)))
        with self.assertRaisesRegex(ai.RecordError, "malformed"):
            run(edited(lambda r: r[-1]["audit"]["owners"][0].pop("low")))
        with self.assertRaisesRegex(ai.RecordError, "integer"):
            run(edited(lambda r: r[-1]["audit"]["owners"][0].update(spent="5")))
        with self.assertRaisesRegex(ai.RecordError, "raw ID"):
            run(edited(lambda r: r[-1]["audit"]["owners"][0]["made_n"].update({"x": 1})))


class ScenarioFixtures(unittest.TestCase):
    def repower(self, **kw):
        row = {"kind": "repower", "scenario": "army-and-home-nexus-lost", "killed_nexus": 1, "lost_pylons": 0,
               "pylons_left": 6, "gateways": 3, "workers": 16, "unpowered": 1, "free_supply": 82, "margin": 20,
               "near_at_start": 0, "kill_frame": 9000, "order_frame": 9024, "free_at_order": 70,
               "margin_at_order": 20, "start_frame": 9162, "window": 576, "price": 100, "audit_first_bad": -1, "ok": 1}
        row.update(kw)
        return [json.dumps(row)]

    def test_repower_records(self):
        case = ("repower", "protoss", 0, 0, "p", 0)
        self.assertTrue(ai.evaluate(case, 0, "\n".join(self.repower()), 1800)["ok"])
        # Each observation that makes the fixture mean something.
        for change in (dict(unpowered=0), dict(near_at_start=1), dict(free_supply=19), dict(gateways=0),
                       dict(workers=8), dict(pylons_left=0), dict(order_frame=-1, start_frame=-1),
                       dict(free_at_order=19), dict(start_frame=9700), dict(audit_first_bad=9100),
                       dict(killed_nexus=0), dict(window=100)):
            with self.assertRaisesRegex(ai.RecordError, "disagrees", msg=str(change)):
                ai.validate_repower(self.repower(**change), 0)
        failed = self.repower(ok=0, order_frame=-1, free_at_order=-1, margin_at_order=-1, start_frame=-1)
        self.assertEqual(ai.validate_repower(failed, 0)["ok"], 0)
        with self.assertRaisesRegex(ai.RecordError, "disagree"):
            ai.evaluate(case, 15, "\n".join(self.repower()), 1800)
        with self.assertRaisesRegex(ai.RecordError, "does not match"):
            ai.validate_repower(self.repower(scenario="other"), 0)
        with self.assertRaisesRegex(ai.RecordError, "exactly one"):
            ai.validate_repower(self.repower() * 2, 0)

    def air_pin(self, scenario, **kw):
        row = {"kind": "air-pin", "scenario": ai.AIR_PIN_SCENARIOS[scenario], "kept": 3, "anti_air_home": 0,
               "enemy_air": 1, "threat_dist": 164, "army_supply": 3, "committed": 3, "commit_frame": 10800,
               "defend_frame": -1, "ok": 1}
        if scenario == 1:
            row.update(anti_air_home=1, committed=0, commit_frame=-1, defend_frame=10704, army_supply=4)
        row.update(kw)
        return [json.dumps(row)]

    def test_air_pin_records(self):
        for scenario in (0, 1):
            self.assertEqual(ai.validate_air_pin(self.air_pin(scenario), scenario)["ok"], 1)
        # The defect: the army defends against a flyer it cannot hit and never leaves.
        pinned = self.air_pin(0, ok=0, committed=0, commit_frame=-1, defend_frame=10704)
        self.assertEqual(ai.validate_air_pin(pinned, 0)["ok"], 0)
        for change in (dict(commit_frame=-1, committed=0), dict(anti_air_home=1), dict(enemy_air=0), dict(threat_dist=320),
                       dict(threat_dist=-1), dict(kept=2), dict(defend_frame=10704), dict(committed=2), dict(army_supply=0)):
            with self.assertRaisesRegex(ai.RecordError, "disagrees", msg=str(change)):
                ai.validate_air_pin(self.air_pin(0, **change), 0)
        # An army that can shoot the flyer must answer it.
        for change in (dict(defend_frame=-1), dict(anti_air_home=0), dict(enemy_air=0)):
            with self.assertRaisesRegex(ai.RecordError, "disagrees", msg=str(change)):
                ai.validate_air_pin(self.air_pin(1, **change), 1)
        with self.assertRaisesRegex(ai.RecordError, "does not match"):
            ai.validate_air_pin(self.air_pin(0), 1)
        with self.assertRaisesRegex(ai.RecordError, "disagree"):
            ai.evaluate(("air-pin", "zerg", 1, 0, "z", 0), 16, "\n".join(self.air_pin(0)), 1800)

    def no_ore(self, scenario, **kw):
        row = {"kind": "no-ore", "scenario": ai.NO_ORE_SCENARIOS[scenario], "start_frame": 6146, "ore_start": 0,
               "ore_end": 0, "workers_start": 18 if scenario == 0 else 5, "bank_start": 351, "bank_end": 34,
               "cheapest_army": 50, "workers_made": 0, "army_made": 3, "units_end": 31, "queued_end": 0,
               "share": 56, "audit_first_bad": -1, "ok": 1}
        row.update(kw)
        return [json.dumps(row)]

    def test_no_ore_records(self):
        for scenario in (0, 1):
            self.assertEqual(ai.validate_no_ore(self.no_ore(scenario), scenario)["ok"], 1)
        for change in (dict(ore_end=40), dict(ore_start=8), dict(workers_made=1), dict(army_made=0),
                       dict(bank_end=52), dict(bank_start=49), dict(audit_first_bad=100), dict(start_frame=-1)):
            with self.assertRaisesRegex(ai.RecordError, "disagrees", msg=str(change)):
                ai.validate_no_ore(self.no_ore(0, **change), 0)
        # A full unit share is as good as an empty bank; two workers short of five is not scenario 1.
        self.assertEqual(ai.validate_no_ore(self.no_ore(0, bank_end=900, units_end=52, queued_end=4), 0)["ok"], 1)
        with self.assertRaisesRegex(ai.RecordError, "disagrees"):
            ai.validate_no_ore(self.no_ore(1, workers_start=7), 1)
        # The baseline holds its bank and refills workers.
        held = self.no_ore(1, ok=0, bank_end=259, workers_made=1, army_made=0)
        self.assertEqual(ai.validate_no_ore(held, 1)["ok"], 0)
        with self.assertRaisesRegex(ai.RecordError, "disagree"):
            ai.evaluate(("no-ore", "terran", 1, 1, "t", 0), 17, "\n".join(self.no_ore(0)), 1800)


def engine_catalog(override=None):
    """The engine's catalog rows for the prerequisite table, reconstructed from it."""
    rows = []
    for raw, need in ai.PREREQS.items():
        race = ai.RACE_OF[raw]
        is_unit = raw in (7, 0, 32, 2, 3, 5, 8, 41, 42, 37, 38, 43, 47, 64, 65, 66, 70)
        need = sorted(need)
        row = {"raw": raw, "race": race, "kind": 4 if is_unit else 1, "producer_raw": 255, "parent_raw": 255,
               "prereq1": 255, "prereq2": 255}
        if is_unit:
            row["producer_raw"] = 35 if race == 1 else need[0]
            rest = need[1:] if race != 1 else [n for n in need if n != 131]
        else:
            rest = need
        if raw in (120, 132, 144, 146):
            row.update(kind=2 if raw == 120 else 3, parent_raw=rest[0] if raw != 132 else 131,
                       producer_raw=rest[0] if raw != 132 else 131)
            rest = [n for n in rest if n != row["parent_raw"]]
        for key, value in zip(("prereq1", "prereq2"), rest):
            row[key] = value
        row.update((override or {}).get(raw, {}))
        rows.append(row)
    return json.dumps({"audit": rows})


class CatalogAndCases(unittest.TestCase):
    def engine(self, override=None):
        return engine_catalog(override)

    def test_catalog_table_matches_a_faithful_engine(self):
        self.assertEqual(len(ai.validate_catalog(self.engine())), len(ai.PREREQS))
        self.assertTrue(ai.evaluate(("catalog", "terran", 1, 0, "t", 0), 0, self.engine(), 1800)["ok"])

    def test_catalog_check_rejects_drift(self):
        with self.assertRaisesRegex(ai.RecordError, "kind 112"):
            ai.validate_catalog(self.engine({112: {"prereq1": 113}}))
        with self.assertRaisesRegex(ai.RecordError, "kind 66"):
            ai.validate_catalog(self.engine({66: {"prereq1": 255}}))
        with self.assertRaisesRegex(ai.RecordError, "kind 37"):
            ai.validate_catalog(self.engine({37: {"race": 0}}))
        with self.assertRaisesRegex(ai.RecordError, "missing"):
            ai.validate_catalog(json.dumps({"audit": json.loads(self.engine())["audit"][1:]}))
        with self.assertRaisesRegex(ai.RecordError, "appears twice"):
            ai.validate_catalog(json.dumps({"audit": json.loads(self.engine())["audit"] * 2}))
        with self.assertRaisesRegex(ai.RecordError, "not a catalog"):
            ai.validate_catalog("not json")
        with self.assertRaisesRegex(ai.RecordError, "harness exit"):
            ai.evaluate(("catalog", "terran", 1, 0, "t", 0), 2, "", 1800)

    def test_new_case_kinds_are_listed_once_per_profile(self):
        cases = ai.cases(36000)
        self.assertEqual(len(cases), len(set(cases)))
        for kind, per_profile in (("catalog", 1), ("audit-control", 6), ("repower", 3), ("air-pin", 6), ("no-ore", 6)):
            self.assertEqual(sum(c[0] == kind for c in cases), per_profile * len(ai.PROFILES), kind)
        matches = [c for c in cases if c[0] == "match"]
        draws = [c for c in cases if c[0] == "match-draw"]
        self.assertEqual({c[1] for c in draws}, set(ai.MATCH_DRAWS))
        self.assertEqual(len(draws), len(ai.MATCH_DRAWS))
        for profile in ai.PROFILES:
            mine = {(c[2], ai.RACES[c[3]], c[4]) for c in matches if c[1] == profile}
            self.assertEqual(len(mine), len(ai.MATCH_LOBBIES) + len(ai.MATCH_EXTRA.get(profile, ())))
            # Both maps, all three human races, one to three opponents, uniform and mixed opponents.
            self.assertEqual({m for m, _, _ in mine}, {0, 1})
            self.assertEqual({h for _, h, _ in mine}, set("tzp"))
            self.assertEqual({len(b) for _, _, b in mine}, {1, 2, 3})
            self.assertTrue(any(len(set(b)) == 1 and len(b) > 1 for _, _, b in mine))
            self.assertTrue(any(len(set(b)) > 1 for _, _, b in mine))
        self.assertTrue(all(c[5] == ai.MATCH_TICKS for c in matches + draws))
        self.assertIn(("match", "protoss", 0, 2, "tzp", ai.MATCH_TICKS), cases)
        self.assertIn(("match", "terran", 1, 0, "pt", ai.MATCH_TICKS), cases)
        self.assertIn(("match", "terran", 1, 2, "pp", ai.MATCH_TICKS), cases)
        self.assertIn(("match", "terran", 1, 1, "zz", ai.MATCH_TICKS), cases)
        # The economy criteria set leaves the fixtures out, as it does the older ones.
        economy = ai.cases(36000, "economy")
        self.assertFalse(any(c[0] in ("catalog", "audit-control", "repower", "air-pin", "no-ore") for c in economy))
        self.assertTrue(any(c[0] == "match" for c in economy))

    def test_run_case_builds_the_harness_command_for_each_new_kind(self):
        seen = []

        class Done:
            returncode, stdout, stderr = 0, "", ""

        def fake(argv, **kw):
            seen.append((argv, kw.get("timeout")))
            return Done()
        real = ai.subprocess.run
        ai.subprocess.run = fake
        try:
            for case in (("match", "zerg", 1, 1, "zzz", ai.MATCH_TICKS), ("match-draw", "zerg", 1, 1, "zzz", ai.MATCH_TICKS),
                         ("catalog", "terran", 1, 0, "t", 0), ("repower", "protoss", 0, 2, "p", 0),
                         ("air-pin", "zerg", 1, 0, "z", 1), ("no-ore", "terran", 1, 1, "t", 1),
                         ("audit-control", "protoss", 0, 2, "z", 1)):
                ai.run_case(Path("build"), case, 600)
        finally:
            ai.subprocess.run = real
        names = [[a for a in argv[1:]] for argv, _ in seen]
        self.assertEqual(names[0], ["--map", "1", "--human", "z", "--bots", "zzz", "--mirror", "--ticks",
                                    str(ai.MATCH_TICKS), "--every", "1800"])
        self.assertEqual(names[1], names[0])
        self.assertEqual(names[2], ["--audit"])
        self.assertEqual(names[3][-2:], ["--repower", "0"])
        self.assertEqual(names[4][-2:], ["--air-pin", "1"])
        self.assertEqual(names[5][-2:], ["--no-ore", "1"])
        self.assertEqual(names[6][-6:], ["--audit-control", "1", "--ticks", "9000", "--every", "1800"])
        self.assertTrue(all(argv[0].endswith("test-sc64-skirmish-ai-progress-" + p) for (argv, _), p in
                            zip(seen, ("zerg", "zerg", "terran", "protoss", "zerg", "terran", "protoss"))))
        self.assertEqual([t for _, t in seen], [3600, 3600, 900, 900, 900, 900, 900])

    def test_sanitizer_reports_fail_a_case_whatever_the_exit_code(self):
        report = ("==1==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x1\n    #0 0x2 in f\n")
        ubsan = "starcraft/game.c:10:3: runtime error: index 5 out of bounds for type 'int [5]'\n"
        self.assertEqual(ai.sanitizer_report("clean run\n"), "")
        self.assertIn("AddressSanitizer: heap-buffer-overflow", ai.sanitizer_report("x\n" + report))
        self.assertIn("runtime error: index 5", ai.sanitizer_report(ubsan))
        self.assertTrue(ai.sanitizer_report(report).startswith(ai.SANITIZER_MARK))

        class Done:
            returncode, stdout, stderr = 0, "", "noise\n" * 500 + report + "more\n" * 500
        real = ai.subprocess.run
        ai.subprocess.run = lambda argv, **kw: Done()
        try:
            _, _, _, _, err = ai.run_case(Path("build"), ("catalog", "terran", 1, 0, "t", 0), 1800)
        finally:
            ai.subprocess.run = real
        # The report is found even when later output pushes it out of the kept tail.
        self.assertTrue(err.startswith(ai.SANITIZER_MARK))

        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            build, out = Path(tmp, "build"), Path(tmp, "out")
            build.mkdir()
            (build / "test-sc64-skirmish-ai-progress-terran").write_bytes(b"x")
            ai.run_case, kept = (lambda b, c, e: (c, ["fake"], 0, engine_catalog(), err)), ai.run_case
            try:
                with contextlib.redirect_stdout(io.StringIO()):
                    code = ai.main(["--build", str(build), "--output", str(out), "--profiles", "terran",
                                    "--kinds", "catalog"])
            finally:
                ai.run_case = kept
            self.assertEqual(code, 1)
            row = json.loads((out / "report.json").read_text())["cases"][0]
            self.assertFalse(row["ok"])
            self.assertIn("AddressSanitizer", row["error"])

    def test_match_evaluation_through_the_case_interface(self):
        case = ("match", "terran", 0, 0, "t", ai.MATCH_TICKS)
        owners = [developed(0, 0), developed(1, 0)]
        frames = list(range(1800, 1800 * 7, 1800))
        text = "\n".join([snapshot("tick", f, owners) for f in frames] + [snapshot("final", frames[-1] + 1, owners, won=1)])
        # The record's own tick limit is the case's, so a short run needs a terminal end.
        result = ai.evaluate(case, 0, text, 1800)
        self.assertTrue(result["ok"], result)
        self.assertEqual(result["outcome"], "won")
        with self.assertRaisesRegex(ai.RecordError, "harness exit"):
            ai.evaluate(case, 1, text, 1800)
        with self.assertRaisesRegex(ai.RecordError, "nonterminal"):
            ai.evaluate(case, 0, "\n".join([snapshot("tick", 1800, owners), snapshot("final", 1801, owners)]), 1800)
        drawn = ai.evaluate(("match-draw", "terran", 0, 0, "t", ai.MATCH_TICKS), 0, text, 1800)
        self.assertFalse(drawn["ok"])

    def test_profiles_option_selects_and_rejects(self):
        with contextlib.redirect_stderr(io.StringIO()):
            for extra in (["--profiles", "terran,bogus"], ["--profiles", "terran,terran"], ["--profiles", ""],
                          ["--profiles", "terran", "--kinds", "nothing"]):
                with self.assertRaises(SystemExit, msg=str(extra)):
                    ai.main(["--output", "/nonexistent"] + extra)
        self.assertEqual(ai.PROFILES, ("terran", "zerg", "protoss", "showcase"))

    def test_main_runs_only_the_selected_profiles_and_reports_per_profile(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            build, out = Path(tmp, "build"), Path(tmp, "out")
            build.mkdir()
            for profile in ("terran", "zerg", "protoss"):       # no showcase binary
                (build / f"test-sc64-skirmish-ai-progress-{profile}").write_bytes(profile.encode())
            ran = []

            def fake(build_dir, case, every):
                ran.append(case)
                return case, ["fake"], 0, engine_catalog(), ""
            real, ai.run_case = ai.run_case, fake
            try:
                with contextlib.redirect_stdout(io.StringIO()) as printed:
                    code = ai.main(["--build", str(build), "--output", str(out), "--profiles",
                                    "terran,zerg,protoss", "--kinds", "catalog", "--jobs", "2"])
            finally:
                ai.run_case = real
            self.assertEqual(code, 0)
            self.assertEqual(sorted(c[1] for c in ran), ["protoss", "terran", "zerg"])
            report = json.loads((out / "report.json").read_text())
            self.assertEqual(report["profiles"], ["terran", "zerg", "protoss"])
            self.assertEqual(sorted(report["binaries"]), ["protoss", "terran", "zerg"])
            self.assertEqual(report["by_profile"]["zerg"], {"passed": 1, "failed": 0})
            self.assertIn("terran: 1 passed, 0 failed", printed.getvalue())
            # The default asks for the showcase binary, which is absent here.
            with self.assertRaises(FileNotFoundError):
                ai.main(["--build", str(build), "--output", str(out), "--kinds", "catalog"])


if __name__ == "__main__":
    unittest.main()
