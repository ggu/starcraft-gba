/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Private no-GDB Terran normal-menu controller replay with sight timing.
 * Replaces common/main.c only; game.c is the build tree's own source compiled
 * with the product flags. Results go to mGBA's debug registers. */
#include "game.c"

volatile u32 demo_frame;
volatile u16 demo_inject_keys;
volatile u16 demo_keys;
volatile u32 demo_last_cycles, demo_peak_cycles;
volatile u32 demo_update_cycles;
volatile uint64_t demo_total_cycles;
volatile u32 demo_tick_buckets[8];

typedef struct { u32 tick; u16 held; } TapeInput;
#include TAPE_HEADER

#define WINDOW_TICKS 500u
#define SIGHT_BINS 32u
#define SIGHT_BIN_CYCLES 1024u

static u32 hash_byte(u32 h, u8 v) { return (h ^ v) * 16777619u; }
static u32 hash_u32(u32 h, u32 v)
{
    h=hash_byte(h,(u8)v); h=hash_byte(h,(u8)(v>>8));
    h=hash_byte(h,(u8)(v>>16)); return hash_byte(h,(u8)(v>>24));
}
static u32 state_digest(int with_fog)
{
    u32 h=2166136261u;
    for (int i=0;i<MAX_UNITS;i++) {
        const Unit *q=&u[i];
        h=hash_u32(h,(u32)q->alive);
        if (!q->alive) continue;
        h=hash_u32(h,(u32)q->x); h=hash_u32(h,(u32)q->y); h=hash_u32(h,(u32)q->hp);
        h=hash_u32(h,(u32)q->owner); h=hash_u32(h,(u32)q->type);
        h=hash_u32(h,(u32)q->order); h=hash_u32(h,(u32)q->target);
    }
    for (int i=0;i<MAX_BUILDINGS;i++) {
        const Building *q=&b[i];
        h=hash_u32(h,(u32)q->alive);
        if (!q->alive) continue;
        h=hash_u32(h,(u32)q->hp); h=hash_u32(h,(u32)q->owner); h=hash_u32(h,(u32)q->type);
        h=hash_u32(h,(u32)q->under_construction);
    }
    if (with_fog) for (unsigned i=0;i<sizeof(visible);i++) { h=hash_byte(h,visible[i]); h=hash_byte(h,explored[i]); }
    h=hash_u32(h,(u32)minerals); h=hash_u32(h,(u32)gas); h=hash_u32(h,(u32)frame);
    return h;
}

static void append_text(char *s,unsigned *n,const char *t) { while (*t) s[(*n)++]=*t++; }
static void append_num(char *s,unsigned *n,u32 v)
{ char t[10]; unsigned k=0; do { t[k++]=(char)('0'+v%10u); v/=10u; } while(v); while(k)s[(*n)++]=t[--k]; }
static void append_hex(char *s,unsigned *n,u32 v)
{ static const char d[]="0123456789abcdef"; for(int i=7;i>=0;i--)s[(*n)++]=d[(v>>(i*4))&15u]; }
static void field(char *s,unsigned *n,const char *k,u32 v) { append_text(s,n,k); append_num(s,n,v); }
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
static u32 tot_n, pk_up, pk_sight, pk_full, sight_hist[SIGHT_BINS], frame_hist[8];

static void emit_window(u32 tick)
{
    char s[240]; unsigned n=0;
    field(s,&n,"W tick=",tick); field(s,&n," n=",win.n); field(s,&n," up=",win.up);
    field(s,&n," sight=",win.sight); field(s,&n," full=",win.full);
    field(s,&n," pk_up=",win.pk_up); field(s,&n," pk_sight=",win.pk_sight);
    field(s,&n," pk_full=",win.pk_full); field(s,&n," frame=",(u32)frame);
    field(s,&n," screen=",(u32)screen); field(s,&n," won=",(u32)won); field(s,&n," lost=",(u32)lost);
    append_text(s,&n," digest="); append_hex(s,&n,state_digest(1)); append_text(s,&n," play="); append_hex(s,&n,state_digest(0));
    emit_line(s,n); win=(Window){0};
}

int main(void)
{
    gba_init(); game_init();
    u32 index=0; u16 held=0, previous=0;
    char s[240]; unsigned n=0;
    append_text(s,&n,"PROBE start tape="); append_text(s,&n,TAPE_NAME); field(s,&n," stop=",TAPE_STOP_TICK);
    emit_line(s,n);
    u32 tick;
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
            u32 bin=sight/SIGHT_BIN_CYCLES; sight_hist[bin<SIGHT_BINS?bin:SIGHT_BINS-1]++;
            u32 fb=(full+140448u)/280896u; frame_hist[fb<7?fb:7]++;
        }
        if (tick==TAPE_LAUNCH_TICK) {
            n=0; field(s,&n,"LAUNCH screen=",(u32)screen); field(s,&n," mission=",(u32)race_selected_mission);
            append_text(s,&n," digest="); append_hex(s,&n,state_digest(1)); append_text(s,&n," play="); append_hex(s,&n,state_digest(0)); emit_line(s,n);
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
    n=0; append_text(s,&n,"SIGHTHIST");
    for (unsigned i=0;i<SIGHT_BINS;i++) field(s,&n," ",sight_hist[i]);
    emit_line(s,n);
    n=0; append_text(s,&n,"FRAMEHIST");
    for (unsigned i=0;i<8;i++) field(s,&n," ",frame_hist[i]);
    emit_line(s,n);
    n=0; field(s,&n,"END tick=",tick); field(s,&n," frame=",(u32)frame); field(s,&n," screen=",(u32)screen);
    field(s,&n," won=",(u32)won); field(s,&n," lost=",(u32)lost);
    append_text(s,&n," digest="); append_hex(s,&n,state_digest(1)); append_text(s,&n," play="); append_hex(s,&n,state_digest(0)); emit_line(s,n);
    n=0; append_text(s,&n,"PROBE DONE"); emit_line(s,n);
    for (;;) {}
}
