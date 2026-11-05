/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Timing-only: link instead of common/main.c with an external tape.h. */
#include "gba.h"
volatile u32 demo_frame; volatile u16 demo_inject_keys,demo_keys; volatile u32 demo_last_cycles,demo_peak_cycles,demo_update_cycles; volatile uint64_t demo_total_cycles; volatile u32 demo_tick_buckets[8];
extern const u8 u[PROBE_U_BYTES],b[PROBE_B_BYTES],visible[PROBE_GRID_BYTES],explored[PROBE_GRID_BYTES]; extern int minerals,gas,frame,screen,won,lost;
volatile uint64_t totals[28]; volatile u32 peaks[28];
volatile u32 checkpoints[90][4];
volatile u32 active_count,active_peak,active_peak_tick;
volatile uint64_t active_full,active_totals[28];
volatile u32 active_peaks[28],active_peak_ticks[28];
__attribute__((noinline)) void diag_done(void) { __asm__ volatile ("nop"); }
#include "tape.h"
static u32 hash(u32 h, const volatile u8 *p,unsigned n){while(n--)h=(h^*p++)*16777619u;return h;}
static u32 pixels(void){volatile u16 *reg=(volatile u16*)0x04000000, *pal=(volatile u16*)0x05000000;volatile u16 *fb=(volatile u16*)(((*reg)&16)?0x0600a000:0x06000000);u32 h=2166136261u;for(int i=0;i<19200;i++){u16 x=fb[i],a=pal[x&255],b=pal[x>>8];h=(h^(a&255))*16777619u;h=(h^(a>>8))*16777619u;h=(h^(b&255))*16777619u;h=(h^(b>>8))*16777619u;}return h;}

int main(void){gba_init();game_init();u16 held=0,prev=0;unsigned idx=0; int previous_frame=frame;for(unsigned t=0;t<9020;t++){while(idx<sizeof(tape)/sizeof(tape[0])&&tape[idx].t==t)held=tape[idx++].h;demo_keys=held;u32 start=gba_clock();game_update(held,held&~prev);audio_update();game_render();(void)start;prev=held;demo_frame++;if(t>=8020){ u32 a[28]={0}; u32 full=gba_clock()-start; if(screen==1 && frame==previous_frame+1) { ++active_count; active_full+=full; if(full>active_peak){active_peak=full;active_peak_tick=t;} for(int i=0;i<28;i++){active_totals[i]+=a[i];if(a[i]>active_peaks[i]){active_peaks[i]=a[i];active_peak_ticks[i]=t;}} } for(int i=0;i<28;i++){totals[i]+=a[i]; if(a[i]>peaks[i])peaks[i]=a[i];}}previous_frame=frame;if((t+1)%100==0){u32 h=hash(2166136261u,u,sizeof(u));h=hash(h,b,sizeof(b));h=hash(h,visible,sizeof(visible));h=hash(h,explored,sizeof(explored));h=hash(h,(u8*)&minerals,4);h=hash(h,(u8*)&gas,4);checkpoints[t/100][0]=t+1;checkpoints[t/100][1]=h;checkpoints[t/100][2]=pixels();checkpoints[t/100][3]=(u32)frame;}}diag_done();for(;;){}}
