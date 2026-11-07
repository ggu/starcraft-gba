#define SC64_OBSERVER_STOP_ON_SIGXFSZ 1
#include "observer_sigxfsz.h"
#include "observer_images.h"
#include "visual_capture.h"
#include "observer_read.h"
#include "hybrid_contract.h"
#include "hybrid_snapshot_contract.h"
#include <fcntl.h>
// SPDX-License-Identifier: GPL-3.0-or-later
// Hybrid frontend: official same-process state restoration; no arbitrary guest-memory writes or host-input API.
#include <ares/ares.hpp>
#include <n64/n64.hpp>
#include <mia/mia.hpp>
#include <atomic>
#include <poll.h>
#include <sys/resource.h>
#include <algorithm>
#include <unistd.h>
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
#include <sstream>
#include <vector>
#include "input_mapping.h"
#include "observer_file_budget.h"

namespace ares::Nintendo64 {
  auto load(Node::System&, string) -> bool;
  auto option(string, string) -> bool;
}

struct Probe final : ares::Platform {
  std::shared_ptr<mia::Pak> systemPak, gamePak;
  ares::Node::Input::Button a, b, start, down, cLeft, cRight, cUp, cDown, right, r;
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
  std::atomic<bool> forceCapture{false};
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
    if(statuses.size()>=256 || text.size()>512) { failed=true; failure="status budget exceeded"; cv.notify_all(); return; }
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
      unsigned id=button==a?1u:button==start?2u:button==down?4u:
          button==cLeft?32u:button==right?64u:button==r?128u:
          button==cRight?256u:button==cUp?512u:button==cDown?1024u:button==b?2048u:0u;
      bool pressed=sc64_button_pressed(id,m);
      if(id) { std::lock_guard lock(mutex); if(inputEvents.size()>=131072) { failed=true; failure="input event budget exceeded"; } else inputEvents.push_back({step.load(),id,pressed?1:0}); }
      button->setValue(pressed);
    } else if(auto axis=node->cast<ares::Node::Input::Axis>()) {
      int value=axis==xAxis?axisX.load():(axis==yAxis?axisY.load():0);
      if(axis==xAxis || axis==yAxis) { std::lock_guard lock(mutex); if(inputEvents.size()>=131072) { failed=true; failure="input event budget exceeded"; } else inputEvents.push_back({step.load(),axis==xAxis?8u:16u,value}); }
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
    if(!forceCapture.exchange(false)) return;
    auto observed=step.load();
    bool special=observed==6400 || observed==6540 || observed==6580 || observed==6650 || observed==6850 || observed==6900;
    (void)special;
    auto bytes=static_cast<std::size_t>(w)*h*sizeof(std::uint32_t);
    if(frames.size()>=3) { ++discarded; return; }
    if(frameBytes+bytes>2u*1024u*1024u) { failed=true; failure="frame byte budget exceeded"; cv.notify_all(); return; }
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
  if((scenario!="interactive" && scenario!="snapshot" && scenario!="neutral" && scenario!="tap" && scenario!="start" && scenario!="mission" && scenario!="advance" && scenario!="calibrate" && scenario!="select" && scenario!="select2" && scenario!="select3" && scenario!="select4" && scenario!="build" && scenario!="grid" && scenario!="custom" && scenario!="customstart" && scenario!="protosssetup" && scenario!="protossstart" && scenario!="protossselect" && scenario!="probe2" && scenario!="probe3") || !end || *end || frames<1 || frames>14000) return 2;
  std::filesystem::path out=argv[2];
  if(!std::filesystem::is_directory(out) || !std::filesystem::is_directory(out/"saves") ||
     !std::filesystem::is_directory(out/"systems")) return 2;
  int diagnostic_fd=::open((out/"sigxfsz.bin").c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
  if(diagnostic_fd<0 || !sc64_observer_signal_install(diagnostic_fd)) return 2;
  rlimit initial_file_limit{};
  if(getrlimit(RLIMIT_FSIZE,&initial_file_limit)!=0 ||
     initial_file_limit.rlim_max!=SC64_OBSERVER_RAM_FILE ||
     (initial_file_limit.rlim_cur!=SC64_OBSERVER_SMALL_FILE && initial_file_limit.rlim_cur!=SC64_OBSERVER_RAM_FILE)) return 2;
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
    if(button->name()=="B") p.b=button;
    if(button->name()=="Start") p.start=button;
    if(button->name()=="Down") p.down=button;
    if(button->name()=="C-Left") p.cLeft=button;
    if(button->name()=="C-Right") p.cRight=button;
    if(button->name()=="C-Up") p.cUp=button;
    if(button->name()=="C-Down") p.cDown=button;
    if(button->name()=="Right") p.right=button;
    if(button->name()=="R") p.r=button;
  }
  for(auto axis:pad->find<ares::Node::Input::Axis>()) {
    if(axis->name()=="X-Axis") p.xAxis=axis;
    if(axis->name()=="Y-Axis") p.yAxis=axis;
  }
  if(!p.a || !p.b || !p.start || !p.down || !p.cLeft || !p.cRight || !p.cUp || !p.cDown || !p.right || !p.r || !p.xAxis || !p.yAxis) return 4;
  root->power();
  if(!ares::Nintendo64::cpu.recompiler.enabled || !ares::Nintendo64::rsp.recompiler.enabled || ares::Nintendo64::system.homebrewMode) { std::cerr<<"unsafe CPU or homebrew configuration\n"; return 3; }
  const bool cpu_jit=ares::Nintendo64::cpu.recompiler.enabled, rsp_jit=ares::Nintendo64::rsp.recompiler.enabled, homebrew=ares::Nintendo64::system.homebrewMode;
  std::ofstream meta(out/"frames.csv",std::ios::binary), steps(out/"steps.csv",std::ios::binary);
  if(!meta || !steps) return 5;
  meta<<"callback,observed_step,width,height,bytes\n";
  steps<<"run,mask,polls,a_polls,start_polls,audio_calls,callbacks\n";
  long completed=0; unsigned totalIdle=0, snapshotCount=0; bool transitioned=false, view_equal=false; auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(175);
  std::ofstream commands(out/"commands.csv"); commands<<"begin,steps,mask,x,y,capture\n";
  int images_fd=::open((out/"images.bin").c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
  if(images_fd<0 || !sc64_observer_images_write(images_fd) || ::close(images_fd)!=0) return 2;
  std::cout<<"READY"<<std::endl;
  while(completed<frames && std::chrono::steady_clock::now()<deadline) {
    {std::lock_guard lock(p.mutex); if(p.frames.size()>=3) break;}
    std::string line; auto commandDeadline=std::min(deadline,std::chrono::steady_clock::now()+std::chrono::seconds(30));
    while(line.size()<128) {
      auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(commandDeadline-std::chrono::steady_clock::now()).count();
      if(totalIdle>=60000 || remaining<=0) break;
      int waitMs=static_cast<int>(std::min<long long>({remaining,60000-totalIdle}));
      pollfd fd{STDIN_FILENO,POLLIN,0}; auto started=std::chrono::steady_clock::now(); int ready=poll(&fd,1,waitMs);
      totalIdle+=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();
      if(ready<=0 || !(fd.revents&POLLIN)) break;
      char ch; if(read(STDIN_FILENO,&ch,1)!=1) break; if(ch=='\n') break; line+=ch;
    }
    if(line.empty() || line.size()>=128 || std::chrono::steady_clock::now()>=commandDeadline || totalIdle>=60000) break;
    std::istringstream in(line); int count,mask,x,y,capture; std::string extra;
    if(!(in>>count>>mask>>x>>y>>capture) || (in>>extra) || count<1 || count>1000 || completed+count>frames || !sc64_visual_mask_allowed(mask) || x < -32767 || x>32767 || y < -32767 || y>32767 || !(sc64_hybrid_capture_allowed(capture) || (scenario=="snapshot" && capture==3)) || (capture==3 && !sc64_hybrid_snapshot_allowed(scenario=="snapshot",transitioned,view_equal,ares::Nintendo64::cpu.recompiler.enabled,ares::Nintendo64::rsp.recompiler.enabled,ares::Nintendo64::system.homebrewMode,ares::Nintendo64::rdram.ram.size,snapshotCount)) || (capture==2 && (transitioned || completed+count!=4200))) { std::cerr<<"invalid command\n"; return 7; }
    commands<<completed<<','<<count<<','<<mask<<','<<x<<','<<y<<','<<capture<<'\n'; commands.flush();
    unsigned previous; {std::lock_guard lock(p.mutex); previous=p.frames.size();}
    for(int i=0;i<count;++i) { p.mask=mask; p.axisX=x; p.axisY=y; p.step=completed; if(capture==1 && i==count-1) p.forceCapture=true; root->run(); ++completed; }
    if(capture==2) {
      using namespace ares::Nintendo64;
      if(transitioned || completed!=4200 || !cpu.recompiler.enabled || !rsp.recompiler.enabled || ares::Nintendo64::system.homebrewMode || rdram.ram.size!=8u*1024u*1024u) return 8;
      auto read_view=[&](std::vector<std::uint8_t>& bytes) {
        auto hit=[](u32 a) { auto &x=cpu.dcache.line((u64)a|0x80000000ull); auto &y=cpu.dcache.line(((u64)a|0x80000000ull)^0x1000ull); bool hx=x.hit(a),hy=y.hit(a);return hx&&hy?-1:hx||hy?1:0; };
        auto cached=[](u32 a) { auto &x=cpu.dcache.line((u64)a|0x80000000ull); auto &y=cpu.dcache.line(((u64)a|0x80000000ull)^0x1000ull);return (std::uint8_t)(x.hit(a)?x.bytes[(a&15u)^3u]:y.bytes[(a&15u)^3u]); };
        auto ram=[](u32 a) { return (std::uint8_t)rdram.ram.read<Byte>(a,RBusDevice::ARES_DEBUGGER); };
        return sc64_read_ram(0,bytes.size(),rdram.ram.size,bytes.data(),hit,cached,ram);
      };
      std::vector<std::uint8_t> before(rdram.ram.size),after(rdram.ram.size);
      bool pre_ok=read_view(before);std::cout<<"PRE_CACHE_OK "<<pre_ok<<std::endl;if(!pre_ok){std::cerr<<"pre-transition cache conflict\n";return 8;}
      auto state=root->serialize(true);
      if(!state || state.size()>16u*1024u*1024u || !ares::Nintendo64::option("Recompiler","false"))return 8;
      nall::serializer same_state(state.data(),state.size());
      {std::lock_guard lock(p.mutex);p.gpuReady=false;}
      if(!root->unserialize(same_state) || cpu.recompiler.enabled || rsp.recompiler.enabled || ares::Nintendo64::system.homebrewMode || rdram.ram.size!=before.size())return 8;
      bool post_ok=read_view(after);std::cout<<"POST_CACHE_OK "<<post_ok<<std::endl;view_equal=sc64_hybrid_views_equal(pre_ok,before.data(),post_ok,after.data(),before.size());
      if(!view_equal){std::cerr<<"post-transition cache conflict or view mismatch\n";return 8;}
      transitioned=true;std::cout<<"HYBRID "<<completed<<" view_equal=1\n";continue;
    }
    if(capture==3) {
      using namespace ares::Nintendo64;
      if(!sc64_hybrid_snapshot_allowed(scenario=="snapshot",transitioned,view_equal,cpu.recompiler.enabled,rsp.recompiler.enabled,ares::Nintendo64::system.homebrewMode,rdram.ram.size,snapshotCount)) return 8;
      std::vector<std::uint8_t> scratch(rdram.ram.size);
      auto hit=[](u32 a) { auto &x=cpu.dcache.line((u64)a|0x80000000ull); auto &y=cpu.dcache.line(((u64)a|0x80000000ull)^0x1000ull); bool hx=x.hit(a),hy=y.hit(a);return hx&&hy?-1:hx||hy?1:0; };
      auto cached=[](u32 a) { auto &x=cpu.dcache.line((u64)a|0x80000000ull); auto &y=cpu.dcache.line(((u64)a|0x80000000ull)^0x1000ull);return (std::uint8_t)(x.hit(a)?x.bytes[(a&15u)^3u]:y.bytes[(a&15u)^3u]); };
      auto ram=[](u32 a) { return (std::uint8_t)rdram.ram.read<Byte>(a,RBusDevice::ARES_DEBUGGER); };
      if(!sc64_read_ram(0,scratch.size(),rdram.ram.size,scratch.data(),hit,cached,ram)) { std::cerr<<"snapshot cache conflict\n";return 8; }
      char name[32];std::snprintf(name,sizeof(name),"ram-%03u.bin",snapshotCount);
      int fd=::open((out/name).c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0)return 5;
      std::size_t written=0;while(written<scratch.size()) { auto n=::write(fd,scratch.data()+written,scratch.size()-written);if(n<=0){::close(fd);return 5;}written+=static_cast<std::size_t>(n); }
      if(::close(fd)!=0)return 5;
      std::ofstream index(out/"ram.csv",std::ios::app); if(!index)return 5; index<<snapshotCount<<","<<completed<<","<<scratch.size()<<"\n";index.close();if(!index)return 5;
      std::cout<<"RAM "<<snapshotCount<<" "<<completed<<" "<<scratch.size()<<std::endl; ++snapshotCount;continue;
    }
    if(!capture) { std::cout<<"OK "<<completed<<std::endl; continue; }
    std::unique_lock lock(p.mutex);
    p.cv.wait_for(lock,std::chrono::seconds(5),[&]{return p.failed || p.frames.size()>previous;});
    if(p.failed || p.frames.size()>previous+1) { std::cerr<<"snapshot failure\n"; return 6; }
    if(p.frames.size()==previous) { p.forceCapture=false; std::cout<<"NOFRAME "<<completed<<std::endl; continue; }
    auto &f=p.frames.back(); auto bytes=f.pixels.size()*sizeof(std::uint32_t);
    char name[32]; std::snprintf(name,sizeof(name),"live-%03zu.argb",p.frames.size()-1);
    std::ofstream frame(out/name,std::ios::binary); frame.write(reinterpret_cast<const char*>(f.pixels.data()),bytes); frame.close(); if(!frame) return 5;
    std::cout<<"FRAME "<<name<<' '<<completed<<' '<<f.width<<' '<<f.height<<' '<<p.polls.load()<<std::endl;
  }
  {
    std::unique_lock lock(p.mutex);
    if(!p.failed && p.callbacks==0) p.cv.wait_for(lock,std::chrono::seconds(5),[&]{return p.failed || p.callbacks>0;});
  }
  root->unload(); root.reset(); // joins the upstream screen thread before reading frames
  std::lock_guard lock(p.mutex);
  std::ofstream status(out/"status.log"), observations(out/"callbacks.csv");
  std::ofstream inputs(out/"inputs.csv"); inputs<<"observed_step,button,pressed\n";
  std::size_t input_bytes=std::strlen("observed_step,button,pressed\n");
  for(auto &e:p.inputEvents) {
    auto row=std::to_string(e.step)+","+std::to_string(e.button)+","+std::to_string(e.pressed)+"\n";
    if(input_bytes+row.size()>2u*1024u*1024u) { std::cerr<<"input CSV budget exceeded\n"; return 5; }
    inputs<<row;input_bytes+=row.size();
  }
  observations<<"callback,observed_step,width,height,nonblack\n";
  for(std::size_t i=0;i<p.observations.size();++i) { auto &o=p.observations[i]; observations<<i<<','<<o.step<<','<<o.width<<','<<o.height<<','<<o.nonblack<<'\n'; }
  for(auto& text:p.statuses) status<<text<<'\n';
  status<<"frontend="<<(scenario=="snapshot"?"hybrid_snapshot":"hybrid_authentic")<<" jit_before_cpu="<<cpu_jit<<" jit_before_rsp="<<rsp_jit<<" homebrew="<<homebrew<<" hybrid_transition="<<transitioned<<" exact_view_equal="<<view_equal<<" cpu_after="<<ares::Nintendo64::cpu.recompiler.enabled<<" rsp_after="<<ares::Nintendo64::rsp.recompiler.enabled<<"\n";
  status<<"requested_steps="<<frames<<" completed_steps="<<completed<<" polls="<<p.polls.load()<<" a_polls="<<p.aPolls.load()<<" start_polls="<<p.startPolls.load()<<" a_pressed_polls="<<p.aPressedPolls.load()<<" start_pressed_polls="<<p.startPressedPolls.load()<<" audio_calls="<<p.audioCalls.load()<<" callbacks_total="<<p.callbacks<<" frames_copied="<<p.frames.size()<<" callbacks_discarded="<<p.discarded<<" gpu_ready="<<p.gpuReady<<" failure="<<p.failure<<'\n';
  if(p.failed || !p.gpuReady || p.frames.empty() || !transitioned || !view_equal || ares::Nintendo64::cpu.recompiler.enabled || ares::Nintendo64::rsp.recompiler.enabled) { std::cerr<<"renderer or callback failure: "<<p.failure<<" callbacks="<<p.callbacks<<"\n"; return 6; }
  for(std::size_t i=0;i<p.frames.size();++i) {
    auto& f=p.frames[i]; auto bytes=f.pixels.size()*sizeof(std::uint32_t);
    meta<<i<<','<<f.step<<','<<f.width<<','<<f.height<<','<<bytes<<'\n';
  }
  meta.flush(); steps.flush(); if(!meta || !steps || !status || !observations || !inputs) return 5;
  return 0;
}
