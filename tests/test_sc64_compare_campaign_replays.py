import copy
import unittest

from tools.sc64_compare_campaign_replays import ReportError, compare_reports


def state(frame=100, *, units=None, minerals=40):
    return {
        "tick": 0,
        "scheduler_tick": 0,
        "host_tick": 0,
        "screen": 1,
        "frame": frame,
        "won": 0,
        "lost": 0,
        "race_human": 2,
        "race_mode": 0,
        "race_selected_mission": 22,
        "race_completed": 0,
        "units": units or {"alive": 2, "by_owner": {"0": 2}, "by_owner_type": {"0": {"10": 1, "11": 1}}, "entities": [
            {"slot": 1, "owner": 0, "type": 10, "x": 100, "y": 200, "raw_type": 55, "hp": 40, "shield": 0},
            {"slot": 2, "owner": 0, "type": 11, "x": 101, "y": 200, "raw_type": 65, "hp": 60, "shield": 0},
        ]},
        "buildings": {"alive": 1, "by_owner": {"0": 1}, "by_owner_type": {"0": {"14": 1}},
                       "completed_by_owner_type": {"0": {"14": 1}}, "entities": [
            {"slot": 0, "owner": 0, "type": 14, "x": 240, "y": 300, "raw_type": 111,
             "under_construction": 0},
        ]},
        "minerals": minerals,
        "gas": 120,
        "race_minerals": [0, 5000, 0, 0, 8000],
        "race_gas": [0, 0, 0, 0, 0],
        "supply": 8,
        "supply_cap": 20,
        "race_campaign": {"mission": 22, "phase": 1, "elapsed_frames": frame,
                          "objective_flags": 0, "objective_stage": 0},
        "race_p3": {"outcome": 0, "sequence": 0, "fired": 3,
                    "transmission": 0, "reserved": 0, "action_ticks": 0},
        "side_units": 2,
        "force2_units": 2,
        "force3_units": 0,
        "profile_cycles_last_tick": {"update": 1000},
        "route_counters": {"hits": 20},
        "route_interval": {"hits": 2},
        "elapsed_hardware_cycles": 100000,
        "frame_delta": 100,
        "counted_live_play_interval": True,
        "effective_fps": 29.5,
        "capture": {"path": "private.png"},
    }


def report(*, prefix=False, profile="protoss"):
    checkpoints = [state(frame=0), state(frame=100), state(frame=200)]
    checkpoints[0].update({"screen": 0, "race_selected_mission": 2,
                           "units": {"alive": 0, "by_owner": {}, "by_owner_type": {}, "entities": []},
                           "buildings": {"alive": 0, "by_owner": {}, "by_owner_type": {},
                                         "completed_by_owner_type": {}, "entities": []}})
    checkpoints[1]["tick"] = checkpoints[1]["scheduler_tick"] = checkpoints[1]["host_tick"] = 1000
    checkpoints[2]["tick"] = checkpoints[2]["scheduler_tick"] = checkpoints[2]["host_tick"] = 2000
    checkpoints[2]["screen"] = 1 if prefix else 3
    checkpoints[2]["won"] = 0 if prefix else 1
    checkpoints[2]["race_campaign"].update({"phase": 1 if prefix else 3,
                                            "objective_flags": 0 if prefix else 1})
    checkpoints[2]["race_p3"].update({"outcome": 0 if prefix else 1,
                                      "sequence": 2 if prefix else 5,
                                      "action_ticks": 0})
    checkpoints[2].update({"race_completed": 0 if prefix else (1 << 22)})
    return {
        "profile": profile,
        "mission": 22,
        "trace_sha256": "a" * 64,
        "requested_ticks": 2000,
        "observed_ticks": 2000,
        "trace_stop_tick": 5000 if prefix else 2000,
        "prefix": prefix,
        "verified_win": not prefix,
        "status": "prefix" if prefix else "won",
        "checkpoints": checkpoints,
        "final": copy.deepcopy(checkpoints[-1]),
        "mean_live_play_effective_fps": 29.1,
        "rom_sha256": "left-rom-hash",
    }


class CompareCampaignReplayTests(unittest.TestCase):
    def test_accepts_timing_and_capture_only_differences(self):
        left, right = report(), report(profile="protoss-p3")
        right["mean_live_play_effective_fps"] = 17.3
        for checkpoint in right["checkpoints"]:
            checkpoint["elapsed_hardware_cycles"] += 999
            checkpoint["effective_fps"] = 21.1
            checkpoint["profile_cycles_last_tick"] = {"update": 999999}
            checkpoint["route_counters"] = {"hits": 99}
            checkpoint["route_interval"] = {"hits": 8}
            checkpoint["capture"] = {"path": "other.png"}
        right["final"] = copy.deepcopy(right["checkpoints"][-1])
        result = compare_reports(left, right)
        self.assertTrue(result["equal"])
        self.assertEqual(result["comparison_scope"], "reported gameplay state")
        self.assertFalse(result["full_ram_equality_claimed"])
        self.assertTrue(result["profile_identity_inferred"])

    def test_reports_actionable_entity_and_model_differences(self):
        left, right = report(), report()
        right["checkpoints"][1]["units"]["entities"][0]["x"] += 1
        right["checkpoints"][1]["race_p3"]["fired"] += 1
        right["final"] = copy.deepcopy(right["checkpoints"][-1])
        result = compare_reports(left, right)
        self.assertFalse(result["equal"])
        self.assertIn("units.entities[0].x", [d["path"] for d in result["differences"]])
        self.assertIn("race_p3.fired", [d["path"] for d in result["differences"]])
        self.assertTrue(all("tick" in d for d in result["differences"]))
        self.assertIn("reports differ", result["claim"])

    def test_rejects_trace_mission_profile_and_extent_mismatch(self):
        mutations = (
            ("trace_sha256", "b" * 64),
            ("mission", 14),
            ("profile", "terran"),
            ("requested_ticks", 1999),
        )
        for field, value in mutations:
            with self.subTest(field=field):
                left, right = report(), report()
                right[field] = value
                with self.assertRaises(ReportError):
                    compare_reports(left, right)

    def test_rejects_missing_or_truncated_checkpoints(self):
        left, right = report(), report()
        right["checkpoints"].pop(1)
        with self.assertRaisesRegex(ReportError, "checkpoint tick sequences"):
            compare_reports(left, right)
        right = report()
        right["checkpoints"][1]["tick"] = right["checkpoints"][0]["tick"]
        with self.assertRaisesRegex(ReportError, "unique, increasing"):
            compare_reports(left, right)

    def test_rejects_required_missing_fields_even_when_missing_on_both(self):
        left, right = report(), report()
        for item in (left, right):
            del item["checkpoints"][1]["race_p3"]
            item["final"] = copy.deepcopy(item["checkpoints"][-1])
        with self.assertRaisesRegex(ReportError, "race_p3 is missing"):
            compare_reports(left, right)

    def test_rejects_null_gameplay_values_even_when_equal(self):
        left, right = report(), report()
        left["checkpoints"][1]["units"]["entities"][0]["hp"] = None
        right["checkpoints"][1]["units"]["entities"][0]["hp"] = None
        with self.assertRaisesRegex(ReportError, "is null"):
            compare_reports(left, right)

    def test_prefix_requires_explicit_option_and_never_claims_win(self):
        left, right = report(prefix=True), report(prefix=True)
        with self.assertRaisesRegex(ReportError, "--allow-prefix"):
            compare_reports(left, right)
        result = compare_reports(left, right, allow_prefix=True)
        self.assertTrue(result["equal"])
        self.assertEqual(result["status"], "prefix")
        right["verified_win"] = True
        with self.assertRaisesRegex(ReportError, "must not claim a verified win"):
            compare_reports(left, right, allow_prefix=True)

    def test_rejects_final_not_matching_last_checkpoint(self):
        left, right = report(), report()
        right["final"]["minerals"] += 1
        with self.assertRaisesRegex(ReportError, "final does not match"):
            compare_reports(left, right)

    def test_accepts_only_the_strict_initial_home_identity(self):
        left, right = report(), report()
        result = compare_reports(left, right)
        self.assertTrue(result["equal"])
        right["checkpoints"][1]["race_selected_mission"] = 2
        right["final"] = copy.deepcopy(right["checkpoints"][-1])
        with self.assertRaisesRegex(ReportError, r"checkpoints\[1\].race_selected_mission"):
            compare_reports(left, right)

    def test_rejects_empty_or_incomplete_nested_model_and_entity_state(self):
        mutations = (
            lambda c: c["race_p3"].pop("fired"),
            lambda c: c["race_campaign"].pop("objective_stage"),
            lambda c: c["units"].pop("entities"),
            lambda c: c["buildings"].update({"entities": []}),
            lambda c: c["race_p3"].clear(),
        )
        for mutate in mutations:
            with self.subTest(mutate=mutate):
                left, right = report(), report()
                mutate(left["checkpoints"][1])
                mutate(right["checkpoints"][1])
                left["final"] = copy.deepcopy(left["checkpoints"][-1])
                right["final"] = copy.deepcopy(right["checkpoints"][-1])
                with self.assertRaises(ReportError):
                    compare_reports(left, right)

    def test_home_allows_zero_actor_counts_but_checks_nonzero_totals(self):
        left, right = report(prefix=True), report(prefix=True)
        result = compare_reports(left, right, allow_prefix=True)
        self.assertTrue(result["equal"])
        left, right = report(prefix=True), report(prefix=True)
        for item in (left, right):
            item["checkpoints"][1]["units"]["alive"] = 3
        with self.assertRaisesRegex(ReportError, "alive does not match entities length"):
            compare_reports(left, right, allow_prefix=True)


if __name__ == "__main__":
    unittest.main()
