/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Private no-GDB Terran normal-menu controller replay with sight timing.
 *
 * This translation unit replaces common/main.c only. starcraft/game.c is
 * compiled as its own unit with the product's flags, so its code matches the
 * product; build_probe.py then makes the state read here global in game.o
 * (objcopy --globalize-symbol). PROBE_*_BYTES come from game.o's symbol sizes.
 * Results go to mGBA's debug registers. */
#include "gba.h"

volatile u32 demo_frame;
volatile u16 demo_inject_keys;
volatile u16 demo_keys;
volatile u32 demo_last_cycles, demo_peak_cycles;
volatile u32 demo_update_cycles;
volatile uint64_t demo_total_cycles;
volatile u32 demo_tick_buckets[8];

extern volatile u32 sc64_sight_cycles;
/* Unit and Building records in the Terran runtime have no padding. */
extern const u8 u[PROBE_U_BYTES], b[PROBE_B_BYTES];
extern const u8 visible[PROBE_GRID_BYTES], explored[PROBE_GRID_BYTES];
extern int minerals, gas, frame, screen, won, lost, race_selected_mission;
enum { SCREEN_TITLE, SCREEN_PLAY, SCREEN_PAUSE, SCREEN_RESULT };

typedef struct { u32 tick; u16 held; } TapeInput;
#include TAPE_HEADER

#define WINDOW_TICKS 500u

static u32 hash_bytes(u32 h, const u8 *p, u32 n)
{ for (u32 i=0;i<n;i++) h=(h^p[i])*16777619u; return h; }
static u32 hash_u32(u32 h, u32 v) { return hash_bytes(h,(const u8 *)&v,4); }
static u32 state_digest(int with_fog)
{
    u32 h=hash_bytes(2166136261u,u,sizeof(u));
    h=hash_bytes(h,b,sizeof(b));
    if (with_fog) { h=hash_bytes(h,visible,sizeof(visible)); h=hash_bytes(h,explored,sizeof(explored)); }
    h=hash_u32(h,(u32)minerals); h=hash_u32(h,(u32)gas); return hash_u32(h,(u32)frame);
}

static void append_text(char *s,unsigned *n,const char *t) { while (*t) s[(*n)++]=*t++; }
static void append_num(char *s,unsigned *n,u32 v)
{ char t[10]; unsigned k=0; do { t[k++]=(char)('0'+v%10u); v/=10u; } while(v); while(k)s[(*n)++]=t[--k]; }
static void append_hex(char *s,unsigned *n,u32 v)
{ static const char d[]="0123456789abcdef"; for(int i=7;i>=0;i--)s[(*n)++]=d[(v>>(i*4))&15u]; }
static void field(char *s,unsigned *n,const char *k,u32 v) { append_text(s,n,k); append_num(s,n,v); }
static void digests(char *s,unsigned *n)
{ append_text(s,n," digest="); append_hex(s,n,state_digest(1)); append_text(s,n," play="); append_hex(s,n,state_digest(0)); }
static void emit_line(char *s,unsigned n)
{
    volatile u16 *enable=(volatile u16 *)0x04fff780u;
    volatile u16 *flags=(volatile u16 *)0x04fff700u;
    volatile char *text=(volatile char *)0x04fff600u;
    s[n]=0; *enable=0xc0de;
    if (*enable==0x1dea) { for(unsigned i=0;i<=n;i++)text[i]=s[i]; *flags=0x102; }
}

typedef struct { u32 n, up, sight, full, pk_up, pk_sight, pk_full; } Window;
static Window win;
static uint64_t tot_up, tot_sight, tot_full;
static u32 tot_n, pk_up, pk_sight, pk_full;

static void emit_window(u32 tick)
{
    char s[240]; unsigned n=0;
    field(s,&n,"W tick=",tick); field(s,&n," n=",win.n); field(s,&n," up=",win.up);
    field(s,&n," sight=",win.sight); field(s,&n," full=",win.full);
    field(s,&n," pk_up=",win.pk_up); field(s,&n," pk_sight=",win.pk_sight);
    field(s,&n," pk_full=",win.pk_full); field(s,&n," frame=",(u32)frame);
    field(s,&n," screen=",(u32)screen); field(s,&n," won=",(u32)won); field(s,&n," lost=",(u32)lost);
    digests(s,&n); emit_line(s,n); win=(Window){0};
}

int main(void)
{
    gba_init(); game_init();
    u32 index=0, tick; u16 held=0, previous=0;
    char s[240]; unsigned n=0;
    append_text(s,&n,"PROBE start tape="); append_text(s,&n,TAPE_NAME); field(s,&n," stop=",TAPE_STOP_TICK);
    emit_line(s,n);
    for (tick=0; tick<TAPE_STOP_TICK; tick++) {
        while (index<TAPE_INPUT_COUNT && tape_inputs[index].tick==tick) held=tape_inputs[index++].held;
        int prior=screen;
        demo_keys=held;
        u32 start=gba_clock();
        game_update(held, held & (u16)~previous);
        u32 up_end=gba_clock(), sight=sc64_sight_cycles;
        audio_update(); game_render();
        u32 full=gba_clock()-start, up=up_end-start;
        previous=held; demo_frame++;
        if (prior==SCREEN_PLAY && screen==SCREEN_PLAY) {
            win.n++; win.up+=up; win.sight+=sight; win.full+=full;
            if(up>win.pk_up)win.pk_up=up; if(sight>win.pk_sight)win.pk_sight=sight; if(full>win.pk_full)win.pk_full=full;
            tot_n++; tot_up+=up; tot_sight+=sight; tot_full+=full;
            if(up>pk_up)pk_up=up; if(sight>pk_sight)pk_sight=sight; if(full>pk_full)pk_full=full;
        }
        if (tick==TAPE_LAUNCH_TICK) {
            n=0; field(s,&n,"LAUNCH screen=",(u32)screen); field(s,&n," mission=",(u32)race_selected_mission);
            digests(s,&n); emit_line(s,n);
        }
        if (tick>TAPE_LAUNCH_TICK && screen==SCREEN_RESULT) { ++tick; break; }
        if ((tick+1u)%WINDOW_TICKS==0u) emit_window(tick+1u);
    }
    emit_window(tick);
    n=0; field(s,&n,"TOTAL n=",tot_n);
    field(s,&n," up_hi=",(u32)(tot_up>>32)); field(s,&n," up_lo=",(u32)tot_up);
    field(s,&n," sight_hi=",(u32)(tot_sight>>32)); field(s,&n," sight_lo=",(u32)tot_sight);
    field(s,&n," full_hi=",(u32)(tot_full>>32)); field(s,&n," full_lo=",(u32)tot_full);
    field(s,&n," pk_up=",pk_up); field(s,&n," pk_sight=",pk_sight); field(s,&n," pk_full=",pk_full);
    emit_line(s,n);
    n=0; field(s,&n,"END tick=",tick); field(s,&n," frame=",(u32)frame); field(s,&n," screen=",(u32)screen);
    field(s,&n," won=",(u32)won); field(s,&n," lost=",(u32)lost); digests(s,&n); emit_line(s,n);
    n=0; append_text(s,&n,"PROBE DONE"); emit_line(s,n);
    for (;;) {}
}
