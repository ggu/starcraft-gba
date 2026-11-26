/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Shared save and load entry points, included by game.c. */
static int
save_game(void)
{
#ifdef SC64_SCARAB_AI_RESTOCK_HOST_STAGE
    /* The stage's payer witness has no authenticated save representation. */
    return 0;
#endif
#ifdef SC64_TEXT_BOXES
    /* Neither a queued message nor a deferred result is serialized. */
    if (sc64_text_box_count() || sc64_text_box_outcome.result) return 0;
#endif
#ifdef SC64_SHOWCASE_ENABLED
    if (sc64_showcase_scene >= 0) return 0;
#endif
#ifdef SC64_IMPORTED_MISSION
    /* The save goes ahead: no save holds a delayed player command. */
    sc64_command_delay_flush();
#endif
#ifdef SC64_RACES
#ifdef SC64_Q8_FLASH_ARM_PROBE
    if(!gba_flash_game_transaction_begin())return 0;
    int saved=race_save_game();
    gba_flash_game_transaction_end();return saved;
#else
    return race_save_game();
#endif
#else
#ifdef SC64_MINERAL_FIFO
    GbaSaveSelection previous;
    int have_previous = worker_mineral_fifo_select(&previous);
    data.version = mineral_fifo_enabled ? SC64_MINERAL_FIFO_SAVE_VERSION : SAVE_VERSION;
#else
    data.version = SAVE_VERSION;
#endif
    memcpy(data.units, u, sizeof(u));
    memcpy(data.buildings, b, sizeof(b));
    memcpy(data.minerals_map, m, sizeof(m));
    memcpy(data.tracer_data, tracers, sizeof(tracers));
    memcpy(data.explored_map, explored, sizeof(explored));
    data.minerals = minerals;
    data.gas = gas;
    data.supply_used = supply_used;
    data.supply_cap = supply_cap;
    data.frame = frame;
    data.cx = cx;
    data.cy = cy;
    data.enemy_timer = enemy_timer;
    data.wave = wave;
    data.weapons_level = weapons_level;
    data.armor_level = armor_level;
    data.weapons_research = weapons_research;
    data.armor_research = armor_research;
    data.train_type = train_type;
    data.selected_building = selected_building;
    data.seed = seed;
#ifdef SC64_IMPORTED_MISSION
    data.mission_objective_stage = mission_objective_stage;
#endif
#ifdef SC64_TERRAN_CAMPAIGN
    data.campaign_mission = campaign_mission;
    data.campaign_completed = campaign_completed;
#endif
#ifdef SC64_BACKWATER
    data.backwater = backwater;
    memcpy(data.invincible_units, invincible_units, sizeof(invincible_units));
    memcpy(data.invincible_buildings, invincible_buildings, sizeof(invincible_buildings));
#endif
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    for (int i = 0; i < MAX_RACE_GEYSERS; i++) { data.geyser_x[i] = geysers[i].x; data.geyser_y[i] = geysers[i].y; }
    data.geyser_count = geyser_count;
#endif
#ifdef SC64_SKIRMISH
    memcpy(data.bots, skirmish_ai, sizeof(skirmish_ai));
    data.bot_count = skirmish_bots;
#endif
#ifdef SC_FRONTIER_FACTION
    data.skirmish_map = frontier_pack_setup(original_skirmish_map, SC_FRONTIER_FACTION,
                                           frontier_human_faction, frontier_cpu_faction,
                                           frontier_cpu_team);
#elif defined(SC_ORIGINAL_SKIRMISH)
    data.skirmish_map = original_skirmish_map;
#endif
#ifdef SC64_MINERAL_FIFO
    if (mineral_fifo_enabled && !worker_mineral_fifo_encode()) return 0;
    int saved = have_previous
        ? save_write_selected(&data, sizeof(data), data.version, &previous, mineral_fifo_save_tags, 2)
        : save_write_first(&data, sizeof(data), data.version);
    if (!saved) return 0;
#else
    save_write(&data, sizeof(data), SAVE_VERSION);
#endif
    toast(7);
    return 1;
#endif
}

static void game_audio_prime(void);

static int
load_game(void)
{
#ifdef SC64_SCARAB_AI_RESTOCK_HOST_STAGE
    /* No saved record authenticates the stage's payer witnesses yet. */
    return 0;
#endif
#ifdef SC64_SHOWCASE_ENABLED
    if (sc64_showcase_scene >= 0) return 0;
#endif
#ifdef SC64_RACES
#ifdef SC64_Q8_FLASH_ARM_PROBE
    if(!gba_flash_game_transaction_begin())return 0;
#endif
    int loaded = race_load_game();
#ifdef SC64_Q8_FLASH_ARM_PROBE
    gba_flash_game_transaction_end();
#endif
#ifdef SC64_TEXT_BOXES
    if (loaded) sc64_text_box_reset();
#endif
    if (loaded) { sc64_walk_reset_all(); game_audio_prime(); sc64_command_delay_reset(); }
#ifdef SC64_UNIT_FACING
    if (loaded) sc64_facing_reset_all();
#endif
#ifdef SC64_FIELD_CONTROLS
    if (loaded) { ping_life=0; ui_action = ui_tick_mode = 0; } /* frame jumped: resolve A afresh */
#endif
    return loaded;
#else
#ifdef SC64_MINERAL_FIFO
    GbaSaveSelection selected;
    if (!worker_mineral_fifo_select(&selected) || !raw_game_save_apply(1)) return 0;
    mineral_fifo = prepared_mineral_fifo;
    mineral_fifo_enabled = selected.tag == SC64_MINERAL_FIFO_SAVE_VERSION;
    return 1;
}

static int
raw_game_save_apply(int apply)
{
#else
    if (!save_read(&data, sizeof(data), SAVE_VERSION) || data.version != SAVE_VERSION)
	return 0;
#endif
    int i, j, computed_supply = 0, computed_cap =
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
        0;
#else
        20;
#endif
#ifdef SC64_IMPORTED_MISSION
    if (data.mission_objective_stage < 0 || data.mission_objective_stage > 1)
	return 0;
#endif
#ifdef SC64_TERRAN_CAMPAIGN
    if (data.campaign_mission < 0 || data.campaign_mission >= SC64_CAMPAIGN_COUNT ||
	data.campaign_completed < 0 || data.campaign_completed > 3) return 0;
#endif
    if (data.minerals < 0 || data.minerals > 100000 || data.gas < 0 || data.gas > 100000 || data.supply_cap < 0 || data.frame < 0)
	return 0;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    if (data.geyser_count < 0 || data.geyser_count > MAX_RACE_GEYSERS) return 0;
#endif
#ifdef SC64_SKIRMISH
    if (data.bot_count < 1 || data.bot_count > 3) return 0;
#endif
#ifdef SC_FRONTIER_FACTION
    int saved_map, saved_human, saved_team;
    unsigned char saved_factions[3];
    if (!frontier_unpack_setup(data.skirmish_map, SC_FRONTIER_FACTION, &saved_map,
                               &saved_human, saved_factions, &saved_team)) return 0;
#elif defined(SC_ORIGINAL_SKIRMISH)
    if ((unsigned)data.skirmish_map >= ORIGINAL_MAP_COUNT) return 0;
#endif
    for (i = 0; i < MAX_UNITS; i++) {
	if (data.units[i].alive) {
	    if (data.units[i].type < U_SCV || data.units[i].type > U_RAYNOR || data.units[i].owner < SIDE || data.units[i].owner > 4 || data.units[i].hp <= 0 || data.units[i].x < 0 || data.units[i].x >= WORLD_W || data.units[i].y < 0 || data.units[i].y >= WORLD_H || data.units[i].order < ORDER_IDLE || data.units[i].order > ORDER_GAS || data.units[i].path_retry < 0 || data.units[i].path_retry > 100)
		return 0;
	    if (data.units[i].owner == SIDE)
		computed_supply += unit_supply(data.units[i].type);
	}
    }
    for (i = 0; i < MAX_BUILDINGS; i++)
	if (data.buildings[i].alive) {
    if (data.buildings[i].type < B_COMMAND || data.buildings[i].type > B_CREEP_COLONY ||
		data.buildings[i].queue_count < 0 || data.buildings[i].queue_count > 5 || data.buildings[i].queue_head < 0 || data.buildings[i].queue_head > 4 || data.buildings[i].queue_progress < 0 || data.buildings[i].build_progress < 0 ||
		data.buildings[i].hp <= 0 || data.buildings[i].x < 0 || data.buildings[i].x >= WORLD_W ||
		data.buildings[i].y < 0 || data.buildings[i].y >= WORLD_H ||
		(data.buildings[i].owner < SIDE || data.buildings[i].owner >
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
                4
#else
                ENEMY
#endif
                ))
		return 0;
	    for (j = 0; j < data.buildings[i].queue_count; j++) {
		int queued = data.buildings[i].queue[(data.buildings[i].queue_head + j) % 5];
		if (queued < U_SCV || queued > U_MEDIC)
		    return 0;
	    }
	    if (data.buildings[i].owner == SIDE && data.buildings[i].type == B_DEPOT && !data.buildings[i].under_construction)
		computed_cap += 16;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
            if (data.buildings[i].owner == SIDE && data.buildings[i].type == B_COMMAND && !data.buildings[i].under_construction) computed_cap += 20;
#endif
	}
    /* Provider loss can leave a legitimate save over its current limit. The
     * stored provider total must still match the actual surviving providers. */
    if (computed_cap != data.supply_cap ||
	data.train_type < U_MARINE || data.train_type > U_MEDIC ||
	data.weapons_level < 0 || data.weapons_level > 2 || data.armor_level < 0 || data.armor_level > 2)
	return 0;
#ifdef SC64_MINERAL_FIFO
    if (!apply) return 1;
#endif
    memcpy(u, data.units, sizeof(u));
    /* Older non-race saves recorded the current upgrade in newly spawned
     * units. Armor is derived from type and the live upgrade is applied on
     * hit, so normalize the validated living units without changing the save
     * format or their other state. */
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive)
            u[i].armor = u[i].type == U_FIREBAT || u[i].type == U_MEDIC;
    memcpy(b, data.buildings, sizeof(b));
    invalidate_path_grid();
    memcpy(m, data.minerals_map, sizeof(m));
    memcpy(tracers, data.tracer_data, sizeof(tracers));
    sc64_trace_duration_cache_reset();
    sc64_trace_slots_reset();
    death_vfx_reset();
    memcpy(explored, data.explored_map, sizeof(explored));
#ifdef SC_ORIGINAL_SKIRMISH
    frontier_reset_exploration_cache();
#endif
#ifdef SC64_TERRAN_CAMPAIGN
    campaign_mission = data.campaign_mission;
    campaign_completed = data.campaign_completed;
#endif
#ifdef SC64_IMPORTED_MISSION
    sc64_visibility_reset();
#endif
    minerals = data.minerals;
    gas = data.gas;
    supply_used = computed_supply;
    supply_cap = data.supply_cap;
    frame = data.frame;
    cx = data.cx;
    cy = data.cy;
    enemy_timer = data.enemy_timer;
    wave = data.wave;
    weapons_level = data.weapons_level;
    armor_level = data.armor_level;
    weapons_research = data.weapons_research;
    armor_research = data.armor_research;
    train_type = data.train_type;
    selected_building = -1;
    seed = data.seed;
#ifdef SC64_IMPORTED_MISSION
    mission_objective_stage = data.mission_objective_stage;
#endif
#ifdef SC64_BACKWATER
    backwater = data.backwater;
    memcpy(invincible_units, data.invincible_units, sizeof(invincible_units));
    memcpy(invincible_buildings, data.invincible_buildings, sizeof(invincible_buildings));
    backwater_message_ticks = 0;
#endif
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    geyser_count = data.geyser_count;
    for (int k = 0; k < MAX_RACE_GEYSERS; k++) { geysers[k].x = data.geyser_x[k]; geysers[k].y = data.geyser_y[k]; }
#endif
#ifdef SC64_SKIRMISH
    memcpy(skirmish_ai, data.bots, sizeof(skirmish_ai)); skirmish_bots = data.bot_count;
#endif
#ifdef SC_FRONTIER_FACTION
    original_skirmish_map = saved_map;
    frontier_human_faction = saved_human;
    memcpy(frontier_cpu_faction, saved_factions, sizeof(frontier_cpu_faction));
    frontier_cpu_team = saved_team;
    for (i = 0; i < skirmish_bots; ++i)
        skirmish_ai[i].team = skirmish_team_for_owner(skirmish_ai[i].player);
    frontier_setup_row = FRONTIER_SETUP_MAP;
#elif defined(SC_ORIGINAL_SKIRMISH)
    original_skirmish_map = data.skirmish_map;
#endif
    mode = MODE_NORMAL;
    menu_open = 0;
    selected_building = data.selected_building >= 0 && data.selected_building < MAX_BUILDINGS && b[data.selected_building].alive ? data.selected_building : -1;
    screen = SCREEN_PLAY;
    won = lost = 0;
    reset_selection_gestures();
#ifdef SC_FIELD_FEEDBACK
    ping_life=0;
#endif
    update_exploration();
    toast(0);
    game_audio_prime();
#ifdef SC64_IMPORTED_MISSION
    sc64_command_delay_reset(); /* pending commands belonged to the old world */
#endif
    return 1;
#endif
}
