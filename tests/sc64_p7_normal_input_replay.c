/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Current-generation P7 ordinary-input replay and bounded performance probe. */
#include "../starcraft/game.c"
#ifdef GBA_HOST
#include <stdio.h>
#endif

typedef struct { unsigned tick; u16 held; } ReplayInput;
static ReplayInput replay_inputs[64];
volatile u16 demo_keys;
typedef struct { unsigned long long up, draw; u32 count, peak; } ProbePerf;
static ProbePerf probe_perf[2];
#ifdef GBA_HOST
static u32 stack_paint(void) { return 0; }
static u32 stack_lowest(u32 top) { (void)top; return 0x03007e00u; }
#else
static u32 stack_paint(void) { u32 sp0; __asm__ volatile("mov %0, sp":"=r"(sp0)); u32 top=(sp0-512u)&~3u; for(volatile u32 *p=(volatile u32 *)0x03005000u;(u32)p<top;++p)*p=0xa55aa55au; return top; }
static u32 stack_lowest(u32 top) { for(volatile u32 *p=(volatile u32 *)0x03005000u;(u32)p<top;++p)if(*p!=0xa55aa55au)return (u32)p; return top; }
#endif

static unsigned replay_input_count, replay_tick;
static u16 replay_previous;

static u32 hash_byte(u32 h, u8 v) { return (h ^ v) * 16777619u; }
static u32 hash_u32(u32 h, u32 v)
{
    h=hash_byte(h,(u8)v); h=hash_byte(h,(u8)(v>>8));
    h=hash_byte(h,(u8)(v>>16)); h=hash_byte(h,(u8)(v>>24));
    return h;
}
#define H(v) do { h=hash_u32(h,(u32)(v)); } while (0)
#define HU(o,f) H((o).f)
static u32 state_digest(void)
{
    u32 h=2166136261u;
    for (int i=0;i<MAX_UNITS;i++) {
        const Unit *q=&u[i];
        H(q->hp); H(q->maxhp); H(q->carry); H(q->shield);
        H(q->x); H(q->y); H(q->tx); H(q->ty); H(q->build); H(q->target);
        H(q->path_x); H(q->path_y); H(q->raw_type); H(q->raw_owner);
        H(q->cd); H(q->work_timer); H(q->weapon); H(q->path_timer);
        H(q->path_retry); H(q->flags); H(q->shield_remainder);
        H(q->owner); H(q->type); H(q->alive); H(q->selected); H(q->order); H(q->armor);
    }
    for (int i=0;i<MAX_BUILDINGS;i++) {
        const Building *q=&b[i];
        H(q->hp); H(q->maxhp); H(q->shield); H(q->build_progress);
        H(q->x); H(q->y); H(q->raw_type); H(q->raw_owner);
        H(q->queue_progress); H(q->flags); H(q->larva_timer); H(q->shield_remainder);
        H(q->owner); H(q->type); H(q->alive);
        for (unsigned k=0;k<5;k++) H(q->queue[k]);
        H(q->queue_head); H(q->queue_count); H(q->under_construction); H(q->level); H(q->larva);
    }
    for (int i=0;i<MAX_MINERALS;i++) { H(m[i].x); H(m[i].y); H(m[i].amount); H(m[i].style); }
    H(geyser_count);
    for (int i=0;i<MAX_RACE_GEYSERS;i++) { H(geysers[i].x); H(geysers[i].y); }
    H(race_campaign.elapsed_frames); H(race_campaign.objective_flags);
    H(race_campaign.rescue_unit); H(race_campaign.chrysalis_unit); H(race_campaign.carrier_unit);
    H(race_campaign.enemy_units_sent); H(race_campaign.message_ticks); H(race_campaign.mission);
    H(race_campaign.objective_stage); H(race_campaign.phase); H(race_campaign.message); H(race_campaign.enemy_wave);
    H(race_p7.runtime.model.started); H(race_p7.runtime.model.outcome);
    for (unsigned i=0;i<SC64_P7_TRIGGER_COUNT;i++) {
        H(race_p7.runtime.model.action_cursor[i]); H(race_p7.runtime.action_ticks[i]);
    }
    H(race_p7.runtime.switch_bits); H(race_p7.runtime.fault);
    for (unsigned i=0;i<8;i++) { H(race_p7.source_minerals[i]); H(race_p7.source_gas[i]); }
    for (unsigned i=0;i<4;i++) H(race_p7.moved_location3[i]);
    H(race_p7.moved_location3_valid); H(race_p7.created_mask);
    for (unsigned i=0;i<3;i++) H(race_p7.reserved[i]);
    for (unsigned i=0;i<SC64_ARENA_WORDS;i++) H(race_p7_projectiles.words[i]);
    H(race_p7_projectile_fault);
    for (unsigned i=0;i<SC64_GRID_STORAGE_BYTES;i++) { H(visible[i]); H(explored[i]); }
    for (unsigned i=0;i<5;i++) { H(race_player[i]); H(race_minerals[i]); H(race_gas[i]); }
    H(minerals); H(gas); H(supply_used); H(supply_cap); H(frame); H(mode); H(menu);
    H(train_type); H(race_mode); H(race_human); H(race_selected_mission); H(race_completed);
    H(cx); H(cy); H(camx); H(camy); H(toast_timer); H(toast_id); H(enemy_timer); H(wave);
    H(weapons_level); H(armor_level); H(selected_building); H(weapons_research); H(armor_research);
    H(screen); H(menu_open); H(menu_cursor); H(help); H(won); H(lost); H(result_timer); H(seed);
#ifdef SC64_IMPORTED_MISSION
    H(mission_unsupported_count); H(mission_objective_stage);
#endif
    return h;
}
#undef HU
#undef H

static void append_text(char *s,unsigned *n,const char *t)
{ while (*t) s[(*n)++]=*t++; }
static void append_num(char *s,unsigned *n,u32 v)
{ char t[10]; unsigned k=0; do { t[k++]=(char)('0'+v%10u); v/=10u; } while(v); while(k)s[(*n)++]=t[--k]; }
static void append_hex(char *s,unsigned *n,u32 v)
{ static const char d[]="0123456789abcdef"; for(int i=7;i>=0;i--)s[(*n)++]=d[(v>>(i*4))&15u]; }
static void emit_line(char *s,unsigned n)
{
    s[n]=0;
#ifdef GBA_HOST
    puts(s); fflush(stdout);
#else
    volatile u16 *enable=(volatile u16 *)0x04fff780u;
    volatile u16 *flags=(volatile u16 *)0x04fff700u;
    volatile char *text=(volatile char *)0x04fff600u;
    *enable=0xc0de;
    if (*enable==0x1dea) { for(unsigned i=0;i<=n;i++)text[i]=s[i]; *flags=0x103; }
#endif
}
static unsigned player_units(void)
{ unsigned n=0; for(int i=0;i<MAX_UNITS;i++) n+=u[i].alive&&u[i].owner==SIDE; return n; }
static unsigned player_buildings(void)
{ unsigned n=0; for(int i=0;i<MAX_BUILDINGS;i++) n+=b[i].alive&&b[i].owner==SIDE; return n; }
static int mission_fault(void)
{
    return race_p7.runtime.fault || race_p7_projectile_fault ||
#ifdef SC64_IMPORTED_MISSION
           mission_unsupported_count ||
#endif
           0;
}
static void snapshot(const char *phase,unsigned phase_tick)
{
    char s[180]; unsigned n=0;
    append_text(s,&n,"SNAP phase="); append_text(s,&n,phase);
    append_text(s,&n," tick="); append_num(s,&n,phase_tick);
    append_text(s,&n," input_tick="); append_num(s,&n,replay_tick);
    append_text(s,&n," frame="); append_num(s,&n,(u32)frame);
    append_text(s,&n," digest="); append_hex(s,&n,state_digest());
    append_text(s,&n," screen="); append_num(s,&n,(u32)screen);
    append_text(s,&n," won="); append_num(s,&n,(u32)won);
    append_text(s,&n," lost="); append_num(s,&n,(u32)lost);
    append_text(s,&n," fault="); append_num(s,&n,(u32)mission_fault());
    append_text(s,&n," player_units="); append_num(s,&n,player_units());
    append_text(s,&n," player_buildings="); append_num(s,&n,player_buildings());
    emit_line(s,n);
}
static int input_frame(u16 held)
{
    if (held!=replay_previous) {
        if(replay_input_count>=sizeof(replay_inputs)/sizeof(replay_inputs[0]))return 0;
        replay_inputs[replay_input_count++]=(ReplayInput){replay_tick+1u,held};
    }
    u16 pressed=held&(u16)~replay_previous;
    int prior_screen=screen; demo_keys=held; u32 start=gba_clock();
    game_update(held,pressed); u32 update_end=gba_clock();
    audio_update(); game_render(); u32 end=gba_clock();
    unsigned tick=replay_tick+1u;
    int phase=(tick>=12u&&tick<=611u)?0:(tick>=716u?1:-1);
    if(prior_screen==SCREEN_PLAY&&phase>=0) {
        ProbePerf *p=&probe_perf[phase]; p->up+=(u32)(update_end-start);
        p->draw+=(u32)(end-update_end); ++p->count;
        if((u32)(end-start)>p->peak)p->peak=(u32)(end-start);
    }
    replay_previous=held; ++replay_tick;
    return 1;
}
static void tap(u16 key) { input_frame(key); input_frame(0); }

static void report_perf(void)
{
    for(unsigned phase=0;phase<2;phase++) {
        const ProbePerf *p=&probe_perf[phase]; char s[220]; unsigned n=0;
        append_text(s,&n,"PERF phase=");append_text(s,&n,phase?"attack":"opening");
        append_text(s,&n," samples=");append_num(s,&n,p->count);
        append_text(s,&n," mean_up=");append_num(s,&n,p->count?(u32)(p->up/p->count):0);
        append_text(s,&n," mean_draw=");append_num(s,&n,p->count?(u32)(p->draw/p->count):0);
        append_text(s,&n," mean_full=");append_num(s,&n,p->count?(u32)((p->up+p->draw)/p->count):0);
        append_text(s,&n," peak=");append_num(s,&n,p->peak);emit_line(s,n);
    }
}
static void report_stack(u32 top)
{
    u32 low=stack_lowest(top);char s[180];unsigned n=0;
    append_text(s,&n,"STACK lowest=");append_num(s,&n,low);
    append_text(s,&n," margin=");append_num(s,&n,low-0x03005000u);
    append_text(s,&n," used=");append_num(s,&n,0x03007e00u-low);emit_line(s,n);
}

#include "sc64_p7_normal_input_tape.h"
int main(void)
{
    gba_init(); u32 painted_top=stack_paint(); game_init();
    unsigned input_index=0; u16 held=0;
    for(unsigned tick=1;tick<=P7_REPLAY_TICKS;tick++) {
        while(input_index<P7_REPLAY_INPUT_COUNT && p7_replay_inputs[input_index].tick==tick)
            held=(u16)p7_replay_inputs[input_index++].held;
        if(!input_frame(held))return 6;
        if(tick==111u && (screen!=SCREEN_PLAY || race_selected_mission!=SC64_P7_GLOBAL_ID)) return 2;
        /* Reject tapes that merely pan while never selecting/ordering a unit. */
        if(tick==619u && (selected_count()==0 || mode!=MODE_ATTACK)) return 7;
        if(tick==715u) {
            int accepted=0;
            for(int i=0;i<MAX_UNITS;i++)
                if(u[i].alive && u[i].owner==SIDE && u[i].selected &&
                   u[i].order==ORDER_ATTACK_MOVE) ++accepted;
            if(!accepted)return 8;
        }
        if(tick>=111u && tick<=611u && (tick-11u)%100u==0u)
            snapshot("opening",tick-11u);
        if(tick>=815u && (tick-715u)%100u==0u)
            snapshot("attack",tick-715u);
        if(mission_fault()) return 3;
    }
    if(input_index!=P7_REPLAY_INPUT_COUNT) return 4;
    unsigned live=player_units()+player_buildings();
    /* An early result or interrupted window is not a complete benchmark. */
    if(!live || screen!=SCREEN_PLAY || won || lost ||
       probe_perf[0].count!=600u || probe_perf[1].count!=1400u) return 5;
    char done[220]; unsigned z=0;
    append_text(done,&z,"TERMINAL frame="); append_num(done,&z,(u32)frame);
    append_text(done,&z," screen="); append_num(done,&z,(u32)screen);
    append_text(done,&z," won="); append_num(done,&z,(u32)won);
    append_text(done,&z," lost="); append_num(done,&z,(u32)lost);
    append_text(done,&z," fault="); append_num(done,&z,(u32)mission_fault());
    append_text(done,&z," player_units="); append_num(done,&z,player_units());
    append_text(done,&z," player_buildings="); append_num(done,&z,player_buildings());
    append_text(done,&z," input_events="); append_num(done,&z,P7_REPLAY_INPUT_COUNT);
    append_text(done,&z," digest="); append_hex(done,&z,state_digest()); emit_line(done,z);
    report_perf(); report_stack(painted_top);
    return 0;
}
