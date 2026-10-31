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
  for(int i=0;i<228;i++) {
   auto *u=api.get_unit_type((UnitTypes)i);
   std::cout<<i<<" "<<u->dimensions.from.x<<" "<<u->dimensions.from.y<<" "
            <<u->dimensions.to.x<<" "<<u->dimensions.to.y<<"\n";
  }

  return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<"\n";return 1;}
}
