// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental source-integrated ares frontend; no host-input or guest-memory API.
#include <ares/ares.hpp>
#include <mia/mia.hpp>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

namespace ares::Nintendo64 {
  auto load(Node::System&, string) -> bool;
  auto option(string, string) -> bool;
}

struct Probe final : ares::Platform {
  std::shared_ptr<mia::Pak> systemPak, gamePak;
  ares::Node::Input::Button a, start;
  std::atomic<unsigned> mask{0};
  std::atomic<unsigned long long> polls{0}, aPolls{0}, startPolls{0};
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<std::uint32_t> pixels;
  unsigned width=0, height=0, callbacks=0;
  bool failed=false, gpuReady=false;
  std::string failure;

  auto pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> override {
    if(node->name()=="Nintendo 64") return systemPak->pak;
    if(node->name()=="Nintendo 64 Cartridge") return gamePak->pak;
    return {};
  }
  auto status(string_view message) -> void override {
    std::string text{message.data(), message.size()};
    std::lock_guard lock(mutex);
    if(text.find("Vulkan Enabled: using paraLLEl-RDP") != std::string::npos) gpuReady=true;
    if(text.find("No RDP rendering support") != std::string::npos) { failed=true; failure=text; cv.notify_all(); }
  }
  auto input(ares::Node::Input::Input node) -> void override {
    ++polls;
    if(auto button=node->cast<ares::Node::Input::Button>()) {
      unsigned m=mask.load();
      if(button==a) ++aPolls;
      if(button==start) ++startPolls;
      button->setValue((button==a && (m&1)) || (button==start && (m&2)));
    } else if(auto axis=node->cast<ares::Node::Input::Axis>()) axis->setValue(0);
  }
  auto audio(ares::Node::Audio::Stream stream) -> void override {
    if(stream->channels()>8) { std::lock_guard lock(mutex); failed=true; failure="audio channel limit"; cv.notify_all(); return; }
    double samples[8]{};
    while(stream->pending()) stream->read(samples);
  }
  auto video(ares::Node::Video::Screen, const u32* data, u32 pitch, u32 w, u32 h) -> void override {
    std::lock_guard lock(mutex);
    if(!data || !w || !h || w>1920 || h>1080 || pitch<w*4u || pitch>32768u) {
      failed=true; failure="invalid frame bounds"; cv.notify_all(); return;
    }
    if(!pixels.empty()) { failed=true; failure="unconsumed frame"; cv.notify_all(); return; }
    width=w; height=h; pixels.resize(static_cast<std::size_t>(w)*h);
    for(unsigned y=0;y<h;++y) std::memcpy(pixels.data()+static_cast<std::size_t>(y)*w,
      reinterpret_cast<const unsigned char*>(data)+static_cast<std::size_t>(y)*pitch,w*4u);
    ++callbacks; cv.notify_all();
  }
};

int main(int argc,char** argv) {
  if(argc!=5) { std::cerr<<"usage: probe ROM OUTPUT_DIR neutral|tap FRAMES\n"; return 2; }
  std::string scenario=argv[3];
  char* end=nullptr; long frames=std::strtol(argv[4],&end,10);
  if((scenario!="neutral" && scenario!="tap") || !end || *end || frames<1 || frames>32) return 2;
  std::filesystem::path out=argv[2];
  if(!std::filesystem::is_directory(out) || !std::filesystem::is_directory(out/"saves") ||
     !std::filesystem::is_directory(out/"systems")) return 2;
  Probe p; ares::platform=&p;
  auto saves=std::filesystem::absolute(out/"saves").string()+"/";
  auto systems=std::filesystem::absolute(out/"systems").string()+"/";
  mia::setSaveLocation([saves] { return string{saves.c_str()}; });
  mia::setHomeLocation([systems] { return string{systems.c_str()}; });
  mia::construct();
  p.gamePak=mia::Medium::create("Nintendo 64");
  p.systemPak=mia::System::create("Nintendo 64");
  if(!p.gamePak || !p.systemPak || p.gamePak->load(argv[1])!=successful || p.systemPak->load()!=successful) return 3;
  // v148 currently returns true even for unknown option names; GPU status is
  // additionally checked on first run, and other settings remain source-scoped.
  if(!ares::Nintendo64::option("Quality","SD") ||
     !ares::Nintendo64::option("Supersampling","false") ||
     !ares::Nintendo64::option("Enable GPU acceleration","true") ||
     !ares::Nintendo64::option("Homebrew Mode","false") ||
     !ares::Nintendo64::option("Recompiler","true") ||
     !ares::Nintendo64::option("Expansion Pak","true")) return 3;
  ares::Node::System root;
  if(!ares::Nintendo64::load(root,"[Nintendo] Nintendo 64 (NTSC)")) return 4;
  struct Unload { ares::Node::System& node; ~Unload() { if(node) node->unload(); ares::platform=nullptr; } } unload{root};
  auto cart=root->find<ares::Node::Port>("Cartridge Slot");
  auto port=root->find<ares::Node::Port>("Controller Port 1");
  if(!cart || !port || !cart->allocate()) return 4;
  cart->connect();
  if(!cart->connected() || !cart->connected()->pak()) return 4;
  auto pad=port->allocate("Gamepad"); if(!pad) return 4; port->connect();
  for(auto button:pad->find<ares::Node::Input::Button>()) {
    if(button->name()=="A") p.a=button;
    if(button->name()=="Start") p.start=button;
  }
  if(!p.a || !p.start) return 4;
  root->power();
  std::ofstream meta(out/"frames.csv",std::ios::binary);
  if(!meta) return 5;
  meta<<"run,mask,polls,a_polls,start_polls,callback,width,height,bytes\n";
  std::uint64_t total=0;
  for(long i=0;i<frames;++i) {
    unsigned mask=scenario=="tap" ? ((i>=10 && i<12)?2u:(i>=20 && i<22)?1u:0u) : 0u;
    p.mask=mask;
    unsigned prior; { std::lock_guard lock(p.mutex); prior=p.callbacks; }
    root->run();
    std::unique_lock lock(p.mutex);
    if(!p.cv.wait_for(lock,std::chrono::seconds(5),[&]{return p.failed || p.callbacks>prior;})) { std::cerr<<"frame callback timeout\n"; return 6; }
    if(p.failed || !p.gpuReady || p.callbacks!=prior+1) { std::cerr<<"renderer or callback failure: "<<p.failure<<"\n"; return 6; }
    auto bytes=p.pixels.size()*sizeof(std::uint32_t); total+=bytes;
    if(total>64u*1024u*1024u) return 6;
    char name[32]; std::snprintf(name,sizeof(name),"frame-%03ld.argb",i);
    std::ofstream frame(out/name,std::ios::binary); frame.write(reinterpret_cast<const char*>(p.pixels.data()),bytes);
    if(!frame) return 5;
    meta<<i<<','<<mask<<','<<p.polls.load()<<','<<p.aPolls.load()<<','<<p.startPolls.load()<<','<<p.callbacks<<','<<p.width<<','<<p.height<<','<<bytes<<'\n';
    p.pixels.clear();
  }
  meta.flush(); if(!meta) return 5;
  return 0;
}
