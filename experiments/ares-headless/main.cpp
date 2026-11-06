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
  ares::Node::Input::Button a, start, down;
  ares::Node::Input::Axis xAxis, yAxis;
  std::atomic<int> axisX{0}, axisY{0};
  std::atomic<unsigned> mask{0};
  std::atomic<unsigned long long> polls{0}, aPolls{0}, startPolls{0}, aPressedPolls{0}, startPressedPolls{0}, audioCalls{0};
  std::atomic<unsigned> step{0};
  struct Frame { unsigned step,width,height; std::vector<std::uint32_t> pixels; };
  std::vector<Frame> frames;
  struct Callback { unsigned step,width,height; bool nonblack; };
  std::vector<Callback> observations;
  struct InputEvent { unsigned step,button; int pressed; };
  std::vector<InputEvent> inputEvents;
  unsigned eligible=0;
  std::vector<std::string> statuses;
  std::size_t frameBytes=0;
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<std::uint32_t> pixels;
  unsigned width=0, height=0, callbacks=0, discarded=0;
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
    statuses.push_back(text);
    if(text.find("Vulkan Enabled: using paraLLEl-RDP") != std::string::npos) gpuReady=true;
    if(text.find("No RDP rendering support") != std::string::npos) { failed=true; failure=text; cv.notify_all(); }
  }
  auto input(ares::Node::Input::Input node) -> void override {
    ++polls;
    if(auto button=node->cast<ares::Node::Input::Button>()) {
      unsigned m=mask.load();
      if(button==a) { ++aPolls; if(m&1) ++aPressedPolls; }
      if(button==start) { ++startPolls; if(m&2) ++startPressedPolls; }
      bool pressed=(button==a && (m&1)) || (button==start && (m&2)) || (button==down && (m&4));
      if(button==a || button==start || button==down) { std::lock_guard lock(mutex); if(inputEvents.size()>=65536) { failed=true; failure="input event budget exceeded"; } else inputEvents.push_back({step.load(),button==a?1u:(button==start?2u:4u),pressed?1u:0u}); }
      button->setValue(pressed);
    } else if(auto axis=node->cast<ares::Node::Input::Axis>()) {
      int value=axis==xAxis?axisX.load():(axis==yAxis?axisY.load():0);
      if(axis==xAxis || axis==yAxis) { std::lock_guard lock(mutex); if(inputEvents.size()>=65536) { failed=true; failure="input event budget exceeded"; } else inputEvents.push_back({step.load(),axis==xAxis?8u:16u,value}); }
      axis->setValue(value);
    }
  }
  auto audio(ares::Node::Audio::Stream stream) -> void override {
    ++audioCalls;
    if(stream->channels()>8) { std::lock_guard lock(mutex); failed=true; failure="audio channel limit"; cv.notify_all(); return; }
    double samples[8]{};
    while(stream->pending()) stream->read(samples);
  }
  auto video(ares::Node::Video::Screen, const u32* data, u32 pitch, u32 w, u32 h) -> void override {
    std::lock_guard lock(mutex);
    if(!data || !w || !h || w>1920 || h>1080 || pitch<w*4u || pitch>32768u) {
      failed=true; failure="invalid frame bounds"; cv.notify_all(); return;
    }
    if(observations.size()>=16384) { failed=true; failure="callback metadata budget exceeded"; cv.notify_all(); return; }
    bool nonblack=false;
    for(unsigned y=0;y<h && !nonblack;++y) {
      auto row=reinterpret_cast<const std::uint32_t*>(reinterpret_cast<const unsigned char*>(data)+static_cast<std::size_t>(y)*pitch);
      for(unsigned x=0;x<w;++x) if(row[x]&0x00ffffffu) { nonblack=true; break; }
    }
    observations.push_back({step.load(),w,h,nonblack}); ++callbacks;
    if(w==1 && h==1) { cv.notify_all(); return; }
    auto observed=step.load();
    bool special=observed==7015 || observed==7038;
    if((eligible++ % 360)!=0 && !special) return;
    auto bytes=static_cast<std::size_t>(w)*h*sizeof(std::uint32_t);
    if(frames.size()>=32) { ++discarded; return; }
    if(frameBytes+bytes>64u*1024u*1024u) { failed=true; failure="frame byte budget exceeded"; cv.notify_all(); return; }
    Frame f{step.load(),w,h,{}}; f.pixels.resize(static_cast<std::size_t>(w)*h);
    for(unsigned y=0;y<h;++y) std::memcpy(f.pixels.data()+static_cast<std::size_t>(y)*w,
      reinterpret_cast<const unsigned char*>(data)+static_cast<std::size_t>(y)*pitch,w*4u);
    frameBytes+=bytes; frames.push_back(std::move(f));
    cv.notify_all();
  }
};

int main(int argc,char** argv) {
  if(argc!=5) { std::cerr<<"usage: probe ROM OUTPUT_DIR neutral|tap FRAMES\n"; return 2; }
  std::string scenario=argv[3];
  char* end=nullptr; long frames=std::strtol(argv[4],&end,10);
  if((scenario!="neutral" && scenario!="tap" && scenario!="start" && scenario!="mission" && scenario!="advance" && scenario!="calibrate") || !end || *end || frames<1 || frames>9000) return 2;
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
    if(button->name()=="Down") p.down=button;
  }
  for(auto axis:pad->find<ares::Node::Input::Axis>()) {
    if(axis->name()=="X-Axis") p.xAxis=axis;
    if(axis->name()=="Y-Axis") p.yAxis=axis;
  }
  if(!p.a || !p.start || !p.down || !p.xAxis || !p.yAxis) return 4;
  root->power();
  std::ofstream meta(out/"frames.csv",std::ios::binary), steps(out/"steps.csv",std::ios::binary);
  if(!meta || !steps) return 5;
  meta<<"callback,observed_step,width,height,bytes\n";
  steps<<"run,mask,polls,a_polls,start_polls,audio_calls,callbacks\n";
  long completed=0;
  for(long i=0;i<frames;++i) {
    unsigned mask=scenario=="tap" ? ((i>=80 && i<90)?2u:(i>=100 && i<105)?1u:0u) : 0u;
    if((scenario=="start" || scenario=="mission" || scenario=="advance" || scenario=="calibrate") && ((i>=700 && i<710)||(i>=1000 && i<1010)||(i>=1300 && i<1310)||(i>=1600 && i<1610))) mask=2u;
    if((scenario=="mission" || scenario=="advance" || scenario=="calibrate") && ((i>=3900 && i<3910)||(i>=4000 && i<4010))) mask=4u;
    if((scenario=="mission" || scenario=="advance" || scenario=="calibrate") && i>=4100 && i<4110) mask=1u;
    if((scenario=="advance" || scenario=="calibrate") && ((i>=5000 && i<5010)||(i>=5600 && i<5610)||(i>=6200 && i<6210)||(i>=6800 && i<6810))) mask=2u;
    p.axisX=(scenario=="calibrate" && i>=7020 && i<7036)?-24000:0;
    p.axisY=0; p.mask=mask; p.step=i;
    root->run(); ++completed;
    std::lock_guard lock(p.mutex);
    steps<<i<<','<<mask<<','<<p.polls.load()<<','<<p.aPolls.load()<<','<<p.startPolls.load()<<','<<p.audioCalls.load()<<','<<p.callbacks<<'\n';
    if(p.failed || p.frames.size()>=32) break;
  }
  {
    std::unique_lock lock(p.mutex);
    if(!p.failed && p.callbacks==0) p.cv.wait_for(lock,std::chrono::seconds(5),[&]{return p.failed || p.callbacks>0;});
  }
  root->unload(); root.reset(); // joins the upstream screen thread before reading frames
  std::lock_guard lock(p.mutex);
  std::ofstream status(out/"status.log"), observations(out/"callbacks.csv");
  std::ofstream inputs(out/"inputs.csv"); inputs<<"observed_step,button,pressed\n";
  for(auto &e:p.inputEvents) inputs<<e.step<<','<<e.button<<','<<e.pressed<<'\n';
  observations<<"callback,observed_step,width,height,nonblack\n";
  for(std::size_t i=0;i<p.observations.size();++i) { auto &o=p.observations[i]; observations<<i<<','<<o.step<<','<<o.width<<','<<o.height<<','<<o.nonblack<<'\n'; }
  for(auto& text:p.statuses) status<<text<<'\n';
  status<<"requested_steps="<<frames<<" completed_steps="<<completed<<" polls="<<p.polls.load()<<" a_polls="<<p.aPolls.load()<<" start_polls="<<p.startPolls.load()<<" a_pressed_polls="<<p.aPressedPolls.load()<<" start_pressed_polls="<<p.startPressedPolls.load()<<" audio_calls="<<p.audioCalls.load()<<" callbacks_total="<<p.callbacks<<" frames_copied="<<p.frames.size()<<" callbacks_discarded="<<p.discarded<<" gpu_ready="<<p.gpuReady<<" failure="<<p.failure<<'\n';
  if(p.failed || !p.gpuReady || p.frames.empty()) { std::cerr<<"renderer or callback failure: "<<p.failure<<" callbacks="<<p.callbacks<<"\n"; return 6; }
  for(std::size_t i=0;i<p.frames.size();++i) {
    auto& f=p.frames[i]; auto bytes=f.pixels.size()*sizeof(std::uint32_t);
    char name[32]; std::snprintf(name,sizeof(name),"frame-%03zu.argb",i);
    std::ofstream frame(out/name,std::ios::binary); frame.write(reinterpret_cast<const char*>(f.pixels.data()),bytes);
    if(!frame) return 5;
    meta<<i<<','<<f.step<<','<<f.width<<','<<f.height<<','<<bytes<<'\n';
  }
  meta.flush(); steps.flush(); if(!meta || !steps || !status || !observations || !inputs) return 5;
  return 0;
}
