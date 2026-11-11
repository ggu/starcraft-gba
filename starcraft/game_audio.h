#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H
/* Presentation-only sound events, included by game.c after its state.
 *
 * game_audio_observe() runs at the start of each rendered frame and compares
 * the live unit and building arrays with a snapshot from the previous frame:
 * a raised weapon cooldown is a shot, alive 1->0 a death, a new player unit a
 * completed unit, a cleared construction flag a completed building, and a new
 * player selection or changed orders after input a selection or command
 * response, and a refusal cue from game.c (toast_refusal) the player race's
 * advisor line. It reads simulation state and never writes it, never touches the
 * game RNG and keeps its snapshot outside the save. Events are skipped after
 * any tick discontinuity (load, restart, pause), so a resumed game never
 * replays history.
 *
 * Advisor alerts are the one input the observer cannot recover by comparing
 * snapshots, so the simulation reports them through two stores into the
 * observer's own state, which the simulation never reads: game_audio_note_hit()
 * from the damage path (a hostile hit on a player actor off screen) and
 * game_audio_cue() where research or an upgrade finishes. Building burn and
 * other damage with no hostile source never reaches the first. The observer
 * turns them into at most one alert per update; see "Voice policy" below.
 *
 * With an imported bank the events select StarCraft 64 samples through the
 * generated unit tables, advisor lines through sc64_audio_advisor, and music
 * follows the menu, the player race and the mission result. Without one,
 * short PSG tones stand in.
 */
#ifdef SC64_AUDIO_IMPORTED
#include "sc64_audio_generated.h"
#endif

enum {
    GAME_AUDIO_ATTACK, GAME_AUDIO_DEATH, GAME_AUDIO_READY, GAME_AUDIO_WHAT,
    GAME_AUDIO_YES, GAME_AUDIO_BUILT, GAME_AUDIO_ADVISOR,
    GAME_AUDIO_WARP_START, GAME_AUDIO_WARP_DONE, GAME_AUDIO_ALERT, GAME_AUDIO_EVENT_KINDS
};
/* Order matches ALERT_LINES in tools/sc64_audio_pack.py. */
enum {
    GAME_AUDIO_ALERT_BASE, GAME_AUDIO_ALERT_FORCES, GAME_AUDIO_ALERT_RESEARCH,
    GAME_AUDIO_ALERT_UPGRADE, GAME_AUDIO_ALERT_ENERGY, GAME_AUDIO_ALERT_NO_BUILD,
    GAME_AUDIO_ALERT_NO_LAND, GAME_AUDIO_ALERT_BUILT, GAME_AUDIO_ALERT_KINDS
};
/* Native repeats were 13.4 to 15.6 s apart under a sustained attack. */
#define GAME_AUDIO_ATTACK_ALERT_TICKS 450
#ifdef SC64_RACES
_Static_assert(R_TER == 0 && R_ZERG == 1 && R_PROTOSS == 2, "music and alert race order");
#define GAME_AUDIO_PLAYER_RACE race_human
#else
#define GAME_AUDIO_PLAYER_RACE 0
#endif
/* The alerts whose trigger and clip were both matched in captured N64 audio,
 * per player race (Terran, Zerg, Protoss). Terran has all eight; Zerg only
 * research (Burrow finishing in Z1). The other Zerg and Protoss alerts would
 * be guesses from clip order, so they stay silent and the bank carries no
 * clip for them. Keep in step with ALERT_SOUNDS in tools/sc64_audio_pack.py. */
static const u8 game_audio_heard[3] = { 0xffu, 1u << GAME_AUDIO_ALERT_RESEARCH, 0 };
#define GAME_AUDIO_HEARD(line) (game_audio_heard[GAME_AUDIO_PLAYER_RACE] >> (line) & 1u)
/* Per observed frame; StarCraft also caps simultaneous combat sounds. */
#define GAME_AUDIO_EFFECTS_PER_FRAME 2
#define GAME_AUDIO_SNAPSHOT_ALIVE 0x100u
/* Transport unloading raises the cooldown without firing. */
#define GAME_AUDIO_SNAPSHOT_LOADED 0x200u
#define GAME_AUDIO_SNAPSHOT_CONSTRUCTING 0x400u

/* Observer words after the snapshot (EWRAM has no room for them in the late
 * campaigns): when the last attack alert played, and the alerts waiting. */
enum { GAME_AUDIO_STATE_ATTACK_TICK, GAME_AUDIO_STATE_PENDING, GAME_AUDIO_STATE_WORDS };
enum { GAME_AUDIO_PENDING_UNIT = 1, GAME_AUDIO_PENDING_BUILDING = 2,
       GAME_AUDIO_PENDING_RESEARCH = 4, GAME_AUDIO_PENDING_UPGRADE = 8 };

#ifdef GBA_HOST
u32 game_audio_events[GAME_AUDIO_EVENT_KINDS];
int game_audio_last_alert = -1;
u32 game_audio_alert_lines[GAME_AUDIO_ALERT_KINDS];
static u16 game_audio_snapshot[MAX_UNITS + MAX_BUILDINGS + GAME_AUDIO_STATE_WORDS];
#define GAME_AUDIO_SNAPSHOT game_audio_snapshot
#else
_Static_assert(GBA_AUDIO_MIX_BYTES + (MAX_UNITS + MAX_BUILDINGS + GAME_AUDIO_STATE_WORDS) * 2u <= GBA_AUDIO_ARENA_BYTES,
               "audio snapshot exceeds the VRAM arena");
#define GAME_AUDIO_SNAPSHOT ((volatile u16 *)(GBA_AUDIO_ARENA + GBA_AUDIO_MIX_BYTES))
#endif
#define GAME_AUDIO_STATE (GAME_AUDIO_SNAPSHOT + MAX_UNITS + MAX_BUILDINGS)

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
static u8 game_audio_needs_prime;
static int game_audio_pending_warp = -1;
static void game_audio_prime(void) { game_audio_needs_prime = 1; game_audio_pending_warp = -1; }
static u32 game_audio_selection, game_audio_orders;
static u8 game_audio_effects;
/* Voice policy. Every alert is an advisor-class request (audio_mixer.h): it
 * takes an effect voice, never the speech voice, so a unit response is never
 * cut off, and the mixer plays one advisor line at a time, so alerts never
 * stack or cut each other off. The observer adds three rules: at most one
 * advisor request per observed frame (this flag), an attack alert or a
 * one-shot (research, upgrade) is held back while audio_advisor_busy(), and a
 * one-shot is kept until it can play while an attack alert is dropped (the
 * fight raises another). Order within a frame: the player's own refusal,
 * attack, research, upgrade, then a finished building (which keeps its
 * selection hum when the advisor voice is taken). */
static u8 game_audio_advised;
#ifdef SC64_AUDIO_IMPORTED
/* Picks among a unit's response variants; presentation-only, not game RNG. */
static u8 game_audio_variant;
/* Music tracks are 4*race + 2 (result) and 4*race + 4 (themes), with the
 * StarCraft race order Terran, Zerg, Protoss. Non-race profiles are Terran. */
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

static int game_audio_voice_free(void)
{
    return !game_audio_advised && !audio_advisor_busy();
}

/* Plays an advisor alert. Returns 0 only when the advisor voice is taken and
 * the caller may try again later; an alert with no matching N64 evidence for
 * this race is silent and counts as done. */
static int game_audio_alert(int line)
{
    if (!GAME_AUDIO_HEARD(line)) return 1;
    if (!game_audio_voice_free()) return 0;
    game_audio_advised = 1;
#ifdef GBA_HOST
    ++game_audio_events[GAME_AUDIO_ALERT];
    game_audio_last_alert = line;
    ++game_audio_alert_lines[line];
#endif
#ifdef SC64_AUDIO_IMPORTED
#ifdef SC64_AUDIO_ALERT_LINES
    _Static_assert(SC64_AUDIO_ALERT_LINES == GAME_AUDIO_ALERT_KINDS, "alert table and observer lines differ");
    u16 sound = sc64_audio_alert[GAME_AUDIO_PLAYER_RACE * SC64_AUDIO_ALERT_LINES + line];
    if (sound != SC64_AUDIO_ALERT_NONE) audio_play_sample(sound, 0, AUDIO_CLASS_ADVISOR);
#endif
#else
    audio_tone(line <= GAME_AUDIO_ALERT_FORCES ? 440 : 600, 8, 2);
#endif
    return 1;
}

/* Simulation side: a hostile hit reached a player actor (hit_target_impl_ex,
 * after its early returns). Only the source owner makes it hostile: burn,
 * traps and other damage with no owner never come through here. Hits on screen
 * are the player's own fight and stay silent. Presentation state only. */
static void __attribute__((noinline)) game_audio_note_hit(int target, int source_owner)
{
    int x, y, bit;
    if (source_owner < 0 || source_owner > 4 || source_owner == SIDE || source_owner == NEUTRAL ||
        (unsigned)target >= MAX_UNITS + MAX_BUILDINGS ||
        !(GAME_AUDIO_HEARD(GAME_AUDIO_ALERT_BASE) | GAME_AUDIO_HEARD(GAME_AUDIO_ALERT_FORCES))) return;
    if (target < MAX_UNITS) {
        const Unit *q = &u[target];
        if (q->owner != SIDE) return;
        x = q->x; y = q->y; bit = GAME_AUDIO_PENDING_UNIT;
    } else {
        const Building *q = &b[target - MAX_UNITS];
        if (q->owner != SIDE) return;
        x = q->x; y = q->y; bit = GAME_AUDIO_PENDING_BUILDING;
    }
    if (!game_audio_on_screen(x, y)) GAME_AUDIO_STATE[GAME_AUDIO_STATE_PENDING] |= (u16)bit;
}

#ifdef SC64_RACES
/* A spell was refused for energy: cue the advisor if a selected caster of this
 * raw type (any type when raw is -1, and then only the first selected unit
 * counts) lacks the energy cost. Carry holds the energy. */
static void game_audio_energy_refused(int raw, int cost)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].selected && (raw < 0 || sc64_race_raw_unit(u[i].type) == raw)) {
            if (u[i].carry < cost) { toast_advisor = TOAST_CUE_ENERGY; break; }
            if (raw < 0) break;
        }
}
#endif

/* Simulation side: the player's research or upgrade just finished. Scripted
 * grants never call this. */
static void game_audio_cue(int cue)
{
    if (GAME_AUDIO_HEARD(cue == GAME_AUDIO_CUE_RESEARCH ? GAME_AUDIO_ALERT_RESEARCH : GAME_AUDIO_ALERT_UPGRADE))
        GAME_AUDIO_STATE[GAME_AUDIO_STATE_PENDING] |= (u16)(GAME_AUDIO_PENDING_RESEARCH << cue);
}

static void game_audio_emit(int kind, int raw)
{
#if defined(SC64_AUDIO_IMPORTED) && !defined(SC64_AUDIO_WARP_CUES)
    /* Older banks have no start clip; silence must not spend effect budget. */
    if (kind == GAME_AUDIO_WARP_START) {
#ifdef GBA_HOST
        ++game_audio_events[kind];
#endif
        return;
    }
#endif
    int speech = kind == GAME_AUDIO_READY || kind == GAME_AUDIO_WHAT || kind == GAME_AUDIO_YES;
    if (!speech) {
        if (game_audio_effects >= GAME_AUDIO_EFFECTS_PER_FRAME) return;
        ++game_audio_effects;
    }
#ifdef GBA_HOST
    ++game_audio_events[kind];
#endif
#ifdef SC64_AUDIO_IMPORTED
    static const u8 bank_kind[] = {
        SC64_AUDIO_ATTACK, SC64_AUDIO_DEATH, SC64_AUDIO_READY, SC64_AUDIO_WHAT,
        SC64_AUDIO_YES, SC64_AUDIO_BUILT
    };
    static const u8 priority[] = { 1, 3, 2, 2, 2, 2 };
    if (kind == GAME_AUDIO_WARP_START || kind == GAME_AUDIO_WARP_DONE) {
#ifdef SC64_AUDIO_WARP_CUES
        audio_play_sample(sc64_audio_warp[kind == GAME_AUDIO_WARP_DONE], 2, 0);
        return;
#else
        if (kind == GAME_AUDIO_WARP_START) return;
        kind = GAME_AUDIO_BUILT; /* Older imported banks keep their fallback. */
#endif
    }
    int sound = -1;
    if (raw >= 0 && raw < SC64_AUDIO_UNIT_COUNT) {
        int row = raw * SC64_AUDIO_KIND_COUNT, k = bank_kind[kind];
        /* Buildings have no completion sound; their selection hum stands in. */
        if (kind == GAME_AUDIO_BUILT && !sc64_audio_unit_count[row + k]) k = SC64_AUDIO_WHAT;
        int count = sc64_audio_unit_count[row + k];
        if (count) sound = sc64_audio_unit_sound[sc64_audio_unit_first[row + k] +
                                                 game_audio_variant++ % count];
    }
    int advisor = 0;
#ifdef SC64_AUDIO_ALERT_LINES
    /* The N64 says its own line when a Terran building or add-on finishes: an
     * advisor line, so it waits its turn. With the voice taken the building
     * keeps its selection hum. */
    if (kind == GAME_AUDIO_BUILT && GAME_AUDIO_HEARD(GAME_AUDIO_ALERT_BUILT) && game_audio_voice_free()) {
        int built = sc64_audio_alert[GAME_AUDIO_PLAYER_RACE * SC64_AUDIO_ALERT_LINES + GAME_AUDIO_ALERT_BUILT];
        if (built != SC64_AUDIO_ALERT_NONE) {
            sound = built;
            advisor = game_audio_advised = 1;
#ifdef GBA_HOST
            game_audio_last_alert = GAME_AUDIO_ALERT_BUILT;
            ++game_audio_alert_lines[GAME_AUDIO_ALERT_BUILT];
#endif
        }
    }
#endif
    /* Units without a sample in the bank stay silent rather than beep. */
    if (sound >= 0)
        audio_play_sample((u16)sound, advisor ? 0 : priority[kind], advisor ? AUDIO_CLASS_ADVISOR : speech ? 1 : 0);
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

/* Called only after a real foundation allocation; no per-slot generation state. */
static void game_audio_foundation(const Building *q)
{
#if defined(SC64_RACES) && !defined(GAME_AUDIO_OFF)
    const Sc64RaceBuildingRule *rule = sc64_race_building_rule(q->type);
    if (game_audio_needs_prime || screen != SCREEN_PLAY || q->owner != SIDE ||
        !rule || rule->race != R_PROTOSS || !game_audio_on_screen(q->x, q->y) ||
        game_audio_pending_warp >= 0) return;
    game_audio_pending_warp = game_audio_building_raw(q);
#else
    (void)q;
#endif
}

/* One refusal cue per observed frame; the mixer plays one advisor line at a
 * time, so presses while it speaks are dropped and the next one replays it. */
static void game_audio_advisor(int cue)
{
    int line = cue == 1 ? 0 : cue == 12 ? 1 : 2;  /* minerals, gas, supply */
    if (cue >= TOAST_CUE_ENERGY) {
        (void)game_audio_alert(cue == TOAST_CUE_ENERGY ? GAME_AUDIO_ALERT_ENERGY :
                               cue == TOAST_CUE_NO_BUILD ? GAME_AUDIO_ALERT_NO_BUILD : GAME_AUDIO_ALERT_NO_LAND);
        return;
    }
    game_audio_advised = 1;
#ifdef GBA_HOST
    ++game_audio_events[GAME_AUDIO_ADVISOR];
#endif
#ifdef SC64_AUDIO_IMPORTED
#ifdef SC64_AUDIO_ADVISOR_LINES
    audio_play_sample(sc64_audio_advisor[GAME_AUDIO_PLAYER_RACE * SC64_AUDIO_ADVISOR_LINES + line],
                      0, AUDIO_CLASS_ADVISOR);
#endif
    /* A bank from before the advisor table stays silent. */
    return;
#else
    audio_tone(line == 0 ? 330 : line == 1 ? 294 : 262, 8, 2);
#endif
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

/* Plays what the simulation reported since the last observed frame. Attack
 * alerts: base before forces, one per GAME_AUDIO_ATTACK_ALERT_TICKS of game
 * time shared by both, and a hit inside that time is dropped, not postponed
 * (a fight that goes on raises it again). The wait starts when a match
 * starts or a save loads (see game_audio_observe), so neither plays an alert
 * for the first 15 s. A discontinuity discards what was reported. */
static void game_audio_pending(int fresh)
{
    volatile u16 *state = GAME_AUDIO_STATE;
    u32 pending = state[GAME_AUDIO_STATE_PENDING];
    /* Keep the age below its 16-bit wrap, so a long quiet spell never reads as recent. */
    if ((u16)((u32)frame - state[GAME_AUDIO_STATE_ATTACK_TICK]) >= 0x8000u)
        state[GAME_AUDIO_STATE_ATTACK_TICK] = (u16)((u32)frame - 0x8000u);
    if (!pending) return;
    if (fresh) { state[GAME_AUDIO_STATE_PENDING] = 0; return; }
    if (pending & (GAME_AUDIO_PENDING_UNIT | GAME_AUDIO_PENDING_BUILDING)) {
        if ((u16)((u32)frame - state[GAME_AUDIO_STATE_ATTACK_TICK]) >= GAME_AUDIO_ATTACK_ALERT_TICKS &&
            game_audio_alert(pending & GAME_AUDIO_PENDING_BUILDING ? GAME_AUDIO_ALERT_BASE : GAME_AUDIO_ALERT_FORCES))
            state[GAME_AUDIO_STATE_ATTACK_TICK] = (u16)frame;
        pending &= ~(u32)(GAME_AUDIO_PENDING_UNIT | GAME_AUDIO_PENDING_BUILDING);
    }
    /* One announcement however many finished in the same update. */
    if ((pending & GAME_AUDIO_PENDING_RESEARCH) && game_audio_alert(GAME_AUDIO_ALERT_RESEARCH))
        pending &= ~(u32)GAME_AUDIO_PENDING_RESEARCH;
    if ((pending & GAME_AUDIO_PENDING_UPGRADE) && game_audio_alert(GAME_AUDIO_ALERT_UPGRADE))
        pending &= ~(u32)GAME_AUDIO_PENDING_UPGRADE;
    state[GAME_AUDIO_STATE_PENDING] = (u16)pending;
}

static void game_audio_observe(void)
{
    volatile u16 *snap = GAME_AUDIO_SNAPSHOT;
    int fresh, i;
    u32 selection = 0, orders = 0;
    int first_selected = -1;
    u16 held = GAME_AUDIO_HELD(), pressed = held & (u16)~game_audio_previous_held;
    int advisor = toast_advisor;
    game_audio_previous_held = held;
    toast_advisor = 0;
#ifdef GAME_AUDIO_OFF
    /* Differential test builds: the same simulation with no observer work. */
    (void)snap; (void)fresh; (void)i; (void)selection; (void)orders; (void)first_selected;
    (void)pressed; (void)advisor;
    return;
#endif
#ifdef SC64_AUDIO_IMPORTED
    game_audio_music();
#endif
    int same_play_frame = screen == SCREEN_PLAY && game_audio_last_screen == SCREEN_PLAY &&
                          frame == game_audio_last_frame;
    fresh = game_audio_needs_prime || screen != SCREEN_PLAY || game_audio_last_screen != SCREEN_PLAY ||
            frame != game_audio_last_frame + 1;
    game_audio_last_screen = screen;
    if (screen != SCREEN_PLAY) return;
    if (frame == game_audio_last_frame && !game_audio_needs_prime) {
        /* Menus can issue an explicit refusal without advancing simulation.
         * Consume that new cue now, without rescanning frame-based events. */
        if (same_play_frame && advisor) game_audio_advisor(advisor);
        return;
    }
    game_audio_last_frame = frame;
    /* A new match, restart or load starts the attack wait from this frame. */
    if (game_audio_needs_prime) GAME_AUDIO_STATE[GAME_AUDIO_STATE_ATTACK_TICK] = (u16)frame;
    game_audio_needs_prime = 0;
    game_audio_effects = 0;
    game_audio_advised = 0;
    if (!fresh && game_audio_pending_warp >= 0)
        game_audio_emit(GAME_AUDIO_WARP_START, game_audio_pending_warp);
    game_audio_pending_warp = -1;
    if (advisor && !fresh) game_audio_advisor(advisor);
    game_audio_pending(fresh);
    for (i = 0; i < MAX_UNITS; i++) {
        const Unit *q = &u[i];
        u32 prev = snap[i];
        if (!q->alive && !prev) continue;
        u32 cd = q->cd > 255 ? 255u : (u32)q->cd;
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
        if (!q->alive && !prev) continue;
        u32 now = q->alive ? GAME_AUDIO_SNAPSHOT_ALIVE |
                  (q->under_construction ? GAME_AUDIO_SNAPSHOT_CONSTRUCTING : 0) : 0;
        if (!fresh) {
            if ((prev & GAME_AUDIO_SNAPSHOT_ALIVE) && !q->alive) {
                if (game_audio_on_screen(q->x, q->y))
                    game_audio_emit(GAME_AUDIO_DEATH, game_audio_building_raw(q));
            }
            else if (q->alive && (prev & GAME_AUDIO_SNAPSHOT_ALIVE) &&
                     (prev & GAME_AUDIO_SNAPSHOT_CONSTRUCTING) && !q->under_construction && q->owner == SIDE) {
#ifdef SC64_RACES
                if (sc64_race_building_rule(q->type) && sc64_race_building_rule(q->type)->race == R_PROTOSS)
                    game_audio_emit(GAME_AUDIO_WARP_DONE, game_audio_building_raw(q));
                else
#endif
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
