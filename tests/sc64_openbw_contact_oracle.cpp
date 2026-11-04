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
 auto *first=api.check_ground_unit_movement_unit_collision(&a.u,step); assert(first==&b.u);
 std::printf("approach,%d\n",first!=nullptr);
 step.position={198,200};assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));std::puts("escape,0");
 b.s.position={200,230};api.unit_finder_reinsert(&b.u);step.position={200,202};
 assert(api.check_ground_unit_movement_unit_collision(&a.u,step)==&b.u);std::puts("south,1");
 b.s.position={210,200};api.unit_finder_reinsert(&b.u);step.position={202,200};
 assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));std::puts("overlap_closer,0");
 step.position={198,200};assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));std::puts("overlap_escape,0");
 b.s.position={228,200};api.unit_finder_reinsert(&b.u);step.position={202,200};
 Actor c(19,200,230,16,16,15,15);api.unit_finder_insert(&c.u);
 step.position={202,202};assert(api.check_ground_unit_movement_unit_collision(&a.u,step)==&b.u);std::puts("diagonal_x_first,1");
 api.unit_finder_remove(&c.u);
 c.s.position={228,200};api.unit_finder_insert(&c.u);step.position={202,200};
 assert(api.check_ground_unit_movement_unit_collision(&a.u,step)==&c.u);std::puts("equal_edge_newer_first,1");api.unit_finder_remove(&c.u);
 a.s.flags=sprite_t::flag_hidden;step.position={202,200};assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));a.s.flags=0;
 a.u.status_flags=0;assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));a.u.status_flags=unit_t::status_flag_ground_unit;
 b.u.status_flags|=unit_t::status_flag_no_collide;assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));b.u.status_flags&=~unit_t::status_flag_no_collide;
 b.u.pathing_flags=0;assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));b.u.pathing_flags=1;
 // Larva ignores ordinary non-buildings in both mover and candidate roles.
 a.type.id=UnitTypes::Zerg_Larva; assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));
 a.type.id=(UnitTypes)67; b.type.id=UnitTypes::Zerg_Larva; assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));
 b.type.id=(UnitTypes)19;
 // Gathering mover passes a non-gatherer; ReturnGas passes gathering actors too.
 a.u.status_flags|=unit_t::status_flag_gathering; assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));
 b.u.status_flags|=unit_t::status_flag_gathering; a.order.id=Orders::ReturnGas;
 assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));
 a.order.id=Orders::Move;b.order.id=Orders::WaitForGas;
 assert(api.check_ground_unit_movement_unit_collision(&a.u,step)==&b.u);
 a.u.status_flags&=~unit_t::status_flag_gathering;b.u.status_flags&=~unit_t::status_flag_gathering;
 // The special status ignores enemies but allows an allied owner.
 b.u.owner=1;world->alliances[0][1]=0;b.u.status_flags|=unit_t::status_flag_400000;
 assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));
 world->alliances[0][1]=1;assert(api.check_ground_unit_movement_unit_collision(&a.u,step)==&b.u);
 b.u.status_flags&=~unit_t::status_flag_400000;b.u.owner=0;
 // The sweep relies on finder membership: a loaded flag alone is not a filter.
 b.u.status_flags|=unit_t::status_flag_loaded;assert(api.check_ground_unit_movement_unit_collision(&a.u,step)==&b.u);
 api.unit_finder_remove(&b.u);assert(!api.check_ground_unit_movement_unit_collision(&a.u,step));
 b.u.status_flags&=~unit_t::status_flag_loaded;api.unit_finder_insert(&b.u);
 // This low-level sweep does not itself discard a burrowed candidate.
 b.u.status_flags|=unit_t::status_flag_burrowed;assert(api.check_ground_unit_movement_unit_collision(&a.u,step)==&b.u);
 b.u.status_flags&=~unit_t::status_flag_burrowed;
 std::fprintf(stderr,"OpenBW leading-edge and eligibility checks passed\n");
}
