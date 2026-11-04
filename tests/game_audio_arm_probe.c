/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * ARM probe for advisor alerts (replaces common/main.c).
 *
 * The probe links a race profile's flags and bank, puts a player unit and a
 * building in a fixture, then edits only those fixture fields and runs the
 * shipped observer and mixer interrupt: each alert must request the player
 * race's clip from the bank. audio_play_sample is wrapped to see the request
 * and still plays it. Results go to the mGBA debug log; no debugger reads or
 * writes game state. Emulator evidence only: it shows requests and the
 * decoding budget, not gameplay and not what a person hears.
 */
#include "../starcraft/game.c"

volatile u16 demo_keys;
extern volatile u32 audio_irq_peak;

#define DBG_ENABLE (*(volatile u16 *)0x04FFF780)
#define DBG_FLAGS (*(volatile u16 *)0x04FFF700)
#define DBG_TEXT ((volatile char *)0x04FFF600)

static char line[200];
static unsigned line_len;
static int last_sample = -1, last_kind = -1;
static u32 requests, failures;

void __real_audio_play_sample(u16 sample, u8 priority, u8 kind);
void __wrap_audio_play_sample(u16 sample, u8 priority, u8 kind);
void __wrap_audio_play_sample(u16 sample, u8 priority, u8 kind)
{
    last_sample = sample;
    last_kind = kind;
    ++requests;
    __real_audio_play_sample(sample, priority, kind);
}

static void out_text(const char *s) { while (*s && line_len < sizeof(line) - 1) line[line_len++] = *s++; }
static void out_num(long v)
{
    char tmp[12]; int n = 0; unsigned long m;
    if (v < 0) { out_text("-"); m = (unsigned long)(-v); } else m = (unsigned long)v;
    do { tmp[n++] = (char)('0' + m % 10u); m /= 10u; } while (m && n < 11);
    while (n) { char c[2] = { tmp[--n], 0 }; out_text(c); }
}
static void out_flush(void)
{
    for (unsigned i = 0; i < line_len; ++i) DBG_TEXT[i] = line[i];
    DBG_TEXT[line_len] = 0;
    DBG_FLAGS = 0x100 | 2;
    line_len = 0;
}

/* gfx_end() waits for the frame, so the mixer interrupt runs between calls. */
static void frame_wait(void)
{
    gfx_begin(C_BLACK);
    gfx_end();
}

static void observe_next(void)
{
    ++frame;
    game_audio_observe();
    audio_update();
    frame_wait();
}

/* Asserts this line's clip was requested, then waits out the advisor voice. */
static void expect(const char *name, int alert)
{
    int want = sc64_audio_alert[GAME_AUDIO_PLAYER_RACE * SC64_AUDIO_ALERT_LINES + alert];
    int silent = want == SC64_AUDIO_ALERT_NONE;
    int ok = silent ? last_sample == -1 : last_sample == want && last_kind == AUDIO_CLASS_ADVISOR;
    if (!ok) ++failures;
    out_text("ALERT race="); out_num(GAME_AUDIO_PLAYER_RACE);
    out_text(" "); out_text(name);
    out_text(" sample="); out_num(last_sample);
    out_text(" want="); out_num(silent ? -1 : want);
    out_text(ok ? " ok" : " FAIL");
    out_flush();
    for (int i = 0; i < 300; ++i) observe_next();
    last_sample = last_kind = -1;
}

static void fixture(void)
{
    memset(u, 0, sizeof u);
    memset(b, 0, sizeof b);
    selected_building = -1;
    menu_open = 0;
    mode = MODE_NORMAL;
    screen = SCREEN_PLAY;
    camx = camy = 0;
    u[10].alive = 1; u[10].owner = SIDE; u[10].x = 900; u[10].y = 900;
    u[10].hp = 30; u[10].maxhp = 40; u[10].target = -1; u[10].raw_type = -1;
    b[0].alive = 1; b[0].owner = SIDE; b[0].type = B_COMMAND; b[0].x = 900; b[0].y = 700;
    b[0].hp = 1000; b[0].maxhp = 1500; b[0].raw_type = -1;
    weapons_level = armor_level = weapons_research = armor_research = 0;
    frame = 1000;
    game_audio_observe();
    for (int k = 0; k <= GAME_AUDIO_ATTACK_ALERT_TICKS; ++k) observe_next();
    last_sample = last_kind = -1;
}

static void run_race(int race)
{
    race_human = race;
    fixture();
    u[10].hp = 20; observe_next(); expect("forces", GAME_AUDIO_ALERT_FORCES);
    for (int k = 0; k < GAME_AUDIO_ATTACK_ALERT_TICKS; ++k) observe_next();
    last_sample = last_kind = -1;
    b[0].hp = 900; observe_next(); expect("base", GAME_AUDIO_ALERT_BASE);
    weapons_research = 600; observe_next();
    weapons_research = 0; ++weapons_level; observe_next(); expect("upgrade", GAME_AUDIO_ALERT_UPGRADE);
    toast_advisor = TOAST_CUE_ENERGY; observe_next(); expect("energy", GAME_AUDIO_ALERT_ENERGY);
    toast_advisor = TOAST_CUE_NO_BUILD; observe_next(); expect("no_build", GAME_AUDIO_ALERT_NO_BUILD);
    toast_advisor = TOAST_CUE_NO_LAND; observe_next(); expect("no_land", GAME_AUDIO_ALERT_NO_LAND);
}

/* The observer's worst case: every slot alive and off screen, nothing changing. */
static void measure_observer(void)
{
    u32 peak = 0;
    uint64_t total = 0;
    fixture();
    for (int i = 0; i < MAX_UNITS; ++i) { u[i] = u[10]; u[i].selected = 0; }
    for (int i = 0; i < MAX_BUILDINGS; ++i) b[i] = b[0];
    observe_next();
    for (int k = 0; k < 120; ++k) {
        u32 start;
        ++frame;
        start = gba_clock();
        game_audio_observe();
        start = gba_clock() - start;
        total += start;
        if (start > peak) peak = start;
        frame_wait();
    }
    out_text("ALERT OBSERVER units="); out_num(MAX_UNITS);
    out_text(" buildings="); out_num(MAX_BUILDINGS);
    out_text(" avg_cycles="); out_num((long)(total / 120u));
    out_text(" peak_cycles="); out_num((long)peak);
    out_flush();
}

int main(void)
{
    DBG_ENABLE = 0xC0DE;
    gba_init();
    game_init();
    out_text("ALERT PROBE start"); out_flush();
    run_race(R_TER);
    run_race(R_ZERG);
    run_race(R_PROTOSS);
    measure_observer();
    out_text("ALERT PROBE done failures="); out_num((long)failures);
    out_text(" requests="); out_num((long)requests);
    out_text(" irq_peak="); out_num((long)audio_irq_peak);
    out_flush();
    for (;;) frame_wait();
}
