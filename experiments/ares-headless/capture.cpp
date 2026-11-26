// SPDX-License-Identifier: GPL-3.0-or-later
// Parity-harness capture spike frontend for the pinned ares commit.
// Read-only guest observation: full-RAM hashes at fixed checkpoints, a per-tick
// tap of live unit rows and clock words, per-step watches of chosen fields,
// closed-loop waits on a guest field, an in-process state round trip used only
// to compare views, and whole-machine state files written by the core's own
// serializer and loaded only before the first step. It never writes guest
// memory directly and uses no host input API. Outputs are ROM-derived and
// belong in a private directory.
#include "capture_view.h"
#include <ares/ares.hpp>
#include <n64/n64.hpp>
#include <mia/mia.hpp>
#include <CommonCrypto/CommonDigest.h>
#include <array>
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
#include <sstream>
#include <string>
#include <time.h>
#include <utility>
#include <vector>

namespace ares::Nintendo64 {
  auto load(Node::System&, string) -> bool;
  auto option(string, string) -> bool;
}

namespace {
// Guest addresses (physical offsets into RDRAM). Candidates until S2 says otherwise.
constexpr std::uint32_t kTick = 0x000B1E84, kSeconds = 0x000B9368, kPause = 0x000F0C20,
                        kSelector = 0x000D13E4, kRunning = 0x000A5AC0, kCountdown = 0x000A5ACC,
                        kBanks = 0x000B1D44, kBanksBytes = 0xC0, kRng = 0x000EFF20, kRngBytes = 12;

struct Probe final : ares::Platform {
  std::shared_ptr<mia::Pak> systemPak, gamePak;
  std::vector<std::pair<ares::Node::Input::Button, unsigned>> buttons;
  ares::Node::Input::Button a;
  ares::Node::Input::Axis xAxis, yAxis;
  std::atomic<int> axisX{0}, axisY{0};
  std::atomic<unsigned> mask{0};
  std::atomic<unsigned long long> polls{0}, audioCalls{0}, callbacks{0};
  std::atomic<unsigned> readsThisStep{0};
  std::atomic<bool> wantFrame{false};
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<std::uint32_t> pixels;
  unsigned width = 0, height = 0;
  bool haveFrame = false, failed = false, gpuReady = false;
  std::string failure;
  std::vector<std::string> statuses;

  auto pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> override {
    if(node->name() == "Nintendo 64") return systemPak->pak;
    if(node->name() == "Nintendo 64 Cartridge") return gamePak->pak;
    return {};
  }
  auto status(string_view message) -> void override {
    std::string text{message.data(), message.size()};
    std::lock_guard lock(mutex);
    if(statuses.size() < 4096) statuses.push_back(text);
    if(text.find("Vulkan Enabled: using paraLLEl-RDP") != std::string::npos) gpuReady = true;
    if(text.find("No RDP rendering support") != std::string::npos) { failed = true; failure = text; cv.notify_all(); }
  }
  auto input(ares::Node::Input::Input node) -> void override {
    ++polls;
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      unsigned bit = 0;
      for(auto& entry : buttons) if(entry.first == button) { bit = entry.second; break; }
      if(button == a) ++readsThisStep;  // one A poll per controller read
      button->setValue((mask.load() & bit) != 0);
    } else if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      axis->setValue(axis == xAxis ? axisX.load() : axis == yAxis ? axisY.load() : 0);
    }
  }
  auto audio(ares::Node::Audio::Stream stream) -> void override {
    ++audioCalls;
    if(stream->channels() > 8) return;
    double samples[8]{};
    while(stream->pending()) stream->read(samples);
  }
  auto video(ares::Node::Video::Screen, const u32* data, u32 pitch, u32 w, u32 h) -> void override {
    ++callbacks;
    if(!wantFrame.load() || !data || w <= 1 || h <= 1 || w > 1920 || h > 1080 || pitch < w * 4u) return;
    std::lock_guard lock(mutex);
    if(!wantFrame.exchange(false)) return;
    pixels.resize(std::size_t(w) * h);
    for(unsigned y = 0; y < h; ++y)
      std::memcpy(pixels.data() + std::size_t(y) * w, reinterpret_cast<const unsigned char*>(data) + std::size_t(y) * pitch, w * 4u);
    width = w; height = h; haveFrame = true;
    cv.notify_all();
  }
};

std::string hex_bytes(const std::uint8_t* p, std::size_t n) {
  static const char digits[] = "0123456789abcdef";
  std::string s; s.reserve(n * 2);
  for(std::size_t i = 0; i < n; ++i) { s += digits[p[i] >> 4]; s += digits[p[i] & 15]; }
  return s;
}
std::string sha256(const std::uint8_t* p, std::size_t n) {
  unsigned char digest[CC_SHA256_DIGEST_LENGTH];
  CC_SHA256(p, static_cast<CC_LONG>(n), digest);
  return hex_bytes(digest, sizeof digest);
}

// Read-only views of guest memory. `view` is what the guest CPU would read
// (data cache over RDRAM); `raw` is RDRAM alone, as a state-file parse sees it.
void raw_read(std::uint32_t address, std::size_t length, std::uint8_t* out) {
  // The backing array stores each 32-bit word host-endian: guest byte a is data[a ^ 3].
  // Same bytes as rdram.ram.read<Byte>(a, ARES_DEBUGGER) without its per-byte dispatch.
  const std::uint8_t* data = ares::Nintendo64::rdram.ram.data;
  for(std::size_t i = 0; i < length; ++i) out[i] = data[(address + (std::uint32_t)i) ^ 3u];
}
bool line_read(std::uint32_t at, int alias, std::uint8_t* out16) {
  using namespace ares::Nintendo64;
  auto& line = cpu.dcache.line(((u64)at | 0x80000000ull) ^ (alias ? 0x1000ull : 0ull));
  if(!line.hit(at)) return false;
  for(unsigned i = 0; i < 16; ++i) out16[i] = line.bytes[i ^ 3u];
  return true;
}
bool guest_view(std::uint32_t address, std::size_t length, std::uint8_t* out, Sc64CaptureReadStats* stats) {
  return sc64_capture_read(address, length, ares::Nintendo64::rdram.ram.size, out, raw_read, line_read, stats);
}
std::uint32_t guest32(std::uint32_t address, Sc64CaptureReadStats* stats) {
  std::uint8_t b[4]{}; guest_view(address, 4, b, stats); return sc64_capture_be32(b);
}
}

int main(int argc, char** argv) {
  if(argc != 4) { std::cerr << "usage: capture ROM OUTPUT_DIR jit|interp\n"; return 2; }
  const std::string startMode = argv[3];
  if(startMode != "jit" && startMode != "interp") return 2;
  std::filesystem::path out = argv[2];
  if(!std::filesystem::is_directory(out) || !std::filesystem::is_directory(out / "saves") ||
     !std::filesystem::is_directory(out / "systems")) return 2;
  Probe p; ares::platform = &p;
  auto saves = std::filesystem::absolute(out / "saves").string() + "/";
  auto systems = std::filesystem::absolute(out / "systems").string() + "/";
  mia::setSaveLocation([saves] { return string{saves.c_str()}; });
  mia::setHomeLocation([systems] { return string{systems.c_str()}; });
  mia::construct();
  p.gamePak = mia::Medium::create("Nintendo 64");
  p.systemPak = mia::System::create("Nintendo 64");
  if(!p.gamePak || !p.systemPak || p.gamePak->load(argv[1]) != successful || p.systemPak->load() != successful) return 3;
  if(!ares::Nintendo64::option("Quality", "SD") ||
     !ares::Nintendo64::option("Supersampling", "false") ||
     !ares::Nintendo64::option("Enable GPU acceleration", "true") ||
     !ares::Nintendo64::option("Homebrew Mode", "false") ||
     !ares::Nintendo64::option("Recompiler", startMode == "jit" ? "true" : "false") ||
     !ares::Nintendo64::option("Expansion Pak", "true")) return 3;
  ares::Node::System root;
  if(!ares::Nintendo64::load(root, "[Nintendo] Nintendo 64 (NTSC)")) return 4;
  struct Unload { ares::Node::System& node; ~Unload() { if(node) node->unload(); ares::platform = nullptr; } } unload{root};
  auto cart = root->find<ares::Node::Port>("Cartridge Slot");
  auto port = root->find<ares::Node::Port>("Controller Port 1");
  if(!cart || !port || !cart->allocate()) return 4;
  cart->connect();
  if(!cart->connected() || !cart->connected()->pak()) return 4;
  auto pad = port->allocate("Gamepad"); if(!pad) return 4; port->connect();
  unsigned bound = 0;
  for(auto button : pad->find<ares::Node::Input::Button>()) {
    std::string name{button->name().data(), button->name().size()};
    unsigned bit = sc64_capture_pad_bit(name.c_str());
    if(bit) { p.buttons.push_back({button, bit}); bound |= bit; }
    if(name == "A") p.a = button;
  }
  for(auto axis : pad->find<ares::Node::Input::Axis>()) {
    if(axis->name() == "X-Axis") p.xAxis = axis;
    if(axis->name() == "Y-Axis") p.yAxis = axis;
  }
  if(bound != 16383u || !p.a || !p.xAxis || !p.yAxis) return 4;
  root->power();
  using namespace ares::Nintendo64;
  const bool wantJit = startMode == "jit";
  if(cpu.recompiler.enabled != wantJit || rsp.recompiler.enabled != wantJit || ares::Nintendo64::system.homebrewMode ||
     rdram.ram.size != SC64_CAPTURE_RAM) { std::cerr << "unexpected CPU or memory configuration\n"; return 3; }

  std::ofstream checkpoints(out / "checkpoints.csv"), regions(out / "regions.csv"), commands(out / "commands.csv"),
                ticks(out / "ticks.csv"), verifyLog(out / "verify.csv"), events(out / "events.csv");
  std::ofstream rawRows, recFile;
  // Required logs are checked whether or not they opened; the raw file only once requested.
  auto outputsOk = [&] {
    for(std::ofstream* stream : {&checkpoints, &regions, &commands, &ticks, &verifyLog, &events}) { stream->flush(); if(!*stream) return false; }
    if(rawRows.is_open()) { rawRows.flush(); if(!rawRows) return false; }
    if(recFile.is_open()) { recFile.flush(); if(!recFile) return false; }
    return (bool)std::cout;
  };
  checkpoints << "step,tick,sha256,cached_lines,dual_hits,dual_differ\n";
  commands << "begin,steps,mask,x,y\n";
  ticks << "step,tick,phase,same,seconds,pause,selector,running,countdown,table,live,bad_links,asymmetric_links,"
           "dual_hits,dual_differ,raw_differs,rows_sha256,globals_sha256\n";
  verifyLog << "step,tick,kind,tick_matched,skew,change_rows_equal,follow_rows_equal,view_equal,view_differ_bytes,first_differ,steps,ms\n";
  events << "step,event,detail\n";

  long long completed = 0;
  std::vector<std::uint8_t> pollLog;
  std::vector<std::uint8_t> full(SC64_CAPTURE_RAM), table(SC64_UNIT_TABLE_BYTES), rows, previousRows, rawTable(SC64_UNIT_TABLE_BYTES);
  long long every = 0;
  // Tap state: 0 off, 1 on tick change plus one follow-up step, 2 every step.
  int tapMode = 0; bool tapRaw = false, tapRec = false; unsigned long long rawEvery = 1; bool rawTick = false;
  bool haveTable = false; std::uint32_t tableOffset = 0; long long lastScan = -1000000;
  std::uint32_t lastTick = 0; bool haveTick = false; unsigned phase = 0; bool follow = false;
  unsigned long long tickChanges = 0, tapReads = 0, tapDual = 0, tapDualDiffer = 0, tapRawDiffers = 0,
                     tapBadLinks = 0, tapAsymmetric = 0, firstReadTorn = 0, firstReadFollowed = 0;
  long long verifyEvery = 0; std::string verifyKind = "branch"; bool verifyFull = false;
  unsigned long long verifyCount = 0, verifyFailures = 0, verifyViewDiffer = 0;
  std::string pendingShot;
  const auto began = std::chrono::steady_clock::now();
  // Main-thread CPU time: emulation plus tap work, insensitive to other load on the host.
  auto threadCpuUs = [] { timespec t{}; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t); return (long long)t.tv_sec * 1000000 + t.tv_nsec / 1000; };
  auto elapsedMs = [&] { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - began).count(); };

  bool observed = false;   // a checkpoint or dump was taken; `load` must come before any
  bool loaded = false;     // one `load` per process
  auto checkpoint = [&]() -> std::string {
    observed = true;
    Sc64CaptureReadStats stats;
    if(!guest_view(0, full.size(), full.data(), &stats)) return {};
    auto digest = sha256(full.data(), full.size());
    checkpoints << completed << ',' << guest32(kTick, nullptr) << ',' << digest << ',' << stats.cached_lines << ','
                << stats.dual_hits << ',' << stats.dual_differ << '\n';
    checkpoints.flush();
    regions << completed;
    for(std::size_t at = 0; at < full.size(); at += 65536) {
      char word[20]; std::snprintf(word, sizeof word, ",%016llx", (unsigned long long)sc64_capture_fnv64(full.data() + at, 65536));
      regions << word;
    }
    regions << '\n'; regions.flush();
    return digest;
  };

  // Reads the tapped regions through the cache-aware view. Returns false when no table is loaded.
  struct Sample { bool table = false; unsigned live = 0; Sc64CaptureLinkStats links; Sc64CaptureReadStats stats;
                  bool rawDiffers = false; std::uint32_t seconds = 0, pause = 0, selector = 0, running = 0, countdown = 0;
                  std::uint8_t globals[kBanksBytes + kRngBytes]{}; std::string globalsSha; };
  auto locate = [&]() {
    if(haveTable) {
      std::uint8_t header[16];
      if(tableOffset >= 16 && guest_view(tableOffset - 16, 16, header, nullptr) && sc64_capture_unit_header_ok(header)) return;
      haveTable = false; events << completed << ",table_lost," << tableOffset << '\n'; events.flush();
    }
    if(completed - lastScan < 20) return;
    lastScan = completed;
    if(!guest_view(0, full.size(), full.data(), nullptr)) return;
    std::uint32_t offset = 0;
    unsigned found = sc64_capture_find_unit_table(full.data(), full.size(), &offset);
    if(found == 1) { haveTable = true; tableOffset = offset; events << completed << ",table_found," << offset << '\n'; events.flush(); }
    else if(found > 1) { events << completed << ",table_ambiguous," << found << '\n'; events.flush(); }
  };
  auto sample = [&](std::vector<std::uint8_t>& liveRows) -> Sample {
    Sample s;
    std::uint8_t *globals = s.globals, half[2]{};
    guest_view(kPause, 2, half, &s.stats); s.pause = (std::uint32_t)half[0] << 8 | half[1];
    s.seconds = guest32(kSeconds, &s.stats); s.selector = guest32(kSelector, &s.stats);
    std::uint8_t running = 0; guest_view(kRunning, 1, &running, &s.stats); s.running = running;
    s.countdown = guest32(kCountdown, &s.stats);
    guest_view(kBanks, kBanksBytes, globals, &s.stats); guest_view(kRng, kRngBytes, globals + kBanksBytes, &s.stats);
    s.globalsSha = sha256(globals, kBanksBytes + kRngBytes);
    liveRows.clear();
    locate();
    if(!haveTable) return s;
    s.table = guest_view(tableOffset, table.size(), table.data(), &s.stats);
    raw_read(tableOffset, rawTable.size(), rawTable.data());
    s.rawDiffers = std::memcmp(table.data(), rawTable.data(), table.size()) != 0;
    s.links = sc64_capture_unit_links(table.data(), 0x80000000u | tableOffset);
    s.live = sc64_capture_live_rows(table.data(), liveRows);
    return s;
  };

  auto rendererFailed = [&] { std::lock_guard lock(p.mutex); return p.failed; };
  auto setRecompiler = [&](bool on) { return ares::Nintendo64::option("Recompiler", on ? "true" : "false"); };
  auto restore = [&](serializer& state, bool jit) -> bool {
    if(!setRecompiler(jit)) return false;
    serializer same(state.data(), state.size());
    {std::lock_guard lock(p.mutex); p.gpuReady = false;}
    return root->unserialize(same) && cpu.recompiler.enabled == jit && rsp.recompiler.enabled == jit;
  };
  // "same": serialize and restore in the same mode at a tick change and compare the
  // records and view (state round trip only). "branch": at a tick change save the
  // state, keep running the main mode to the next tick change plus one step, then
  // replay the same pad inputs from the saved state in the other CPU mode and compare
  // the records at the first step showing the next tick and the step after (and the
  // full view, which is only like-for-like when the skew is zero); then
  // restore the main mode's state and continue. The main trajectory is unchanged.
  struct Branch { bool active = false; serializer start; std::uint32_t tick = 0; std::vector<std::array<int, 3>> pads;
                  std::size_t changeAt = 0; std::uint32_t changeTick = 0; std::vector<std::uint8_t> changeRows; };
  Branch branch;
  auto roundTrip = [&](std::uint32_t tick, const std::vector<std::uint8_t>& liveRows) -> bool {
    auto startedMs = elapsedMs();
    const bool jit = cpu.recompiler.enabled;
    std::vector<std::uint8_t> before, after, afterRows;
    if(verifyFull) { before.resize(SC64_CAPTURE_RAM); guest_view(0, before.size(), before.data(), nullptr); }
    auto state = root->serialize(true);
    if(!state || !restore(state, jit)) return false;
    sample(afterRows);
    int rowsEqual = afterRows == liveRows, viewEqual = -1;
    if(verifyFull) { after.resize(SC64_CAPTURE_RAM); guest_view(0, after.size(), after.data(), nullptr); viewEqual = after == before; }
    ++verifyCount; if(!rowsEqual || viewEqual == 0) ++verifyFailures;
    verifyLog << completed << ',' << tick << ",same,1,0," << rowsEqual << ',' << rowsEqual << ',' << viewEqual << ",0,-1,0,"
              << (elapsedMs() - startedMs) << '\n';
    verifyLog.flush();
    return true;
  };
  auto finishBranch = [&](const std::vector<std::uint8_t>& followRows) -> bool {
    auto startedMs = elapsedMs();
    const bool jit = cpu.recompiler.enabled;
    std::vector<std::uint8_t> mainView, otherView, otherChange, otherFollow;
    if(verifyFull) { mainView.resize(SC64_CAPTURE_RAM); guest_view(0, mainView.size(), mainView.data(), nullptr); }
    auto resume = root->serialize(true);
    if(!resume || !restore(branch.start, !jit)) return false;
    // Records are matched by tick value: the other mode may cross the tick boundary
    // on a different VI step, so it may run up to 8 steps past the main mode's inputs
    // (holding the last pad) and the step offset is recorded as skew.
    std::size_t otherAt = 0, ran = 0; std::uint32_t otherTick = 0; bool haveFollow = false;
    while(ran < branch.pads.size() + 8 && !haveFollow) {
      auto& pad = ran < branch.pads.size() ? branch.pads[ran] : branch.pads.back();
      p.mask = (unsigned)pad[0]; p.axisX = pad[1]; p.axisY = pad[2];
      root->run(); ++ran;
      if(rendererFailed()) return false;   // runStep reports it as exit 6
      std::uint32_t tick = guest32(kTick, nullptr);
      if(!otherAt) { if(tick != branch.tick) { otherAt = ran; otherTick = tick; sample(otherChange); } }
      else if(ran == otherAt + 1) { sample(otherFollow); haveFollow = true; }
    }
    long long skew = otherAt ? (long long)otherAt - (long long)branch.changeAt : 999;
    bool changeStepEqual = otherAt && otherTick == branch.changeTick && haveFollow;
    int viewEqual = -1; std::size_t differ = 0; long long firstDiffer = -1;
    if(verifyFull) {
      otherView.resize(SC64_CAPTURE_RAM); guest_view(0, otherView.size(), otherView.data(), nullptr);
      for(std::size_t a = 0; a < otherView.size(); ++a) if(otherView[a] != mainView[a]) { if(firstDiffer < 0) firstDiffer = (long long)a; ++differ; }
      viewEqual = differ == 0;
    }
    // Empty records (no table on either side) are never evidence of a match.
    int changeEqual = !branch.changeRows.empty() && otherChange == branch.changeRows;
    int followEqual = !followRows.empty() && otherFollow == followRows;
    if(!restore(resume, jit)) return false;
    ++verifyCount; if(!changeStepEqual || !changeEqual || !followEqual) ++verifyFailures;
    if(viewEqual == 0) ++verifyViewDiffer;
    verifyLog << completed << ',' << branch.tick << ",branch," << changeStepEqual << ',' << skew << ',' << changeEqual << ',' << followEqual << ','
              << viewEqual << ',' << differ << ',' << firstDiffer << ',' << ran << ',' << (elapsedMs() - startedMs) << '\n';
    verifyLog.flush();
    branch.active = false; branch.pads.clear(); branch.changeAt = 0;
    return true;
  };

  auto tap = [&]() -> bool {
    Sc64CaptureReadStats tickStats;
    std::uint32_t tick = guest32(kTick, &tickStats);
    bool changed = !haveTick || tick != lastTick;
    if(!changed && tapMode != 2 && !follow) return true;
    Sample s = sample(rows);
    s.stats.dual_hits += tickStats.dual_hits; s.stats.dual_differ += tickStats.dual_differ;
    bool same = !changed && rows == previousRows;
    if(changed) { phase = 0; ++tickChanges; follow = true; rawTick = tickChanges % rawEvery == 0; }
    else {
      ++phase;
      if(phase == 1) { ++firstReadFollowed; if(!same) ++firstReadTorn; }
      if(tapMode != 2) follow = false;
    }
    ++tapReads; tapDual += s.stats.dual_hits; tapDualDiffer += s.stats.dual_differ; tapRawDiffers += s.rawDiffers;
    tapBadLinks += s.links.bad != 0; tapAsymmetric += s.links.asymmetric != 0;
    auto rowsSha = sha256(rows.data(), rows.size());
    ticks << completed << ',' << tick << ',' << phase << ',' << same << ',' << s.seconds << ',' << s.pause << ',' << s.selector << ','
          << s.running << ',' << s.countdown << ',' << (haveTable ? tableOffset : 0) << ',' << s.live << ',' << s.links.bad << ','
          << s.links.asymmetric << ',' << s.stats.dual_hits << ',' << s.stats.dual_differ << ',' << s.rawDiffers << ','
          << rowsSha << ',' << s.globalsSha << '\n';
    if(tapRec) {
      // tap.bin record (format in capture_view.h): clocks and the bank and RNG
      // globals on every tapped read; live rows only on the follow-up read, the
      // one a tick's record is taken from.
      const bool withRows = !changed && phase == 1;
      std::uint32_t flags = (withRows ? 1u : 0u) | (haveTable ? 2u : 0u) | (s.stats.dual_differ ? 4u : 0u) |
                            ((s.links.bad || s.links.asymmetric) ? 8u : 0u);
      std::uint32_t head[SC64_TAP_HEADER_WORDS] = {(std::uint32_t)completed, tick, phase, s.live, s.seconds, s.pause,
                                                   s.selector, s.running, s.countdown, haveTable ? tableOffset : 0u, flags,
                                                   withRows ? s.live : 0u};
      std::uint8_t bytes[sizeof head];
      for(unsigned w = 0; w < SC64_TAP_HEADER_WORDS; ++w) sc64_capture_put_le32(bytes + w * 4, head[w]);
      recFile.write(reinterpret_cast<const char*>(bytes), sizeof bytes);
      recFile.write(reinterpret_cast<const char*>(s.globals), kBanksBytes + kRngBytes);
      if(withRows) recFile.write(reinterpret_cast<const char*>(rows.data()), (std::streamsize)rows.size());
    }
    if(tapRaw && rawTick && (changed || !same)) {
      std::uint32_t head[4] = {(std::uint32_t)completed, tick, phase, s.live};
      rawRows.write(reinterpret_cast<const char*>(head), sizeof head);
      rawRows.write(reinterpret_cast<const char*>(rows.data()), (std::streamsize)rows.size());
    }
    bool ok = true;
    if(branch.active && changed) { if(!branch.changeAt) { branch.changeAt = branch.pads.size(); branch.changeTick = tick; branch.changeRows = rows; } }
    else if(branch.active && phase == 1 && branch.changeAt) ok = finishBranch(rows);
    else if(changed && verifyEvery > 0 && tickChanges % (unsigned long long)verifyEvery == 0) {
      if(verifyKind == "same") ok = roundTrip(tick, rows);
      else { branch.start = root->serialize(true); branch.tick = tick; branch.pads.clear(); branch.changeAt = 0; branch.active = (bool)branch.start; ok = branch.active; }
    }
    previousRows.swap(rows); lastTick = tick; haveTick = true;
    return ok;
  };

  // Per-step watches: a row in watch.csv whenever a watched field changes.
  struct Watch { std::uint32_t address = 0; unsigned bytes = 0; std::uint32_t last = 0; bool have = false; };
  std::vector<Watch> watches; std::ofstream watchLog;
  auto field = [&](std::uint32_t address, unsigned bytes, std::uint32_t* value) -> bool {
    std::uint8_t b[4]{};
    if(!guest_view(address, bytes, b, nullptr)) return false;
    *value = sc64_capture_be(b, bytes); return true;
  };
  auto watchStep = [&]() -> bool {
    if(watches.empty()) return true;
    std::uint32_t tick = guest32(kTick, nullptr);
    for(auto& w : watches) {
      std::uint32_t value = 0;
      if(!field(w.address, w.bytes, &value)) return false;
      if(w.have && value == w.last) continue;
      char row[64]; std::snprintf(row, sizeof row, ",%u,%08x,%u,%x\n", tick, 0x80000000u | w.address, w.bytes, value);
      watchLog << completed << row; w.last = value; w.have = true;
    }
    return true;
  };
  auto parseAddress = [](const std::string& text, std::uint32_t* address) {
    std::uint32_t value = 0;
    return sc64_capture_hex32(text.c_str(), &value) && sc64_capture_guest_offset(value, address);
  };
  // One frontend step with the pad held: the shared body of "STEPS MASK X Y" and "until".
  // Returns 0 or the process exit code for a failure.
  auto runStep = [&](unsigned mask, int x, int y, bool lastOfCommand) -> int {
    if(completed >= 0xFFFFFFFFll) { std::cerr << "step count would pass the u32 record field\n"; return 7; }
    p.mask = mask; p.axisX = x; p.axisY = y; p.readsThisStep = 0;
    if(branch.active) branch.pads.push_back({(int)mask, x, y});
    if(!pendingShot.empty() && lastOfCommand) { std::lock_guard lock(p.mutex); p.haveFrame = false; p.wantFrame = true; }
    root->run(); ++completed;
    unsigned reads = p.readsThisStep.load(); pollLog.push_back((std::uint8_t)(reads > 255 ? 255 : reads));
    if(rendererFailed()) { std::cerr << "renderer failure: " << p.failure << "\n"; return 6; }
    if(tapMode && !tap()) {
      if(rendererFailed()) { std::cerr << "renderer failure: " << p.failure << "\n"; return 6; }
      std::cerr << "verify failure\n"; return 8;
    }
    if(!watchStep()) return 8;
    if(every && completed % every == 0 && checkpoint().empty()) return 8;
    return 0;
  };
  std::filesystem::path flashFile = out / "saves" / "Nintendo 64" / "game.flash";

  std::cout << "READY" << std::endl;
  std::string line;
  while(std::getline(std::cin, line)) {
    if(line.empty() || line[0] == '#') continue;
    // An embedded NUL would let c_str()-based checks see only a prefix of a token.
    if(line.find('\0') != std::string::npos) { std::cerr << "invalid command: embedded NUL\n"; return 7; }
    std::istringstream in(line); std::string word; in >> word;
    if(word == "quit") break;
    if(word == "hash") { auto digest = checkpoint(); if(digest.empty()) return 8; if(!outputsOk()) return 5; std::cout << "HASH " << completed << ' ' << digest << std::endl; continue; }
    if(word == "every") { if(!(in >> every) || every < 0) return 7; std::cout << "OK " << completed << std::endl; continue; }
    if(word == "tap") {
      std::string mode; in >> mode;
      if(mode == "off") tapMode = 0; else if(mode == "tick") tapMode = 1; else if(mode == "step") tapMode = 2; else return 7;
      // Options in any order: "raw" (optionally followed by a stride K: keep every Kth tick's
      // rows) and "rec". A number is accepted only right after "raw".
      // The whole line is checked before any file is opened.
      std::string extra; bool wantRaw = false, wantRec = false; unsigned long long stride = 1;
      std::vector<std::string> options;
      while(in >> extra) options.push_back(extra);
      for(std::size_t i = 0; i < options.size(); ++i) {
        if(options[i] == "raw" && !wantRaw) {
          wantRaw = true;
          if(i + 1 < options.size() && sc64_capture_decimal_ok(options[i + 1].c_str())) {
            stride = std::strtoull(options[++i].c_str(), nullptr, 10);
            if(stride < 1 || stride > 1000000) return 7;
          }
        } else if(options[i] == "rec" && !wantRec) wantRec = true;
        else return 7;
      }
      tapRaw = false; tapRec = false; rawEvery = stride;
      if(wantRaw) {
        if(!rawRows.is_open()) rawRows.open(out / "ticks.raw", std::ios::binary);
        if(!rawRows) { std::cerr << "output open failure: ticks.raw\n"; return 5; }
        tapRaw = true;
      }
      if(wantRec) {
        if(!recFile.is_open()) {
          recFile.open(out / "tap.bin", std::ios::binary);
          if(!recFile) { std::cerr << "output open failure: tap.bin\n"; return 5; }
          std::uint8_t header[SC64_TAP_FILE_HEADER_BYTES];
          sc64_capture_tap_file_header(header, kBanksBytes + kRngBytes);
          recFile.write(reinterpret_cast<const char*>(header), sizeof header);
        }
        tapRec = true;
      }
      haveTick = false; follow = false; ticks.flush();
      branch.active = false; branch.pads.clear(); branch.changeAt = 0;  // a pending branch never spans a tap change
      std::cout << "OK " << completed << std::endl; continue;
    }
    if(word == "verify") {
      std::string full_flag;
      if(!(in >> verifyEvery >> verifyKind) || verifyEvery < 0 || (verifyKind != "branch" && verifyKind != "same")) return 7;
      verifyFull = static_cast<bool>(in >> full_flag) && full_flag == "full";
      branch.active = false; branch.pads.clear(); branch.changeAt = 0;
      std::cout << "OK " << completed << std::endl; continue;
    }
    if(word == "mode") {
      std::string mode; in >> mode; if(mode != "jit" && mode != "interp") return 7;
      auto state = root->serialize(true);
      if(!state || !restore(state, mode == "jit")) return 8;
      events << completed << ",mode," << mode << '\n'; events.flush();
      if(!outputsOk()) return 5; std::cout << "MODE " << completed << ' ' << mode << ' ' << elapsedMs() << std::endl; continue;
    }
    if(word == "rawcheck") {
      // The direct copy must equal the core's own debugger read for every byte.
      std::vector<std::uint8_t> direct(SC64_CAPTURE_RAM);
      raw_read(0, direct.size(), direct.data());
      std::size_t differ = 0;
      for(std::uint32_t a = 0; a < SC64_CAPTURE_RAM; ++a)
        differ += direct[a] != (std::uint8_t)rdram.ram.read<Byte>(a, RBusDevice::ARES_DEBUGGER);
      std::cout << "RAWCHECK " << completed << ' ' << differ << std::endl;
      if(differ) return 8;
      continue;
    }
    if(word == "peek") {
      std::string addressText; std::size_t length = 0; in >> addressText >> length;
      std::uint32_t address = 0;
      if(!parseAddress(addressText, &address) || !length || length > 4096) return 7;
      std::vector<std::uint8_t> bytes(length); Sc64CaptureReadStats stats;
      if(!guest_view(address, length, bytes.data(), &stats)) return 7;
      std::cout << "PEEK " << completed << ' ' << addressText << ' ' << hex_bytes(bytes.data(), bytes.size()) << ' ' << stats.dual_hits << std::endl; continue;
    }
    if(word == "dump") {
      observed = true;
      std::string name; in >> name;
      if(name.empty() || name.find('/') != std::string::npos || name.find('\0') != std::string::npos ||
         std::filesystem::exists(out / name)) return 7;
      if(!guest_view(0, full.size(), full.data(), nullptr)) return 8;
      std::ofstream file(out / name, std::ios::binary); file.write(reinterpret_cast<const char*>(full.data()), (std::streamsize)full.size());
      file.close(); if(!file) return 5;
      std::cout << "DUMP " << completed << ' ' << name << ' ' << sha256(full.data(), full.size()) << std::endl; continue;
    }
    if(word == "watch") {
      // "watch ADDR BYTES" adds a field (1, 2 or 4 bytes); "watch off" clears them all.
      std::string addressText; unsigned bytes = 0; in >> addressText;
      if(addressText == "off") { watches.clear(); std::cout << "OK " << completed << std::endl; continue; }
      std::uint32_t address = 0;
      if(!parseAddress(addressText, &address) || !(in >> bytes) || (bytes != 1 && bytes != 2 && bytes != 4) ||
         address % bytes || address + bytes > rdram.ram.size || watches.size() >= 32) return 7;
      if(!watchLog.is_open()) {
        if(std::filesystem::exists(out / "watch.csv")) return 7;   // never overwrite, e.g. a dump of that name
        watchLog.open(out / "watch.csv"); watchLog << "step,tick,address,bytes,value\n";
      }
      if(!watchLog) { std::cerr << "output open failure: watch.csv\n"; return 5; }
      Watch w; w.address = address; w.bytes = bytes; watches.push_back(w);
      std::cout << "OK " << completed << std::endl; continue;
    }
    if(word == "until") {
      // "until ADDR BYTES OP VALUE MAX MASK X Y [HOLD GAP]": hold the pad until the field meets
      // the condition after a step, for at most MAX steps. With HOLD and GAP the buttons are
      // pulsed instead (pressed HOLD steps, released GAP steps, repeated; axes stay held), for
      // prompts that ignore presses for a while. A miss ends the run (exit 9).
      std::string addressText, opText, valueText; unsigned bytes = 0; long long limit = 0, hold = 0, gap = 0; long mask = 0, x = 0, y = 0;
      std::uint32_t address = 0, target = 0, start = 0;
      if(!(in >> addressText >> bytes >> opText >> valueText >> limit >> mask >> x >> y) || !parseAddress(addressText, &address) ||
         (bytes != 1 && bytes != 2 && bytes != 4) || address % bytes || address + bytes > rdram.ram.size ||
         !sc64_capture_hex32(valueText.c_str(), &target) ||
         limit < 1 || limit > 10000000 || !sc64_capture_mask_allowed(mask) || x < -32767 || x > 32767 || y < -32767 || y > 32767 ||
         !pendingShot.empty()) { std::cerr << "invalid command: " << line << "\n"; return 7; }
      {
        std::vector<std::string> rest; std::string token;
        while(in >> token) rest.push_back(token);
        if(!rest.empty()) {
          if(rest.size() != 2 || !sc64_capture_decimal_ok(rest[0].c_str()) || !sc64_capture_decimal_ok(rest[1].c_str())) {
            std::cerr << "invalid command: " << line << "\n"; return 7;
          }
          hold = std::atoll(rest[0].c_str()); gap = std::atoll(rest[1].c_str());
          if(hold < 1 || hold > 1000 || gap < 1 || gap > 1000) { std::cerr << "invalid command: " << line << "\n"; return 7; }
        }
      }
      Sc64UntilOp op = sc64_capture_until_op(opText.c_str());
      if(op == Sc64UntilOp::invalid || !field(address, bytes, &start)) return 7;
      const long long begin = completed; const auto cpuStarted = threadCpuUs();
      std::uint32_t value = start; bool met = false;
      for(long long i = 0; i < limit && !met; ++i) {
        unsigned now = (unsigned)mask;
        if(hold && i % (hold + gap) >= hold) now = 0;
        if(int code = runStep(now, (int)x, (int)y, false)) return code;
        if(!field(address, bytes, &value)) return 8;
        met = sc64_capture_until_met(op, value, target, start);
      }
      // A pulsed wait is logged as one row with its button mask; events.csv names the pulse.
      commands << begin << ',' << (completed - begin) << ',' << mask << ',' << x << ',' << y << '\n'; commands.flush();
      if(hold) events << completed << ",pulse," << hold << ' ' << gap << '\n';
      events << completed << ",until," << (met ? "met" : "miss") << ' ' << addressText << ' ' << opText << ' ' << valueText << ' ' << std::hex << value << std::dec << '\n';
      if(!outputsOk() || (watchLog.is_open() && !watchLog.flush())) { std::cerr << "output write failure\n"; return 5; }
      std::cout << (met ? "UNTIL " : "UNTILMISS ") << completed << ' ' << (completed - begin) << ' ' << std::hex << value << std::dec
                << ' ' << (threadCpuUs() - cpuStarted) / 1000 << std::endl;
      if(!met) return 9;
      continue;
    }
    if(word == "save") {
      // Whole-machine state from the core's serializer, the same bytes desktop ares writes to a slot file.
      // Names end in ".state", so a state can never take the name of a log opened later.
      std::string name; in >> name;
      // The whole token is checked: c_str() would stop at an embedded NUL.
      if(!sc64_capture_file_name_ok(name.c_str()) || std::strlen(name.c_str()) != name.size() || name.size() <= 6 || name.compare(name.size() - 6, 6, ".state") ||
         std::filesystem::exists(out / name)) return 7;
      const auto cpuStarted = threadCpuUs();
      auto state = root->serialize(true);
      if(!state) return 8;
      std::ofstream file(out / name, std::ios::binary);
      file.write(reinterpret_cast<const char*>(state.data()), (std::streamsize)state.size());
      file.close(); if(!file) { std::cerr << "state write failure\n"; return 5; }
      auto digest = sha256(state.data(), state.size());
      events << completed << ",save," << name << ' ' << state.size() << ' ' << digest << '\n';
      if(!outputsOk()) return 5;
      std::cout << "SAVE " << completed << ' ' << name << ' ' << state.size() << ' ' << digest << ' ' << (threadCpuUs() - cpuStarted) / 1000 << std::endl;
      continue;
    }
    if(word == "load") {
      // "load NAME [STEP]": restore a state staged in input/ before the first step of a fresh
      // process (an in-process restore perturbs the trajectory; see the S1 audit). STEP renumbers
      // the run so checkpoints line up with the run that wrote the state.
      std::string name, stepText, extra; long long step = 0; in >> name;
      if(in >> stepText) {
        // Steps are recorded as u32 in ticks.raw and tap.bin: 1 to 10 digits, at most 2^32 - 1.
        if(stepText.empty() || stepText.size() > 10 || stepText.find_first_not_of("0123456789") != std::string::npos ||
           (in >> extra)) return 7;
        step = std::atoll(stepText.c_str());
        if(step > 0xFFFFFFFFll) return 7;
      }
      // Also refused after a checkpoint or dump: a hash taken before the load would describe a
      // machine the rest of the run never continues.
      if(completed != 0 || observed || loaded || tapMode || every || !watches.empty() || verifyEvery || !sc64_capture_file_name_ok(name.c_str()) ||
         std::strlen(name.c_str()) != name.size()) return 7;
      // A load-mode serializer zero-fills reads past its end, so a truncated or padded file must
      // be refused here: its size must equal this core's synchronized state size. At most one
      // byte more than that is read, so an oversized file cannot exhaust memory.
      auto reference = root->serialize(true);
      const std::size_t expected = reference.size();
      std::ifstream file(out / "input" / name, std::ios::binary);
      std::vector<std::uint8_t> bytes(expected + 1);
      file.read(reinterpret_cast<char*>(bytes.data()), (std::streamsize)bytes.size());
      bytes.resize((std::size_t)file.gcount());
      if(bytes.size() != expected) {
        std::cerr << "state size is not " << expected << ": " << name << "\n"; return 7;
      }
      // Header: u32 signature, synchronize flag, char version[16]. The core reads the version
      // as a NUL-terminated string, so it must match this core's own header byte for byte.
      constexpr std::size_t headerBytes = 4 + 1 + 16;
      if(expected < headerBytes || std::memcmp(bytes.data(), reference.data(), headerBytes) != 0) {
        std::cerr << "state header does not match this core: " << name << "\n"; return 7;
      }
      // A load-mode serializer reports size() 0, so unserialize the file buffer directly
      // rather than through restore(), which copies an in-process (save-mode) state.
      serializer state(bytes.data(), (u32)bytes.size());
      const bool jitNow = cpu.recompiler.enabled;   // keep a mode set by an earlier `mode` line
      if(!setRecompiler(jitNow)) return 8;
      {std::lock_guard lock(p.mutex); p.gpuReady = false;}
      if(!root->unserialize(state) || state.size() != bytes.size() ||
         cpu.recompiler.enabled != jitNow || rsp.recompiler.enabled != jitNow) {
        std::cerr << "state load failure: " << name << "\n"; return 8;
      }
      completed = step; loaded = true;
      auto digest = sha256(bytes.data(), bytes.size());
      events << completed << ",load," << name << ' ' << bytes.size() << ' ' << digest << '\n';
      if(!outputsOk()) return 5;
      std::cout << "LOAD " << completed << ' ' << name << ' ' << digest << std::endl;
      continue;
    }
    if(word == "flush") {
      // Write the cartridge save (FlashRAM) through the core, as desktop ares does on unload.
      // The pinned mia save reports success even when it cannot write, so the file is read
      // back and must equal the cartridge's FlashRAM (host-endian words, written big-endian).
      root->save();
      if(!p.gamePak->save(p.gamePak->location)) { std::cerr << "flash save failure\n"; return 5; }
      const auto& flash = ares::Nintendo64::cartridge.flash;
      std::ifstream saved(flashFile, std::ios::binary);
      std::vector<std::uint8_t> onDisk((std::istreambuf_iterator<char>(saved)), std::istreambuf_iterator<char>());
      bool same = flash.data && flash.size && onDisk.size() == flash.size;
      for(std::size_t i = 0; same && i < onDisk.size(); ++i) same = onDisk[i] == flash.data[i ^ 3];
      if(!same) { std::cerr << "flash file does not match the cartridge FlashRAM\n"; return 5; }
      auto digest = sha256(onDisk.data(), onDisk.size());
      events << completed << ",flush," << digest << '\n';
      if(!outputsOk()) return 5;
      std::cout << "FLUSH " << completed << ' ' << digest << std::endl;
      continue;
    }
    if(word == "shot") { in >> pendingShot; if(pendingShot.empty() || pendingShot.find('/') != std::string::npos || pendingShot.find('\0') != std::string::npos || std::filesystem::exists(out / (pendingShot + ".argb"))) return 7; std::cout << "OK " << completed << std::endl; continue; }
    // Default: "steps mask x y"; a fifth column from older tapes is accepted and ignored.
    long long count = 0; long mask = 0, x = 0, y = 0;
    { std::istringstream run(line); if(!(run >> count >> mask >> x >> y) || count < 1 || count > 10000000 || !sc64_capture_mask_allowed(mask) || x < -32767 || x > 32767 || y < -32767 || y > 32767) { std::cerr << "invalid command: " << line << "\n"; return 7; } }
    if(completed + count > 0xFFFFFFFFll) { std::cerr << "step count would pass the u32 record field\n"; return 7; }
    commands << completed << ',' << count << ',' << mask << ',' << x << ',' << y << '\n'; commands.flush();
    const auto runStarted = elapsedMs(); const auto cpuStarted = threadCpuUs();
    for(long long i = 0; i < count; ++i)
      if(int code = runStep((unsigned)mask, (int)x, (int)y, i == count - 1)) return code;
    if(!pendingShot.empty()) {
      std::unique_lock lock(p.mutex);
      p.cv.wait_for(lock, std::chrono::seconds(5), [&] { return p.failed || p.haveFrame; });
      if(p.haveFrame) {
        std::ofstream frame(out / (pendingShot + ".argb"), std::ios::binary);
        frame.write(reinterpret_cast<const char*>(p.pixels.data()), (std::streamsize)(p.pixels.size() * 4));
        frame.close(); if(!frame) { std::cerr << "frame write failure\n"; return 5; }
        std::cout << "FRAME " << completed << ' ' << pendingShot << ' ' << p.width << ' ' << p.height << std::endl;
      } else { p.wantFrame = false; std::cout << "NOFRAME " << completed << std::endl; }
      pendingShot.clear();
    }
    // A failed write (for example a full disk) must end the run, never silently drop records.
    if(!outputsOk() || (watchLog.is_open() && !watchLog.flush())) { std::cerr << "output write failure\n"; return 5; }
    std::cout << "OK " << completed << ' ' << (elapsedMs() - runStarted) << ' ' << (threadCpuUs() - cpuStarted) / 1000 << std::endl;
    {std::lock_guard lock(p.mutex); if(p.failed) { std::cerr << "renderer failure: " << p.failure << "\n"; return 6; }}
  }
  const auto totalMs = elapsedMs();
  const bool cpuJit = cpu.recompiler.enabled, rspJit = rsp.recompiler.enabled;
  root->unload(); root.reset();
  std::lock_guard lock(p.mutex);
  std::ofstream status(out / "status.log"), pollFile(out / "polls.bin", std::ios::binary);
  pollFile.write(reinterpret_cast<const char*>(pollLog.data()), (std::streamsize)pollLog.size());
  for(auto& text : p.statuses) status << text << '\n';
  status << "frontend=capture start_mode=" << startMode << " cpu_jit_end=" << cpuJit << " rsp_jit_end=" << rspJit
         << " completed_steps=" << completed << " wall_ms=" << totalMs << " polls=" << p.polls.load()
         << " audio_calls=" << p.audioCalls.load() << " callbacks=" << p.callbacks.load() << " gpu_ready=" << p.gpuReady << '\n';
  status << "tap_reads=" << tapReads << " tick_changes=" << tickChanges << " first_read_followed=" << firstReadFollowed
         << " first_read_torn=" << firstReadTorn << " reads_bad_links=" << tapBadLinks << " reads_asymmetric_links=" << tapAsymmetric
         << " dual_hits=" << tapDual << " dual_differ=" << tapDualDiffer << " reads_raw_differs=" << tapRawDiffers
         << " verify_count=" << verifyCount << " verify_failures=" << verifyFailures << " verify_view_differ=" << verifyViewDiffer << '\n';
  status.close(); pollFile.close();
  if(!outputsOk() || !status || !pollFile) { std::cerr << "output write failure\n"; return 5; }
  for(std::ofstream* stream : {&checkpoints, &regions, &commands, &ticks, &verifyLog, &events}) { stream->close(); if(!*stream) return 5; }
  if(rawRows.is_open()) { rawRows.close(); if(!rawRows) return 5; }
  if(recFile.is_open()) { recFile.close(); if(!recFile) return 5; }
  if(watchLog.is_open()) { watchLog.close(); if(!watchLog) return 5; }
  return p.failed ? 6 : 0;
}
