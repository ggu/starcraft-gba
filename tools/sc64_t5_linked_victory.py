#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Continue the qualified T4 victory through a paid T5 air campaign on a normal Terran ROM."""
from pathlib import Path

from sc64_linked_stage_replay import run_route
from sc64_skirmish_sram_continuation import UNIT_FIELDS

ROUTE = Path(__file__).resolve().parents[1] / "tests/fixtures/terran-t5-paid-route.json"


def verify_result(game):
    state = game.state()
    units = (dict(zip(UNIT_FIELDS, row[1:])) for row in state["units"])
    heroes = sorted(unit["raw_type"] for unit in units if unit["owner"] == 0
                    and unit["raw_type"] in (16, 19) and unit["hp"] > 0)
    enemies = sum(row[1] == 1 for row in state["buildings"])
    if (state["screen"], game.u32("won"), game.u32("lost"), game.u32("race_mode"),
            game.u32("race_selected_mission"), game.u32("race_completed"),
            enemies, heroes) != (3, 1, 0, 0, 8, 0x1f0, 0, [16, 19]):
        raise RuntimeError("T5 result requires both heroes alive, all enemy buildings destroyed and T1–T5 complete")
    return state


if __name__ == "__main__":
    run_route(__doc__, ROUTE, verify_result, "T6", 9, 0x1f0)
