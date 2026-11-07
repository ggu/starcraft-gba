# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_egg_custom_physical as physical
import sc64_skirmish_sram_continuation as world


def unit(slot, typ, target=4, timer=5):
    row = [0] * (1 + len(world.UNIT_FIELDS))
    row[0] = slot
    for key, value in [('owner', 0), ('type', typ), ('target', target), ('work_timer', timer)]:
        row[1 + world.UNIT_FIELDS.index(key)] = value
    return row


def fixture():
    return ({'screen': 1, 'race_mode': 1, 'race_human': 1, 'race_bots': 1, 'race_skirmish_map': 0, 'supply_used': 8, 'supply_cap': 18,
             'units': [unit(12, 40)]},
            {'side': 0, 'egg_type': 40, 'zergling_type': 4, 'egg_slot': 12,
             'egg_timer': 5, 'supply': 8, 'supply_cap': 18})


class SnapshotTest(unittest.TestCase):
    def test_active_and_pair(self):
        state, info = fixture()
        physical.check_state(state, info, True)
        state['units'] = [unit(12, 4), unit(13, 4)]
        physical.check_state(state, info, False)

    def test_reject_bad_controller_row_timer_supply_and_pair(self):
        for change in ('controller', 'slot', 'timer', 'supply', 'supply_mismatch', 'target', 'pair'):
            state, info = copy.deepcopy(fixture())
            active = True
            if change == 'controller': state['race_human'] = 2
            if change == 'slot': info['egg_slot'] = 13
            if change == 'timer': info['egg_timer'] = 4
            if change == 'supply': info['supply'] = 19
            if change == 'supply_mismatch': state['supply_used'] = 9
            if change == 'target': state['units'][0][1 + world.UNIT_FIELDS.index('target')] = 5
            if change == 'pair': state['units'] = [unit(12, 4)]; active = False
            with self.subTest(change=change), self.assertRaises(ValueError):
                physical.check_state(state, info, active)

    def test_guest_payment(self):
        after, info = fixture()
        info.update(hall_type=10, pool_type=11)
        def building(slot, typ, larvae):
            row = [0] * (2 + len(world.BUILDING_FIELDS))
            row[0] = slot
            for key, value in [('owner', 0), ('type', typ), ('larva', larvae)]:
                row[1 + world.BUILDING_FIELDS.index(key)] = value
            return row
        before = copy.deepcopy(after)
        before.update(minerals=100, buildings=[building(1, 10, 2), building(2, 11, 0)])
        after.update(minerals=50, buildings=[building(1, 10, 1), building(2, 11, 0)])
        physical.check_paid(before, after, info)
        after['units'][0][1 + world.UNIT_FIELDS.index('work_timer')] = 6
        after['supply_used'] = 9
        physical.check_paid(before, after, info)  # Paid-time values may precede the save.
        after['minerals'] = 51
        with self.assertRaises(ValueError): physical.check_paid(before, after, info)


if __name__ == '__main__': unittest.main()
