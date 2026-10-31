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
 if(argc!=4){std::cerr<<"usage: world-probe table-dir image-dir scenario\n";return 2;}
 std::string scenario=argv[3];

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
  int raw=0,occupancy=0; if(sscanf(scenario.c_str(),"%d:%d",&raw,&occupancy)!=2 || (raw!=0 && raw!=70) || occupancy<98 || occupancy>100)return 2;
  auto *actor=api.create_starting_unit(api.get_unit_type((UnitTypes)raw),{320,320},0);
  auto *victim=api.create_starting_unit(api.get_unit_type(UnitTypes::Terran_Battlecruiser),{440,320},1);
  auto *filler=api.create_starting_unit(api.get_unit_type(UnitTypes::Terran_Marine),{100,100},2);
  if(!actor||!victim||!filler)throw std::runtime_error("actor create failed");
  auto *weapon=api.unit_air_weapon(actor); if(!weapon)throw std::runtime_error("no air weapon");
  std::cout<<"weapon="<<(int)weapon->id<<" bullet_count="<<(int)weapon->bullet_count<<" cooldown="<<(int)weapon->cooldown<<"\n";
  action_state actions{};action_functions input(*world,actions);
  if(!input.action_select(0,actor)||!input.action_order(0,api.get_order_type(Orders::AttackDefault),victim->position,victim,nullptr,false))throw std::runtime_error("attack rejected");
  for(int t=1;t<=40;t++){
   while((int)world->active_bullets_size<occupancy) { if(!api.create_bullet(api.get_weapon_type((WeaponTypes)0),filler,filler->position,2,filler->heading))throw std::runtime_error("fill failed"); }
   int before=world->active_bullets_size; api.next_frame(); int own=0;for(const bullet_t *b:ptr(world->active_bullets))if(b->bullet_owner_unit==actor)++own;
   if(t==1 && actor->air_weapon_cooldown==0)throw std::runtime_error("attack did not spend cooldown");
   const int release=raw==0 ? 3 : 1;
   if(t==release) {
    int expected=raw==0 ? (occupancy<100 ? 1 : 0) : 100-occupancy;
    if(own!=expected)throw std::runtime_error("unexpected first-release allocation");
    if(occupancy==100 && victim->hp.raw_value!=128000)throw std::runtime_error("damage landed despite full pool");
   }
   std::cout<<"t="<<t<<" before="<<before<<" total="<<world->active_bullets_size<<" own="<<own<<" cd="<<(int)actor->air_weapon_cooldown<<" hp="<<victim->hp.raw_value<<" order="<<(int)actor->order_type->id<<"\n";
  }

  return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<"\n";return 1;}
}
