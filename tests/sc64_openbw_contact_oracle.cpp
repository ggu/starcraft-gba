/* SPDX-License-Identifier: GPL-3.0-or-later
 * Calls the pinned OpenBW sweep itself on synthetic actors, without ROM data.
 * Source dimensions below are audited authored facts, in source pixels.
 * Run via tools/sc64_contact_oracle_run.py with the pinned OpenBW checkout. */
#include <cassert>
#include <cstdio>
#include <memory>
#include "bwgame.h"
using namespace bwgame;
struct Actor {
 unit_t u{}; sprite_t s{}; unit_type_t type{}; order_type_t order{};
 Actor(int id,int x,int y,int l,int up,int r,int down) {
  type.id=(UnitTypes)id; type.dimensions={{l,up},{r,down}};
  s.position={x,y};s.flags=0;s.owner=0;
  u.unit_type=&type;u.sprite=&s;u.order_type=&order;u.owner=0;
  u.status_flags=unit_t::status_flag_ground_unit;u.pathing_flags=1;
 }
};
int main() {
 auto global=std::make_unique<global_state>();auto game=std::make_unique<game_state>();auto world=std::make_unique<state>();
 world->global=global.get();world->game=game.get();state_functions api(*world);
 // Pinned authored bounds: High Templar raw67 12/10/11/13; Raynor Vulture raw19 16/16/15/15.
 Actor a(67,200,200,12,10,11,13), b(19,228,200,16,16,15,15);
 api.unit_finder_insert(&a.u);api.unit_finder_insert(&b.u);
 state_functions::execute_movement_struct step{};step.position={202,200};
 assert(api.check_ground_unit_movement_unit_collision(&a.u,step)==&b.u);
 step.position={198,200};assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));
 a.s.flags=sprite_t::flag_hidden;step.position={202,200};assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));a.s.flags=0;
 a.u.status_flags=0;assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));a.u.status_flags=unit_t::status_flag_ground_unit;
 b.u.status_flags|=unit_t::status_flag_no_collide;assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));b.u.status_flags&=~unit_t::status_flag_no_collide;
 b.u.pathing_flags=0;assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));b.u.pathing_flags=1;
 std::puts("OpenBW leading-edge checks passed");
}
