/* SPDX-License-Identifier: GPL-3.0-or-later
 * Private, bounded resumed-input rendering diagnostic. No game-state writes. */
#if defined(GBA_HOST) || Z7_ARM_MODE != 2
#error "Z7 busy diagnostic requires an ARM resume replay"
#endif
#define Z7_BUSY_CALLS 1000u
static u32 busy_tick, busy_calls, busy_advancing, busy_text_before, busy_text_after;
static u32 busy_screen_before[4], busy_screen_after[4], busy_mode_before[4], busy_mode_after[4];
static u32 busy_counts[3], busy_peaks[3], busy_top[50];
static uint64_t busy_cycles[3];
static u32 busy_min[4] = {~0u, ~0u, ~0u, ~0u};
static u32 busy_first[10], busy_last[10], busy_samples[10][3];
static int busy_pre_frame, busy_pre_screen, busy_pre_mode, busy_pre_text;

static void busy_before(void)
{
    busy_pre_frame=frame; busy_pre_screen=screen; busy_pre_mode=mode;
    busy_pre_text=(int)sc64_text_box_count();
}
static u32 busy_hash(u32 h,const volatile u8 *p,unsigned n)
{
    while(n--) h=(h^*p++)*16777619u;
    return h;
}
static void busy_after(u32 cycles)
{
    u32 values[10]={0};
    unsigned cat=(busy_pre_screen==SCREEN_PLAY && !busy_pre_text) ?
        ((frame>busy_pre_frame) ? 0u : 2u) : 1u;
    ++busy_calls; if(frame>busy_pre_frame)++busy_advancing;
    if(busy_pre_text)++busy_text_before;
    if(sc64_text_box_count())++busy_text_after;
    if((unsigned)busy_pre_screen<4)++busy_screen_before[busy_pre_screen];
    if((unsigned)screen<4)++busy_screen_after[screen];
    if((unsigned)busy_pre_mode<4)++busy_mode_before[busy_pre_mode];
    if((unsigned)mode<4)++busy_mode_after[mode];
    ++busy_counts[cat];busy_cycles[cat]+=cycles;
    if(cycles>busy_peaks[cat])busy_peaks[cat]=cycles;
    unsigned j=50;while(j && cycles>busy_top[j-1]){if(j<50)busy_top[j]=busy_top[j-1];--j;}
    if(j<50)busy_top[j]=cycles;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive) ++values[u[i].owner==SIDE?0:1];
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive) ++values[b[i].owner==SIDE?2:3];
    values[4]=(u32)minerals;values[5]=(u32)gas;values[6]=(u32)frame;
    values[7]=(u32)screen;values[8]=(u32)mode;values[9]=(u32)sc64_text_box_count();
    if(busy_calls==1)for(unsigned i=0;i<10;++i)busy_first[i]=values[i];
    for(unsigned i=0;i<10;++i)busy_last[i]=values[i];
    for(unsigned i=0;i<4;++i)if(values[i]<busy_min[i])busy_min[i]=values[i];
    if(busy_calls%100u==0) {
        unsigned n=busy_calls/100u-1u;
        u32 h=busy_hash(2166136261u,(const volatile u8 *)u,sizeof(u));
        h=busy_hash(h,(const volatile u8 *)b,sizeof(b));
        h=busy_hash(h,(const volatile u8 *)visible,sizeof(visible));
        h=busy_hash(h,(const volatile u8 *)explored,sizeof(explored));
        h=busy_hash(h,(const volatile u8 *)values,sizeof(values));
        volatile const u16 *display=(volatile const u16 *)0x04000000;
        volatile const u8 *pixels=(volatile const u8 *)((*display&16u)?0x0600a000:0x06000000);
        u32 p=busy_hash(2166136261u,pixels,240u*160u);
        p=busy_hash(p,(const volatile u8 *)0x05000000,512u);
        busy_samples[n][0]=busy_tick;busy_samples[n][1]=h;busy_samples[n][2]=p;
    }
}
static void busy_report(void)
{
    out_text("Z7B COUNTS");out_field("calls",busy_calls);out_field("advancing",busy_advancing);
    out_field("text_before",busy_text_before);out_field("text_after",busy_text_after);
    for(unsigned i=0;i<3;++i){out_text(" n");out_u64(i);out_text("=");out_u64(busy_counts[i]);out_text(" sum");out_u64(i);out_text("=");out_u64(busy_cycles[i]);out_text(" peak");out_u64(i);out_text("=");out_u64(busy_peaks[i]);}
    out_text(" high50=");out_u64(busy_top[49]);out_text(" high10=");out_u64(busy_top[9]);out_flush();
    for(unsigned i=0;i<4;++i){out_text("Z7B MODAL");out_field("index",i);out_field("screen_before",busy_screen_before[i]);out_field("screen_after",busy_screen_after[i]);out_field("mode_before",busy_mode_before[i]);out_field("mode_after",busy_mode_after[i]);out_flush();}
    for(unsigned i=0;i<10;++i){out_text("Z7B WORK");out_field("index",i);out_field("first",busy_first[i]);out_field("last",busy_last[i]);if(i<4)out_field("minimum",busy_min[i]);out_flush();}
    for(unsigned i=0;i<10;++i){out_text("Z7B SAMPLE");out_field("tick",busy_samples[i][0]);out_text(" state=");out_u64(busy_samples[i][1]);out_text(" pixels_palette=");out_u64(busy_samples[i][2]);out_flush();}
    out_text("Z7B DONE");out_flush();
}
