/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Layer facts from the authenticated USA unit table, 00C/001. Ordinary
 * units, finished buildings and mineral fields share layer 4. */
static unsigned
sc64_world_actor_layer(int raw, unsigned fallback)
{
    switch (raw) {
    case 194: case 195: case 196: case 197: case 198: case 199:
        return 0;
    case 191: case 192: case 193: case 204: case 214:
        return 1;
    case 35: case 142:
        return 2;
    case 125:
        return 3;
    case 9: case 11: case 12: case 22: case 27: case 28: case 29:
    case 42: case 57: case 58: case 71: case 72: case 82: case 86: case 102:
        return 16;
    case 8: case 14: case 21: case 33: case 43: case 44: case 45: case 47:
    case 49: case 55: case 56: case 59: case 60: case 62: case 69: case 70:
    case 73: case 80: case 84: case 88: case 91: case 92: case 94: case 98: case 101:
        return 18;
    case 105: case 202:
        return 19;
    default:
        return (unsigned)raw < 228u ? 4u : fallback;
    }
}

static unsigned
sc64_building_world_layer(unsigned slot)
{
    const Building *q = &b[slot];
    /* The destruction scan can still read the flight flag after the actor
     * is marked dead. Landing clears that flag and restores ground depth. */
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_building_in_flight(q)) return 12;
#endif
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (race_bc_facility_in_flight(q)) return 12;
#endif
    int raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type);
    return sc64_world_actor_layer(raw, 4);
}

static unsigned
sc64_unit_world_layer(const Unit *q)
{
    /* Native Burrow lowers the sprite to layer 1 until Unburrow. Read the
     * retained instance flag even after death clears the actor's alive bit. */
    if (q->flags & 2u) return 1;
#ifdef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
    if (q->type == U_P7_INTERCEPTOR) return race_p7_interceptor_flight_layer(q);
#endif
    unsigned layer = sc64_world_actor_layer(sc64_unit_art_raw(q),
        sc64_race_unit_flying(q->type) ? 16u : 4u);
    /* Native SCVs gain one layer while attached to a foundation. Use the
     * same work radius as update_orders, including after a save/load. */
    if (q->order == ORDER_BUILD && q->type == U_SCV &&
        (unsigned)q->target < MAX_BUILDINGS) {
        const Building *foundation = &b[q->target];
        if (foundation->alive && foundation->under_construction &&
            foundation->owner == q->owner &&
            dist2(q->x, q->y, foundation->x, foundation->y) <=
                BUILD_WORK_RANGE * BUILD_WORK_RANGE)
            ++layer;
    }
    return layer;
}

enum {
    DEPTH_MINERAL, DEPTH_GEYSER, DEPTH_BUILDING, DEPTH_UNIT, DEPTH_PROJECTILE,
    DEPTH_DEATH, DEPTH_NUKE_EFFECT, DEPTH_STORM,
    DEPTH_SLOT_BITS = 10,
    DEPTH_SLOT_MASK = (1 << DEPTH_SLOT_BITS) - 1,
    DEPTH_KIND_MASK = 7,
    DEPTH_TURRET_BIT = 1 << (DEPTH_SLOT_BITS + 3),
    DEPTH_Y_BITS = DEPTH_SLOT_BITS + 4,
    DEPTH_WORLD_Y_BITS = 13,
    DEPTH_LAYER_BITS = DEPTH_Y_BITS + DEPTH_WORLD_Y_BITS,
};

_Static_assert(MAX_UNITS <= DEPTH_SLOT_MASK + 1 &&
               MAX_BUILDINGS <= DEPTH_SLOT_MASK + 1 &&
               MAX_MINERALS <= DEPTH_SLOT_MASK + 1 &&
               MAX_RACE_GEYSERS <= DEPTH_SLOT_MASK + 1,
               "painter keys must hold every actor slot");
_Static_assert(DEPTH_STORM <= DEPTH_KIND_MASK, "painter keys must hold every kind");
#ifdef SC64_UNIT_FACING
_Static_assert(MAX_UNITS * 2 <= DEPTH_SLOT_MASK + 1,
               "turret keys must hold the hull's slot and stride lift");
#endif

enum { DEPTH_ATTACK_SPRITE_BASE = MAX_TRACERS
#if defined(SC64_PROJECTILE_DEPTH) && defined(SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT)
                                 + SC64_BC_SHOT_LIMIT
#endif
};
#ifdef SC64_PROJECTILE_DEPTH
enum { DEPTH_FLIGHT_TRAIL_BASE = DEPTH_ATTACK_SPRITE_BASE + MAX_TRACERS };
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
enum { DEPTH_INTERCEPTOR_BASE =
#ifdef SC64_PROJECTILE_DEPTH
                               DEPTH_FLIGHT_TRAIL_BASE + MAX_TRACERS
#else
                               DEPTH_ATTACK_SPRITE_BASE
#endif
};
_Static_assert(DEPTH_INTERCEPTOR_BASE + SC64_POOL_CAPACITY <= DEPTH_SLOT_MASK + 1,
               "painter keys must hold every projectile slot");
#endif

static void
sc64_depth_push_key(u32 *heap, unsigned *size, u32 key)
{
    unsigned at = (*size)++;
    while (at) {
        unsigned parent = (at - 1) / 2;
        if (heap[parent] <= key) break;
        heap[at] = heap[parent];
        at = parent;
    }
    heap[at] = key;
}

/* Ground depth uses the world anchor. The turret bit sorts after all
 * ordinary sprites at the same elevation and Y. Kind and slot break ties. */
static void
sc64_depth_push(u32 *heap, unsigned *size, unsigned layer, int y,
                unsigned kind, unsigned slot)
{
    unsigned row = layer <= 4 ? (unsigned)limit(y, 0, (1 << DEPTH_WORLD_Y_BITS) - 1) : 0;
    u32 key = (layer << DEPTH_LAYER_BITS) | (row << DEPTH_Y_BITS) |
              (kind << DEPTH_SLOT_BITS) | slot;
    sc64_depth_push_key(heap, size, key);
}

static u32
sc64_depth_pop(u32 *heap, unsigned *size)
{
    u32 first = heap[0], key = heap[--*size];
    unsigned at = 0;
    while (at * 2 + 1 < *size) {
        unsigned child = at * 2 + 1;
        if (child + 1 < *size && heap[child + 1] < heap[child]) ++child;
        if (key <= heap[child]) break;
        heap[at] = heap[child];
        at = child;
    }
    heap[at] = key;
    return first;
}

#ifdef SC64_PROJECTILE_DEPTH
_Static_assert(MAX_TRACERS <= 255 && MAX_UNITS + MAX_BUILDINGS <= 32767,
               "attacker image lists must hold every trace and actor");
_Static_assert(DEPTH_FLIGHT_TRAIL_BASE + MAX_TRACERS <= DEPTH_SLOT_MASK + 1,
               "painter keys must hold every projectile slot");

/* These prepared moving projectiles use SPROL for their flight trails.
 * The child gets parent elevation plus one at birth. Above layer 4,
 * its fixed birth anchor affects raster placement but no longer sorts by Y. */
static int
sc64_depth_separate_flight_trails(const Sc64TraceSlot *slot, unsigned layer)
{
    if (layer < 4 || layer >= 31 || !slot->profile_plus1) return 0;
    const Sc64WeaponSourceAttackProfile *profile =
        &sc64_weapon_source_profiles[slot->profile_plus1 - 1u];
    const unsigned image = profile->projectile_image;
    return image == 528 || image == 529 || image == 532;
}

/* A projectile at a ground layer shares the same world-Y tie as actors.
 * Use the current projectile anchor, including the fixed anchor of a
 * target- or source-position effect. */
static int
sc64_trace_world_y(unsigned i)
{
    const Tracer *trace = &tracers[i];
    const Sc64TraceSlot *slot = &sc64_trace_slots[i];
    Sc64WeaponSourceVfxKey key;
    if (!sc64_weapon_source_vfx_unpack((uint32_t)trace->color, &key))
        return trace->y0;
    const Sc64WeaponSourceAttackProfile *profile = slot->profile_plus1 ?
        &sc64_weapon_source_profiles[slot->profile_plus1 - 1] :
        sc64_weapon_source_trace_profile((uint32_t)trace->color);
    if (!profile) return trace->y0;

    int x, y;
    sc64_weapon_source_trace_launch_origin(profile, key.direction,
        trace->x0, trace->y0, &x, &y);
    int age = sc64_tracer_duration((int)i) - trace->life;
    uint32_t source_age = sc64_weapon_source_vfx_source_tick(age > 0 ? (unsigned)age : 0u);
    uint32_t start = sc64_weapon_source_trace_release_ticks(profile) +
        sc64_weapon_source_trace_startup_ticks(profile);
    Sc64WeaponSourceMotion prepared = {
        .distance = (uint64_t)slot->root << 9, .flight = slot->flight
    };
    int px, py;
    if (sc64_weapon_source_trace_position_motion(profile, x, y, trace->x1, trace->y1,
            source_age > start ? source_age - start : 0u, &px, &py,
            slot->profile_plus1 ? &prepared : 0))
        return py;
    return y;
}
#endif

#ifdef SC64_DEATH_WORLD_DEPTH
_Static_assert(SC64_DEATH_VFX_POOL * SC64_DEATH_SOURCE_MAX_LAYERS <= DEPTH_SLOT_MASK + 1 &&
               SC64_DEATH_SOURCE_DEPTH_LAYERS <= SC64_DEATH_SOURCE_MAX_LAYERS,
               "painter keys must hold every source death layer");

static void __attribute__((noinline))
sc64_depth_push_deaths(u32 *heap, unsigned *size)
{
    u32 now = death_vfx_now();
    for (unsigned i = 0; i < SC64_DEATH_VFX_POOL; ++i) {
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
        Sc64DeathVfx effect;
        death_vfx_load_row((int)i, &effect);
        const Sc64DeathVfx *row = &effect;
#else
        const Sc64DeathVfx *row = &death_vfx[i];
#endif
        if (!row->source || sc64_death_vfx_expired(row, now)) continue;
        int x = row->x - camx, y = row->y - camy + 12;
        if (!death_vfx_in_view(row, x, y)) continue;
        const Sc64DeathSource *art = sc64_death_source(row->raw);
        u32 age = sc64_death_vfx_age_su(row->birth, now);
        for (unsigned j = 0; j < art->layer_count; ++j) {
            const Sc64DeathSourceLayer *piece = &art->layers[j];
            if (age < piece->start_su || age >= piece->end_su) continue;
            int layer = death_vfx_world_layer(row, piece);
            if (layer < 0) continue;
            int world_y = row->y;
            if (piece->flags & (SC64_DEATH_SOURCE_FIXED_LOW | SC64_DEATH_SOURCE_DETACHED_SPRITE))
                world_y += piece->y;
            sc64_depth_push(heap, size, (unsigned)layer, world_y, DEPTH_DEATH,
                            i * SC64_DEATH_SOURCE_MAX_LAYERS + j);
        }
    }
}

static void __attribute__((noinline))
sc64_depth_draw_death(unsigned slot)
{
    unsigned index = slot / SC64_DEATH_SOURCE_MAX_LAYERS;
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
    Sc64DeathVfx effect;
    death_vfx_load_row((int)index, &effect);
    const Sc64DeathVfx *row = &effect;
#else
    const Sc64DeathVfx *row = &death_vfx[index];
#endif
    const Sc64DeathSource *art = sc64_death_source(row->raw);
    const Sc64DeathSourceLayer *piece = &art->layers[slot % SC64_DEATH_SOURCE_MAX_LAYERS];
    unsigned frame_index;
    if (sc64_death_source_layer_frame(piece, sc64_death_vfx_age_su(row->birth, death_vfx_now()),
                                     &frame_index))
        death_vfx_draw_source_layer(piece, row->x - camx, row->y - camy + 12, frame_index);
}
#endif

static void
draw_world_building_ring(Building *q)
{
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (sc64_game_building_has_flag(q->type, SC64_GAME_ACTOR_INVINCIBLE)) return;
#endif
    int width = 24, bottom = q->y - camy + 24;
#ifdef SC64_HAS_SPRITES
    const Sc64SpriteFrame *sprite = building_art_sprite(q);
    if (sprite) {
        width = sprite->w;
        bottom = q->y - camy + 12 - sprite->canvas_h / 2 + sprite->offset_y + sprite->h;
    }
#endif
    draw_building_ring(q, q->x - camx, bottom, width, SELECTION_RING, 0);
}

static void __attribute__((noinline))
sc64_draw_depth_ordered_actors(void)
{
    u32 heap[MAX_UNITS + MAX_BUILDINGS + MAX_MINERALS + MAX_RACE_GEYSERS
#ifdef SC64_PROJECTILE_DEPTH
             + MAX_TRACERS * 2
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
             + SC64_BC_SHOT_LIMIT
#endif
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
             + SC64_POOL_CAPACITY
#endif
#ifdef SC64_DEATH_WORLD_DEPTH
             + SC64_DEATH_VFX_POOL * SC64_DEATH_SOURCE_DEPTH_LAYERS
#endif
#ifdef SC64_SHARED_NUKE_PROBE
             + SC64_NUKE_FLIGHT_LIMIT * SC64_NUKE_MAX_DRAW_PARTS
#endif
#ifdef SC64_STORM_RENDER
             + SC64_STORM_MAX_ACTIVE
#endif
             ];
    unsigned size = 0;
    for (unsigned i = 0; i < MAX_MINERALS; ++i) {
        if (m[i].amount <= 0) continue;
        int x = m[i].x - camx, y = m[i].y - camy + 12;
        if (mineral_in_view(&m[i], x, y) &&
            currently_visible(m[i].x, m[i].y))
            sc64_depth_push(heap, &size, 4, m[i].y, DEPTH_MINERAL, i);
    }
    for (int i = 0; i < geyser_count; ++i) {
        int x = geysers[i].x - camx, y = geysers[i].y - camy + 12;
        if (geyser_in_view(x, y) && currently_visible(geysers[i].x, geysers[i].y))
            sc64_depth_push(heap, &size, 4, geysers[i].y, DEPTH_GEYSER, (unsigned)i);
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive) continue;
        int x = b[i].x - camx, y = b[i].y - camy + 12;
        if (x < -128 || x > 368 || y < -128 || y > 248) continue;
        if (b[i].owner != SIDE && !currently_visible(b[i].x, b[i].y)) continue;
        sc64_depth_push(heap, &size, sc64_building_world_layer(i), b[i].y, DEPTH_BUILDING, i);
    }
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (!q->alive) continue;
        int x = q->x - camx, y = q->y - camy + 12;
        if (!sc64_unit_anchor_in_view(q, x, y)) continue;
        if (q->owner != SIDE && !currently_visible(q->x, q->y)) continue;
#ifdef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
        if (q->type == U_P7_INTERCEPTOR && SC64_CARRIER_STATE_HIDDEN(q)) continue;
#endif
        sc64_depth_push(heap, &size, sc64_unit_world_layer(q), q->y, DEPTH_UNIT, i);
    }
#ifdef SC64_PROJECTILE_DEPTH
    /* Traces sharing a shooter paint in slot order within its existing
     * draw turn. Use the actor key directly for both image and body. */
    u8 image_heads[MAX_UNITS + MAX_BUILDINGS], image_next[MAX_TRACERS];
    int have_images = 0;
    for (unsigned remaining = MAX_TRACERS; remaining; --remaining) {
        unsigned i = remaining - 1;
        const Sc64TraceSlot *slot = &sc64_trace_slots[i];
        if (!tracers[i].life) continue;
        if (slot->attacker_plus1 > 0 && (slot->attacker_parts & SC64_TRACE_ATTACKER_IMAGE)) {
            unsigned ref = (unsigned)slot->attacker_plus1 - 1;
            if (!have_images) {
                memset(image_heads, 0, sizeof(image_heads));
                have_images = 1;
            }
            image_next[i] = image_heads[ref];
            image_heads[ref] = (u8)(i + 1);
        }
        if (slot->bullet_layer && !slot->release_life) {
            unsigned layer = slot->bullet_layer;
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
            layer &= ~SC64_TRACE_EXPLICIT_DEPTH;
#endif
            int y = layer <= 4 ? sc64_trace_world_y(i) : 0;
            sc64_depth_push(heap, &size, layer, y, DEPTH_PROJECTILE, i);
        }
        Sc64AttackSpriteDraw draw;
        if (slot->attack_sprite_layer && sc64_attack_sprite_draw_state(i, &draw))
            sc64_depth_push(heap, &size, slot->attack_sprite_layer, draw.y,
                            DEPTH_PROJECTILE, DEPTH_ATTACK_SPRITE_BASE + i);
    }
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
    for (unsigned i = 0; i < SC64_BC_SHOT_LIMIT; ++i) {
        const Sc64BcYamatoShot *shot = &sc64_bc_shots[i];
        Sc64BcMotionFields motion;
        if (sc64_bc_motion_unpack(shot, &motion))
            sc64_depth_push(heap, &size, race_bc_yamato_draw_layer(shot, &motion),
                            (int)(motion.y_q8 / 512u), DEPTH_PROJECTILE, MAX_TRACERS + i);
    }
#endif
#endif
#ifdef SC64_DEATH_WORLD_DEPTH
    sc64_depth_push_deaths(heap, &size);
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    if (race_protoss_services_active()) race_p7_projectile_push_depth(heap, &size);
#endif
#ifdef SC64_SHARED_NUKE_PROBE
    sc64_depth_push_nuke_effects(heap, &size);
#endif
#ifdef SC64_STORM_RENDER
    const Sc64PsionicStormState *storms = race_storm_for_draw();
    if (storms)
        for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i)
            /* Current adapters cast at points. The native bullet takes
             * the caster's elevation (4) plus one for a point cast. */
            if (storms->storm[i].active)
                sc64_depth_push(heap, &size, 5, storms->storm[i].y, DEPTH_STORM, i);
#endif
    while (size) {
        u32 key = sc64_depth_pop(heap, &size);
        unsigned i = key & DEPTH_SLOT_MASK;
        switch ((key >> DEPTH_SLOT_BITS) & DEPTH_KIND_MASK) {
        case DEPTH_MINERAL:
            draw_map_mineral((int)i);
            break;
        case DEPTH_GEYSER:
            draw_map_geyser((int)i);
            break;
        case DEPTH_BUILDING: {
            if (selected_building == (int)i) draw_world_building_ring(&b[i]);
#ifdef SC64_PROJECTILE_DEPTH
            unsigned first = have_images ? image_heads[MAX_UNITS + i] : 0;
            if (first) sc64_draw_attacker_images(first, image_next, SC64_TRACE_ATTACKER_UNDER);
#endif
            draw_building_art(&b[i]);
#ifdef SC64_PROJECTILE_DEPTH
            if (first) sc64_draw_attacker_images(first, image_next, SC64_TRACE_ATTACKER_OVER);
#endif
            draw_building_overlay(&b[i]);
            break;
        }
#ifdef SC64_DEATH_WORLD_DEPTH
        case DEPTH_DEATH:
            sc64_depth_draw_death(i);
            break;
#endif
#ifdef SC64_SHARED_NUKE_PROBE
        case DEPTH_NUKE_EFFECT:
            race_nuke_draw_part(i);
            break;
#endif
#ifdef SC64_STORM_RENDER
        case DEPTH_STORM:
            draw_storm_marker(&storms->storm[i]);
            break;
#endif
        case DEPTH_UNIT: {
#ifdef SC64_UNIT_FACING
            if (key & DEPTH_TURRET_BIT) {
                unsigned hull = i >> 1;
                sc64_facing_draw_tank_turret(hull, i & 1);
#ifdef SC64_PROJECTILE_DEPTH
                /* Both Tank firing flashes attach to the turret image. */
                if (have_images && image_heads[hull])
                    sc64_draw_attacker_images(image_heads[hull], image_next,
                                              SC64_TRACE_ATTACKER_OVER);
#endif
                break;
            }
#endif
            if (u[i].selected) draw_unit_pass(&u[i], 1);
            int turret_lift = -1;
#ifdef SC64_PROJECTILE_DEPTH
            draw_unit_with_images(&u[i], 0, have_images ? image_heads[i] : 0,
                                  image_next, &turret_lift);
#else
            draw_unit_with_images(&u[i], 0, 0, 0, &turret_lift);
#endif
#ifdef SC64_UNIT_FACING
            /* The hull already left the heap. Its turret takes that
             * capacity, so a full queue needs no extra storage. */
            if (turret_lift >= 0)
                sc64_depth_push_key(heap, &size, (key & ~DEPTH_SLOT_MASK) |
                                    DEPTH_TURRET_BIT | (i << 1) | (unsigned)turret_lift);
#endif
            break;
        }
#if defined(SC64_PROJECTILE_DEPTH) || defined(SC64_P7_PROJECTILES_ENABLED)
        case DEPTH_PROJECTILE: {
#ifdef SC64_P7_PROJECTILES_ENABLED
            if (i >= DEPTH_INTERCEPTOR_BASE) {
                race_p7_projectile_draw_depth(i - DEPTH_INTERCEPTOR_BASE);
                break;
            }
#endif
#ifdef SC64_PROJECTILE_DEPTH
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
            u32 start = gba_clock();
#endif
            if (i >= DEPTH_FLIGHT_TRAIL_BASE) {
                draw_tracer((int)(i - DEPTH_FLIGHT_TRAIL_BASE),
                            SC64_TRACE_PROJECTILE | SC64_TRACE_ONLY_FLIGHT_TRAILS);
            } else if (i >= DEPTH_ATTACK_SPRITE_BASE) {
                Sc64AttackSpriteDraw draw;
                if (sc64_attack_sprite_draw_state(i - DEPTH_ATTACK_SPRITE_BASE, &draw))
                    sc64_weapon_source_vfx_draw_image(draw.image, 0, draw.age,
                                                       draw.direction, draw.x, draw.y);
            }
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
            else if (i >= MAX_TRACERS) draw_yamato_flight(i - MAX_TRACERS);
#endif
            else {
                unsigned parts = SC64_TRACE_PROJECTILE;
                unsigned layer = key >> DEPTH_LAYER_BITS;
                if (sc64_depth_separate_flight_trails(&sc64_trace_slots[i], layer)) {
                    /* The trail group sorts after the parent and reuses
                     * its vacated heap entry. */
                    sc64_depth_push(heap, &size, layer + 1, 0, DEPTH_PROJECTILE,
                                    DEPTH_FLIGHT_TRAIL_BASE + i);
                    parts |= SC64_TRACE_WITHOUT_FLIGHT_TRAILS;
                }
                draw_tracer((int)i, parts);
            }
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
            diag_tracer_cycles += gba_clock() - start;
#endif
#endif
            break;
        }
#endif
        }
    }
}
