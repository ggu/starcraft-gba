/* Focused deterministic fixtures for the authored Backwater trigger rules. */
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_backwater.h"

typedef struct {
    struct { int player, type, location, count; } units[64];
    int unit_count;
    int ore, gas, victory, defeats;
    int fail_spawn;
    int messages[15][16];
    int centers[65];
    int invincible;
} Fixture;

static int
fixture_count(void *context, int player, int type, int location)
{
    Fixture *f = (Fixture *)context;
    int i, total = 0;
    for (i = 0; i < f->unit_count; i++) {
        if ((f->units[i].player != player && player != SC64_BW_ALL_PLAYERS) ||
            (location != SC64_BW_LOC_ANYWHERE &&
             f->units[i].location != location))
            continue;
        if (type == SC64_BW_ANY_UNIT ||
            (type == SC64_BW_MEN && f->units[i].type < 106) ||
            (type == SC64_BW_BUILDINGS && f->units[i].type >= 106) ||
            (type == SC64_BW_FACTORIES && f->units[i].type >= 106) ||
            f->units[i].type == type) total += f->units[i].count;
    }
    return total;
}

static int
fixture_spawn(void *context, int player, int type, int location, int count)
{
    Fixture *f = (Fixture *)context;
    int i, made = f->fail_spawn ? 0 : count;
    if (!made) return 0;
    for (i = 0; i < f->unit_count; i++)
        if (f->units[i].player == player && f->units[i].type == type &&
            f->units[i].location == location) {
            f->units[i].count += made;
            return made;
        }
    if (f->unit_count == 64) return 0;
    f->units[f->unit_count].player = player;
    f->units[f->unit_count].type = type;
    f->units[f->unit_count].location = location;
    f->units[f->unit_count].count = made;
    f->unit_count++;
    return made;
}

static void fixture_resources(void *c, int ore, int gas)
{ Fixture *f = (Fixture *)c; f->ore = ore; f->gas = gas; }
static void fixture_invincible(void *c, int p, int t, int l, int on)
{ Fixture *f = (Fixture *)c; (void)p; (void)t; (void)l; f->invincible = on; }
static void fixture_center(void *c, int l)
{ Fixture *f = (Fixture *)c; if (l >= 0 && l < 65) f->centers[l]++; }
static void fixture_result(void *c, int victory)
{ Fixture *f = (Fixture *)c; if (victory) f->victory++; else f->defeats++; }
static void fixture_message(void *c, int t, int a)
{ Fixture *f = (Fixture *)c; if (t >= 0 && t < 15 && a >= 0 && a < 16) f->messages[t][a]++; }
static void fixture_kill(void *c, int p, int t, int l)
{ (void)c; (void)p; (void)t; (void)l; }
static void fixture_give(void *c, int from, int to, int type, int loc)
{ (void)c; (void)from; (void)to; (void)type; (void)loc; }

static Sc64BackwaterWorld
world_for(Fixture *f)
{
    Sc64BackwaterWorld w;
    memset(&w, 0, sizeof(w));
    w.context = f;
    w.count = fixture_count;
    w.spawn = fixture_spawn;
    w.kill = fixture_kill;
    w.give = fixture_give;
    w.resources = fixture_resources;
    w.invincible = fixture_invincible;
    w.center = fixture_center;
    w.result = fixture_result;
    w.message = fixture_message;
    return w;
}

static int
check_init_and_rescue_retry(void)
{
    Fixture f;
    Sc64BackwaterState s;
    Sc64BackwaterWorld w;
    memset(&f, 0, sizeof(f)); w = world_for(&f);
    sc64_backwater_init(&s, &w);
    if (f.ore != 200 || f.gas != 200 || sc64_backwater_objective(&s)) return 0;
    fixture_spawn(&f, SC64_BW_SIDE, SC64_BW_BUNKER,
                  SC64_BW_LOC_FIREBAT_BUNKER_2, 1);
    f.fail_spawn = 1;
    sc64_backwater_update(&s, &w, 1);
    if (fixture_count(&f, SC64_BW_OUTPOST, SC64_BW_FIREBAT,
                      SC64_BW_LOC_FIREBAT_BUNKER_2) != 0 || !s.spawn_trigger)
        return 0;
    f.fail_spawn = 0;
    sc64_backwater_update(&s, &w, 2);
    return fixture_count(&f, SC64_BW_OUTPOST, SC64_BW_FIREBAT,
                         SC64_BW_LOC_FIREBAT_BUNKER_2) == 2 &&
           (s.executed & (1u << 3));
}

static int
check_scv_bunker_wait(void)
{
    Fixture f;
    Sc64BackwaterState s;
    Sc64BackwaterWorld w;
    int i;
    memset(&f, 0, sizeof(f)); w = world_for(&f);
    sc64_backwater_init(&s, &w);
    fixture_spawn(&f, SC64_BW_SIDE, SC64_BW_BUNKER,
                  SC64_BW_LOC_SCV_BUNKER, 1);
    sc64_backwater_update(&s, &w, 1);
    if (s.timer[4] != SC64_BW_WAIT_TICKS(200) || s.phase[4] != 1 ||
        f.centers[SC64_BW_LOC_SCV_BUNKER] != 1 ||
        fixture_count(&f, SC64_BW_OUTPOST, SC64_BW_SCV,
                      SC64_BW_LOC_SCV_BUNKER)) return 0;
    for (i = 0; i < (int)SC64_BW_WAIT_TICKS(200) - 1; i++)
        sc64_backwater_update(&s, &w, (u32)i + 2);
    if (s.phase[4] != 1 || fixture_count(&f, SC64_BW_OUTPOST, SC64_BW_SCV,
                                         SC64_BW_LOC_SCV_BUNKER)) return 0;
    sc64_backwater_update(&s, &w, 20);
    if (s.phase[4] != 3 || s.timer[4] != SC64_BW_WAIT_TICKS(2000) ||
        fixture_count(&f, SC64_BW_OUTPOST, SC64_BW_SCV,
                      SC64_BW_LOC_SCV_BUNKER) != 2 || f.messages[4][5] != 1) return 0;
    return 1;
}

static int
check_factory_gate_and_victory(void)
{
    Fixture f;
    Sc64BackwaterState s;
    Sc64BackwaterWorld w;
    int i;
    memset(&f, 0, sizeof(f)); w = world_for(&f);
    sc64_backwater_init(&s, &w);
    fixture_spawn(&f, SC64_BW_FORCE_2, SC64_BW_INFESTED_CC,
                  SC64_BW_LOC_INFESTED_BUILDING, 1);
    fixture_spawn(&f, SC64_BW_SIDE, SC64_BW_RAYNOR_VULTURE,
                  SC64_BW_LOC_ANYWHERE, 1);
    fixture_spawn(&f, SC64_BW_SIDE, SC64_BW_MARINE,
                  SC64_BW_LOC_INFESTED_BUILDING, 1);
    sc64_backwater_update(&s, &w, 1);
    /* The reveal runs in the same trigger pass because the unit that reveals
     * the factory also satisfies trigger 10's bring condition. */
    if (s.switches || s.outcome || f.messages[10][2] != 1) return 0;
    /* The win sequence cannot start until the Force 2 Infested CC is gone. */
    if (s.phase[14]) return 0;
    f.units[0].count = 0;
    sc64_backwater_update(&s, &w, 2);
    if (s.timer[14] != SC64_BW_WAIT_TICKS(4000)) return 0;
    for (i = 0; i < (int)SC64_BW_WAIT_TICKS(4000) - 1; i++)
        sc64_backwater_update(&s, &w, (u32)i + 3);
    if (s.phase[14] != 1 || s.timer[14] != 1 || f.victory || !f.invincible) return 0;
    sc64_backwater_update(&s, &w, 5000);
    if (s.phase[14] != 2 || s.timer[14] != SC64_BW_WAIT_TICKS(1000) ||
        f.centers[SC64_BW_LOC_RAYNOR] != 1 || f.messages[14][5]) return 0;
    for (i = 0; i < (int)SC64_BW_WAIT_TICKS(1000) - 1; i++)
        sc64_backwater_update(&s, &w, (u32)i + 5001);
    if (s.phase[14] != 2 || s.timer[14] != 1 || f.messages[14][5]) return 0;
    sc64_backwater_update(&s, &w, 6000);
    if (s.phase[14] != 3 || s.timer[14] != SC64_BW_WAIT_TICKS(500) ||
        f.messages[14][5] != 1 || f.messages[14][9] != 1 || f.victory) return 0;
    for (i = 0; i < (int)SC64_BW_WAIT_TICKS(500) - 1; i++)
        sc64_backwater_update(&s, &w, (u32)i + 6001);
    if (f.victory) return 0;
    sc64_backwater_update(&s, &w, 7000);
    return f.victory == 1 && !f.defeats &&
           sc64_backwater_outcome(&s) == SC64_BACKWATER_WON &&
           sc64_backwater_objective(&s);
}

static int
check_raynor_defeat(void)
{
    Fixture f;
    Sc64BackwaterState s;
    Sc64BackwaterWorld w;
    int i;
    memset(&f, 0, sizeof(f)); w = world_for(&f);
    sc64_backwater_init(&s, &w);
    sc64_backwater_update(&s, &w, 0);
    if (s.timer[6] != SC64_BW_WAIT_TICKS(1000)) return 0;
    for (i = 0; i < (int)SC64_BW_WAIT_TICKS(1000) - 1; i++)
        sc64_backwater_update(&s, &w, (u32)i + 1);
    if (f.messages[6][2] || s.timer[6] != 1) return 0;
    sc64_backwater_update(&s, &w, 40);
    if (f.messages[6][2] != 1 || s.timer[6] != SC64_BW_WAIT_TICKS(500) || f.defeats) return 0;
    for (i = 0; i < (int)SC64_BW_WAIT_TICKS(500) - 1; i++)
        sc64_backwater_update(&s, &w, (u32)i + 41);
    if (f.defeats) return 0;
    sc64_backwater_update(&s, &w, 60);
    return f.defeats == 1 && !f.victory &&
           sc64_backwater_outcome(&s) == SC64_BACKWATER_LOST &&
           f.messages[6][2] == 1;
}

int
main(void)
{
    if (!check_init_and_rescue_retry()) { fputs("Backwater rescue rules failed\n", stderr); return 1; }
    if (!check_scv_bunker_wait()) { fputs("Backwater SCV rescue wait failed\n", stderr); return 1; }
    if (!check_factory_gate_and_victory()) { fputs("Backwater factory/win rules failed\n", stderr); return 1; }
    if (!check_raynor_defeat()) { fputs("Backwater Raynor loss rules failed\n", stderr); return 1; }
    puts("Backwater rules passed");
    return 0;
}
