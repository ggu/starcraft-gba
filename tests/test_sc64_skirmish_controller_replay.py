"""Strict asset-free validation for unified skirmish controller traces."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))
SPEC = importlib.util.spec_from_file_location(
    "sc64_skirmish_controller_replay", TOOLS / "sc64_skirmish_controller_replay.py")
replay = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(replay)


class TimingSafetyTests(unittest.TestCase):
    def test_clock_retries_torn_high_word(self):
        rsp = mock.Mock()
        rsp.read.side_effect = [b'\x01\x00', b'\xff\xff', b'\x02\x00',
                                b'\x02\x00', b'\x05\x00', b'\x02\x00']
        self.assertEqual(replay.coherent_clock(rsp), 0x20005)

    def test_zero_delta_rejected_and_rollover_is_modular(self):
        with self.assertRaisesRegex(RuntimeError, "zero hardware"):
            replay.sampled_delta(3, 3)
        self.assertEqual(replay.sampled_delta(0xfffffffe, 2), 4)

    def test_rejects_invalid_port_before_connecting(self):
        with mock.patch.object(replay.socket, "socket") as sock:
            with self.assertRaisesRegex(ValueError, "invalid GDB port"):
                replay.check_debugger_port_free(0)
            sock.assert_not_called()

    def test_sample_is_nonterminal_and_does_not_claim_fps(self):
        rsp = mock.Mock()
        rsp.cmd.return_value = "OK"
        tick = [-1]
        rsp.tick.side_effect = lambda **_kw: tick.__setitem__(0, tick[0] + 1)
        sym = {"game_update": 100, "demo_schedule_tick": 1, "screen": 2, "frame": 3}
        def read(_rsp, address):
            if address == 1:
                return tick[0]
            if address == 2:
                return 0 if tick[0] == 0 else 1
            if address == 3:
                return max(0, tick[0] - 1)
            raise AssertionError(address)
        plan = {"start": {"host_tick": 1}, "trace_inputs": []}
        with tempfile.TemporaryDirectory() as directory, \
             mock.patch.object(replay.races, "read_u32", side_effect=read), \
             mock.patch.object(replay, "coherent_clock", side_effect=[1, 11, 21, 31, 41]), \
             mock.patch.object(replay, "snapshot", return_value={}), \
             mock.patch.object(replay, "verify_configuration"):
            result = replay.run_bounded_sample(rsp, sym, {}, {}, plan, [], lambda _: None,
                                               1000, 2, 2, Path(directory), "t", "r", "e")
        self.assertEqual(result["status"], "nonterminal-performance-sample")
        self.assertEqual(result["observed_play_frame_count"], 2)
        self.assertEqual(result["valid_timing_frame_count"], 0)
        self.assertIsNone(result["mean_live_play_effective_fps"])

    def test_64_bit_clock_counts_cycles_beyond_32_bit_wrap(self):
        rsp = mock.Mock()
        rsp.cmd.return_value = "OK"
        tick = [-1]
        rsp.tick.side_effect = lambda **_kw: tick.__setitem__(0, tick[0] + 1)
        rsp.clock.side_effect = [1, 11, 21, 4294967326, 4294967336]
        sym = {"game_update": 100, "demo_schedule_tick": 1, "screen": 2, "frame": 3}
        def read(_rsp, address):
            return tick[0] if address == 1 else (0 if tick[0] == 0 else 1) if address == 2 else max(0, tick[0] - 1)
        with tempfile.TemporaryDirectory() as directory, \
             mock.patch.object(replay.races, "read_u32", side_effect=read), \
             mock.patch.object(replay, "snapshot", return_value={}), \
             mock.patch.object(replay, "verify_configuration"):
            result = replay.run_bounded_sample(rsp, sym, {}, {}, {"start": {"host_tick": 1}, "trace_inputs": []},
                                               [], lambda _: None, 1000, 2, 2, Path(directory),
                                               "t", "r", "e", clock64=True)
        self.assertEqual(result["valid_timing_frame_count"], 2)
        self.assertEqual(result["sample_cycles"], 4294967315)
        self.assertIsNotNone(result["mean_live_play_effective_fps"])

    def test_paused_ui_updates_are_excluded_from_play_fps(self):
        rsp = mock.Mock()
        rsp.cmd.return_value = "OK"
        tick = [-1]
        rsp.tick.side_effect = lambda **_kw: tick.__setitem__(0, tick[0] + 1)
        rsp.clock.side_effect = [0, 10, 20, 50, 60]
        sym = {"game_update": 100, "demo_schedule_tick": 1, "screen": 2, "frame": 3}
        def read(_rsp, address):
            if address == 1: return tick[0]
            if address == 2: return 0 if tick[0] == 0 else 1
            return [0, 0, 1, 1, 2][tick[0]]
        with tempfile.TemporaryDirectory() as directory, \
             mock.patch.object(replay.races, "read_u32", side_effect=read), \
             mock.patch.object(replay, "snapshot", return_value={}), \
             mock.patch.object(replay, "verify_configuration"):
            result = replay.run_bounded_sample(rsp, sym, {}, {}, {"start": {"host_tick": 1}, "trace_inputs": []},
                                               [], lambda _: None, 1000, 2, 2, Path(directory),
                                               "t", "r", "e", clock64=True)
        self.assertEqual(result["nonadvancing_play_updates"], 1)
        self.assertEqual(result["nonadvancing_play_cycles"], 30)
        self.assertEqual(result["valid_timing_frame_count"], 1)
        self.assertEqual(result["sample_cycles"], 10)


def valid_trace() -> dict:
    events = [
        {"kind": "input", "host_tick": 1, "frame": 0, "held": 128, "pressed": 128},
        {"kind": "input", "host_tick": 2, "frame": 0, "held": 0, "pressed": 0},
        {"kind": "input", "host_tick": 3, "frame": 0, "held": 1, "pressed": 1},
        {"kind": "input", "host_tick": 4, "frame": 0, "held": 0, "pressed": 0},
        {"kind": "input", "host_tick": 5, "frame": 0, "held": 16, "pressed": 16},
        {"kind": "input", "host_tick": 6, "frame": 0, "held": 0, "pressed": 0},
        {"kind": "input", "host_tick": 7, "frame": 0, "held": 1, "pressed": 1},
        {"kind": "input", "host_tick": 8, "frame": 1, "held": 0, "pressed": 0},
        {"kind": "start", "host_tick": 8, "frame": 1, "map": 0, "human_race": 1,
         "bots": [{"owner": 1, "race": 2}]},
        {"kind": "input", "host_tick": 9, "frame": 2, "held": 1, "pressed": 1},
        {"kind": "input", "host_tick": 10, "frame": 3, "held": 0, "pressed": 0},
        {"kind": "action", "action": "queue", "host_tick": 10, "frame": 3,
         "producer": 2, "type": 10, "minerals_before": 200, "minerals_after": 100,
         "gas_before": 0, "gas_after": 0},
    ]
    return {
        "profile": "unified-race-skirmish",
        "events": events,
        "terminal": {
            "kind": "won", "host_tick": 20, "frame": 11, "won": 1, "lost": 0,
            "map": 0, "human_race": 1, "bot_count": 1,
            "human": {"owner": 0, "race": 1, "units": 5, "buildings": 1},
            "bots": [{"owner": 1, "race": 2, "units": 4, "buildings": 0}],
        },
    }


class TraceValidationTests(unittest.TestCase):
    def test_accepts_full_menu_input_and_completed_win(self):
        plan = replay.validate_trace(valid_trace(), capacity=32)
        self.assertEqual(plan["stop_tick"], 20)
        self.assertEqual(plan["inputs"][0], (0, 128))
        self.assertEqual(plan["start"]["bots"], [{"owner": 1, "race": 2}])
        self.assertEqual(plan["terminal"]["kind"], "won")
        self.assertEqual(plan["peak_page_transitions"], 10)

    def test_rejects_wrong_profile_nonwindowed_or_extra_top_level_fields(self):
        trace = valid_trace()
        trace["profile"] = "campaign"
        with self.assertRaisesRegex(ValueError, "profile"):
            replay.validate_trace(trace, 32)
        with self.assertRaisesRegex(ValueError, "windowed"):
            replay.validate_trace(valid_trace(), 32, windowed=False)
        trace = valid_trace()
        trace["host_world"] = {}
        with self.assertRaisesRegex(ValueError, "fields differ"):
            replay.validate_trace(trace, 32)

    def test_rejects_bad_home_prefix_menu_key_and_start_marker(self):
        trace = valid_trace()
        trace["events"][0]["held"] = 1
        trace["events"][0]["pressed"] = 1
        with self.assertRaisesRegex(ValueError, "HOME navigation"):
            replay.validate_trace(trace, 32)
        trace = valid_trace()
        trace["events"][5]["held"] = 256
        trace["events"][5]["pressed"] = 256
        with self.assertRaisesRegex(ValueError, "prelaunch"):
            replay.validate_trace(trace, 32)
        trace = valid_trace()
        trace["events"][8]["host_tick"] = 9
        with self.assertRaisesRegex(ValueError, "start marker"):
            replay.validate_trace(trace, 32)

    def test_rejects_malformed_or_unordered_input_transitions(self):
        trace = valid_trace()
        trace["events"][4]["pressed"] = 0
        with self.assertRaisesRegex(ValueError, "pressed does not match"):
            replay.validate_trace(trace, 32)
        trace = valid_trace()
        trace["events"][5]["host_tick"] = 5
        with self.assertRaisesRegex(ValueError, "strictly increasing"):
            replay.validate_trace(trace, 32)
        trace = valid_trace()
        trace["events"][0]["held"] = 0x400
        trace["events"][0]["pressed"] = 0x400
        with self.assertRaisesRegex(ValueError, "held"):
            replay.validate_trace(trace, 32)

    def test_rejects_unknown_actions_and_action_without_same_tick_input(self):
        trace = valid_trace()
        trace["events"][-1]["action"] = "spawn"
        with self.assertRaisesRegex(ValueError, "unsupported action"):
            replay.validate_trace(trace, 32)
        trace = valid_trace()
        trace["events"][-1]["host_tick"] = 11
        with self.assertRaisesRegex(ValueError, "same frame"):
            replay.validate_trace(trace, 32)

    def test_rejects_inactive_bot_terminal_forgery(self):
        trace = valid_trace()
        trace["terminal"]["bots"].append(
            {"owner": 3, "race": 0, "units": 1, "buildings": 1})
        with self.assertRaisesRegex(ValueError, "bot list"):
            replay.validate_trace(trace, 32)
        trace = valid_trace()
        trace["terminal"]["bots"][0]["owner"] = 4
        with self.assertRaisesRegex(ValueError, "identity"):
            replay.validate_trace(trace, 32)

    def test_rejects_boolean_configuration_values(self):
        trace = valid_trace()
        trace["events"][8]["bots"][0]["owner"] = True
        with self.assertRaisesRegex(ValueError, "owner must be an integer"):
            replay.validate_trace(trace, 32)

    def test_rejects_forged_win_loss_and_result_flags(self):
        trace = valid_trace()
        trace["terminal"]["bots"][0]["buildings"] = 1
        with self.assertRaisesRegex(ValueError, "forged win"):
            replay.validate_trace(trace, 32)
        trace = valid_trace()
        trace["terminal"]["kind"] = "lost"
        trace["terminal"]["won"] = 0
        trace["terminal"]["lost"] = 1
        with self.assertRaisesRegex(ValueError, "forged loss"):
            replay.validate_trace(trace, 32)
        trace = valid_trace()
        trace["terminal"]["won"] = 0
        with self.assertRaisesRegex(ValueError, "contradict"):
            replay.validate_trace(trace, 32)

    def test_rejects_terminal_limit_and_config_mismatch(self):
        trace = valid_trace()
        trace["terminal"]["kind"] = "limit"
        with self.assertRaisesRegex(ValueError, "completed win or loss"):
            replay.validate_trace(trace, 32)
        trace = valid_trace()
        trace["terminal"]["map"] = 1
        with self.assertRaisesRegex(ValueError, "configuration"):
            replay.validate_trace(trace, 32)

    def test_windowed_capacity_is_bounded_per_1000_tick_page(self):
        trace = valid_trace()
        events = trace["events"]
        held = 0
        for i in range(20):
            held = 1 if held == 0 else 0
            tick = 100 + i * 200
            events.append({"kind": "input", "host_tick": tick, "frame": tick - 7,
                           "held": held, "pressed": held & (0x3FF ^ (1 - held))})
        events.sort(key=lambda row: (row["host_tick"], 0 if row["kind"] == "input" else 1))
        trace["terminal"]["host_tick"] = 5000
        trace["terminal"]["frame"] = 4990
        with self.assertRaisesRegex(ValueError, "capacity"):
            replay.validate_trace(trace, capacity=4)

    def test_dense_window_capacity_is_bounded_per_250_tick_page(self):
        trace = valid_trace()
        events = trace["events"]
        held = 0
        for i in range(8):
            held = 1 if held == 0 else 0
            tick = 20 + i * 40
            events.append({"kind": "input", "host_tick": tick, "frame": tick - 7,
                           "held": held, "pressed": held & (0x3FF ^ (1 - held))})
        events.sort(key=lambda row: (row["host_tick"], 0 if row["kind"] == "input" else 1))
        trace["terminal"]["host_tick"] = 1000
        trace["terminal"]["frame"] = 990
        with self.assertRaisesRegex(ValueError, "capacity"):
            replay.validate_trace(trace, capacity=4, checkpoint_interval=250)


class LayoutDecodeTests(unittest.TestCase):
    def test_checkpoint_period_comes_from_compiled_observer_symbol(self):
        self.assertEqual(replay.scheduler.checkpoint_ticks({}), 1000)
        dense_symbols = {"demo_replay_dense_checkpoint": 0x08001235,
                         "demo_replay_checkpoint": 0x08004567}
        self.assertEqual(replay.scheduler.checkpoint_ticks(dense_symbols), 250)
        self.assertEqual(replay.checkpoint_observer_symbol(dense_symbols),
                         "demo_replay_checkpoint")

    def test_bank_layout_uses_compact_building_typedef_for_p6(self):
        unit_offsets = {"owner": 0, "type": 1, "alive": 2, "x": 4, "y": 6,
                        "hp": 8, "raw_type": 26}
        building_offsets = {"owner": 0, "type": 1, "alive": 2,
                            "under_construction": 3, "x": 4, "y": 6,
                            "hp": 8, "raw_type": 16}

        def dwarf(_elf, name):
            if name == "Unit":
                return 56, unit_offsets
            if name == "Sc64ZpCompactBuilding":
                return 48, building_offsets
            raise AssertionError(f"unexpected DWARF type request: {name}")

        with mock.patch.object(replay.races, "dwarf_struct", side_effect=dwarf):
            layout = replay.bank_layout(Path("mock.elf"), {"u": 240 * 56, "b": 160 * 48})
        self.assertEqual(layout["building_type"], "Sc64ZpCompactBuilding")
        self.assertEqual(layout["building_capacity"], 160)
        self.assertEqual(layout["building_stride"], 48)

    def test_bank_layout_falls_back_to_building_alias(self):
        unit_offsets = {"owner": 0, "type": 4, "alive": 8, "x": 12, "y": 16,
                        "hp": 20, "raw_type": 24}
        building_offsets = {"owner": 0, "type": 4, "alive": 8,
                            "under_construction": 12, "x": 16, "y": 20,
                            "hp": 24, "raw_type": 28}

        def dwarf(_elf, name):
            if name == "Unit":
                return 108, unit_offsets
            if name == "Sc64ZpCompactBuilding":
                raise RuntimeError("typedef unavailable")
            if name == "Building":
                return 48, building_offsets
            raise AssertionError(f"unexpected DWARF type request: {name}")

        with mock.patch.object(replay.races, "dwarf_struct", side_effect=dwarf):
            layout = replay.bank_layout(Path("mock.elf"), {"u": 240 * 108, "b": 160 * 48})
        self.assertEqual(layout["building_type"], "Building")
        self.assertEqual(layout["unit_stride"], 108)

    def test_scalar_array_uses_explicit_element_width_and_signedness(self):
        self.assertEqual(replay.decode_scalar_array(bytes([0xFF, 0xFF, 0x80, 0x00]), 2, 2,
                                                    signed=True), [-1, 128])
        with self.assertRaisesRegex(ValueError, "byte count"):
            replay.decode_scalar_array(b"\0", 2, 1)

    def test_compact_unit_and_building_rows_decode_mixed_width_fields(self):
        unit_offsets = {"owner": 0, "type": 1, "alive": 2, "x": 4, "y": 6,
                        "hp": 8, "raw_type": 26}
        unit = bytearray(56)
        unit[0:3] = bytes([3, 7, 1])
        unit[4:6] = (321).to_bytes(2, "little", signed=True)
        unit[6:8] = (654).to_bytes(2, "little", signed=True)
        unit[8:12] = (1200).to_bytes(4, "little", signed=True)
        unit[26:28] = (37).to_bytes(2, "little", signed=True)
        decoded = replay.decode_entity_bank(bytes(unit), 1, 56, unit_offsets,
                                            replay.UNIT_WIDTHS,
                                            required=("owner", "type", "alive", "x", "y", "hp", "raw_type"))
        self.assertEqual(decoded["by_owner"], {"3": 1})
        self.assertEqual(decoded["entities"][0]["raw_type"], 37)
        self.assertEqual(decoded["entities"][0]["x"], 321)

        building_offsets = {"owner": 0, "type": 1, "alive": 2, "under_construction": 3,
                            "x": 4, "y": 6, "hp": 8, "raw_type": 16}
        building = bytearray(48)
        building[:4] = bytes([4, 9, 1, 0])
        building[4:6] = (800).to_bytes(2, "little", signed=True)
        building[6:8] = (1200).to_bytes(2, "little", signed=True)
        building[8:12] = (9000).to_bytes(4, "little", signed=True)
        building[16:18] = (131).to_bytes(2, "little", signed=True)
        decoded = replay.decode_entity_bank(bytes(building), 1, 48, building_offsets,
                                            replay.COMPACT_BUILDING_WIDTHS,
                                            required=("owner", "type", "alive", "x", "y", "hp",
                                                      "raw_type", "under_construction"))
        self.assertEqual(decoded["entities"][0]["owner"], 4)
        self.assertEqual(decoded["entities"][0]["hp"], 9000)
        self.assertEqual(decoded["entities"][0]["raw_type"], 131)

    def test_terminal_verification_checks_both_result_flags(self):
        trace = valid_trace()["terminal"]
        state = {"screen": replay.SCREEN_RESULT, "frame": trace["frame"], "won": 1,
                 "lost": 0, "race_mode": 1, "race_human": trace["human_race"],
                 "race_bots": trace["bot_count"], "race_skirmish_map": trace["map"],
                 "units": {"by_owner": {"0": 5, "1": 4}},
                 "buildings": {"by_owner": {"0": 1, "1": 0}}}
        replay.verify_terminal(trace, state)
        state["lost"] = 1
        with self.assertRaisesRegex(RuntimeError, "terminal lost"):
            replay.verify_terminal(trace, state)


if __name__ == "__main__":
    unittest.main()
