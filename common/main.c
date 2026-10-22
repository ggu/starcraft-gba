#include "gba.h"
#if (defined(SC64_Z3_WINDOWED_SCHEDULE) + defined(SC64_P3_WINDOWED_SCHEDULE) + \
     defined(SC64_T4_WINDOWED_SCHEDULE)) > 1
#error "Z3, P3 and T4 windowed scheduler profiles are mutually exclusive"
#endif
#if defined(SC64_Z3_WINDOWED_SCHEDULE) || defined(SC64_P3_WINDOWED_SCHEDULE) || \
    defined(SC64_T4_WINDOWED_SCHEDULE)
#include "demo_schedule.h"
#define SC64_WINDOWED_SCHEDULE 1
#endif
#ifdef SC64_Z3_WINDOWED_SCHEDULE
#if !defined(SC64_RACES) || !defined(SC64_UNIFIED) || \
    !defined(SC64_Z3_ENABLED) || !defined(SC64_CAMPAIGN_RACE) || \
    SC64_CAMPAIGN_RACE != 1
#error "SC64_Z3_WINDOWED_SCHEDULE requires the unified Zerg campaign profile"
#endif
#endif
#ifdef SC64_P3_WINDOWED_SCHEDULE
#if !defined(SC64_RACES) || !defined(SC64_UNIFIED) || \
    !defined(SC64_P3_ENABLED) || !defined(SC64_P3_RUNTIME) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 2
#error "SC64_P3_WINDOWED_SCHEDULE requires the unified Protoss P3 runtime profile"
#endif
#endif
#ifdef SC64_T4_WINDOWED_SCHEDULE
#if !defined(SC64_T4_RUNTIME) || !defined(SC64_T4_ENABLED) || \
    !defined(SC64_RACES) || !defined(SC64_UNIFIED) || \
    !defined(SC64_LARGE_MAPS) || !defined(SC64_ROUTE_PACK3) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 0
#error "SC64_T4_WINDOWED_SCHEDULE requires the unified Terran T4 packed large-map profile"
#endif
#endif
volatile u32 demo_frame;
volatile u16 demo_inject_keys;
volatile u16 demo_keys;
volatile u32 demo_last_cycles, demo_peak_cycles;
volatile u32 demo_update_cycles;
volatile uint64_t demo_total_cycles;
volatile u32 demo_tick_buckets[8];
/* Optional debugger-fed controller events. Disabled in ordinary play. This
 * avoids a debugger round trip every tick during full-mission ARM replays. */
#if defined(SC64_BACKWATER) || defined(D2_DEN_OF_EVIL) || defined(SC64_RACES)
#ifdef SC64_WINDOWED_SCHEDULE
#define DEMO_SCHEDULE_CAPACITY DEMO_SCHEDULE_WINDOW_CAPACITY
#else
#define DEMO_SCHEDULE_CAPACITY 2048
#endif
#else
#define DEMO_SCHEDULE_CAPACITY 128
#endif
volatile u32 demo_schedule_events[DEMO_SCHEDULE_CAPACITY][2];
volatile u32 demo_schedule_count, demo_schedule_length, demo_schedule_tick;
#ifdef SC64_WINDOWED_SCHEDULE
volatile u32 demo_schedule_active, demo_schedule_index;
#endif
__attribute__((noinline)) void demo_replay_checkpoint(void) {
#ifdef D2_DEN_OF_EVIL
    extern void den_debug_checkpoint(void);
    den_debug_checkpoint();
#endif
    __asm__ volatile ("" ::: "memory");
}
int main(void) {
    gba_init();
    game_init();
    u16 previous=0;
#ifdef SC64_WINDOWED_SCHEDULE
    u16 schedule_keys=0;
#else
    u32 schedule_index=0;
    u16 schedule_keys=0;
#endif
    for (;;) {
        u32 schedule_count=demo_schedule_count;
#ifdef SC64_WINDOWED_SCHEDULE
        u32 schedule_active=demo_schedule_active;
        u32 schedule_enabled=schedule_active || schedule_count;
        if(schedule_enabled && !demo_schedule_tick && !demo_schedule_index)
            demo_replay_checkpoint();
#else
        if(schedule_count && !demo_schedule_tick && !schedule_index)
            demo_replay_checkpoint();
#endif
        u32 start=gba_clock();
        u16 held=gba_keys() | demo_inject_keys;
#ifdef SC64_WINDOWED_SCHEDULE
        if(schedule_enabled) {
            held=demo_schedule_window_held(demo_schedule_events,schedule_count,
                                           &demo_schedule_index,
                                           demo_schedule_tick,&schedule_keys);
        }
#else
        if(schedule_count) {
            while(schedule_index<schedule_count && schedule_index<DEMO_SCHEDULE_CAPACITY &&
                  demo_schedule_events[schedule_index][0]==demo_schedule_tick) {
                schedule_keys=(u16)demo_schedule_events[schedule_index][1];
                schedule_index++;
            }
            held=schedule_keys;
        }
#endif
        demo_keys=held;
        game_update(held, held & (u16)~previous);
        demo_update_cycles=gba_clock()-start;
        audio_update();
        game_render();
        demo_last_cycles=gba_clock()-start;
        demo_total_cycles+=demo_last_cycles;
        u32 bucket=(demo_last_cycles+140448u)/280896u;
        demo_tick_buckets[bucket<7?bucket:7]++;
        if(demo_last_cycles>demo_peak_cycles)demo_peak_cycles=demo_last_cycles;
        previous=held;
        demo_frame++;
#ifdef SC64_WINDOWED_SCHEDULE
        if(schedule_enabled) {
            demo_schedule_tick++;
            if(demo_schedule_tick%1000u==0 || demo_schedule_tick==demo_schedule_length) {
                demo_replay_checkpoint();
                /* The host checks the consumed cursor while stopped in the
                 * observer, then installs the next page before this reset. */
                if(demo_schedule_active)
                    demo_schedule_index=0;
            }
        }
#else
        if(schedule_count) {
            demo_schedule_tick++;
            if(demo_schedule_tick%1000u==0 || demo_schedule_tick==demo_schedule_length)
                demo_replay_checkpoint();
        }
#endif
    }
}
