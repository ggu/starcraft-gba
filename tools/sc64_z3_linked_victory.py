#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay the Z3 paid air and ground campaign from the qualified Z2 victory."""
from pathlib import Path

from sc64_skirmish_sram_continuation import BUILDING_FIELDS
from sc64_linked_stage_replay import run_route

ROUTE = Path(__file__).resolve().parents[1] / "tests/fixtures/zerg-z3-paid-route.json"


def verify_result(game):
    world = game.state()
    buildings = [dict(zip(BUILDING_FIELDS, row[1:])) for row in world["buildings"]]
    live_chrysalis = any(b["raw_type"] == 150 and b["owner"] == 0 and b["hp"] > 0 for b in buildings)
    if (world["screen"], game.u32("won"), game.u32("lost"), game.u32("race_mode"),
            game.u32("race_selected_mission"), game.u32("race_completed"),
            game.u32("mission_unsupported_count"), live_chrysalis) != (3, 1, 0, 0, 14, 0x4003, 0, True):
        raise RuntimeError("Z3 requires victory with the Chrysalis alive and all three completion bits")
    return world


if __name__ == "__main__":
    run_route(__doc__, ROUTE, verify_result, "Z4", 15, 0x4003)
