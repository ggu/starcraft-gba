/* SPDX-License-Identifier: GPL-3.0-or-later
 * Experimental OpenBW flat-map world initialization and actor frame probe.
 * All DAT, GRP, LO and IScript data come from authenticated private inputs.
 * The walkable map/tilesets are synthetic; this is not native N64 execution. */
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include "bwgame.h"
#include "actions.h"
using namespace bwgame;
static std::vector<uint8_t> read(const std::string &path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("missing input: "+path);return {std::istreambuf_iterator<char>(f),{}};}
static void u16le(std::vector<uint8_t>&v,unsigned n){v.push_back(n&255);v.push_back((n>>8)&255);}
static void u32le(std::vector<uint8_t>&v,unsigned n){u16le(v,n);u16le(v,n>>16);}
static void chunk(std::vector<uint8_t>&v,const char *tag,const std::vector<uint8_t>&data){for(int i=0;i<4;i++)v.push_back(tag[i]);u32le(v,(unsigned)data.size());v.insert(v.end(),data.begin(),data.end());}
static std::vector<uint8_t> flat_chk(){
 std::vector<uint8_t> v;
 chunk(v,"VER ",{59,0});chunk(v,"DIM ",{32,0,32,0});chunk(v,"ERA ",{0,0});
 chunk(v,"OWNR",std::vector<uint8_t>(12,0));chunk(v,"SIDE",std::vector<uint8_t>(12,0));
 chunk(v,"STR ",{0,0});chunk(v,"SPRP",{0,0,0,0});chunk(v,"FORC",{});chunk(v,"VCOD",{});
 chunk(v,"MTXM",std::vector<uint8_t>(32*32*2,0));chunk(v,"THG2",{});chunk(v,"UNIT",{});
 return v;
}
int main(int argc,char **argv){
 if(argc!=5){std::cerr<<"usage: world-probe table-dir image-dir scenario\n";return 2;}
 std::string scenario=argv[3]; int phase=std::stoi(argv[4]); if(phase<0 || phase>7)return 2;
 if(scenario!="east"&&scenario!="south"&&scenario!="far"&&scenario!="stop"&&scenario!="targetdead"&&scenario!="parentdead"&&scenario!="poolfull"&&scenario!="initfail"&&scenario!="diagonal"&&scenario!="turn"&&scenario!="north"&&scenario!="west"&&scenario!="slope")return 2;
 try{
  std::string tables=std::string(argv[1])+"/",images=std::string(argv[2])+"/";
  auto global=std::make_unique<global_state>();auto game=std::make_unique<game_state>();auto world=std::make_unique<state>();
  auto provider=[&](auto &out,auto name_value){std::string name=name_value;
   if(name=="triggers/Melee.trg"){out.clear();return;}
   if(name.rfind("Tileset/",0)==0){
    if(name.size()>=4&&name.substr(name.size()-4)==".vf4"){out.assign(32,0);for(unsigned i=0;i<16;i++)out[2*i]=1;return;}
    if(name.size()>=4&&name.substr(name.size()-4)==".cv5"){out.assign(52,0);return;}
    throw std::runtime_error("unexpected tileset request: "+name);
   }
   std::string path;
   if(name=="scripts/iscript.bin")path=tables+"iscript.bin";
   else if(name=="arr/images.tbl")path=images+"images.tbl";
   else if(name.rfind("arr/",0)==0)path=tables+name.substr(4);
   else if(name.rfind("unit/",0)==0)path=images+name;
   else throw std::runtime_error("unexpected input request: "+name);
   auto v=read(path);out.assign(v.begin(),v.end());
  };
  global_init(*global,provider);world->global=global.get();world->game=game.get();game_load_functions api(*world);
  api.setup_info.victory_condition=2;api.setup_info.create_no_units=true;
  api.setup_info.resource_type=1;api.setup_info.starting_minerals=500;
  auto map=flat_chk();api.load_map_data(map.data(),map.size(),{},false);
  std::cout<<"flat map initialized "<<game->map_tile_width<<"x"<<game->map_tile_height<<"\n";
  auto *reaver_type=api.get_unit_type(UnitTypes::Protoss_Reaver);
  auto *scarab_type=api.get_unit_type(UnitTypes::Protoss_Scarab);
  auto *marine_type=api.get_unit_type(UnitTypes::Terran_Marine);
  auto *weapon82=api.get_weapon_type((WeaponTypes)82);
  // flat_chk has no UNIS/UNIx overrides; these are authenticated DAT values.
  if(reaver_type->hitpoints.raw_value!=100*256 || reaver_type->shield_points!=80 ||
     scarab_type->hitpoints.raw_value!=20*256 || marine_type->hitpoints.raw_value!=40*256 ||
     weapon82->damage_amount!=100 || weapon82->damage_bonus!=25)
   throw std::runtime_error("unexpected stock unit or weapon stats");
  std::cout<<"stats reaver_hp="<<reaver_type->hitpoints.raw_value<<" reaver_shields="<<reaver_type->shield_points
    <<" scarab_hp="<<scarab_type->hitpoints.raw_value<<" marine_hp="<<marine_type->hitpoints.raw_value
    <<" weapon82_damage="<<weapon82->damage_amount<<" weapon82_bonus="<<weapon82->damage_bonus<<"\n";
  auto *parent=api.create_starting_unit(api.get_unit_type(UnitTypes::Protoss_Reaver),{320,320},0);
  xy victim_pos=scenario=="south"?xy{320,400}:scenario=="north"?xy{320,192}:scenario=="west"?xy{192,320}:scenario=="slope"?xy{448,384}:scenario=="diagonal"?xy{448,448}:(scenario=="far"||scenario=="turn"||scenario=="parentdead"||scenario=="poolfull"||scenario=="initfail")?xy{448,320}:xy{400,320};
  auto *victim=api.create_starting_unit(api.get_unit_type(UnitTypes::Terran_Marine),victim_pos,1);
  if(!parent||!victim||api.us_hidden(parent)||api.us_hidden(victim))throw std::runtime_error("actor creation or placement failed");
  if(parent->owner!=0 || victim->owner!=1 || parent->hp.raw_value!=100*256 || victim->hp.raw_value!=40*256)
   throw std::runtime_error("unexpected actor ownership or initial health");
  action_state actions{};action_functions input(*world,actions);
  if(!input.action_select(0,parent)||!input.action_train_fighter(0))throw std::runtime_error("native fighter training action rejected");
  if(world->current_minerals[0]!=485)throw std::runtime_error("native training did not charge 15 minerals");
  for(int i=0;i<1000 && parent->reaver.inside_count==0;i++)api.next_frame();
  if(parent->reaver.inside_count!=1)throw std::runtime_error("native fighter training did not complete");
  unit_t *inside=nullptr;for(unit_t *child:ptr(parent->reaver.inside_units)){if(inside)throw std::runtime_error("duplicate inside child");inside=child;}
  if(!inside || inside->owner!=0 || inside->hp.raw_value!=20*256 || inside->unit_type->id!=UnitTypes::Protoss_Scarab || !api.u_completed(inside) || inside->fighter.parent!=parent || !api.us_hidden(inside) || parent->reaver.outside_count)throw std::runtime_error("invalid completed inside linkage");
  std::cout<<"trained frame="<<world->current_frame<<" inside="<<parent->reaver.inside_count<<" minerals="<<world->current_minerals[0]<<"\n";
  for(int i=0;i<phase;i++)api.next_frame();
  if(!input.action_order(0,api.get_order_type(Orders::AttackDefault),victim->position,victim,nullptr,false))throw std::runtime_error("native attack action rejected");
  for(int i=0;i<300&&parent->reaver.outside_count==0;i++) {
   api.next_frame();
   if(i%50==0)std::cerr<<"tick="<<world->current_frame<<" order="<<(int)parent->order_type->id
      <<" state="<<parent->order_state<<" pos="<<parent->position.x<<","<<parent->position.y
      <<" target="<<(parent->order_target.unit==victim)<<" hidden="<<api.us_hidden(victim)
      <<" visible="<<api.unit_can_see_target(parent,victim)<<" inside="<<parent->reaver.inside_count<<"\n";
  }
  if(parent->reaver.outside_count!=1)throw std::runtime_error("native Reaver release did not occur");
  std::cout<<"released frame="<<world->current_frame<<" outside="<<parent->reaver.outside_count<<"\n";
  unit_t *shot=nullptr;
  for(unit_t *child:ptr(parent->reaver.outside_units)) { if(shot)throw std::runtime_error("multiple outside fighters");shot=child; }
  if(!shot || shot!=inside || shot->owner!=0 || shot->fighter.parent!=parent || parent->reaver.inside_count!=0 || api.us_hidden(shot))throw std::runtime_error("invalid released fighter linkage");
  if(scenario=="turn") {
   /* Controlled injected heading after real launch, not a natural action. */
   shot->heading=direction_t::from_raw(0);shot->next_velocity_direction=direction_t::from_raw(0);
   shot->current_velocity_direction=direction_t::from_raw(0);
  }
  if(scenario=="initfail") {
   auto hp=victim->hp.raw_value;bool exhausted=false,saw_arrival=false,saw_die=false;
   for(int i=0;i<100;i++) {
    if(!exhausted && shot->order_type && shot->order_type->id==Orders::ScarabAttack &&
       shot->order_state==2 && api.unit_target_in_range(shot,victim,32)) {
      int created=0;
      while(api.create_sprite(api.get_sprite_type((SpriteTypes)318),{100,100},0))++created;
      if(!world->bullets_container.top())throw std::runtime_error("bullet pool unexpectedly full");
      if(api.create_bullet(api.get_weapon_type((WeaponTypes)0),victim,victim->position,1,victim->heading))
         throw std::runtime_error("bullet initialization unexpectedly succeeded");
      exhausted=true;std::cout<<"sprite/image allocation exhausted frame="<<world->current_frame<<" created="<<created<<"\n";
    }
    api.next_frame();
    if(shot->order_type&&shot->order_type->id==Orders::ScarabAttack&&shot->order_state==6)saw_arrival=true;
    if(shot->order_type&&shot->order_type->id==Orders::Die)saw_die=true;
    for(const bullet_t *bullet:ptr(world->active_bullets))if(bullet->weapon_type&&(int)bullet->weapon_type->id==82)
       throw std::runtime_error("weapon82 allocated after image/sprite failure");
    if(victim->hp.raw_value!=hp)throw std::runtime_error("initialization failure damaged victim");
    if(exhausted && saw_arrival && i>25)break;
   }
   if(!exhausted||!saw_arrival||!saw_die || parent->reaver.inside_count || parent->reaver.outside_count)throw std::runtime_error("initialization failure did not consume the released child");
   std::cout<<"result scenario=initfail no weapon82 or damage; child consumed\n";return 0;
  }
  if(scenario=="poolfull") {
   auto hp=victim->hp.raw_value;bool saturated=false,saw_arrival=false,saw_die=false;
   for(int i=0;i<100;i++) {
    if(!saturated && shot->order_type && shot->order_type->id==Orders::ScarabAttack &&
       shot->order_state==2 && api.unit_target_in_range(shot,victim,32)) {
      int created=0;
      while(api.create_bullet(api.get_weapon_type((WeaponTypes)0),victim,victim->position,1,victim->heading))++created;
      if(world->bullets_container.top())throw std::runtime_error("bullet creation failed before pool exhaustion");
      saturated=true;std::cout<<"pool saturated frame="<<world->current_frame<<" created="<<created<<"\n";
    }
    api.next_frame();
    if(shot->order_type&&shot->order_type->id==Orders::ScarabAttack&&shot->order_state==6)saw_arrival=true;
    if(shot->order_type&&shot->order_type->id==Orders::Die)saw_die=true;
    for(const bullet_t *bullet:ptr(world->active_bullets))if(bullet->weapon_type&&(int)bullet->weapon_type->id==82)
       throw std::runtime_error("weapon82 allocated despite pool-full test");
    if(victim->hp.raw_value!=hp)throw std::runtime_error("pool-full attack damaged victim");
    if(saturated && saw_arrival && i>25)break;
   }
   if(!saturated||!saw_arrival||!saw_die || parent->reaver.inside_count || parent->reaver.outside_count)throw std::runtime_error("pool-full did not consume the released child");
   std::cout<<"result scenario=poolfull no weapon82 or damage; child consumed\n";return 0;
  }
  if(scenario=="parentdead") {
   api.kill_unit(parent);bool detached=false,attributed=false;auto hp=victim->hp.raw_value;
   for(int i=0;i<120;i++){
    api.next_frame();
    if(!shot->fighter.parent)detached=true;
    if(detached && shot->fighter.parent)throw std::runtime_error("detached shot regained parent");
    for(const bullet_t *bullet:ptr(world->active_bullets))
     if(bullet->weapon_type&&(int)bullet->weapon_type->id==82) {
      if(bullet->owner!=0 || bullet->bullet_owner_unit || bullet->bullet_target!=victim)
       throw std::runtime_error("detached weapon82 attribution mismatch");
      attributed=true;
     }
    if(victim->hp.raw_value!=hp)break;
   }
   if(!detached)throw std::runtime_error("source parent death did not detach shot");
   if(!attributed || victim->hp.raw_value==hp)throw std::runtime_error("detached shot weapon/damage not observed");
   std::cout<<"result scenario=parentdead detached shot damages target\n";
   return 0;
  }
  if(scenario=="targetdead") {
   api.kill_unit(victim);bool cleared=false;
   for(int i=0;i<120;i++){
    api.next_frame();
    if(shot->order_type&&shot->order_type->id==Orders::ScarabAttack && !shot->order_target.unit)cleared=true;
    if(cleared && shot->order_type && shot->order_type->id==Orders::ScarabAttack && shot->order_target.unit)
      throw std::runtime_error("shot reacquired target after source cleanup");
   }
   if(!cleared)throw std::runtime_error("dead target reference was not cleared during attack");
   std::cout<<"result scenario=targetdead cleared target; no reacquisition during attack\n";
   return 0;
  }
  if(scenario=="stop") {
   auto original_hp=victim->hp.raw_value;
   if(!input.action_reaver_stop(0))throw std::runtime_error("native Reaver STOP rejected");
   bool saw_die=false;
   for(int i=0;i<30;i++){
    api.next_frame();
    if(shot->order_type&&shot->order_type->id==Orders::Die)saw_die=true;
    for(const bullet_t *bullet:ptr(world->active_bullets))
     if(bullet->weapon_type&&(int)bullet->weapon_type->id==82)throw std::runtime_error("STOP emitted weapon82");
    if(victim->hp.raw_value!=original_hp)throw std::runtime_error("STOP caused victim damage");
   }
   if(!saw_die)throw std::runtime_error("native STOP did not kill outside Scarab");
   std::cout<<"result scenario=stop no weapon82/damage; outside Scarab died\n";
   return 0;
  }
  auto initial_hp=victim->hp.raw_value;
  int observed_init=0, observed_protected_passes=0, arrival_frame=-1, weapon82_before_damage=0, damage_frame=-1;
  for(int i=0;i<140;i++) {
   std::cout<<"observe frame="<<world->current_frame<<" shot_order="<<(shot->order_type?(int)shot->order_type->id:-1)
    <<" state="<<shot->order_state<<" pos="<<shot->position.x<<","<<shot->position.y
    <<" exact="<<shot->exact_position.x.raw_value<<","<<shot->exact_position.y.raw_value
    <<" heading="<<(int)(uint8_t)shot->heading.raw_value<<" vel_heading="<<(int)(uint8_t)shot->current_velocity_direction.raw_value
    <<" desired="<<(int)(uint8_t)shot->desired_velocity_direction.raw_value
    <<" next="<<shot->next_movement_waypoint.x<<","<<shot->next_movement_waypoint.y
    <<" dest="<<shot->move_target.pos.x<<","<<shot->move_target.pos.y
    <<" flags="<<shot->movement_flags<<" speed="<<shot->current_speed.raw_value
    <<" shot_hp="<<shot->hp.raw_value<<" victim_hp="<<victim->hp.raw_value<<" remove="<<shot->remove_timer<<" order_timer="<<shot->main_order_timer
    <<" no_collide="<<api.u_no_collide(shot)<<" bullets="<<world->active_bullets_size
    <<" child_signal="<<shot->order_signal<<" child_sprite="<<(shot->sprite!=nullptr)
    <<" child_wait="<<(shot->sprite && shot->sprite->main_image ? shot->sprite->main_image->iscript_state.wait : -1)
    <<" child_anim="<<(shot->sprite && shot->sprite->main_image ? shot->sprite->main_image->iscript_state.animation : -1)<<"\n";
   if(shot->order_type && shot->order_type->id==Orders::ScarabAttack && shot->order_state==2 && !observed_init){
    if(shot->remove_timer>90 || shot->main_order_timer>7)throw std::runtime_error("invalid observed initialized timers");
    observed_init=1;
   }
   if(observed_init && observed_protected_passes<7 && shot->order_type && shot->order_type->id==Orders::ScarabAttack){
    if(api.u_no_collide(shot)) ++observed_protected_passes;
   }
   if(observed_protected_passes==7 && shot->order_type && shot->order_type->id==Orders::ScarabAttack && shot->main_order_timer==0 && api.u_no_collide(shot))throw std::runtime_error("no-collision failed to clear");
   if(shot->order_type && shot->order_type->id==Orders::ScarabAttack && shot->order_state==6 && arrival_frame<0)arrival_frame=world->current_frame;
   weapon82_before_damage=0;
   for(const bullet_t *bullet:ptr(world->active_bullets)) {
    if(bullet->weapon_type && (int)bullet->weapon_type->id==82 && bullet->owner==0){
     if(bullet->bullet_owner_unit!=parent || bullet->bullet_target!=victim)throw std::runtime_error("weapon82 parent/target attribution mismatch");
     weapon82_before_damage=1;
    }
    std::cout<<"bullet frame="<<world->current_frame<<" weapon="<<(bullet->weapon_type?(int)bullet->weapon_type->id:-1)
      <<" owner="<<bullet->owner<<" state="<<bullet->bullet_state<<" signal="<<bullet->order_signal
      <<" wait="<<(bullet->sprite && bullet->sprite->main_image ? bullet->sprite->main_image->iscript_state.wait : -1)
      <<" anim="<<(bullet->sprite && bullet->sprite->main_image ? bullet->sprite->main_image->iscript_state.animation : -1)<<"\n";
   }
   api.next_frame();
   if(victim->hp.raw_value!=initial_hp && damage_frame<0){damage_frame=world->current_frame;std::cout<<"victim_hp_changed frame="<<damage_frame<<" victim_hp="<<victim->hp.raw_value<<"\n";}
   if(damage_frame>=0 && world->current_frame>=damage_frame+8)break;
  }
  if(!observed_init)throw std::runtime_error("Scarab attack did not initialize");
  if(victim->hp.raw_value==initial_hp)throw std::runtime_error("no victim damage observed in bounded window");
  if(damage_frame<0 || arrival_frame<0)throw std::runtime_error("damage lacks observed source weapon/arrival");

  std::cout<<"result scenario="<<scenario<<" arrival="<<arrival_frame<<" damage="<<damage_frame<<" protected_passes="<<observed_protected_passes<<"\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}
}
