# SPDX-License-Identifier: GPL-3.0-or-later
import json
import sys
import tempfile
import unittest
from hashlib import sha256
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from sc64_p3_normal_home_replay import (
    DISK_RESERVE, FIELDS, LOG_LIMIT, OUTPUT_BUDGET,
    check_report_plan, has_disk_reserve, load_plan, run, within_output_budget,
)


class P3PlanTest(unittest.TestCase):
    def fixture(self):
        rows = []
        before = 0
        for i, mask in enumerate((1, 0, 16, 0, 1, 0, 1, 0, 1, 0), 1):
            rows.append(
                dict(kind="input", host_tick=i, held=mask, pressed=mask & ~before)
            )
            before = mask
        rows.insert(
            8, dict(dict.fromkeys(FIELDS, 0), kind="start", host_tick=8, screen=1)
        )
        rows.append(
            dict(
                dict.fromkeys(FIELDS, 0),
                kind="won",
                host_tick=10,
                won=1,
                screen=3,
                p3_outcome=1,
                p3_sequence=5,
                campaign_phase=3,
                objective_flags=1,
            )
        )
        return dict(
            schema=1,
            mission=22,
            policy="protoss-stock-economy-cannons-assault",
            events=rows,
        ), "6 briefing 0 1\n8 0 2 0 5 1 0 1\n"

    def test_call_mapping(self):
        stop, keys, checks, briefs, modal = load_plan(*self.fixture())
        self.assertEqual((stop, list(checks), briefs), (10, [8, 10], {6: (0, 1)}))
        self.assertEqual((keys[8], keys[9], modal[8][1:5]), (1, 0, (2, 0, 5, 1)))

    def test_rejects_wrong_modal_edges(self):
        trace, pages = self.fixture()
        with self.assertRaisesRegex(ValueError, "authored P3 input"):
            load_plan(trace, pages.replace("5 1 0 1", "5 1 0 8"))

    def test_rejects_nonterminal_objective(self):
        trace, pages = self.fixture()
        trace["events"][-1]["p3_sequence"] = 4
        with self.assertRaisesRegex(ValueError, "strict terminal"):
            load_plan(trace, pages)

    def test_report_budget_accepts_small_plan_and_rejects_dense_plan(self):
        stop, _, checks, briefs, modal = load_plan(*self.fixture())
        check_report_plan(stop, checks, briefs, modal, {})
        row = dict.fromkeys(FIELDS, 10)
        dense = dict.fromkeys(range(1, 1001), row)
        check_report_plan(10, dense, briefs, modal, {})
        with self.assertRaisesRegex(ValueError, "report budget"):
            check_report_plan(1000, dense, briefs, modal, {})

    def test_disk_budget_checks_start_and_continuation_boundaries(self):
        values = (DISK_RESERVE + OUTPUT_BUDGET - 1,
                  DISK_RESERVE + OUTPUT_BUDGET,
                  DISK_RESERVE, DISK_RESERVE - 1)
        with patch("sc64_p3_normal_home_replay.shutil.disk_usage",
                   side_effect=[SimpleNamespace(free=n) for n in values]):
            self.assertFalse(has_disk_reserve(Path("."), OUTPUT_BUDGET))
            self.assertTrue(has_disk_reserve(Path("."), OUTPUT_BUDGET))
            self.assertTrue(has_disk_reserve(Path(".")))
            self.assertFalse(has_disk_reserve(Path(".")))

    def test_output_budget_accounts_for_guest_files_and_the_report(self):
        with tempfile.TemporaryDirectory() as out:
            path = Path(out)
            home = path / "guest" / "home"
            home.mkdir(parents=True)
            for name in ("a", "b"):
                with (home / name).open("wb") as f:
                    f.truncate(LOG_LIMIT // 2)
            self.assertTrue(within_output_budget(path))
            (home / "d").write_bytes(b"1")
            self.assertFalse(within_output_budget(path))

    def test_shutdown_failure_cannot_publish_a_success_receipt(self):
        trace, pages = self.fixture()
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            args = SimpleNamespace(out=base / "out", stop=1, plan=False)
            for name, data in (("trace", json.dumps(trace)), ("pages", pages),
                               ("rom", "rom"), ("elf", "elf"), ("runner", "runner")):
                path = base / name
                payload = data.encode()
                path.write_bytes(payload)
                setattr(args, name, path)
                setattr(args, name + "_sha256", sha256(payload).hexdigest())
            game = Mock()
            game.sym = dict(screen=1, frame=2)
            game.sizes = dict.fromkeys(game.sym, 4)
            game.front_field.return_value = 0
            game.rsp.read.side_effect = lambda address, length: bytes(length)
            game.proc.returncode = -25
            with (
                patch("sc64_p3_normal_home_replay.PhysicalGame", return_value=game),
                patch("sc64_p3_normal_home_replay.dwarf_struct_layout"),
                patch("sc64_p3_normal_home_replay.resource.setrlimit"),
                patch("sc64_p3_normal_home_replay.has_disk_reserve", return_value=True),
            ):
                with self.assertRaisesRegex(ValueError, "runner exited"):
                    run(args)
            game.close.assert_called_once()
            self.assertFalse((args.out / "result.json").exists())
