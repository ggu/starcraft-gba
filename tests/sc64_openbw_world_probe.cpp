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
 if(argc!=3){std::cerr<<"usage: world-probe table-dir image-dir\n";return 2;}
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
  auto *parent=api.create_starting_unit(api.get_unit_type(UnitTypes::Protoss_Reaver),{320,320},0);
  auto *victim=api.create_starting_unit(api.get_unit_type(UnitTypes::Terran_Marine),{400,320},1);
  if(!parent||!victim||api.us_hidden(parent)||api.us_hidden(victim))throw std::runtime_error("actor creation or placement failed");
  action_state actions{};action_functions input(*world,actions);
  if(!input.action_select(0,parent)||!input.action_train_fighter(0))throw std::runtime_error("native fighter training action rejected");
  for(int i=0;i<1000 && parent->reaver.inside_count==0;i++)api.next_frame();
  if(parent->reaver.inside_count!=1)throw std::runtime_error("native fighter training did not complete");
  std::cout<<"trained frame="<<world->current_frame<<" inside="<<parent->reaver.inside_count<<" minerals="<<world->current_minerals[0]<<"\n";
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
  if(!shot || shot->fighter.parent!=parent || api.us_hidden(shot))throw std::runtime_error("invalid released fighter linkage");
  auto initial_hp=victim->hp.raw_value;
  for(int i=0;i<140;i++) {
   std::cout<<"observe frame="<<world->current_frame<<" shot_order="<<(shot->order_type?(int)shot->order_type->id:-1)
    <<" state="<<shot->order_state<<" pos="<<shot->position.x<<","<<shot->position.y
    <<" hp="<<victim->hp.raw_value<<" remove="<<shot->remove_timer<<" order_timer="<<shot->main_order_timer
    <<" no_collide="<<api.u_no_collide(shot)<<" bullets="<<world->active_bullets_size<<"\n";
   for(const bullet_t *bullet:ptr(world->active_bullets))
    std::cout<<"bullet frame="<<world->current_frame<<" weapon="<<(bullet->weapon_type?(int)bullet->weapon_type->id:-1)
      <<" owner="<<bullet->owner<<" state="<<bullet->bullet_state<<"\n";
   api.next_frame();
   if(victim->hp.raw_value!=initial_hp){std::cout<<"victim_hp_changed frame="<<world->current_frame<<" hp="<<victim->hp.raw_value<<"\n";break;}
  }
  if(victim->hp.raw_value==initial_hp)throw std::runtime_error("no victim damage observed in bounded window");
 }catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}
}
