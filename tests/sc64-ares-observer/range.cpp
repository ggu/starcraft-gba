/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../../experiments/ares-headless/observer_read.h"
#include <cassert>
#include <cstdint>
#include <limits>
static_assert(sc64_observer_mask_allowed(256) && sc64_observer_mask_allowed(512));
static_assert(sc64_observer_mask_allowed(1023) && !sc64_observer_mask_allowed(1024) && !sc64_observer_mask_allowed(-1));
int main() {
    std::uint8_t out[8]={}; unsigned hits=0,cache=0,ram=0;
    auto h=[&](std::uint32_t a){++hits;return a%2==0;};
    auto c=[&](std::uint32_t a){++cache;return std::uint8_t(a+10);};
    auto r=[&](std::uint32_t a){++ram;return std::uint8_t(a+20);};
    assert(sc64_read_ram(2,4,8,out,h,c,r));
    assert(out[0]==12&&out[1]==23&&out[2]==14&&out[3]==25&&hits==4&&cache==2&&ram==2);
    hits=cache=ram=0;
    assert(!sc64_read_ram(7,2,8,out,h,c,r));
    assert(!sc64_read_ram(8,1,8,out,h,c,r));
    assert(!sc64_read_ram(UINT32_MAX,8,8,out,h,c,r));
    assert(!sc64_read_ram(0,1,8,nullptr,h,c,r));
    assert(!sc64_read_ram(0,1,0,out,h,c,r));
    assert(!sc64_read_ram(0,1,8*1024*1024+1,out,h,c,r));
    assert(hits==0&&cache==0&&ram==0);
    auto conflict=[](std::uint32_t){return -1;};
    assert(!sc64_read_ram(0,1,8,out,conflict,c,r));
    assert(cache==0&&ram==0);
}
