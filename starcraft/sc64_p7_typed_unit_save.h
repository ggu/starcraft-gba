/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P7_TYPED_UNIT_SAVE_H
#define SC64_P7_TYPED_UNIT_SAVE_H

/* Generation-40 P7 row.  This codec is deliberately separate from both the
 * generation-22 ZP row and the Z7 derived row: the 16-bit weapon field remains
 * opaque so P7 cloak/cache state can survive a save exactly. */
static inline int sc64_zp_actor_default_hp(int type);
static inline int sc64_zp_actor_default_shields(int type);

#define SC64_P7_TYPED_UNIT_SAVE_BYTES 28
typedef struct { u8 bytes[SC64_P7_TYPED_UNIT_SAVE_BYTES]; }
    Sc64P7TypedUnitSave;
_Static_assert(sizeof(Sc64P7TypedUnitSave) == 28,
               "P7 typed unit row budget");

static int sc64_p7_typed_unit_put(Sc64P7TypedUnitSave *r, unsigned *bit,
                                  unsigned width, u32 value)
{
    if (!r || !bit || !width || width > 32 || *bit > 224 ||
        width > 224 - *bit ||
        (width < 32 && value >= ((u32)1u << width))) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        if ((value >> i) & 1u)
            r->bytes[*bit >> 3] |= (u8)(1u << (*bit & 7u));
    return 1;
}

static int sc64_p7_typed_unit_get(const Sc64P7TypedUnitSave *r,
                                  unsigned *bit, unsigned width, u32 *value)
{
    u32 v = 0;
    if (!r || !bit || !value || !width || width > 32 || *bit > 224 ||
        width > 224 - *bit) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        v |= (u32)((r->bytes[*bit >> 3] >> (*bit & 7u)) & 1u) << i;
    *value = v;
    return 1;
}

static int sc64_p7_typed_unit_zero(const Sc64P7TypedUnitSave *r)
{
    if (!r) return 0;
    for (unsigned i = 0; i < sizeof(r->bytes); ++i)
        if (r->bytes[i]) return 0;
    return 1;
}

static int sc64_p7_typed_unit_canonical(int type, int *maxhp, int *armor,
                                        int *maxshield)
{
    const Sc64RaceUnitRule *rule;
    int hp, shields;
    if (type < 0 || type >= SC_RU_COUNT) return 0;
    rule = sc64_race_unit_rule(type);
    hp = sc64_zp_actor_default_hp(type);
    if (!hp && rule) hp = rule->hp;
    shields = sc64_zp_actor_default_shields(type);
    if (!shields && rule) shields = rule->shields;
    if (hp <= 0 || hp > 131071 || shields < 0 || shields > 400) return 0;
    if (maxhp) *maxhp = hp;
    if (armor) *armor = sc64_race_unit_armor(type);
    if (maxshield) *maxshield = shields;
    return 1;
}

static int sc64_p7_typed_unit_fields_valid(const Unit *q,
                                           unsigned world_width,
                                           unsigned world_height)
{
    int maxhp, armor, maxshield;
    if (!q || !world_width || !world_height || world_width > 2048 ||
        world_height > 2048 || q->x < 0 || (unsigned)q->x >= world_width ||
        q->y < 0 || (unsigned)q->y >= world_height ||
        q->hp < -131072 || q->hp >= 131072 || (q->alive && q->hp < 0) ||
        q->owner < 0 || q->owner > 4 || q->type < 0 ||
        !sc64_p7_typed_unit_canonical(q->type, &maxhp, &armor, &maxshield) ||
        q->maxhp != maxhp || q->armor != armor ||
        (q->alive != 0 && q->alive != 1) ||
        (q->selected != 0 && q->selected != 1) || q->cd < 0 || q->cd > 255 ||
        q->order < 0 || q->order > 15 || q->tx < 0 || q->tx > 2047 ||
        q->ty < 0 || q->ty > 2047 ||
        q->carry < -65536 || q->carry >= 65536 || q->build < -2 ||
        q->build > 239 || q->work_timer < 0 || q->work_timer > 2047 ||
        q->weapon < 0 || q->weapon > 65535 || q->target < -1 ||
        q->target >= MAX_UNITS + MAX_BUILDINGS || q->path_timer < 0 ||
        q->path_timer > 24 || q->path_retry < 0 || q->path_retry > 12 ||
        q->path_x < 0 || q->path_x > 2047 || q->path_y < 0 ||
        q->path_y > 2047 || q->shield < 0 || q->shield > maxshield ||
        q->raw_type < -1 || q->raw_type > 254 || q->raw_owner < -1 ||
        q->raw_owner > 14 || q->flags < 0 || q->flags > 0x3fff ||
        q->shield_remainder < 0 || q->shield_remainder > 1279) return 0;
    return 1;
}

static int sc64_p7_typed_unit_encode(Sc64P7TypedUnitSave *row,
                                     const Unit *q, unsigned ww,
                                     unsigned wh)
{
    Sc64P7TypedUnitSave out;
    unsigned bit = 0;
    if (!row || !q || !ww || !wh || ww > 2048 || wh > 2048) return 0;
    memset(&out, 0, sizeof(out));
    static const Unit zero = {0};
    if (memcmp(q, &zero, sizeof(*q)) == 0) { *row = out; return 1; }
    if (!sc64_p7_typed_unit_fields_valid(q, ww, wh)) return 0;
#define PUT(w, v) do { if (!sc64_p7_typed_unit_put(&out, &bit, (w), (u32)(v))) return 0; } while (0)
    PUT(11, q->x); PUT(11, q->y); PUT(18, q->hp + 131072);
    PUT(3, q->owner); PUT(6, q->type); PUT(1, q->alive); PUT(1, q->selected);
    PUT(8, q->cd); PUT(4, q->order); PUT(11, q->tx); PUT(11, q->ty);
    PUT(17, q->carry + 65536); PUT(8, q->build + 2); PUT(11, q->work_timer);
    PUT(16, q->weapon); PUT(9, q->target + 1); PUT(5, q->path_timer);
    PUT(4, q->path_retry); PUT(11, q->path_x); PUT(11, q->path_y);
    PUT(9, q->shield); PUT(8, q->raw_type + 1); PUT(4, q->raw_owner + 1);
    PUT(14, q->flags); PUT(11, q->shield_remainder); PUT(1, 1);
#undef PUT
    if (bit != 224) return 0;
    *row = out;
    return 1;
}

static int sc64_p7_typed_unit_decode(Unit *out,
                                     const Sc64P7TypedUnitSave *row,
                                     unsigned ww, unsigned wh)
{
    Unit q;
    unsigned bit = 0;
    u32 v;
    int maxhp, armor, maxshield;
    if (!out || !row || !ww || !wh || ww > 2048 || wh > 2048) return 0;
    memset(&q, 0, sizeof(q));
    if (sc64_p7_typed_unit_zero(row)) { *out = q; return 1; }
#define GET(w, dst) do { if (!sc64_p7_typed_unit_get(row, &bit, (w), &v)) return 0; (dst) = (int)v; } while (0)
    GET(11, q.x); GET(11, q.y);
    if (!sc64_p7_typed_unit_get(row, &bit, 18, &v)) return 0;
    q.hp = (int)v - 131072;
    GET(3, q.owner); GET(6, q.type); GET(1, q.alive); GET(1, q.selected);
    GET(8, q.cd); GET(4, q.order); GET(11, q.tx); GET(11, q.ty);
    if (!sc64_p7_typed_unit_get(row, &bit, 17, &v)) return 0;
    q.carry = (int)v - 65536;
    GET(8, v); q.build = (int)v - 2; GET(11, q.work_timer);
    GET(16, q.weapon); GET(9, v); q.target = (int)v - 1;
    GET(5, q.path_timer); GET(4, q.path_retry); GET(11, q.path_x);
    GET(11, q.path_y); GET(9, q.shield); GET(8, v); q.raw_type = (int)v - 1;
    GET(4, v); q.raw_owner = (int)v - 1; GET(14, q.flags);
    GET(11, q.shield_remainder);
    if (!sc64_p7_typed_unit_get(row, &bit, 1, &v) || v != 1 || bit != 224 ||
        !sc64_p7_typed_unit_canonical(q.type, &maxhp, &armor, &maxshield)) return 0;
    q.maxhp = maxhp; q.armor = armor;
    if (!sc64_p7_typed_unit_fields_valid(&q, ww, wh)) return 0;
    *out = q;
    return 1;
#undef GET
}

#endif
