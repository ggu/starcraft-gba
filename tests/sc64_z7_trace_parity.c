/* Normal-input Z7 replay driver for source-level state parity checks. */
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../starcraft/game.c"

#if defined(SC64_Z7_COUNT_SNAPSHOT) && !defined(SC64_Z7_PREVALIDATED_TICK)
#error "SC64_Z7_COUNT_SNAPSHOT requires SC64_Z7_PREVALIDATED_TICK"
#endif

enum { MAX_REPLAY_TICKS = 1000000u, KEY_MASK = 0x3ffu };

static uint64_t hash_bytes(uint64_t h,const void *ptr,size_t n)
{
    const unsigned char *p=(const unsigned char *)ptr;
    while(n--){h^=*p++;h*=UINT64_C(1099511628211);}
    return h;
}
#define HASH(value) (h=hash_bytes(h,&(value),sizeof(value)))

/* Candidate-only tick capability and count cache are implementation state.
   The parity hash covers actors, resources, fog, campaign/source model, AI
   and the selected shared input/UI fields below. */
static uint64_t common_state_hash(void)
{
    uint64_t h=UINT64_C(1469598103934665603);
    HASH(u); HASH(b); HASH(m); HASH(geysers); HASH(geyser_count);
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    HASH(invincible_units); HASH(invincible_buildings);
#endif
    HASH(target_unit_head); HASH(target_building_head);
    HASH(target_unit_next); HASH(target_building_next); HASH(target_lists_valid);
#if defined(SC64_RACES) && (defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY))
    HASH(target_hostile_mask); HASH(target_hostile_masks_valid);
#endif
    HASH(visible); HASH(explored);
    HASH(sc64_visible_sources);
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    HASH(sc64_visible_sources_high);
#endif
    HASH(sc64_minimap_visible); HASH(sc64_minimap_explored);
    HASH(sc64_unit_visibility); HASH(sc64_building_visibility);
    HASH(race_cols); HASH(race_rows);
    HASH(race_campaign); HASH(race_z7); HASH(race_player);
    HASH(race_minerals); HASH(race_gas);
    HASH(race_z7_world.source_current_player);
    HASH(race_z7_world.source_slot_active);
    HASH(race_z7_world.live_owner_for_source);
    HASH(race_z7_world.source_force_for_slot);
    HASH(race_z7_world.elapsed_frames);
    HASH(race_z7_world.presentation_opcode);
    HASH(race_z7_world.last_string_id);
    HASH(race_z7_world.victory_requested);
    HASH(race_z7_world.defeat_requested);
    HASH(race_z7_world.objective_refresh_requested);
    HASH(minerals); HASH(gas); HASH(supply_used); HASH(supply_cap);
    HASH(seed); HASH(frame); HASH(screen); HASH(won); HASH(lost);
    HASH(mode); HASH(menu); HASH(menu_open); HASH(menu_cursor); HASH(train_type);
    HASH(cx); HASH(cy); HASH(camx); HASH(camy); HASH(help);
    HASH(toast_timer); HASH(toast_id); HASH(enemy_timer); HASH(wave);
    HASH(box_state); HASH(box_anchor_x); HASH(box_anchor_y);
    HASH(box_end_x); HASH(box_end_y); HASH(box_suppress_a);
    HASH(unit_tap); HASH(unit_tap_x); HASH(unit_tap_y); HASH(unit_tap_frame);
    HASH(result_timer); HASH(selected_building);
    HASH(weapons_level); HASH(armor_level); HASH(weapons_research); HASH(armor_research);
    HASH(race_mode); HASH(race_selected_mission); HASH(race_completed);
    HASH(race_front); HASH(race_front_transition); HASH(race_front_load_error);
    for(int owner=0;owner<5;++owner) {
        int used=race_owner_supply(owner),cap=race_owner_supply_cap(owner);
        int reserved=race_owner_reserved_supply(owner),queued=race_owner_queued_supply(owner);
        HASH(used); HASH(cap); HASH(reserved); HASH(queued);
    }
    return h;
}

static int read_header(unsigned *ticks,unsigned *event_count,unsigned *final_frame,
                       unsigned *final_screen,unsigned *final_outcome,
                       unsigned *final_started,int *final_minerals,int *final_gas)
{
    char line[256],extra;
    unsigned long long t,n,f,s,o,started;
    long long minerals_value,gas_value;
    if(!fgets(line,sizeof(line),stdin))return 0;
    if(!strchr(line,'\n')&&!feof(stdin))return 0;
    if(sscanf(line,"%llu %llu %llu %llu %llu %llu %lld %lld %c",
       &t,&n,&f,&s,&o,&started,&minerals_value,&gas_value,&extra)!=8) return 0;
    if(t<1||t>MAX_REPLAY_TICKS||n<1||n>t||f>INT_MAX||s>INT_MAX||
       o>INT_MAX||started>INT_MAX||minerals_value<INT_MIN||minerals_value>INT_MAX||
       gas_value<INT_MIN||gas_value>INT_MAX)return 0;
    *ticks=(unsigned)t;*event_count=(unsigned)n;*final_frame=(unsigned)f;
    *final_screen=(unsigned)s;*final_outcome=(unsigned)o;
    *final_started=(unsigned)started;*final_minerals=(int)minerals_value;
    *final_gas=(int)gas_value;
    return 1;
}

static int read_event(unsigned *host_tick,unsigned *held,unsigned *pressed)
{
    char line[256],extra;
    unsigned long long t,h,p;
    if(!fgets(line,sizeof(line),stdin))return 0;
    if(!strchr(line,'\n')&&!feof(stdin))return 0;
    if(sscanf(line,"%llu %llu %llu %c",&t,&h,&p,&extra)!=3)return 0;
    if(t>MAX_REPLAY_TICKS||h>KEY_MASK||p>KEY_MASK)return 0;
    *host_tick=(unsigned)t;*held=(unsigned)h;*pressed=(unsigned)p;
    return 1;
}

static int only_trailing_space(void)
{
    int ch;
    while((ch=getchar())!=EOF)if(!isspace((unsigned char)ch))return 0;
    return !ferror(stdin);
}

int main(void)
{
    unsigned ticks,event_count,expected_frame,expected_screen,expected_outcome;
    unsigned expected_started;
    int expected_minerals,expected_gas;
    if(!read_header(&ticks,&event_count,&expected_frame,&expected_screen,
                    &expected_outcome,&expected_started,&expected_minerals,&expected_gas)) {
        fprintf(stderr,"invalid replay header\n");return 2;
    }
    game_init();
    unsigned next_tick=0,next_held=0,next_pressed=0,previous_held=0;
    unsigned event_index=0;
    int have_event=read_event(&next_tick,&next_held,&next_pressed);
    if(!have_event||next_tick<1||next_tick>ticks||next_pressed!=next_held) {
        fprintf(stderr,"invalid first replay transition\n");return 2;
    }
    /* Hold the last recorded state between sparse transitions. */
    unsigned current_held=0;
    unsigned long long rows=0;
    for(unsigned tick=1;tick<=ticks;++tick) {
        unsigned pressed=0;
        if(have_event&&tick==next_tick) {
            current_held=next_held;pressed=next_pressed;
            previous_held=next_held;
            ++event_index;
            if(event_index<event_count) {
                if(!read_event(&next_tick,&next_held,&next_pressed)||
                   next_tick<=tick||next_tick>ticks||next_held==previous_held||
                   next_pressed!=(next_held&~previous_held)) {
                    fprintf(stderr,"invalid or unordered replay transition %u\n",event_index+1);
                    return 2;
                }
            } else have_event=0;
        }
        if(have_event&&next_tick<tick) {
            fprintf(stderr,"missed replay transition at update %u\n",next_tick);return 2;
        }
        game_update((u16)current_held,(u16)pressed);
        if(tick>=12u&&(!race_z7_active()||race_selected_mission!=SC64_Z7_GLOBAL_ID)) {
            fprintf(stderr,"normal Z7 route inactive at update %u\n",tick);return 3;
        }
        if(race_z7.runtime.fault) {
            fprintf(stderr,"Z7 runtime fault %u at update %u\n",race_z7.runtime.fault,tick);return 3;
        }
        printf("%u %016llx %d %d %d %d %d %u %u %u %u %u %u %u %d %d %d %d ",
            tick,(unsigned long long)common_state_hash(),frame,screen,mode,won,lost,
            (unsigned)race_z7.runtime.model.outcome,
            (unsigned)race_z7.runtime.model.started,(unsigned)race_z7.runtime.switch10,
            (unsigned)race_z7.runtime.ai_script_mask,(unsigned)race_z7.runtime.ai_plan_ticks,
            (unsigned)race_z7.runtime.ai_plan_phase,(unsigned)race_z7.runtime.fault,
            minerals,gas,race_owner_supply(SIDE),race_owner_supply_cap(SIDE));
        for(unsigned i=0;i<SC64_Z7_TRIGGER_COUNT;++i)
            printf("%s%u",i?",":"",(unsigned)race_z7.runtime.model.action_cursor[i]);
        printf(" %u\n",(unsigned)race_z7_world.source_current_player);
        ++rows;
    }
    if(event_index!=event_count||have_event||!only_trailing_space()) {
        fprintf(stderr,"replay has unread transitions or trailing data\n");return 2;
    }
    if(rows!=ticks||frame!=(int)expected_frame||screen!=(int)expected_screen||
       race_z7.runtime.model.outcome!=expected_outcome||
       race_z7.runtime.model.started!=expected_started||minerals!=expected_minerals||
       gas!=expected_gas) {
        fprintf(stderr,"terminal state does not match authenticated trace expectation\n");return 4;
    }
    if((expected_outcome==1u&&(!won||lost))||
       (expected_outcome==2u&&(!lost||won))||
       (expected_outcome==0u&&(won||lost))) {
        fprintf(stderr,"terminal result flags do not match authenticated outcome\n");return 4;
    }
    return 0;
}
