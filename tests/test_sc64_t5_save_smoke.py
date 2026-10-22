"""Asset-free checks for the T5 SRAM smoke menu-entry controls."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "sc64_t5_save_smoke", ROOT / "tools/sc64_t5_save_smoke.py")
assert SPEC and SPEC.loader
smoke = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(smoke)


class FakeRsp:
    def __init__(self):
        self.writes: list[tuple[int, bytes]] = []
        self.ticks: list[int] = []

    def write(self, address: int, data: bytes) -> None:
        self.writes.append((address, data))

    def tick(self, count: int = 1) -> None:
        self.ticks.append(count)


class T5SaveSmokeMenuTests(unittest.TestCase):
    def test_normal_menu_entry_uses_campaign_mission_briefing_deploy_sequence(self):
        rsp = FakeRsp()
        smoke.enter_t5_campaign_menu(rsp, 0x1234)
        self.assertEqual(smoke.MENU_ENTRY_BUTTONS, (1, 16, 16, 1, 1))
        self.assertEqual([data for _, data in rsp.writes], [
            b"\x01\x00", b"\0\0", b"\x10\x00", b"\0\0",
            b"\x10\x00", b"\0\0", b"\x01\x00", b"\0\0",
            b"\x01\x00", b"\0\0",
        ])
        self.assertTrue(all(address == 0x1234 for address, _ in rsp.writes))
        self.assertEqual(rsp.ticks, [2] * 10)


if __name__ == "__main__":
    unittest.main()
