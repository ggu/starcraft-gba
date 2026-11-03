/* The race HUD's selection panel, in the manner of StarCraft 64's status bar.
 *
 * A portrait with shield and health bars under it, the name and hit points,
 * then what the selection is doing as icons and numbers: a building's
 * production queue or research with its progress bar, a unit's upgrades,
 * energy and cargo, a group by type. A key cap and a mark stand in for the old
 * hint text. The redraw key (hud_panel_key) folds into one existing HUD key,
 * and the panel adds no saved or static state.
 *
 * A panel redraw is costly, so it stays rare. Progress bars are drawn again
 * each frame over the cached band (hud_panel_bars), which costs far less than
 * redrawing the band for each step of the bar. */
#ifndef SC64_HUD_ICONS
#error "hud_panel.h needs the imported HUD icons"
#endif

enum {
    HUD_PORTRAIT_X = 3, HUD_PORTRAIT_Y = 122, HUD_COLUMN = 30, HUD_RIGHT = 178,
    HUD_NAME_Y = 121, HUD_NUMBER_Y = 122, HUD_SLOT_Y = 130, HUD_SLOT_PITCH = 21,
    HUD_QUEUE_SLOTS = 5, HUD_BAR_Y = 151, HUD_BAR_H = 4,
    HUD_BAR_W = HUD_QUEUE_SLOTS * HUD_SLOT_PITCH - 1,
    HUD_FULL_BAR_W = HUD_RIGHT - HUD_COLUMN + 1, HUD_BAR_LEVELS = HUD_BAR_W,
    HUD_CHIP_X = 137, HUD_CHIP_KEYS_Y = 151, HUD_HINT_Y = 141, HUD_GROUP_HINT_Y = 150,
    HUD_CLOCK_Y = 151, HUD_GROUP_CELLS = 7
};

static int
hud_level(int value, int maximum, int levels)
{
    if (value <= 0 || maximum <= 0) return 0;
    return value >= maximum ? levels : value * levels / maximum;
}

/* ---- What a selected building is doing -------------------------------- */

/* Icons in production order. A queue shares one bar; separate research jobs
 * each fill their own, under their slot. */
typedef struct {
    int count, separate, eggs, level[2];
    short icon[HUD_QUEUE_SLOTS];
} HudJobs;

static void
hud_job_level(HudJobs *jobs, int job, int value, int maximum)
{
    jobs->level[job] = hud_level(value, maximum, jobs->separate ? CARD_CELL : HUD_BAR_LEVELS);
}

static void hud_jobs_bars(const HudJobs *jobs);

static int
hud_research_building(int type)
{
    return type == (race_human == R_ZERG ? B_EVOLUTION_CHAMBER : race_human == R_PROTOSS ? B_FORGE : B_ENG_BAY);
}

#ifdef SC64_Z7_EGG_PRODUCTION
static int
hud_egg_hall(const Building *q)
{
    return race_z7_active() && sc64_race_is_zerg_townhall(q->type);
}

/* Z7 Hatcheries spawn Eggs instead of queueing. The ones beside this hall
 * are its queue, soonest first. The search is too slow for every frame, so
 * their bar is drawn and redrawn with the band. */
static int
hud_egg_jobs(const Building *q, HudJobs *jobs)
{
    int i, remaining = 0;
    if (!hud_egg_hall(q)) return 0;
    jobs->eggs = 1;
    for (i = 0; i < MAX_UNITS && jobs->count < HUD_QUEUE_SLOTS; i++) {
        int left, duration;
        if (!u[i].alive || u[i].owner != SIDE || u[i].type != U_Z7_EGG ||
            dist2(u[i].x, u[i].y, q->x, q->y) > 64 * 64) continue;
        duration = sc64_z7_egg_duration(u[i].target);
        left = u[i].work_timer;
        jobs->icon[jobs->count] = (short)card_portrait(sc64_race_raw_unit(u[i].target));
        if (!jobs->count || left < remaining) {
            short first = jobs->icon[0];
            jobs->icon[0] = jobs->icon[jobs->count];
            jobs->icon[jobs->count] = first;
            remaining = left;
            hud_job_level(jobs, 0, duration - left, duration);
        }
        jobs->count++;
    }
    return 1;
}
#endif

static void
hud_building_jobs(int slot, HudJobs *jobs, int with_eggs)
{
    const Building *q = &b[slot];
    int i;
    jobs->count = jobs->separate = jobs->eggs = jobs->level[0] = jobs->level[1] = 0;
    if (q->queue_count > 0 && q->queue_count <= HUD_QUEUE_SLOTS) {
        int head = q->queue[q->queue_head];
        for (i = 0; i < q->queue_count; i++)
            jobs->icon[i] = (short)card_portrait(sc64_race_raw_unit(q->queue[(q->queue_head + i) % 5]));
        jobs->count = q->queue_count;
        hud_job_level(jobs, 0, q->queue_progress, unit_build_time(head));
        return;
    }
    if (q->under_construction) return;
#ifdef SC64_Z7_EGG_PRODUCTION
    if (with_eggs && hud_egg_jobs(q, jobs)) return;
#endif
    (void)with_eggs;
#ifdef SC64_BATTLECRUISER_RESEARCH_EXPERIMENT
    if (sc64_race_raw_building(q->type) == 118 && q->level) { /* a Physics Lab's one job */
        int duration = (int)sc64_battlecruiser_research_duration((unsigned)q->level);
        jobs->icon[0] = (short)card_portrait(12);
        jobs->count = 1;
        hud_job_level(jobs, 0, duration - q->queue_progress, duration);
        return;
    }
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (race_z6_active() && race_z6.transport.ticks_remaining &&
        race_z6.transport.producer_slot_plus1 == (unsigned)slot + 1u) {
        jobs->icon[0] = SC64_CMD_ICON_VENTRAL_SACS;
        jobs->count = 1;
        hud_job_level(jobs, 0, (int)SC64_Z6_TRANSPORT_RESEARCH_TICKS - race_z6.transport.ticks_remaining,
                      (int)SC64_Z6_TRANSPORT_RESEARCH_TICKS);
        return;
    }
#endif
#ifdef SC64_SKIRMISH_TRANSPORT_TECH
    if (race_mode) {
        const Sc64SkTransportJob *job = &race_skirmish_transport_tech.owner[sc64_sk_transport_owner_index(SIDE)];
        if (job->remaining && job->producer_plus_one == slot + 1) {
            jobs->icon[0] = SC64_CMD_ICON_VENTRAL_SACS;
            jobs->count = 1;
            hud_job_level(jobs, 0, SC64_SK_TRANSPORT_RESEARCH_TICKS - job->remaining,
                          SC64_SK_TRANSPORT_RESEARCH_TICKS);
            return;
        }
    }
#endif
    if (hud_research_building(q->type)) {
        jobs->separate = 1;
        jobs->count = 2;
        jobs->icon[0] = (short)card_weapons_icon();
        jobs->icon[1] = (short)card_armor_icon();
        hud_job_level(jobs, 0, weapons_research, 600);
        hud_job_level(jobs, 1, armor_research, 600);
    }
}

/* The unit A would train here, as interact() resolves it, or -1. Read-only:
 * interact() itself still corrects train_type when A is pressed. */
static int
hud_next_product(int slot)
{
    const Building *q = &b[slot];
    const Sc64RaceUnitRule *rule;
    int type = q->type;
    if (q->under_construction) return -1;
    if (race_mode) {
        unsigned producer = (unsigned)(sc64_race_is_zerg_townhall(type) ? 35 : sc64_race_raw_building(type));
        int raw = sc64_race_raw_unit(train_type);
        unsigned count = sc64_skirmish_product_count(producer), p;
        int product = selected_building;
        for (p = 0; p < count; p++) {
            const Sc64SkirmishRecord *candidate = sc64_skirmish_product_at(producer, p);
            if (candidate->raw_id == raw && race_skirmish_product_available(candidate, &product))
                return train_type;
        }
        raw = sc64_skirmish_menu_next_product(producer, SC64_SK_NONE, race_skirmish_product_available, &product);
        return raw == SC64_SK_NONE ? -1 : sc64_race_unit_from_raw(raw);
    }
    if (type == B_COMMAND) return U_SCV;
    if (type == B_NEXUS) return U_PROBE;
    if (type == B_GATEWAY) return train_type == U_DRAGOON ? U_DRAGOON : U_ZEALOT;
    if (type == B_BARRACKS)
        return train_type == U_FIREBAT || train_type == U_MEDIC ? train_type : U_MARINE;
#ifdef SC64_T5_RUNTIME
    if (type == B_FACTORY)
        return train_type == U_GOLIATH || train_type == U_TANK ? train_type : U_VULTURE;
    if (type == B_STARPORT) return train_type == U_DROPSHIP ? U_DROPSHIP : U_WRAITH;
#endif
    rule = sc64_race_unit_rule(train_type);
    return rule && sc64_race_producer_compatible(type, rule->producer) ? train_type : -1;
}

/* R can only change the product where the producer has a choice. */
static int
hud_product_choice(int slot, int next)
{
    int type = b[slot].type;
    if (next < 0) return 0;
    if (race_mode)
        return sc64_skirmish_product_count((unsigned)(sc64_race_is_zerg_townhall(type) ? 35 :
                                                      sc64_race_raw_building(type))) > 1;
    return type != B_COMMAND && type != B_NEXUS;
}

/* ---- Units ------------------------------------------------------------ */

/* A caster's energy, or -1 for a unit without it. */
static int
hud_unit_energy(const Unit *q)
{
#ifdef SC64_Z6_CLOAK_ENABLED
    if (race_z6_cloak_actor(q)) return q->carry / 256;
#endif
#ifdef SC64_P6_STORM_ENABLED
    if (race_p6_active() && q->raw_type == 79 && q->raw_owner == 2) return q->carry / 256;
#endif
#ifdef SC64_Z5_CLOAK_ENABLED
    if (race_z5_cloak_actor(q)) return q->carry / 256;
#endif
    (void)q;
    return -1;
}

/* A unit's spell or cloak button while it has one, or -1. */
static int
hud_unit_spell(const Unit *q)
{
#ifdef SC64_Z6_CLOAK_ENABLED
    if (race_z6_cloak_actor(q)) return SC64_CMD_ICON_CLOAK;
#endif
#ifdef SC64_P6_STORM_ENABLED
    if (race_p6_active() && q->raw_type == 79 && q->raw_owner == 2) return SC64_CMD_ICON_STORM;
#endif
#ifdef SC64_Z5_CLOAK_ENABLED
    if (race_z5_cloak_actor(q)) return SC64_CMD_ICON_CLOAK;
#endif
    (void)q;
    return -1;
}

static int
hud_unit_cloaked(const Unit *q)
{
#ifdef SC64_Z6_CLOAK_ENABLED
    if (race_z6_cloak_actor(q)) return race_z6.cloak.active;
#endif
#ifdef SC64_Z5_CLOAK_ENABLED
    if (race_z5_cloak_actor(q)) return race_z5.cloak.active;
#endif
    (void)q;
    return 0;
}

/* Cargo space a selected transport holds, or -1 for anything else. */
static int
hud_unit_cargo(int slot)
{
#ifdef SC64_TRANSPORT_RUNTIME
#if defined(SC64_T5_RUNTIME) && !defined(SC64_SHARED_TRANSPORT)
    if (u[slot].type == U_DROPSHIP) return race_transport_cargo_space(slot);
#else
    if (race_transport_carrier_type(&u[slot])) return race_transport_cargo_space(slot);
#endif
#endif
    (void)slot;
    return -1;
}

static int
hud_unit_hangar(int slot)
{
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    if (race_protoss_services_active() && u[slot].type == U_P7_CARRIER)
        return (int)race_p7_carrier_training_child_count(slot);
#endif
    (void)slot;
    return -1;
}

/* The product an Egg will hatch into, or -1. */
static int
hud_egg_product(const Unit *q)
{
#ifdef SC64_Z7_EGG_PRODUCTION
    if (q->type == U_Z7_EGG) return q->target;
#endif
    (void)q;
    return -1;
}

static int
hud_egg_level(const Unit *q)
{
#ifdef SC64_Z7_EGG_PRODUCTION
    int duration = sc64_z7_egg_duration(q->target);
    return hud_level(duration - q->work_timer, duration, CARD_CELL);
#else
    (void)q;
    return 0;
#endif
}

/* ---- Redraw key ------------------------------------------------------- */

#define HUD_MIX(h, v) ((h) = ((h) ^ (u32)(v)) * 16777619u)

/* Everything in the band that the shared keys do not already cover. It runs
 * on every render, so it stays cheap: the band's once-a-second refresh catches
 * a custom game's product list changing with its tech. */
static int __attribute__((noinline))
hud_panel_key(int first, int selected)
{
    u32 h = 2166136261u;
    int i;
    if (selected_building >= 0 && b[selected_building].alive) {
        const Building *q = &b[selected_building];
        const Sc64RaceUnitRule *rule = sc64_race_unit_rule(race_mode ? train_type : hud_next_product(selected_building));
        /* Hit points step too, so a building under attack is not redrawn per hit. */
        HUD_MIX(h, hud_level(q->hp, q->maxhp, 64)); HUD_MIX(h, q->maxhp);
        HUD_MIX(h, q->queue_count); HUD_MIX(h, q->under_construction);
        for (i = 0; i < q->queue_count && i < HUD_QUEUE_SLOTS; i++)
            HUD_MIX(h, q->queue[(q->queue_head + i) % 5]);
        if (!q->queue_count) {
            HudJobs jobs;
            hud_building_jobs(selected_building, &jobs, 1);
            HUD_MIX(h, jobs.count);
            if (jobs.eggs) HUD_MIX(h, jobs.level[0]);
            for (i = 0; i < jobs.count; i++) HUD_MIX(h, jobs.icon[i]);
            if (jobs.separate) {
                HUD_MIX(h, weapons_research != 0); HUD_MIX(h, armor_research != 0);
                HUD_MIX(h, weapons_level); HUD_MIX(h, armor_level);
            }
        }
        if (sc64_race_is_zerg_townhall(q->type)) HUD_MIX(h, q->larva);
        if (rule) { HUD_MIX(h, minerals >= rule->minerals); HUD_MIX(h, gas >= rule->gas); }
#ifdef SC64_BATTERY_SERVICE_ENABLED
        if (!q->under_construction && sc64_race_raw_building(q->type) == 172) {
            HUD_MIX(h, q->queue_progress / 256);
            HUD_MIX(h, race_battery_powered_live(selected_building));
        }
#endif
#ifdef SC64_SHARED_BUNKER
        if (!q->under_construction && sc64_race_raw_building(q->type) == 125)
            HUD_MIX(h, race_bunker_cargo_space(selected_building));
#endif
    } else if (selected == 1 && first >= 0) {
        const Unit *q = &u[first];
        HUD_MIX(h, q->maxhp); HUD_MIX(h, hud_unit_energy(q)); HUD_MIX(h, hud_unit_cloaked(q));
        HUD_MIX(h, hud_unit_cargo(first)); HUD_MIX(h, hud_unit_hangar(first));
        HUD_MIX(h, weapons_level); HUD_MIX(h, armor_level);
        if (IS_WORKER(q->type)) HUD_MIX(h, q->carry > 0 ? q->order : -1);
        if (hud_egg_product(q) >= 0) HUD_MIX(h, hud_egg_level(q));
    } else if (selected > 1) {
        /* A group's bars move in steps, so a fight does not redraw every hit. */
        int hp = 0, maximum = 0;
        for (i = 0; i < MAX_UNITS; i++)
            if (u[i].alive && u[i].selected && u[i].owner == SIDE) {
                hp += u[i].hp + u[i].shield;
                maximum += u[i].maxhp + sc64_actor_max_shields(u[i].type, 0);
                HUD_MIX(h, u[i].raw_type >= 0 ? u[i].raw_type : u[i].type);
            }
        HUD_MIX(h, hud_level(hp, maximum, 32));
    }
    return (int)h;
}

/* Keys that search every unit run on one tick in four. */
static int
hud_panel_key_slow(int selected)
{
#ifdef SC64_Z7_EGG_PRODUCTION
    if (selected_building >= 0 && b[selected_building].alive && hud_egg_hall(&b[selected_building])) return 1;
#endif
    return selected > 1;
}

/* ---- Drawing ---------------------------------------------------------- */

/* A 20-pixel slot with an icon, or the drawn spark when the ROM has none. */
static void
hud_slot(int icon, int x, int y, const u8 *ramp)
{
    gfx_rect(x, y, CARD_CELL, CARD_CELL, C_SHADOW);
    if (icon >= 0) card_icon_draw(icon, x, y, ramp);
    else gfx_diamond(x + CARD_CELL / 2, y + CARD_CELL / 2, 4, 4, ramp[1]);
}

static void
hud_empty_slot(int x, int y)
{
    gfx_rect(x, y, CARD_CELL, CARD_CELL, C_SHADOW);
    gfx_frame(x, y, CARD_CELL, CARD_CELL, C_EDGE);
}

/* A number over a slot's lower right corner, as StarCraft marks upgrades. */
static void
hud_slot_number(int x, int y, int value, u8 color)
{
    int w = ui_num_width(value) + 1;
    if (!value) return;
    gfx_rect(x + CARD_CELL - w, y + CARD_CELL - 8, w, 8, C_SHADOW);
    ui_num_right(x + CARD_CELL, y + CARD_CELL - 8, value, color);
}

/* Current over maximum, right-aligned; returns the x to its left. */
static int
hud_fraction(int right, int y, int value, int maximum, u8 color)
{
    int x;
    ui_num_right(right, y, maximum, color);
    x = right - ui_num_width(maximum) - 6;
    gfx_text(x, y, "/", color);
    x -= 1;
    ui_num_right(x, y, value, color);
    return x - ui_num_width(value);
}

static u8
hud_health_color(int hp, int maxhp)
{
    return hp * 3 > maxhp * 2 ? C_GREEN : hp * 3 > maxhp ? C_GOLD : C_RED;
}

/* The portrait with shields over health below it. */
static void
hud_portrait(int icon, int hp, int maxhp, int shield, int max_shield)
{
    gfx_rect(HUD_PORTRAIT_X, HUD_PORTRAIT_Y, 22, 22, C_SHADOW);
    gfx_frame(HUD_PORTRAIT_X, HUD_PORTRAIT_Y, 22, 22, C_EDGE);
    if (icon >= 0) card_icon_draw(icon, HUD_PORTRAIT_X + 1, HUD_PORTRAIT_Y + 1, card_green);
    if (max_shield > 0)
        gfx_bar(HUD_PORTRAIT_X, HUD_PORTRAIT_Y + 23, 22, 1, shield, max_shield, C_CYAN, C_SHADOW);
    gfx_bar(HUD_PORTRAIT_X, HUD_PORTRAIT_Y + 25, 22, 2, hp, maxhp, hud_health_color(hp, maxhp), C_RED_DARK);
}

/* The name, then its hit points and shields at the panel's right edge. A
 * long name keeps its room by dropping the maximum, which the bar shows. */
static int
hud_title(const char *name, int hp, int maxhp, int shield, int max_shield)
{
    char title[48];
    u8 color = hud_health_color(hp, maxhp);
    int shield_w = max_shield > 0 ? ui_num_width(shield) + 5 : 0, left;
    int room = HUD_RIGHT - HUD_COLUMN - shield_w - 4 - sc64_font_width(name);
    if (room >= ui_num_width(hp) + ui_num_width(maxhp) + 7)
        left = hud_fraction(HUD_RIGHT, HUD_NUMBER_Y, hp, maxhp, color);
    else {
        ui_num_right(HUD_RIGHT, HUD_NUMBER_Y, hp, color);
        left = HUD_RIGHT - ui_num_width(hp);
    }
    if (max_shield > 0) {
        ui_num_right(left - 5, HUD_NUMBER_Y, shield, C_CYAN);
        left -= shield_w;
    }
    left -= 4 + HUD_COLUMN;
    sc64_font_ellipsis(name, left, title, sizeof title);
    sc64_font_text(HUD_COLUMN, HUD_NAME_Y, left, title, C_WHITE);
    return HUD_COLUMN + sc64_font_width(title);
}

/* A key cap with a mark; returns the x after it. */
static int
hud_hint(int x, int y, const char *key, int glyph)
{
    x = ui_key(x, y, key, 0, C_DIM);
    ui_glyph(glyph, x + 3, y + 3);
    return x + 10;
}

/* A's mark for the cursor's current action, then B's list. */
static int
hud_action_hint(int x, int y)
{
    switch (ui_action) {
    case UI_A_SELECT: x = hud_hint(x, y, "A", UI_GLYPH_SELECT); break;
    case UI_A_DESELECT: x = hud_hint(x, y, "A", UI_GLYPH_CROSS); break;
    case UI_A_MINE: x = hud_hint(x, y, "A", UI_GLYPH_CRYSTAL); break;
    case UI_A_GAS: x = hud_hint(x, y, "A", UI_GLYPH_GAS); break;
    case UI_A_MOVE: case UI_A_MOVE_HERE:
        x = ui_key(x, y, "A", 0, C_DIM);
        ui_order_icon(MENU_MOVE, x + 4, y + 3);
        x += 11;
        break;
    case UI_A_ATTACK: case UI_A_ATTACK_HERE:
        x = ui_key(x, y, "A", 0, C_DIM);
        ui_order_icon(MENU_ATTACK, x + 4, y + 3);
        x += 11;
        break;
    default: break;
    }
    return hud_hint(x, y, "B", UI_GLYPH_ORDERS);
}

/* The next unit, its price, and the keys that change or cancel it. */
static void
hud_product_chip(int slot, int next)
{
    const Sc64RaceUnitRule *rule = next >= 0 ? sc64_race_unit_rule(next) : 0;
    int x = HUD_CHIP_X, y = HUD_SLOT_Y;
    if (rule) {
        hud_slot(card_portrait(sc64_race_raw_unit(next)), x, y, card_green);
        ui_key(x, y + 1, "A", 0, C_DIM);
        if (rule->minerals)
            gfx_num(x + CARD_CELL + 2, y + 2, rule->minerals, minerals < rule->minerals ? C_RED : C_GOLD);
        if (rule->gas)
            gfx_num(x + CARD_CELL + 2, y + 11, rule->gas, gas < rule->gas ? C_RED : C_CYAN);
    }
    if (hud_product_choice(slot, next)) x = hud_hint(x, HUD_CHIP_KEYS_Y, "R", UI_GLYPH_CYCLE);
    /* tick_game cancels on L only for Barracks; elsewhere L cycles the selection. */
    if (b[slot].queue_count && b[slot].type == B_BARRACKS) hud_hint(x, HUD_CHIP_KEYS_Y, "L", UI_GLYPH_CROSS);
}

/* Loaded units as icons, for a Bunker or a transport. */
static void
hud_cargo_slots(int target, int x, int y, int slots)
{
#ifdef SC64_TRANSPORT_RUNTIME
    int i, used = 0;
    for (i = 0; i < MAX_UNITS && used < slots; i++)
        if (u[i].alive && u[i].order == ORDER_TRANSPORTED && u[i].target == target) {
            hud_slot(card_unit_icon(&u[i]), x + used * HUD_SLOT_PITCH, y, card_green);
            used++;
        }
    for (; used < slots; used++) hud_empty_slot(x + used * HUD_SLOT_PITCH, y);
#else
    (void)target; (void)x; (void)y; (void)slots;
#endif
}

static void
hud_jobs_bars(const HudJobs *jobs)
{
    if (!jobs->separate) {
        gfx_bar(HUD_COLUMN, HUD_BAR_Y, HUD_BAR_W, HUD_BAR_H, jobs->level[0], HUD_BAR_LEVELS, C_GOLD, C_SHADOW);
        return;
    }
    gfx_bar(HUD_COLUMN, HUD_BAR_Y, CARD_CELL, HUD_BAR_H, jobs->level[0], CARD_CELL, C_GOLD, C_SHADOW);
    gfx_bar(HUD_COLUMN + HUD_SLOT_PITCH, HUD_BAR_Y, CARD_CELL, HUD_BAR_H, jobs->level[1], CARD_CELL, C_GOLD, C_SHADOW);
}

static void
hud_draw_jobs(const HudJobs *jobs)
{
    int i;
    for (i = 0; i < HUD_QUEUE_SLOTS; i++) {
        int x = HUD_COLUMN + i * HUD_SLOT_PITCH;
        if (i < jobs->count) {
            int busy = jobs->separate ? jobs->level[i] > 0 : !i;
            hud_slot(jobs->icon[i], x, HUD_SLOT_Y, busy ? card_gold : card_green);
            if (busy) gfx_frame(x, HUD_SLOT_Y, CARD_CELL, CARD_CELL, C_GOLD);
            if (jobs->separate) hud_slot_number(x, HUD_SLOT_Y, i ? armor_level : weapons_level, C_WHITE);
        } else if (!jobs->separate) hud_empty_slot(x, HUD_SLOT_Y);
    }
    if (jobs->eggs) hud_jobs_bars(jobs);
}

static void
hud_draw_building(void)
{
    const Building *q = &b[selected_building];
    const Sc64RaceBuildingRule *rule = sc64_race_building_rule(q->type);
    int raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type);
    int shields = sc64_actor_max_shields(q->type, 1), title_end, next;
    HudJobs jobs;
    hud_portrait(card_portrait(raw), q->hp, q->maxhp, q->shield, shields);
    title_end = hud_title(sc64_race_building_name(q->type), q->hp, q->maxhp, q->shield, shields);
    if (sc64_race_is_zerg_townhall(q->type)) { /* larva beside the name */
        gfx_diamond(title_end + 7, HUD_NUMBER_Y + 3, 3, 2, C_ZERG);
        gfx_num(title_end + 12, HUD_NUMBER_Y, q->larva, C_WHITE);
    }
    hud_building_jobs(selected_building, &jobs, 1);
    if (q->under_construction && !jobs.count) return; /* hud_panel_bars() shows its progress */
#ifdef SC64_BATTERY_SERVICE_ENABLED
    if (sc64_race_raw_building(q->type) == 172) {
        int powered = race_battery_powered_live(selected_building);
        gfx_bar(HUD_COLUMN, HUD_SLOT_Y + 2, 60, 3, q->queue_progress, SC64_P7_BATTERY_MAX_ENERGY_Q8, C_PURPLE, C_SHADOW);
        gfx_num(HUD_COLUMN + 64, HUD_SLOT_Y, q->queue_progress / 256, C_WHITE);
        if (powered) hud_hint(HUD_COLUMN, HUD_HINT_Y, "B", UI_GLYPH_ORDERS);
        else ui_glyph(UI_GLYPH_CROSS, HUD_PORTRAIT_X + 11, HUD_PORTRAIT_Y + 11);
        return;
    }
#endif
#ifdef SC64_SHARED_BUNKER
    if (sc64_race_raw_building(q->type) == 125) {
        hud_cargo_slots(sc64_bunker_target_encode(MAX_UNITS, MAX_BUILDINGS, selected_building),
                        HUD_COLUMN, HUD_SLOT_Y, 4);
        if (race_bunker_cargo_space(selected_building) > 0) {
            int x = ui_key(HUD_CHIP_X + 8, HUD_SLOT_Y + 11, "B", 0, C_DIM);
            hud_slot(SC64_CMD_ICON_UNLOAD, x, HUD_SLOT_Y, card_green);
        }
        return;
    }
#endif
    next = hud_next_product(selected_building);
    if (jobs.count || next >= 0 || (rule && (rule->flags & SC_RACE_F_PRODUCER))) {
        hud_draw_jobs(&jobs);
        hud_product_chip(selected_building, next);
    } else if (rule && rule->supply_provided) {
        gfx_blit_sprite(HUD_COLUMN, HUD_SLOT_Y, sc64_hud_sprite_size[card_supply_sprite()][0],
                        sc64_hud_sprite_size[card_supply_sprite()][1],
                        sc64_hud_sprite_pixels + sc64_hud_sprite_offset[card_supply_sprite()]);
        gfx_text(HUD_COLUMN + 15, HUD_SLOT_Y + 2, "+", C_WHITE);
        gfx_num(HUD_COLUMN + 21, HUD_SLOT_Y + 2, rule->supply_provided / 2, C_WHITE);
    }
}

static void
hud_draw_unit(int slot)
{
    const Unit *q = &u[slot];
    int shields = sc64_actor_max_shields(q->type, 0), x = HUD_COLUMN, value, product = hud_egg_product(q);
    hud_portrait(card_unit_icon(q), q->hp, q->maxhp, q->shield, shields);
    hud_title(unit_name(q->type), q->hp, q->maxhp, q->shield, shields);
    if (product >= 0) {
        hud_slot(card_portrait(sc64_race_raw_unit(product)), x, HUD_SLOT_Y, card_gold);
        gfx_frame(x, HUD_SLOT_Y, CARD_CELL, CARD_CELL, C_GOLD);
        gfx_bar(x, HUD_BAR_Y, CARD_CELL, HUD_BAR_H, hud_egg_level(q), CARD_CELL, C_GOLD, C_SHADOW);
        hud_hint(x + HUD_SLOT_PITCH + 3, HUD_HINT_Y, "B", UI_GLYPH_ORDERS);
        return;
    }
    if (sc64_race_unit_damage(q->type) > 0) {
        hud_slot(card_weapons_icon(), x, HUD_SLOT_Y, card_green);
        hud_slot_number(x, HUD_SLOT_Y, weapons_level, C_WHITE);
        x += HUD_SLOT_PITCH;
    }
    hud_slot(card_armor_icon(), x, HUD_SLOT_Y, card_green);
    hud_slot_number(x, HUD_SLOT_Y, armor_level, C_WHITE);
    x += HUD_SLOT_PITCH;
    value = hud_unit_hangar(slot);
    if (value >= 0) {
        hud_slot(card_portrait(73), x, HUD_SLOT_Y, card_green);
        hud_slot_number(x, HUD_SLOT_Y, value, C_WHITE);
        x += HUD_SLOT_PITCH;
    }
    value = hud_unit_cargo(slot);
    if (value >= 0) {
        hud_cargo_slots(slot, x, HUD_SLOT_Y, 4);
        x += 4 * HUD_SLOT_PITCH;
    }
    value = hud_unit_spell(q);
    if (value >= 0) {
        hud_slot(value, x, HUD_SLOT_Y, hud_unit_cloaked(q) ? card_gold : card_green);
        x += HUD_SLOT_PITCH;
    }
    if (IS_WORKER(q->type) && q->carry > 0) {
        ui_glyph(q->order == ORDER_GAS ? UI_GLYPH_GAS : UI_GLYPH_CRYSTAL, x + 4, HUD_SLOT_Y + 4);
        gfx_num(x + 10, HUD_SLOT_Y + 1, q->carry, C_WHITE);
    }
    value = hud_unit_energy(q);
    if (value >= 0) {
        gfx_diamond(x + 4, HUD_SLOT_Y + 4, 3, 3, C_PURPLE);
        gfx_num(x + 10, HUD_SLOT_Y + 1, value, C_WHITE);
    }
    hud_action_hint(x < 76 ? 76 : x + 3, HUD_HINT_Y);
}

/* A group by type: each type's icon, count and share of health. */
static void
hud_draw_group(int first)
{
    int raws[HUD_GROUP_CELLS], counts[HUD_GROUP_CELLS], hps[HUD_GROUP_CELLS], maxima[HUD_GROUP_CELLS];
    int cells = 0, i, n, hp = 0, maximum = 0, x;
    for (i = 0; i < MAX_UNITS; i++) {
        int raw, gain, top;
        if (!u[i].alive || !u[i].selected || u[i].owner != SIDE) continue;
        raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        gain = u[i].hp + u[i].shield;
        top = u[i].maxhp + sc64_actor_max_shields(u[i].type, 0);
        hp += gain; maximum += top;
        for (n = 0; n < cells && raws[n] != raw; n++) {}
        if (n == cells) {
            if (cells == HUD_GROUP_CELLS) continue;
            raws[n] = raw; counts[n] = hps[n] = maxima[n] = 0;
            cells++;
        }
        counts[n]++; hps[n] += gain; maxima[n] += top;
    }
    hud_portrait(card_unit_icon(&u[first]), hp, maximum, 0, 0);
    for (n = 0; n < cells; n++) {
        x = HUD_COLUMN + n * HUD_SLOT_PITCH;
        hud_slot(card_portrait(raws[n]), x, HUD_PORTRAIT_Y, card_green);
        hud_slot_number(x, HUD_PORTRAIT_Y, counts[n], C_WHITE);
        gfx_bar(x, HUD_PORTRAIT_Y + CARD_CELL + 1, CARD_CELL, 2,
                hud_level(hps[n], maxima[n], CARD_CELL), CARD_CELL,
                hud_health_color(hps[n], maxima[n]), C_RED_DARK);
    }
    hud_action_hint(HUD_COLUMN, HUD_GROUP_HINT_Y);
}

/* The selection's panel; 0 when nothing is selected. */
static int __attribute__((noinline))
hud_panel_selection(int selected)
{
    int i;
    if (selected_building >= 0 && b[selected_building].alive) {
        hud_draw_building();
        return 1;
    }
    if (!selected) return 0;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].selected && !SC64_TRANSPORT_LOADED(&u[i])) {
            if (selected == 1) hud_draw_unit(i);
            else hud_draw_group(i);
            return 1;
        }
    return 0;
}

/* A selected building's progress bars, over the band on every frame. */
static void __attribute__((noinline))
hud_panel_bars(void)
{
    const Building *q;
    HudJobs jobs;
    if (selected_building < 0 || !b[selected_building].alive || box_state == 2 || menu_open ||
        mode == MODE_BUILD || mode == MODE_MOVE || mode == MODE_ATTACK) return;
    q = &b[selected_building];
    if (q->queue_count > 0) { /* the common case, without resolving its icons */
        gfx_bar(HUD_COLUMN, HUD_BAR_Y, HUD_BAR_W, HUD_BAR_H, q->queue_progress,
                unit_build_time(q->queue[q->queue_head]), C_GOLD, C_SHADOW);
        return;
    }
    hud_building_jobs(selected_building, &jobs, 0);
    if (jobs.count) hud_jobs_bars(&jobs);
    else if (q->under_construction)
        gfx_bar(HUD_COLUMN, HUD_BAR_Y, HUD_FULL_BAR_W, HUD_BAR_H, q->build_progress,
                building_build_time(q->type), C_GOLD, C_SHADOW);
}

/* The field with nothing selected: the objective above the keys. */
static void
hud_panel_objective(const char *text)
{
    ui_text_wrap(5, 124, 29, 2, text, C_GOLD);
}

static void
hud_panel_idle_hints(void)
{
    hud_action_hint(5, HUD_GROUP_HINT_Y);
}

/* A timed mission's clock stays visible under the portrait whatever else the
 * panel shows. */
static void
hud_panel_clock(void)
{
    if (race_mode) return;
    if (race_selected_mission == 3 && race_campaign.elapsed_frames < 27000u)
        ui_clock(0, HUD_CLOCK_Y, (27000u - race_campaign.elapsed_frames + 29u) / 30u * 30u, C_WHITE);
#ifdef SC64_T3_ENABLED
    if (race_selected_mission == 6)
        ui_clock(0, HUD_CLOCK_Y, (race_t3.remaining_ticks + 29u) / 30u * 30u, C_WHITE);
#endif
#ifdef SC64_Z4_RUNTIME
    if (race_selected_mission == 15)
        ui_clock(0, HUD_CLOCK_Y, race_z4_hud_seconds() * 30u, C_WHITE);
#endif
}
