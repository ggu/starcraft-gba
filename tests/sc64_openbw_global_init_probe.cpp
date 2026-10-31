/* SPDX-License-Identifier: GPL-3.0-or-later
 * Exercise pinned global_init through IScript and image loading, then stop at
 * the first tileset request. Not a world-reset or scheduler test. */
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include "bwgame.h"
using namespace bwgame;
struct StopBeforeTilesets {};
static std::vector<uint8_t> read(const std::string &path) {
    std::ifstream f(path,std::ios::binary);
    if(!f)throw std::runtime_error("missing required input: "+path);
    return {std::istreambuf_iterator<char>(f),{}};
}
int main(int argc,char **argv) {
 if(argc!=3){std::cerr<<"usage: probe table-dir image-dir\n";return 2;}
 try {
  std::string tables=std::string(argv[1])+"/",images=std::string(argv[2])+"/";
  global_state actual{};bool stopped=false;
  auto provider=[&](auto &out,auto name_value) {
   std::string name=name_value;
   if(name.rfind("Tileset/",0)==0)throw StopBeforeTilesets{};
   if(name=="triggers/Melee.trg"){out.clear();return;} // loaded but not used before deliberate stop
   std::string path;
   if(name=="scripts/iscript.bin")path=tables+"iscript.bin";
   else if(name=="arr/images.tbl")path=images+"images.tbl";
   else if(name.rfind("arr/",0)==0)path=tables+name.substr(4);
   else if(name.rfind("unit/",0)==0)path=images+name;
   else throw std::runtime_error("unexpected OpenBW request: "+name);
   auto v=read(path);out.assign(v.begin(),v.end());
  };
  try {global_init(actual,provider);}catch(const StopBeforeTilesets &){stopped=true;}
  if(!stopped)throw std::runtime_error("global_init did not reach tileset boundary");
  global_state st{};
  auto load_data_file=[&](auto &out,const char *name){
   if(std::string(name)!="scripts/iscript.bin")throw std::runtime_error("unexpected loader request");
   auto v=read(tables+"iscript.bin");out.assign(v.begin(),v.end());
  };
#include "openbw_iscript_loader.inc"
  if(st.iscript.program_data!=actual.iscript.program_data || st.iscript.scripts.size()!=actual.iscript.scripts.size())return 1;
  for(const auto &item:st.iscript.scripts){auto i=actual.iscript.scripts.find(item.first);if(i==actual.iscript.scripts.end()||i->second.animation_pc!=item.second.animation_pc)return 1;}
  std::cout<<"actual global_init image/IScript load completed; "<<actual.grps.size()<<" GRPs; "<<actual.lo_offsets.size()<<" LO tables; extracted loader matches full program and PCs\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<"\n";return 1;}
}
