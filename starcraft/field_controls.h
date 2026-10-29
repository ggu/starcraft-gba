/* Default-profile controls and menus for the race engine (SC64_FIELD_CONTROLS).
 *
 * The race-engine port of field_ui.h, used by every race ROM: A on open
 * ground moves the selected units there, A trains only on the selected
 * structure, A on a visible hostile target attacks, SELECT drops the
 * selection, R acts on a clean tap, and the order list reopens on the last
 * command. The list is framed with icons, costs and dimmed rows; the HUD
 * names the highlighted command or why it is unavailable; placement snaps to
 * an 8-pixel grid (gas to its geyser) with a green/gold/red footprint; pause
 * and result are navigable panels. The functions share field_ui.h's names so
 * game.c's hooks serve both. Presentation and input routing only: no saved
 * state, and every action goes through the existing order and build paths. */
#ifndef SC64_FIELD_CONTROLS
#error "field_controls.h is the race-engine port of the default controls"
#endif
#ifdef SC_DEFAULT_UI
#error "the default profile uses field_ui.h"
#endif
#ifndef SC64_RACES
#error "field_controls.h reads the race rules"
#endif

static int currently_visible(int x, int y);
static int nearest_refinery(int x, int y);
static int field_menu_count(void);
static const char *race_menu_label(int i);
static int race_owner_identity(int owner);
static int race_owner_has_prereq(int owner, unsigned short prereq);
/* Defined by the custom-game adapter included after the UI declarations. */
static int race_skirmish_record_available(const Sc64SkirmishRecord *record);
static const char *race_campaign_objective_text(void);
static const char *race_campaign_victory_text(void);
static const char *race_campaign_failure_text(void);
#ifdef SC64_T5_RUNTIME
static int race_t5_building_product_available(int owner, int raw_owner, int type);
#endif
#ifdef SC64_T6_RUNTIME
static int race_t6_building_product_available(int owner, int raw_owner, int type);
#endif

#include "ui_widgets.h"
#include "sc64_font.h"
#include "sc64_icons.h"

/* ui_action (game.c, beside the other field state) holds one of these. */
enum {
    UI_A_NONE, UI_A_SELECT, UI_A_DESELECT, UI_A_TRAIN, UI_A_ATTACK,
    UI_A_MOVE_HERE, UI_A_ATTACK_HERE, UI_A_PLACE, UI_A_GAS, UI_A_MOVE
};

/* Why a command or site is unavailable, or UI_SITE_OK. */
enum {
    UI_SITE_OK, UI_SITE_WORKER, UI_SITE_UNITS, UI_SITE_TECH, UI_SITE_MINERALS,
    UI_SITE_GAS, UI_SITE_DONE, UI_SITE_BUSY, UI_SITE_BLOCKED, UI_SITE_FULL,
    UI_SITE_POWER, UI_SITE_CREEP, UI_SITE_GEYSER
};

/* What a command row does: an order, research, a worker-built structure, an
 * in-place morph/add-on (cost only), or a unit's special command. */
enum { UI_ROW_ORDER, UI_ROW_RESEARCH, UI_ROW_BUILD, UI_ROW_MORPH, UI_ROW_SPECIAL };

/* Visible units and structures the player's side may fight: the engine's
 * owners_hostile(), so skirmish opponents 1, 3 and 4 all count. */
static int
ui_hostile_at(int x, int y, int *tx, int *ty)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && owners_hostile(SIDE, u[i].owner) &&
            dist2(x, y, u[i].x, u[i].y) < 9 * 9 && currently_visible(u[i].x, u[i].y)) {
            *tx = u[i].x; *ty = u[i].y;
            return 1;
        }
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && owners_hostile(SIDE, b[i].owner) && dist2(x, y, b[i].x, b[i].y) < 16 * 16 &&
            currently_visible(b[i].x, b[i].y)) {
            *tx = b[i].x; *ty = b[i].y;
            return 1;
        }
    return 0;
}

static int
field_ui_on_selected_building(void)
{
    return selected_building >= 0 &&
        dist2(cx, cy, b[selected_building].x, b[selected_building].y) < 26 * 26;
}

/* What A does at the cursor, in interact()'s order (double tap, gas,
 * structure, attack, select, move); drives the hint row and attack cursor. */
static int
field_ui_a_action(int selected)
{
    int hx, hy, target;
    if (mode == MODE_MOVE) return UI_A_MOVE_HERE;
    if (mode == MODE_ATTACK) return UI_A_ATTACK_HERE;
    if (mode == MODE_BUILD) return UI_A_PLACE;
    if (mode != MODE_NORMAL) return UI_A_NONE;
    target = selection_target();
    if (selected_building < 0 && unit_tap >= 0 && target == unit_tap &&
        (u32)frame - unit_tap_frame <= UNIT_DOUBLE_TAP_TICKS &&
        dist2(cx, cy, unit_tap_x, unit_tap_y) <= UNIT_DOUBLE_TAP_RADIUS * UNIT_DOUBLE_TAP_RADIUS)
        return UI_A_SELECT;
    if (find_selected_worker() >= 0) {
        int refinery = nearest_refinery(cx, cy);
        if (refinery >= 0 && dist2(cx, cy, b[refinery].x, b[refinery].y) < 24 * 24)
            return UI_A_GAS;
    }
    if (selected_building >= 0 && b[selected_building].alive && field_ui_on_selected_building())
        return b[selected_building].under_construction ? UI_A_NONE : UI_A_TRAIN;
    if (selected && ui_hostile_at(cx, cy, &hx, &hy)) return UI_A_ATTACK;
    if (target >= 0) return UI_A_SELECT;
    if (selected_building >= 0) return UI_A_DESELECT;
    return selected ? UI_A_MOVE : UI_A_NONE;
}

/* The renderer draws several frames per simulation update in crowded scenes,
 * and resolving every drawn frame cost about 4% of Carrier Approach's update
 * rate with the army selected. The cursor and selection only change in
 * updates that advance frame; arming an order from the list changes mode
 * without advancing it. So resolve once per (frame, mode). A restart or a
 * load jumps frame, so init_state() and load_game() clear the key
 * (ui_tick_mode is mode + 1; 0 forces a resolve). */

static void
field_ui_refresh(void)
{
    if ((u8)frame == ui_tick && (u8)(mode + 1) == ui_tick_mode) return;
    ui_tick = (u8)frame; ui_tick_mode = (u8)(mode + 1);
    ui_action = (u8)(box_state == 2 ? UI_A_NONE : field_ui_a_action(selected_count()));
}

static void
field_ui_ping(int x, int y, u8 color)
{
    ping_x = (s16)x; ping_y = (s16)y; ping_color = color;
#ifdef SC64_ORDER_MARKER_ART
    ping_life = color==C_GREEN || color==C_RED ? SC64_ORDER_MARKER_GBA_TICKS : 18;
#else
    ping_life = 18;
#endif
}

/* A on a visible enemy with units selected attacks it; selection keeps
 * priority everywhere else. */
static int
field_ui_smart_attack(void)
{
    int tx, ty;
    if (!selected_count() || !ui_hostile_at(cx, cy, &tx, &ty)) return 0;
    if (issue_order(ORDER_ATTACK_MOVE, tx, ty)) field_ui_ping(tx, ty, C_RED);
    return 1;
}

/* A on open ground moves the selected units there, as a right-click would;
 * interact() has already tried every target under the cursor. */
static int
field_ui_smart_move(void)
{
    if (selected_building >= 0 || !selected_count() || selection_target() >= 0) return 0;
    if (issue_order(ORDER_MOVE, cx, cy)) field_ui_ping(cx, cy, C_GREEN);
    return 1;
}

/* R's army/type action fires on a clean release, so holding R to speed the
 * cursor keeps the current selection. */
static int
field_ui_r_tap(u16 held, u16 pressed)
{
    if (pressed & KEY_R) r_tap_state = 1;
    if (!(held & KEY_R)) {
        int fire = r_tap_state == 1;
        r_tap_state = 0;
        return fire;
    }
    if ((held & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN)) || box_state) r_tap_state = 2;
    return 0;
}

/* ---- Structures, commands and their availability ---------------------- */

static int
ui_same(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static int
ui_gas_building(int type)
{
    return type == B_REFINERY || type == B_EXTRACTOR || type == B_ASSIMILATOR;
}

/* Structures snap to an 8-pixel grid so rows line up; gas structures snap
 * to the geyser they cover. Preview and placement share this point. */
static void
field_ui_site(int type, int *x, int *y)
{
    if (ui_gas_building(type)) {
        int geyser = geyser_at(*x, *y);
        if (geyser >= 0) { *x = geysers[geyser].x; *y = geysers[geyser].y; }
        return;
    }
    *x = limit((*x + 4) & ~7, 8, WORLD_W - 8);
    *y = limit((*y + 4) & ~7, 8, WORLD_H - 8);
}

/* The worker that builds type: the selected one, else the nearest free one
 * of the right kind, so placing needs no worker selected first. */
static int
field_ui_build_worker(int type)
{
    if (race_mode) return race_skirmish_worker(type);
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(type);
    int i, best = -1, d, best_d = 0x7fffffff;
    if (!r) return -1;
    for (i = 0; i < MAX_UNITS; i++) {
        if (!build_worker_available(i, SIDE, r->worker)) continue;
        if (u[i].selected) return i;
        d = dist2(cx, cy, u[i].x, u[i].y);
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

/* The first missing prerequisite of type, "" when this profile cannot build
 * it at all, or 0 when the side has the tech. race_start_build()'s checks. */
static const char *
ui_missing_tech(int type, int worker)
{
    if (race_mode) {
        const Sc64SkirmishRecord *record = sc64_skirmish_menu_record((unsigned)race_human,
            (unsigned)sc64_race_raw_building(type));
        return record && race_skirmish_record_available(record) ? 0 : "";
    }
    static const char *const names[14] = {
        "HATCHERY", "SPAWNING POOL", "HYDRALISK DEN", "NEXUS", "GATEWAY",
        "CYBERNETICS CORE", "PYLON", "FORGE", "SUPPLY DEPOT", "BARRACKS",
        "FACTORY", "CONTROL TOWER", "SPIRE", "LAIR"
    };
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(type);
    unsigned need;
    int bit;
    (void)worker;
    if (!r || r->race != race_owner_identity(SIDE) ||
        type == B_LAIR || type == B_HIVE || type == B_GREATER_SPIRE) return "";
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (!race_late_building_product_available(SIDE, worker >= 0 ? u[worker].raw_owner : -1, type))
        return "";
#endif
#ifdef SC64_T5_RUNTIME
    if (!race_t5_building_product_available(SIDE, worker >= 0 ? u[worker].raw_owner : -1, type))
        return "";
#endif
#ifdef SC64_T6_RUNTIME
    if (!race_t6_building_product_available(SIDE, worker >= 0 ? u[worker].raw_owner : -1, type))
        return "";
#endif
    need = r->prereq;
#ifdef SC64_Z7_EGG_PRODUCTION
    if (race_z7_active() && type == B_EXTRACTOR) need &= ~(unsigned)SC_PR_HATCHERY;
#endif
    for (bit = 0; bit < 14; bit++)
        if ((need >> bit & 1) && !race_owner_has_prereq(SIDE, (unsigned short)(1u << bit)))
            return names[bit];
    return 0;
}

/* Row kind, with the structure (or morph) it makes in *type. Contextual rows
 * are recognised by their label, as choose_command() resolves them. */
static int
ui_row_kind(int i, int *type)
{
    static const u8 base[3][5] = {
        {B_BARRACKS, B_DEPOT, B_REFINERY, B_ACADEMY, B_ENG_BAY},
        {B_SPAWNING_POOL, B_HATCHERY, B_EXTRACTOR, B_HYDRALISK_DEN, B_EVOLUTION_CHAMBER},
        {B_GATEWAY, B_PYLON, B_ASSIMILATOR, B_CYBERNETICS_CORE, B_FORGE}};
    const char *label;
    *type = -1;
    /* Custom games use catalog row kinds directly. The campaign menu below
     * keeps its legacy label mapping and compile-profile-specific rows. */
    if (race_mode) {
        Sc64SkirmishMenuRow row;
        if (!sc64_skirmish_menu_at((unsigned)race_human, (unsigned)i, &row))
            return UI_ROW_SPECIAL;
        if (row.kind == SC64_SK_MENU_MOVE || row.kind == SC64_SK_MENU_ATTACK ||
            row.kind == SC64_SK_MENU_STOP) return UI_ROW_ORDER;
        if (row.kind == SC64_SK_MENU_WEAPONS || row.kind == SC64_SK_MENU_ARMOR)
            return UI_ROW_RESEARCH;
        if (!row.record) return UI_ROW_SPECIAL;
        *type = sc64_race_building_from_raw(row.record->raw_id);
        if (row.kind == SC64_SK_MENU_BUILD) return UI_ROW_BUILD;
        if (row.kind == SC64_SK_MENU_ADDON || row.kind == SC64_SK_MENU_MORPH)
            return UI_ROW_MORPH;
        return UI_ROW_SPECIAL;
    }
    if (i == MENU_MOVE || i == MENU_ATTACK || i == MENU_STOP) return UI_ROW_ORDER;
    if (i == MENU_WEAPONS || i == MENU_ARMOR) return UI_ROW_RESEARCH;
    if (i < MENU_STOP) { *type = base[race_human][i - MENU_BARRACKS]; return UI_ROW_BUILD; }
    label = race_menu_label(i);
    if (ui_same(label, "NEXUS")) *type = B_NEXUS;
    else if (ui_same(label, "CANNON")) *type = B_PHOTON_CANNON;
#ifdef SC64_P7_BATTERY_ENABLED
    else if (ui_same(label, "SHIELD BATTERY")) *type = B_P7_SHIELD_BATTERY;
#endif
#ifdef SC64_T5_RUNTIME
    else if (ui_same(label, "FACTORY")) *type = B_FACTORY;
    else if (ui_same(label, "STARPORT")) *type = B_STARPORT;
    else if (ui_same(label, "CONTROL TOWER")) { *type = B_CONTROL_TOWER; return UI_ROW_MORPH; }
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    else if (ui_same(label, "COMMAND CENTER")) *type = B_COMMAND;
#endif
#ifdef SC64_ZP_CAMPAIGN_STORAGE
    else if (ui_same(label, "SPIRE")) *type = B_SPIRE;
    else if (ui_same(label, "UPGRADE TO LAIR")) { *type = B_LAIR; return UI_ROW_MORPH; }
#endif
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    else if (ui_same(label, "CREEP COLONY")) *type = B_CREEP_COLONY;
#endif
    return *type >= 0 ? UI_ROW_BUILD : UI_ROW_SPECIAL;
}

/* start_research()'s checks: the upgrade structure, level, and funds. */
static int
ui_research_state(int i, const char **need)
{
    int lab = race_human == R_ZERG ? B_EVOLUTION_CHAMBER : race_human == R_PROTOSS ? B_FORGE : B_ENG_BAY;
    if (!has_building(SIDE, lab)) { *need = sc64_race_building_name(lab); return UI_SITE_TECH; }
    if ((i == MENU_WEAPONS ? weapons_level : armor_level) >= 2) return UI_SITE_DONE;
    if (i == MENU_WEAPONS ? weapons_research : armor_research) return UI_SITE_BUSY;
    return minerals < 100 ? UI_SITE_MINERALS : gas < 100 ? UI_SITE_GAS : UI_SITE_OK;
}

/* State of command row i, its mineral and gas costs, and for UI_SITE_TECH
 * the missing structure's name ("" when not buildable here). */
static int
ui_row_state(int i, int *cost, int *gas_cost, const char **need)
{
    int type, kind = ui_row_kind(i, &type);
    *cost = *gas_cost = 0;
    *need = "";
    if (kind == UI_ROW_ORDER)
        return selected_count() || selected_building >= 0 ? UI_SITE_OK : UI_SITE_UNITS;
    if (kind == UI_ROW_RESEARCH) {
        *cost = *gas_cost = 100;
        return ui_research_state(i, need);
    }
    if (kind == UI_ROW_SPECIAL) return UI_SITE_OK;
    if (race_mode) {
        Sc64SkirmishMenuRow row;
        const Sc64StockUnitProfile *profile;
        if (!sc64_skirmish_menu_at((unsigned)race_human, (unsigned)i, &row) || !row.record)
            return UI_SITE_OK;
        profile = sc64_stock_unit_profile(row.record->raw_id);
        if (!profile) return UI_SITE_TECH;
        *cost = profile->minerals;
        *gas_cost = profile->gas;
        if (!race_skirmish_record_available(row.record)) return UI_SITE_TECH;
        if (type < 0) return UI_SITE_TECH;
        if (kind == UI_ROW_BUILD && field_ui_build_worker(type) < 0)
            return UI_SITE_WORKER;
        if (minerals < *cost) return UI_SITE_MINERALS;
        if (gas < *gas_cost) return UI_SITE_GAS;
        return UI_SITE_OK;
    }
    *cost = building_cost(type);
    *gas_cost = building_gas_cost(type);
    if (kind == UI_ROW_BUILD) {
        int worker = field_ui_build_worker(type);
        const char *missing = ui_missing_tech(type, worker);
        if (missing) { *need = missing; return UI_SITE_TECH; }
        if (minerals < *cost) return UI_SITE_MINERALS;
        if (gas < *gas_cost) return UI_SITE_GAS;
        return worker < 0 ? UI_SITE_WORKER : UI_SITE_OK;
    }
    return minerals < *cost ? UI_SITE_MINERALS : gas < *gas_cost ? UI_SITE_GAS : UI_SITE_OK;
}

/* Why type cannot be placed at the snapped site x,y, start_build()'s order;
 * *ground is 0 when the ground itself rejects it (the red footprint). */
static int
ui_site_problem(int type, int x, int y, const char **need, int *ground)
{
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(type);
    int worker = field_ui_build_worker(type), problem = UI_SITE_OK;
    const char *missing = ui_missing_tech(type, worker);
    int site = race_can_site(SIDE, type, x, y);
    *need = missing ? missing : "";
    *ground = can_place(type, x, y) && (missing || site);
    if (missing) problem = UI_SITE_TECH;
    else if (minerals < building_cost(type)) problem = UI_SITE_MINERALS;
    else if (gas < building_gas_cost(type)) problem = UI_SITE_GAS;
    else if (worker < 0) problem = UI_SITE_WORKER;
    else if (!site)
        problem = ui_gas_building(type) ? UI_SITE_GEYSER :
            r && (r->flags & SC_RACE_F_REQUIRES_POWER) ? UI_SITE_POWER :
            race_human == R_ZERG && type != B_HATCHERY ? UI_SITE_CREEP : UI_SITE_BLOCKED;
    else if (!can_place(type, x, y)) problem = UI_SITE_BLOCKED;
    else if (find_build_slot() < 0) problem = UI_SITE_FULL;
    return problem;
}

/* One line naming the problem (red), or note (dim) when there is none;
 * cost and gas_cost size a shortfall. */
static void
ui_status(int x, int y, int problem, const char *need, const char *note, int cost, int gas_cost)
{
    static const char *const text[] = {
        0, "NO FREE WORKER", "SELECT UNITS FIRST", 0, 0, 0, "FULLY UPGRADED", "RESEARCHING",
        "SITE BLOCKED", "NO FREE SLOT", "NEEDS PYLON POWER", "NEEDS CREEP", "PLACE ON A GEYSER"
    };
    if (problem == UI_SITE_OK) {
        if (note) gfx_text(x, y, note, C_DIM);
        return;
    }
    if (problem == UI_SITE_TECH) {
        if (!*need) { gfx_text(x, y, "NOT AVAILABLE HERE", C_RED); return; }
        gfx_text(x, y, "NEEDS", C_RED);
        gfx_text(x + 36, y, need, C_RED);
        return;
    }
    if (problem == UI_SITE_MINERALS || problem == UI_SITE_GAS) {
        int short_by = problem == UI_SITE_MINERALS ? cost - minerals : gas_cost - gas;
        gfx_text(x, y, "NEED", C_RED);
        gfx_num(x + 30, y, short_by, C_RED);
        gfx_text(x + 36 + ui_num_width(short_by), y, problem == UI_SITE_MINERALS ?
                 "MORE MINERALS" : "MORE GAS", C_RED);
        return;
    }
    if (problem == UI_SITE_DONE || problem == UI_SITE_BUSY) {
        gfx_text(x, y, text[problem], C_GOLD);
        return;
    }
    gfx_text(x, y, text[problem], C_RED);
}

/* ---- Drawing ---------------------------------------------------------- */

static void
ui_structure_glyph(int x, int y)
{
    if (race_human == R_ZERG) {
        gfx_diamond(x, y, 6, 4, C_ZERG);
        gfx_diamond(x, y, 3, 2, C_PURPLE);
    } else if (race_human == R_PROTOSS) {
        gfx_rect(x - 4, y - 1, 9, 5, C_PROTOSS);
        gfx_rect(x - 4, y - 1, 9, 1, C_GOLD);
        gfx_diamond(x, y - 3, 2, 2, C_CYAN);
    } else {
        gfx_rect(x - 5, y - 3, 11, 8, C_STONE);
        gfx_rect(x - 5, y - 3, 11, 1, C_SILVER);
        gfx_rect(x - 3, y - 5, 7, 2, C_TERRAN);
        gfx_rect(x - 1, y - 1, 3, 3, C_ORANGE);
    }
}

/* Public to game.c's shared HUD painter. The caller owns the 22px panel and
 * surrounding health/resource text; source IDs always come from the imported
 * raw identity, with the race adapter as a fallback for synthetic actors. */
static int
field_controls_draw_selected_icon(int center_x, int center_y, int size)
{
    int raw = -1;
    if (selected_building >= 0 && selected_building < MAX_BUILDINGS &&
        b[selected_building].alive) {
        raw = b[selected_building].raw_type >= 0 ? b[selected_building].raw_type :
              sc64_race_raw_building(b[selected_building].type);
    } else {
        int i;
        for (i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].selected && !SC64_TRANSPORT_LOADED(&u[i])) {
                raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
                break;
            }
    }
    return raw >= 0 && raw < 228 && sc64_icon_draw_centered((unsigned)raw,
                                                             center_x, center_y, size);
}

/* Source icon binding for the visible command row. All menu decisions and
 * costs remain in the existing rule paths; an unavailable source frame keeps
 * the compact procedural mark below. */
static int
ui_source_icon_for_row(int i, int kind, int type)
{
#ifdef SC64_ICON_TABLE_PRESENT
    const char *label;
    if (race_mode) {
        Sc64SkirmishMenuRow row;
        if (sc64_skirmish_menu_at((unsigned)race_human, (unsigned)i, &row) && row.record &&
            row.record->raw_id < SC64_ICON_UNIT_COUNT) return row.record->raw_id;
    }
    if (i == MENU_MOVE) return SC64_ICON_MOVE;
    if (i == MENU_ATTACK) return SC64_ICON_ATTACK;
    if (i == MENU_STOP) return SC64_ICON_STOP;
    if (kind == UI_ROW_RESEARCH) {
        if (i == MENU_WEAPONS)
            return race_human == R_ZERG ? SC64_ICON_ZERG_WEAPONS :
                   race_human == R_PROTOSS ? SC64_ICON_PROTOSS_WEAPONS : SC64_ICON_TERRAN_WEAPONS;
        return race_human == R_ZERG ? SC64_ICON_ZERG_ARMOR :
               race_human == R_PROTOSS ? SC64_ICON_PROTOSS_ARMOR : SC64_ICON_TERRAN_ARMOR;
    }
    if ((kind == UI_ROW_BUILD || kind == UI_ROW_MORPH) && type >= 0) {
        int raw = sc64_race_raw_building(type);
        return raw >= 0 && raw < SC64_ICON_UNIT_COUNT ? raw : -1;
    }
    if (kind != UI_ROW_SPECIAL) return -1;
    label = race_menu_label(i);
    if (ui_same(label, "STORM")) return SC64_ICON_STORM;
    if (ui_same(label, "CLOAK")) return SC64_ICON_CLOAK;
    if (ui_same(label, "CONSUME")) return SC64_ICON_CONSUME;
    if (ui_same(label, "VENTRAL SACS")) return SC64_ICON_VENTRAL_SACS;
    if (ui_same(label, "UNLOAD CARGO")) return SC64_ICON_UNLOAD;
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (ui_same(label, "LIFT / LAND"))
        return race_late_airborne(selected_building) ? SC64_ICON_LAND : SC64_ICON_LIFT;
#endif
    if (ui_same(label, "BUILD INTERCEPTOR")) return 73;
    if (ui_same(label, "CANCEL EGG")) return 36;
    if (ui_same(label, "SUNKEN")) return 146;
    if (ui_same(label, "SPORE")) return 144;
    if (ui_same(label, "RECHARGE")) return 172;
#else
    (void)i; (void)kind; (void)type;
#endif
    return -1;
}

static void
ui_row_icon(int i, int x, int y)
{
    int type, kind;
    int source_icon;
    kind = ui_row_kind(i, &type);
    source_icon = ui_source_icon_for_row(i, kind, type);
    if (source_icon >= 0 && sc64_icon_draw_centered((unsigned)source_icon, x, y, 18)) return;
    if (ui_order_icon(i, x, y)) return;
    if (kind == UI_ROW_BUILD || kind == UI_ROW_MORPH) ui_structure_glyph(x, y);
    else { /* a unit's own command: a spark */
        gfx_diamond(x, y, 4, 4, C_CYAN);
        gfx_pixel(x, y, C_WHITE);
    }
}

#ifdef SC64_HUD_ICONS
#include "hud_icons.h"
#endif
#ifdef SC64_COMMAND_ICONS
#include "command_card.h"
#endif

/* The framed order list: readable source icons, labels and costs. Four rows
 * fit within the playfield; scrolling follows the unchanged command cursor.
 * With the command card, lists that fit the card draw as its grid instead. */
static void
field_controls_draw_command_menu(void)
{
    enum { UI_MENU_VISIBLE_ROWS = 4, UI_MENU_ROW_HEIGHT = 22 };
    int i, count = field_menu_count();
    int first = menu_cursor >= UI_MENU_VISIBLE_ROWS ? menu_cursor - UI_MENU_VISIBLE_ROWS + 1 : 0;
    int rows = count - first < UI_MENU_VISIBLE_ROWS ? count - first : UI_MENU_VISIBLE_ROWS;
    int x = 100, y = 14, w = 138;
#ifdef SC64_COMMAND_ICONS
    if (card_fits()) {
        field_controls_draw_command_card();
        return;
    }
#endif
    gfx_rect(x,y,w,rows*UI_MENU_ROW_HEIGHT+4,C_PANEL);
    sc64_chrome_panel_world(x,y,w,rows*UI_MENU_ROW_HEIGHT+4);
    for (i = first; i < first + rows; i++) {
        int row = y + 4 + (i - first) * UI_MENU_ROW_HEIGHT, cost, gas_cost;
        int label_width;
        char clipped[32];
        const char *need;
        int state = ui_row_state(i, &cost, &gas_cost, &need);
        int short_funds = state == UI_SITE_MINERALS || state == UI_SITE_GAS;
        u8 label = state == UI_SITE_OK || short_funds ? C_SILVER : C_DIM;
        if (i == menu_cursor) {
            ui_highlight(x + 2, row - 1, w - 4);
            if (label == C_SILVER) label = C_WHITE;
        }
        ui_row_icon(i, x + 14, row + 9);
        label_width = cost ? 78 : 106;
        sc64_font_ellipsis(race_menu_label(i), label_width, clipped, sizeof(clipped));
        sc64_font_text(x + 23, row + 6, label_width, clipped, label);
        if (cost && label != C_DIM) /* the HUD explains disabled rows */
            ui_num_right(x + w - 5, row, cost, short_funds ? C_RED : C_GOLD);
    }
    if (first) gfx_diamond(x + w - 8, y + 1, 2, 1, C_GOLD);
    if (first + UI_MENU_VISIBLE_ROWS < count)
        gfx_diamond(x + w - 8, y + rows * UI_MENU_ROW_HEIGHT + 3, 2, 1, C_GOLD);
}

static int
ui_footprint_big(int type)
{
    return type == B_COMMAND || type == B_HATCHERY || type == B_NEXUS || type == B_LAIR;
}

/* Ground marker, the placement footprint and the armed-order cursors, drawn
 * over the race cursor. */
static void
field_controls_draw_cursor(int x, int y)
{
    if (ping_life) {
        int r = 3 + ping_life / 2, px = ping_x - camx, py = ping_y - camy + 12;
#ifdef SC64_ORDER_MARKER_ART
        if (ping_color==C_GREEN || ping_color==C_RED) {
            unsigned phase;
            if (sc64_order_marker_phase(ping_life,&phase)) sc64_order_marker_draw(px,py,phase);
        } else
#endif
        {
        gfx_diamond(px, py, r, r / 2 + 1, ping_color);
        if (ping_life > 9) gfx_diamond(px, py, r - 3, (r - 3) / 2 + 1, ping_color);
        }
    }
    if (mode == MODE_BUILD && box_state != 2) {
        int site_x = cx, site_y = cy, ground, problem, big = ui_footprint_big(menu);
        const char *need;
        field_ui_site(menu, &site_x, &site_y);
        problem = ui_site_problem(menu, site_x, site_y, &need, &ground);
        {
            /* Red: the ground rejects it. Gold: clear ground, but tech,
             * funds or a worker are missing. Green: A places it. */
            int sx = site_x - camx, sy = site_y - camy + 12;
            u8 color = !ground ? C_RED : problem == UI_SITE_OK ? C_GREEN : C_GOLD;
#ifdef SC64_HAS_SPRITES
            const Sc64SpriteFrame *ghost=mission_building_sprite(menu);
            if (ghost && sc64_building_placement_outline(sx,sy,
                    ghost->canvas_w,ghost->canvas_h,ghost->offset_x,ghost->offset_y,
                    ghost->w,ghost->h,ghost->pixels,
                    problem==UI_SITE_OK ? C_GREEN : C_RED)) {
                /* Actual building alpha supplies the shape. Manual-confirmed
                 * red/green meanings; exact N64 placement raster is untraced. */
            } else
#endif
            {
            gfx_frame(sx - (big ? 12 : 8), sy - (big ? 8 : 7), big ? 25 : 17, big ? 18 : 15, color);
            gfx_frame(sx - (big ? 13 : 9), sy - (big ? 9 : 8), big ? 27 : 19, big ? 20 : 17, color);
            gfx_rect(sx - 6, sy - 10, 13, 3, color);
            if (sx != x || sy != y) gfx_line(x, y, sx, sy, C_DIM);
            }
        }
    }
    if (mode == MODE_ATTACK) {
        gfx_frame(x - 5, y - 5, 11, 11, C_RED);
        gfx_rect(x - 9, y, 7, 1, C_RED); gfx_rect(x + 3, y, 7, 1, C_RED);
        gfx_rect(x, y - 9, 1, 7, C_RED); gfx_rect(x, y + 3, 1, 7, C_RED);
        gfx_pixel(x, y, C_WHITE);
    } else if (mode == MODE_MOVE) {
        gfx_rect(x - 7, y, 15, 1, C_GREEN);
        gfx_rect(x, y - 7, 1, 15, C_GREEN);
        gfx_line(x - 6, y - 3, x - 3, y, C_GREEN); gfx_line(x - 6, y + 3, x - 3, y, C_GREEN);
        gfx_line(x + 6, y - 3, x + 3, y, C_GREEN); gfx_line(x + 6, y + 3, x + 3, y, C_GREEN);
    } else if (!menu_open && ui_action == UI_A_ATTACK) {
        gfx_frame(x - 6, y - 6, 13, 13, C_RED);
    } else if (!menu_open && ui_action == UI_A_MOVE) {
        gfx_line(x - 6, y - 3, x - 3, y, C_GREEN); gfx_line(x - 6, y + 3, x - 3, y, C_GREEN);
        gfx_line(x + 6, y - 3, x + 3, y, C_GREEN); gfx_line(x + 6, y + 3, x + 3, y, C_GREEN);
    }
}

/* Colour of the HUD title for the current mode (armed orders stand out). */
static u8
field_controls_title_color(void)
{
    return mode == MODE_MOVE ? C_GREEN : mode == MODE_ATTACK ? C_RED : C_WHITE;
}

/* Hint row text for MODE_NORMAL/MOVE/ATTACK; 0 keeps the profile's own row.
 * While the order list is open its A and B choose and close instead. */
static const char *
field_controls_hint(void)
{
    if (menu_open) return "A CHOOSE  B CLOSE";
    switch (ui_action) {
    case UI_A_MOVE_HERE: return "A MOVE HERE  B BACK";
    case UI_A_ATTACK_HERE: return "A ATTACK HERE  B BACK";
    case UI_A_ATTACK: return "B COMMANDS  A ATTACK";
    case UI_A_SELECT: return "B COMMANDS  A SELECT";
    case UI_A_DESELECT: return "B COMMANDS  A DESELECT";
    case UI_A_GAS: return "B COMMANDS  A GATHER GAS";
    case UI_A_MOVE: return "B COMMANDS  A MOVE";
    default: return 0;
    }
}

/* The shorter hint beside a selected unit's hit points. */
static const char *
field_controls_unit_hint(void)
{
    switch (ui_action) {
    case UI_A_ATTACK: return "A ATTACK B ORDERS";
    case UI_A_SELECT: return "A SELECT B ORDERS";
    case UI_A_GAS: return "A GAS B ORDERS";
    case UI_A_MOVE: return "A MOVE B ORDERS";
    default: return "B ORDERS SEL DROP";
    }
}

/* What a structure row adds: its full name when the label shortens it,
 * else its supply, gas or production role. */
static void
ui_structure_note(int x, int y, int type, const char *label)
{
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(type);
    const char *name = sc64_race_building_name(type);
    if (!ui_same(name, label)) gfx_text(x, y, name, C_DIM);
#ifdef SC64_BATTERY_SERVICE_ENABLED
    else if (sc64_race_raw_building(type)==172)
        gfx_text(x,y,"RECHARGES SHIELDS",C_DIM);
#endif
    else if (r && r->supply_provided) {
        gfx_text(x, y, "+", C_DIM);
        gfx_num(x + 6, y, r->supply_provided / 2, C_DIM);
        gfx_text(x + 12 + ui_num_width(r->supply_provided / 2), y, "SUPPLY", C_DIM);
    } else gfx_text(x, y, ui_gas_building(type) ? "HARVESTS GEYSER GAS" :
                    r && (r->flags & SC_RACE_F_PRODUCER) ? "TRAINS UNITS" : "UNLOCKS TECH", C_DIM);
}

/* Name, then mineral (gold) and gas (cyan) costs, on the HUD title row. */
static void
ui_title_costs(int x, const char *name, int cost, int gas_cost)
{
    gfx_text(x, 126, name, C_WHITE);
    x += ui_text_width(name) + 7;
    if (cost) { gfx_num(x, 126, cost, C_GOLD); x += ui_num_width(cost) + 6; }
    if (gas_cost) gfx_num(x, 126, gas_cost, C_CYAN);
}

/* The HUD's top two rows while the order list is open, a structure is being
 * placed, or a move/attack is armed; 0 leaves the profile's own HUD. With
 * the command card, the command's button sits left of both rows. */
static int
field_controls_draw_hud(void)
{
    const char *need;
#ifdef SC64_COMMAND_ICONS
    const int x = 27;
#else
    const int x = 5;
#endif
    if (menu_open) {
        static const char *const notes[MENU_STOP + 1] = {
            "WALK TO A POINT", "FIGHT ALONG THE WAY", 0, 0, 0, 0, 0,
            "+1 WEAPON DAMAGE", "+1 ARMOR", "HALT AND HOLD"
        };
        int cost, gas_cost, type, state = ui_row_state(menu_cursor, &cost, &gas_cost, &need);
        int kind = ui_row_kind(menu_cursor, &type);
        const char *label = race_menu_label(menu_cursor);
#ifdef SC64_COMMAND_ICONS
        card_hud_icon(card_icon(menu_cursor));
#endif
        ui_title_costs(x, label, cost, gas_cost);
        if (state == UI_SITE_OK && type >= 0) ui_structure_note(x, 137, type, label);
        else ui_status(x, 137, state, need, menu_cursor <= MENU_STOP && notes[menu_cursor] ?
                       notes[menu_cursor] : kind == UI_ROW_SPECIAL ? "A USES IT" : 0, cost, gas_cost);
        return 1;
    }
    if (mode == MODE_BUILD) {
        int site_x = cx, site_y = cy, ground, problem;
        field_ui_site(menu, &site_x, &site_y);
        problem = ui_site_problem(menu, site_x, site_y, &need, &ground);
#ifdef SC64_COMMAND_ICONS
        card_hud_icon(card_building_icon(menu));
#endif
        ui_title_costs(x, sc64_race_building_name(menu), building_cost(menu), building_gas_cost(menu));
        if (problem == UI_SITE_OK) gfx_text(x, 137, "A PLACE  B BACK", C_GREEN);
        else ui_status(x, 137, problem, need, 0, building_cost(menu), building_gas_cost(menu));
        return 1;
    }
    if (mode == MODE_MOVE || mode == MODE_ATTACK) {
        int n = selected_count();
#ifdef SC64_COMMAND_ICONS
        card_hud_icon(mode == MODE_MOVE ? SC64_CMD_ICON_MOVE : SC64_CMD_ICON_ATTACK);
#endif
        gfx_text(x, 126, mode == MODE_MOVE ? "MOVE ORDER" : "ATTACK MOVE", field_controls_title_color());
        if (n) {
            gfx_num(x + 72, 126, n, C_WHITE);
            gfx_text(x + 78 + ui_num_width(n), 126, n == 1 ? "UNIT" : "UNITS", C_DIM);
        }
        gfx_text(x, 137, mode == MODE_MOVE ? "A MOVE HERE  B BACK" : "A ATTACK HERE  B BACK", C_GOLD);
        return 1;
    }
    return 0;
}

/* ---- Pause and result ------------------------------------------------- */

/* Up to lines rows of text wrapped at cols characters, 9 pixels apart. */
static void
ui_text_wrap(int x, int y, int cols, int lines, const char *text, u8 color)
{
    char line[40];
    if (cols > 39) cols = 39;
    while (*text && lines--) {
        int n = 0;
        while (text[n] && n < cols) { line[n] = text[n]; n++; }
        if (text[n] && text[n] != ' ') {
            int last = n;
            while (last > 0 && line[last - 1] != ' ') last--;
            if (last > 0) n = last;
        }
        line[n] = 0;
        gfx_text(x, y, line, color);
        text += n;
        while (*text == ' ') text++;
        y += 9;
    }
}

/* pause_notice: the message in the top two bits, its countdown below. */
enum { UI_NOTICE_SAVED = 1 << 6, UI_NOTICE_SAVE_FAILED = 2 << 6, UI_NOTICE_NO_SAVE = 3 << 6 };

/* Pause list: A chooses the highlighted row. The old one-key shortcuts
 * (B save, L load, R restart, START resume) still work and leave the
 * highlight alone, so A after a B save still resumes. */
static __attribute__((noinline)) void
field_ui_pause_input(u16 pressed)
{
    int choice = -1;
    if (pause_notice && !(--pause_notice & 0x3f)) pause_notice = 0;
    if (pressed & KEY_UP) pause_cursor = (u8)((pause_cursor + 3) % 4);
    if (pressed & KEY_DOWN) pause_cursor = (u8)((pause_cursor + 1) % 4);
    if (pressed & KEY_A) choice = pause_cursor;
    else if (pressed & (KEY_START | KEY_SELECT)) choice = 0;
    else if (pressed & KEY_B) choice = 1;
    else if (pressed & KEY_L) choice = 2;
    else if (pressed & KEY_R) choice = 3;
    if (choice < 0) return;
    if (choice == 0) screen = SCREEN_PLAY;
    else if (choice == 1) pause_notice = (u8)((save_game() ? UI_NOTICE_SAVED : UI_NOTICE_SAVE_FAILED) | 60);
    else if (choice == 2) {
        if (!load_game()) { pause_notice = UI_NOTICE_NO_SAVE | 60; return; }
    } else init_state();
    if (screen != SCREEN_PAUSE) pause_cursor = pause_notice = 0;
}

static void
field_controls_draw_pause(void)
{
    static const char *const items[4] = { "RESUME", "SAVE GAME", "LOAD SAVE", "RESTART" };
    static const char *const keys[4] = { "A", "B", "L", "R" };
    const Sc64MissionInfo *info = race_mode ? 0 : sc64_catalog_get(race_selected_mission);
    int i;
    gfx_rect(24,14,192,132,C_PANEL);
    sc64_chrome_panel_world(24,14,192,132);
    sc64_font_text(36,20,130,"Paused",C_GOLD);
    ui_clock(178,21,(unsigned)frame,C_WHITE);
    for (i = 0; i < 4; i++) {
        int y = 33 + i * 11;
        if (i == pause_cursor) ui_highlight(30, y - 1, 92);
        sc64_font_text(36,y,72,items[i],i==pause_cursor?C_WHITE:C_SILVER);
        ui_key(110, y, keys[i], 0, C_DIM);
    }
    if (pause_notice) {
        int kind = pause_notice & 0xc0;
        gfx_text(130, 33, kind == UI_NOTICE_NO_SAVE ? "NO SAVE FOUND" : kind == UI_NOTICE_SAVE_FAILED ?
                 "SAVE FAILED" : "GAME SAVED", kind == UI_NOTICE_SAVED ? C_GREEN : C_RED);
    }
    gfx_rect(30, 79, 180, 1, C_EDGE);
    gfx_text(30, 83, race_mode ? "SKIRMISH" : "OBJECTIVE", C_GOLD);
    if (info) {
        char title[64];
        sc64_font_ellipsis(info->title,120,title,sizeof(title));
        sc64_font_text(90,81,120,title,C_DIM);
    }
    ui_text_wrap(30, 93, 30, 3, race_mode ? "Destroy all enemy buildings." :
                 race_campaign_objective_text(), C_SILVER);
    gfx_rect(30, 120, 180, 1, C_EDGE);
    ui_key(ui_key(ui_key(30, 124, "A", "SELECT", C_SILVER), 124, "B", "ORDERS", C_SILVER),
           124, "SEL", "DROP", C_SILVER);
    ui_key(ui_key(30, 135, "L", "NEXT  HOLD: BOX", C_SILVER), 135, "R", "ARMY", C_SILVER);
}

static void
field_controls_draw_result(void)
{
    int i, units = 0, buildings = 0, next = won && !race_mode &&
        sc64_catalog_available_next(sc64_catalog_mission_faction(race_selected_mission),
                                    race_selected_mission) >= 0;
    u8 accent = won ? C_GOLD : C_RED;
    const char *title = won ? "MISSION CLEAR" : "MISSION FAILED";
    for (i = 0; i < MAX_UNITS; i++) units += u[i].alive && u[i].owner == SIDE;
    for (i = 0; i < MAX_BUILDINGS; i++) buildings += b[i].alive && b[i].owner == SIDE;
    gfx_rect(28,30,184,86,C_PANEL);
    sc64_chrome_panel_world(28,30,184,86);
    sc64_font_text(120-sc64_font_width(title)/2,35,172,title,accent);
    ui_text_wrap(34, 49, 29, 2, won ? (race_mode ? "OPPONENTS ELIMINATED" : race_campaign_victory_text()) :
                 race_mode ? "YOUR FORCES WERE ELIMINATED" : race_campaign_failure_text(), C_WHITE);
    gfx_rect(40, 68, 160, 1, C_EDGE);
    gfx_text(44, 72, "TIME", C_DIM); ui_clock(44, 82, (unsigned)frame, C_WHITE);
    gfx_text(98, 72, "UNITS", C_DIM); gfx_num(98, 82, units, C_WHITE);
    gfx_text(146, 72, "BUILDINGS", C_DIM); gfx_num(146, 82, buildings, C_WHITE);
    ui_key(ui_key(next ? 42 : 58, 101, "A", next ? "NEXT MISSION" : "RETRY", C_WHITE),
           101, "B", "TITLE SCREEN", C_DIM);
}
