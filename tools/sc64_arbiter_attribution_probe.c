/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Timing-only: link instead of common/main.c with an external tape.h. */
#include "gba.h"
volatile u32 demo_frame; volatile u16 demo_inject_keys,demo_keys; volatile u32 demo_last_cycles,demo_peak_cycles,demo_update_cycles; volatile uint64_t demo_total_cycles; volatile u32 demo_tick_buckets[8];
extern const u8 u[PROBE_U_BYTES],b[PROBE_B_BYTES],visible[PROBE_GRID_BYTES],explored[PROBE_GRID_BYTES]; extern int minerals,gas,frame,screen,won,lost,game_text_box_up(void);
#ifdef SC64_ARBITER_ATTRIBUTION
extern volatile u32 sc64_arbiter_attribution[10];
extern volatile u32 sc64_setup_arbiter,sc64_arbiter_reset_cycles,sc64_arbiter_aura_cycles,sc64_arbiter_update_cycles,sc64_arbiter_detect_cycles,sc64_optical_unit_cycles,sc64_optical_building_cycles,sc64_optical_refresh_count;
#endif
volatile uint64_t totals[18]; volatile u32 peaks[18];
volatile u32 checkpoints[90][4];
volatile u32 active_count,active_peak,active_peak_tick,modal_acknowledged;
static u16 previous_held;
#define sc64_text_box_count() game_text_box_up()
#include "../tests/sc64_text_box_replay.h"
static void frame_with(u16 held) { demo_keys=held; game_update(held,held & (u16)~previous_held); audio_update(); game_render(); previous_held=held; }
volatile uint64_t active_full,active_totals[18];
volatile u32 active_peaks[18],active_peak_ticks[18];
__attribute__((noinline)) void diag_done(void) { __asm__ volatile ("nop"); }
#include "tape.h"
static u32 hash(u32 h, const volatile u8 *p,unsigned n){while(n--)h=(h^*p++)*16777619u;return h;}
static u32 pixels(void){volatile u16 *reg=(volatile u16*)0x04000000, *pal=(volatile u16*)0x05000000;volatile u16 *fb=(volatile u16*)(((*reg)&16)?0x0600a000:0x06000000);u32 h=2166136261u;for(int i=0;i<19200;i++){u16 x=fb[i],a=pal[x&255],b=pal[x>>8];h=(h^(a&255))*16777619u;h=(h^(a>>8))*16777619u;h=(h^(b&255))*16777619u;h=(h^(b>>8))*16777619u;}return h;}

int main(void){gba_init();game_init();u16 held=0;unsigned idx=0; int previous_frame=frame;for(unsigned t=0;t<9020;t++){while(idx<sizeof(tape)/sizeof(tape[0])&&tape[idx].t==t)held=tape[idx++].h;u32 before[18]={0};
#ifdef SC64_ARBITER_ATTRIBUTION
for(int i=0;i<10;i++)before[i]=sc64_arbiter_attribution[i];
#endif
u32 start=gba_clock();frame_with(held);u32 full=gba_clock()-start;demo_frame++;if(t>=8020){ u32 a[18]={0};
#ifdef SC64_ARBITER_ATTRIBUTION
a[0]=sc64_setup_arbiter; a[1]=sc64_arbiter_reset_cycles; a[2]=sc64_arbiter_aura_cycles; a[3]=sc64_arbiter_update_cycles; a[4]=sc64_arbiter_detect_cycles; a[5]=sc64_optical_unit_cycles; a[6]=sc64_optical_building_cycles; a[7]=sc64_optical_refresh_count; for(int i=0;i<10;i++)a[8+i]=sc64_arbiter_attribution[i]-before[i];
#else
(void)before;
#endif
 if(screen==1 && frame==previous_frame+1) { ++active_count; active_full+=full; if(full>active_peak){active_peak=full;active_peak_tick=t;} for(int i=0;i<18;i++){active_totals[i]+=a[i];if(a[i]>active_peaks[i]){active_peaks[i]=a[i];active_peak_ticks[i]=t;}} } for(int i=0;i<18;i++){totals[i]+=a[i]; if(a[i]>peaks[i])peaks[i]=a[i];}}TEXT_BOXES_DISMISS(frame_with);modal_acknowledged=text_boxes_acknowledged;previous_frame=frame;if((t+1)%100==0){u32 h=hash(2166136261u,u,sizeof(u));h=hash(h,b,sizeof(b));h=hash(h,visible,sizeof(visible));h=hash(h,explored,sizeof(explored));h=hash(h,(u8*)&minerals,4);h=hash(h,(u8*)&gas,4);checkpoints[t/100][0]=t+1;checkpoints[t/100][1]=h;checkpoints[t/100][2]=pixels();checkpoints[t/100][3]=(u32)frame;}}diag_done();for(;;){}}
