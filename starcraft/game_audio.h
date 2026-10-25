#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H
/* Presentation-only sound events, included by game.c after its state.
 *
 * game_audio_observe() runs at the start of each rendered frame and compares
 * the live unit and building arrays with a snapshot from the previous frame:
 * a raised weapon cooldown is a shot, alive 1->0 a death, a new player unit a
 * completed unit, a cleared construction flag a completed building, and a new
 * player selection or changed orders after input a selection or command
 * response. It reads simulation state and never writes it, never touches the
 * game RNG and keeps its snapshot outside the save. Events are skipped after
 * any tick discontinuity (load, restart, pause), so a resumed game never
 * replays history.
 *
 * With an imported bank the events select StarCraft 64 samples through the
 * generated unit tables, and music follows the menu, the player race and the
 * mission result. Without one, short PSG tones stand in.
 */
#ifdef SC64_AUDIO_IMPORTED
#include "sc64_audio_generated.h"
#endif

enum {
    GAME_AUDIO_ATTACK, GAME_AUDIO_DEATH, GAME_AUDIO_READY, GAME_AUDIO_WHAT,
    GAME_AUDIO_YES, GAME_AUDIO_BUILT, GAME_AUDIO_EVENT_KINDS
};
/* Per observed frame; StarCraft also caps simultaneous combat sounds. */
#define GAME_AUDIO_EFFECTS_PER_FRAME 2
#define GAME_AUDIO_SNAPSHOT_ALIVE 0x100u
/* Transport unloading raises the cooldown without firing. */
#define GAME_AUDIO_SNAPSHOT_LOADED 0x200u
#define GAME_AUDIO_SNAPSHOT_CONSTRUCTING 0x400u

#ifdef GBA_HOST
u32 game_audio_events[GAME_AUDIO_EVENT_KINDS];
static u16 game_audio_snapshot[MAX_UNITS + MAX_BUILDINGS];
#define GAME_AUDIO_SNAPSHOT game_audio_snapshot
#else
_Static_assert(GBA_AUDIO_MIX_BYTES + (MAX_UNITS + MAX_BUILDINGS) * 2u <= GBA_AUDIO_ARENA_BYTES,
               "audio snapshot exceeds the VRAM arena");
#define GAME_AUDIO_SNAPSHOT ((volatile u16 *)(GBA_AUDIO_ARENA + GBA_AUDIO_MIX_BYTES))
#endif

/* Input edges come from the main loop's record of this tick's held keys, so
 * game_update() itself stays untouched (its inlining shapes IWRAM use). */
#ifdef GBA_HOST
u16 game_audio_host_held;  /* Host fixtures have no main loop. */
#define GAME_AUDIO_HELD() game_audio_host_held
#else
extern volatile u16 demo_keys;
#define GAME_AUDIO_HELD() demo_keys
#endif
static u16 game_audio_previous_held;

static int game_audio_last_frame = -1, game_audio_last_screen = -1;
static u32 game_audio_selection, game_audio_orders;
static u8 game_audio_effects;
#ifdef SC64_AUDIO_IMPORTED
/* Picks among a unit's response variants; presentation-only, not game RNG. */
static u8 game_audio_variant;
/* Music tracks are 4*race + 2 (result) and 4*race + 4 (themes), with the
 * StarCraft race order Terran, Zerg, Protoss. Non-race profiles are Terran. */
#ifdef SC64_RACES
_Static_assert(R_TER == 0 && R_ZERG == 1 && R_PROTOSS == 2, "music race order");
#define GAME_AUDIO_PLAYER_RACE race_human
#else
#define GAME_AUDIO_PLAYER_RACE 0
#endif
static int game_audio_music_want = -1;
static u8 game_audio_theme;
#endif

static int game_audio_unit_raw(const Unit *q)
{
#ifdef SC64_RACES
    return q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type);
#else
    switch (q->type) {
    case U_SCV: return 7; case U_MARINE: return 0; case U_FIREBAT: return 32;
    case U_MEDIC: return 34; case U_ZERGLING: return 37; case U_HYDRALISK: return 38;
    case U_RAYNOR: return 20;
    default: return -1;
    }
#endif
}

static int game_audio_building_raw(const Building *q)
{
#ifdef SC64_RACES
    return q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type);
#else
    switch (q->type) {
    case B_COMMAND: return 106; case B_BARRACKS: return 111; case B_DEPOT: return 109;
    case B_REFINERY: return 110; case B_ACADEMY: return 112; case B_ENG_BAY: return 122;
    case B_HATCHERY: return 131; case B_BUNKER: return 125;
    case B_INFESTED_COMMAND: return 130; case B_CREEP_COLONY: return 143;
    default: return -1;
    }
#endif
}

static int game_audio_on_screen(int x, int y)
{
    int dx = x - camx, dy = y - camy;
    return dx >= -16 && dx < 256 && dy >= -16 && dy < 132;
}

static void game_audio_emit(int kind, int raw)
{
    int speech = kind == GAME_AUDIO_READY || kind == GAME_AUDIO_WHAT || kind == GAME_AUDIO_YES;
    if (!speech) {
        if (game_audio_effects >= GAME_AUDIO_EFFECTS_PER_FRAME) return;
        ++game_audio_effects;
    }
#ifdef GBA_HOST
    ++game_audio_events[kind];
#endif
#ifdef SC64_AUDIO_IMPORTED
    static const u8 bank_kind[GAME_AUDIO_EVENT_KINDS] = {
        SC64_AUDIO_ATTACK, SC64_AUDIO_DEATH, SC64_AUDIO_READY, SC64_AUDIO_WHAT,
        SC64_AUDIO_YES, SC64_AUDIO_BUILT
    };
    static const u8 priority[GAME_AUDIO_EVENT_KINDS] = { 1, 3, 2, 2, 2, 2 };
    int sound = -1;
    if (raw >= 0 && raw < SC64_AUDIO_UNIT_COUNT) {
        int row = raw * SC64_AUDIO_KIND_COUNT, k = bank_kind[kind];
        /* Buildings have no completion sound; their selection hum stands in. */
        if (kind == GAME_AUDIO_BUILT && !sc64_audio_unit_count[row + k]) k = SC64_AUDIO_WHAT;
        int count = sc64_audio_unit_count[row + k];
        if (count) sound = sc64_audio_unit_sound[sc64_audio_unit_first[row + k] +
                                                 game_audio_variant++ % count];
    }
    /* Units without a sample in the bank stay silent rather than beep. */
    if (sound >= 0) audio_play_sample((u16)sound, priority[kind], speech ? 1 : 0);
    return;
#else
    (void)raw;
#endif
    switch (kind) {
    case GAME_AUDIO_ATTACK: audio_tone(1400, 1, 1); break;
    case GAME_AUDIO_DEATH: audio_noise(8, 4); break;
    case GAME_AUDIO_READY: audio_tone(660, 4, 2); break;
    case GAME_AUDIO_WHAT: audio_tone(880, 2, 2); break;
    case GAME_AUDIO_YES: audio_tone(990, 2, 2); break;
    default: audio_tone(520, 6, 2); break;
    }
}

#ifdef SC64_AUDIO_IMPORTED
static void game_audio_music(void)
{
    int want;
    if (screen == SCREEN_TITLE) want = 0;
    else if (screen == SCREEN_RESULT) {
        int race = GAME_AUDIO_PLAYER_RACE;
        want = 2 + race * 4 + (won ? 0 : 1);
    } else {
        int race = GAME_AUDIO_PLAYER_RACE;
        /* The two race themes alternate as each one ends. */
        if (game_audio_music_want >= 4 + race * 4 && game_audio_music_want <= 5 + race * 4 &&
            audio_music_track() < 0)
            game_audio_theme ^= 1;
        want = 4 + race * 4 + game_audio_theme;
    }
    if (want != game_audio_music_want) {
        game_audio_music_want = want;
        audio_play_music(want, screen == SCREEN_TITLE);
    }
}
#endif

static void game_audio_observe(void)
{
    volatile u16 *snap = GAME_AUDIO_SNAPSHOT;
    int fresh, i;
    u32 selection = 0, orders = 0;
    int first_selected = -1;
    u16 held = GAME_AUDIO_HELD(), pressed = held & (u16)~game_audio_previous_held;
    game_audio_previous_held = held;
#ifdef GAME_AUDIO_OFF
    /* Differential test builds: the same simulation with no observer work. */
    (void)snap; (void)fresh; (void)i; (void)selection; (void)orders; (void)first_selected;
    (void)pressed;
    return;
#endif
#ifdef SC64_AUDIO_IMPORTED
    game_audio_music();
#endif
    fresh = screen != SCREEN_PLAY || game_audio_last_screen != SCREEN_PLAY ||
            frame != game_audio_last_frame + 1;
    game_audio_last_screen = screen;
    if (screen != SCREEN_PLAY) return;
    if (frame == game_audio_last_frame) return;
    game_audio_last_frame = frame;
    game_audio_effects = 0;
    for (i = 0; i < MAX_UNITS; i++) {
        const Unit *q = &u[i];
        u32 prev = snap[i], cd = q->cd > 255 ? 255u : (u32)q->cd;
        u32 now = q->alive ? GAME_AUDIO_SNAPSHOT_ALIVE | cd : 0;
        if (q->alive && SC64_TRANSPORT_LOADED(q)) now |= GAME_AUDIO_SNAPSHOT_LOADED;
        if (!fresh) {
            if ((prev & GAME_AUDIO_SNAPSHOT_ALIVE) && !q->alive) {
                if (game_audio_on_screen(q->x, q->y))
                    game_audio_emit(GAME_AUDIO_DEATH, game_audio_unit_raw(q));
            } else if (!(prev & GAME_AUDIO_SNAPSHOT_ALIVE) && q->alive) {
                if (q->owner == SIDE) game_audio_emit(GAME_AUDIO_READY, game_audio_unit_raw(q));
            } else if (q->alive && cd > (prev & 0xffu) && !(prev & GAME_AUDIO_SNAPSHOT_LOADED) &&
                       game_audio_on_screen(q->x, q->y)) {
                game_audio_emit(GAME_AUDIO_ATTACK, game_audio_unit_raw(q));
            }
        }
        snap[i] = (u16)now;
        if (q->alive && q->selected && q->owner == SIDE) {
            if (first_selected < 0) first_selected = i;
            selection = selection * 31u + (u32)i + 1u;
            orders = orders * 131u + (u32)q->order * 7u + (u32)q->tx * 3u + (u32)q->ty +
                     (u32)(q->target + 1) * 5u;
        }
    }
    for (i = 0; i < MAX_BUILDINGS; i++) {
        const Building *q = &b[i];
        u32 prev = snap[MAX_UNITS + i];
        u32 now = q->alive ? GAME_AUDIO_SNAPSHOT_ALIVE |
                  (q->under_construction ? GAME_AUDIO_SNAPSHOT_CONSTRUCTING : 0) : 0;
        if (!fresh) {
            if ((prev & GAME_AUDIO_SNAPSHOT_ALIVE) && !q->alive) {
                if (game_audio_on_screen(q->x, q->y))
                    game_audio_emit(GAME_AUDIO_DEATH, game_audio_building_raw(q));
            } else if (q->alive && (prev & GAME_AUDIO_SNAPSHOT_CONSTRUCTING) &&
                       !q->under_construction && q->owner == SIDE) {
                game_audio_emit(GAME_AUDIO_BUILT, game_audio_building_raw(q));
            }
        }
        snap[MAX_UNITS + i] = (u16)now;
    }
    if (selected_building >= 0 && selected_building < MAX_BUILDINGS)
        selection = selection * 31u + 0x10000u + (u32)selected_building;
    if (!fresh && selection && selection != game_audio_selection) {
        if (first_selected >= 0)
            game_audio_emit(GAME_AUDIO_WHAT, game_audio_unit_raw(&u[first_selected]));
        else
            game_audio_emit(GAME_AUDIO_WHAT, game_audio_building_raw(&b[selected_building]));
    } else if (!fresh && selection && orders != game_audio_orders && pressed &&
               first_selected >= 0) {
        game_audio_emit(GAME_AUDIO_YES, game_audio_unit_raw(&u[first_selected]));
    }
    game_audio_selection = selection;
    game_audio_orders = orders;
}
#endif
