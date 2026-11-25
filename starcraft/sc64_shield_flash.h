/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SHIELD_FLASH_H
#define SC64_SHIELD_FLASH_H

/* Protoss shield hit overlay, presentation only.
 *
 * A hit that takes shield points and leaves some shield standing shows the
 * StarCraft 64 shield overlay (images.dat row 424, OpenBW
 * IMAGEID_Shield_Overlay) on the side the hit came from, as OpenBW's
 * create_shield_damage_effect does: offset index (heading - 124) / 8 mod 32
 * into LO kind 5 of the target's main image, at the image's current facing
 * frame (mirrored with it when the image is directional). The overlay plays
 * its own Init stream (four frames, one source tick each in the
 * authenticated bank) and then ends. Units and buildings whose units.dat row
 * enables shields are covered (sc64_weapon_source_shield_actors).
 *
 * Nothing here is read by the simulation or saved. The hit direction is the
 * attacker's position at the attack decision: immediate hits set it around
 * their damage, and queued impacts carry it in the impact adapter's
 * presentation-only row directions. A hit with no known direction (spells,
 * splash, an impact restored from a save) uses the target's own facing. An
 * overlay is shown only on a unit the renderer drew this frame, so loaded,
 * hidden, burrowed or undetected units never show one. Slots are bounded; a
 * new hit replaces the oldest overlay.
 *
 * The experimental Q8 combat path commits shields elsewhere and shows none.
 * Interceptor, Yamato and trap hits use the facing fallback.
 *
 * Mirroring follows OpenBW's get_image_lo_offset: a mirrored facing negates
 * the offset's x. The ROM's shield tables hold the same 32 offsets in every
 * facing frame, so for a west-facing unit this puts the overlay on the side
 * away from the attacker. It is kept as the reference behaviour until an N64
 * capture shows otherwise. Builds with the Scarab weapon probe queue keep no
 * row directions; their queued hits use the facing fallback. */

#if !defined(SC64_WEAPON_SOURCE_SHIELD_OVERLAY)
#error "SC64 shield flash needs a weapon source bank with the shield overlay; run ./dev sc64 prepare for this profile"
#endif

#define SC64_SHIELD_FLASH_SLOTS 8u
#define SC64_SHIELD_DIR_NONE 0xffu

typedef struct {
    u16 actor;    /* unit slot, or MAX_UNITS + building slot */
    u16 born;     /* low 16 bits of the game update counter */
    s8 dx, dy;    /* LO offset resolved at the hit (mirror applied), half-scale world */
    u8 type;      /* actor type at the hit, guards a dead or replaced actor */
    u8 live;
    u8 seq;       /* insertion order, so a full pool replaces the oldest */
} Sc64ShieldFlash;

static Sc64ShieldFlash sc64_shield_flashes[SC64_SHIELD_FLASH_SLOTS];
static u8 sc64_shield_flash_next, sc64_shield_flash_seq;
static u8 sc64_shield_drawn[(MAX_UNITS + 7) / 8];  /* units drawn this frame */

static void
sc64_shield_flash_reset(void)
{
    memset(sc64_shield_flashes, 0, sizeof(sc64_shield_flashes));
    memset(sc64_shield_drawn, 0, sizeof(sc64_shield_drawn));
    sc64_shield_flash_next = 0;
    sc64_shield_hit_dir = SC64_SHIELD_DIR_NONE;
    sc64_shield_launch_dir = SC64_SHIELD_DIR_NONE;
}

/* Slot reuse: a new actor in this slot never inherits an overlay. */
static void
sc64_shield_flash_clear_actor(int actor)
{
    for (unsigned i = 0; i < SC64_SHIELD_FLASH_SLOTS; ++i)
        if (sc64_shield_flashes[i].live && sc64_shield_flashes[i].actor == (u16)actor)
            sc64_shield_flashes[i].live = 0;
}

static void
sc64_shield_mark_drawn(int slot)
{
    if ((unsigned)slot < MAX_UNITS) sc64_shield_drawn[slot >> 3] |= (u8)(1u << (slot & 7));
}

static inline u8
sc64_shield_dir_between(int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0, dy = y1 - y0;
    return (dx || dy) ? (u8)sc64_facing_direction(dx, dy) : SC64_SHIELD_DIR_NONE;
}

static inline void
sc64_shield_hit_from(int x, int y, int target)
{
    sc64_shield_hit_dir = sc64_shield_dir_between(x, y, target_x(target), target_y(target));
}

static inline void
sc64_shield_hit_done(void)
{
    sc64_shield_hit_dir = SC64_SHIELD_DIR_NONE;
}

/* The direction a queued impact launched now will carry (read by the
 * impact adapter when the row is queued). */
static inline void
sc64_shield_flash_note_launch(int target, int x, int y)
{
    sc64_shield_launch_dir = sc64_shield_dir_between(x, y, target_x(target), target_y(target));
}

/* Shield actor row for a unit or building, or null. */
static const Sc64WeaponSourceShieldActor *
sc64_shield_actor_row(int actor)
{
    int raw;
    if (actor < 0) return 0;
    if (actor < MAX_UNITS) {
        const Unit *q = &u[actor];
        raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type);
    } else if (actor < MAX_UNITS + MAX_BUILDINGS) {
        const Building *q = &b[actor - MAX_UNITS];
        raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type);
    } else return 0;
    for (unsigned i = 0; i < SC64_WEAPON_SOURCE_SHIELD_ACTOR_COUNT; ++i)
        if (sc64_weapon_source_shield_actors[i].raw_unit == raw)
            return &sc64_weapon_source_shield_actors[i];
    return 0;
}

static int
sc64_shield_actor_live(int actor, u8 type)
{
    if (actor < MAX_UNITS) return u[actor].alive && u[actor].type == type;
    return b[actor - MAX_UNITS].alive && b[actor - MAX_UNITS].type == type;
}

/* Called by the hit path when shields absorbed part of a hit. */
static void
sc64_shield_flash_hit(int target, int absorbed, int shield_left)
{
    u8 dir;
    Sc64ShieldFlash *f;
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS || absorbed <= 0 || shield_left <= 0) return;
    if (!sc64_shield_actor_row(target)) return;
    const Sc64WeaponSourceShieldActor *row = sc64_shield_actor_row(target);
    int parent_frame = 0, mirror = 0, lo_index;
    int16_t ox, oy;
    dir = sc64_shield_hit_dir;
    if (dir == SC64_SHIELD_DIR_NONE)       /* no attacker position: face-on */
        dir = target < MAX_UNITS ? (u8)((sc64_facing[target] & SC64_FACING_MASK) ^ 16u) : 16u;
    /* OpenBW: (heading - 124) / 8 mod 32 with a 256-way heading; for the
     * 32-way heading d (heading 8d) this is d + 16: the side facing the
     * attacker. The offset is resolved once, at the hit, from the target's
     * facing frame then, as OpenBW's create_image does. */
    lo_index = (int)((dir + 16u) & 31u);
    if (target < MAX_UNITS && row->directional) {
        int face = sc64_facing[target] & SC64_FACING_MASK;
        parent_frame = face <= 16 ? face : 32 - face;
        mirror = face > 16;
    }
    if (!sc64_weapon_source_lo_lookup(row->image_id, 5u, (uint16_t)parent_frame,
                                      (uint8_t)lo_index, &ox, &oy)) return;
    /* A free slot first; when all are live, the one inserted longest ago
     * (eight live slots never span a u8 wrap of the counter). */
    f = 0;
    for (unsigned k = 0; k < SC64_SHIELD_FLASH_SLOTS; ++k) {
        Sc64ShieldFlash *c = &sc64_shield_flashes[k];
        if (!c->live) { f = c; break; }
        if (!f || (u8)(sc64_shield_flash_seq - c->seq) > (u8)(sc64_shield_flash_seq - f->seq)) f = c;
    }
    f->seq = sc64_shield_flash_seq++;
    sc64_shield_flash_next = (u8)(f - sc64_shield_flashes + 1u) % SC64_SHIELD_FLASH_SLOTS;
    f->actor = (u16)target;
    f->born = (u16)frame;
    /* Source LO offsets are full-size pixels; the world is half scale, as for
     * every other source overlay (sc64_weapon_source_vfx_draw_overlays_motion). */
    f->dx = (s8)((mirror ? -ox : ox) / 2);
    f->dy = (s8)(oy / 2);
    f->type = (u8)(target < MAX_UNITS ? u[target].type : b[target - MAX_UNITS].type);
    f->live = 1;
}

/* After the units have been drawn: overlays on actors drawn this frame, then
 * the drawn marks are cleared for the next frame. */
static void
sc64_shield_flash_draw_actor(int actor)
{
    const Sc64WeaponSourceAnimation *init =
        sc64_weapon_source_animation_lookup(SC64_WEAPON_SOURCE_SHIELD_OVERLAY_IMAGE, 0u);
    uint32_t life = init ? init->duration_ticks : 0u;
    for (unsigned i = 0; i < SC64_SHIELD_FLASH_SLOTS; ++i) {
        Sc64ShieldFlash *f = &sc64_shield_flashes[i];
        int ax, ay, x, y, sx, sy;
        uint32_t age;
        if (!f->live) continue;
        age = sc64_weapon_source_vfx_source_tick((u16)((u16)frame - f->born));
        if (!sc64_shield_actor_live(f->actor, f->type) || age >= life) { f->live = 0; continue; }
        if ((int)f->actor != actor) continue;
        if (f->actor < MAX_UNITS) {
            if (!(sc64_shield_drawn[f->actor >> 3] & (1u << (f->actor & 7)))) continue;
            ax = u[f->actor].x; ay = u[f->actor].y;
        } else {
            const Building *q = &b[f->actor - MAX_UNITS];
            ax = q->x; ay = q->y;
            if (!currently_visible(ax, ay)) continue;
        }
        x = ax + f->dx;
        y = ay + f->dy;
        sx = x - camx; sy = y - camy + 12;
        if (sx < -64 || sx > 304 || sy < -52 || sy > 172) continue;
        sc64_weapon_source_vfx_draw_image(SC64_WEAPON_SOURCE_SHIELD_OVERLAY_IMAGE, 0u, age, 0u, x, y);
    }
    if ((unsigned)actor < MAX_UNITS)
        sc64_shield_drawn[actor >> 3] &= (u8)~(1u << (actor & 7));
}

static void
sc64_shield_flash_expire(void)
{
    sc64_shield_flash_draw_actor(-1);
    memset(sc64_shield_drawn, 0, sizeof(sc64_shield_drawn));
}

#endif
