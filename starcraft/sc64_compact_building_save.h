#ifndef SC64_COMPACT_BUILDING_SAVE_H
#define SC64_COMPACT_BUILDING_SAVE_H

/* Versioned, slot-preserving building save codec for the opt-in T6 storage
 * profile. Every field in every slot is retained, including dead source
 * structures whose raw type/owner/position remain part of T4 save validation.
 * Live field bounds are checked before encoding; no value is saturated. */
#define SC64_PACKED_BUILDING_SAVE_BYTES 28
#define SC64_PACKED_BUILDING_HP_BIAS 131072

typedef struct {
    u8 bytes[SC64_PACKED_BUILDING_SAVE_BYTES];
} Sc64PackedBuildingSave;

_Static_assert(sizeof(Sc64PackedBuildingSave) == SC64_PACKED_BUILDING_SAVE_BYTES,
               "packed building save row size");

static int
sc64_packed_building_is_zero(const Sc64PackedBuildingSave *row)
{
    if (!row) return 0;
    for (unsigned i = 0; i < SC64_PACKED_BUILDING_SAVE_BYTES; ++i)
        if (row->bytes[i]) return 0;
    return 1;
}

static int
sc64_packed_building_put(Sc64PackedBuildingSave *row, unsigned *bit,
                         unsigned width, u32 value)
{
    if (!row || !bit || !width || width > 32 ||
        *bit > SC64_PACKED_BUILDING_SAVE_BYTES * 8u ||
        width > SC64_PACKED_BUILDING_SAVE_BYTES * 8u - *bit ||
        (width < 32 && value >= ((u32)1u << width))) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        if ((value >> i) & 1u)
            row->bytes[*bit >> 3] |= (u8)(1u << (*bit & 7u));
    return 1;
}

static int
sc64_packed_building_get(const Sc64PackedBuildingSave *row, unsigned *bit,
                         unsigned width, u32 *value)
{
    u32 result = 0;
    if (!row || !bit || !value || !width || width > 32 ||
        *bit > SC64_PACKED_BUILDING_SAVE_BYTES * 8u ||
        width > SC64_PACKED_BUILDING_SAVE_BYTES * 8u - *bit) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        result |= (u32)((row->bytes[*bit >> 3] >> (*bit & 7u)) & 1u) << i;
    *value = result;
    return 1;
}

/* All values below are bounded by the current game representation: map
 * coordinates are at most 2047, max HP/shields at most 131071 (100000 is the
 * canonical T4 prop value), and HP uses a biased signed 18-bit range
 * because hit_target may leave a killed building below zero. Unit/building
 * enums fit six bits, source IDs fit one byte, and the engine's
 * queue/construction/cooldown counters fit 16 bits. The encoder fails closed
 * if a future rule exceeds these bounds. */
static int
sc64_building_save_encode(Sc64PackedBuildingSave *row, const Building *bld,
                          unsigned world_width, unsigned world_height)
{
    unsigned bit = 0;
    Sc64PackedBuildingSave encoded;
    u32 raw_type, raw_owner, build_progress;
    int hp, maxhp;
    if (!row || !bld || world_width > 2048 || world_height > 2048) return 0;
    memset(&encoded, 0, sizeof(encoded));
    if (bld->alive != 0 && bld->alive != 1) return 0;
    hp = bld->hp; maxhp = bld->maxhp;
#if defined(SC64_T4_RUNTIME) && !defined(SC64_T6_STORAGE)
    /* Preserve the prior s16 save's canonical-zero representation for static
     * 100000-HP T4 props only in legacy layouts. The packed T6 format has
     * enough range to retain their exact fields. */
    if (sc64_race_t4_building_intrinsically_invincible(bld->type)) {
        if (hp != 100000 || maxhp != 100000 || bld->under_construction ||
            bld->queue_count || bld->shield) return 0;
        hp = maxhp = 0;
    }
#endif
    if (bld->x < 0 || (unsigned)bld->x >= world_width ||
        bld->y < 0 || (unsigned)bld->y >= world_height ||
        hp < -SC64_PACKED_BUILDING_HP_BIAS || hp >= SC64_PACKED_BUILDING_HP_BIAS ||
        (bld->alive && hp < 0) || maxhp < 0 || maxhp > 131071 ||
        bld->owner < 0 || bld->owner > 4 || bld->type < 0 || bld->type >= SC_RB_COUNT ||
        SC_RB_COUNT > 64 || bld->queue_head < 0 || bld->queue_head > 4 ||
        bld->queue_count < 0 || bld->queue_count > 5 ||
        bld->queue_progress < 0 || bld->queue_progress > 65535 ||
        bld->build_progress < -1 || bld->build_progress > 65534 ||
        (bld->under_construction != 0 && bld->under_construction != 1) ||
        bld->level < 0 || bld->level > 127 ||
        bld->shield < 0 || bld->shield > 131071 ||
        bld->raw_type < -1 || bld->raw_type > 255 ||
        bld->raw_owner < -1 || bld->raw_owner > 255 ||
        bld->flags < 0 || bld->flags > 65535 ||
        bld->larva < 0 || bld->larva > 3 ||
        bld->larva_timer < 0 || bld->larva_timer > 511 ||
        bld->shield_remainder < 0 || bld->shield_remainder > 1279 ||
        SC_RU_COUNT > 64) return 0;
    for (unsigned i = 0; i < 5; ++i)
        if (bld->queue[i] < 0 || bld->queue[i] >= SC_RU_COUNT) return 0;
    raw_type = (u32)(bld->raw_type + 1);
    raw_owner = (u32)(bld->raw_owner + 1);
    build_progress = (u32)(bld->build_progress + 1);
#define SC64_PUT(width, value) do { if (!sc64_packed_building_put(&encoded, &bit, (width), (u32)(value))) return 0; } while (0)
    SC64_PUT(11, bld->x); SC64_PUT(11, bld->y);
    SC64_PUT(18, hp + SC64_PACKED_BUILDING_HP_BIAS); SC64_PUT(17, maxhp);
    SC64_PUT(3, bld->owner); SC64_PUT(6, bld->type); SC64_PUT(1, bld->alive);
    for (unsigned i = 0; i < 5; ++i) SC64_PUT(6, bld->queue[i]);
    SC64_PUT(3, bld->queue_head); SC64_PUT(3, bld->queue_count);
    SC64_PUT(16, bld->queue_progress); SC64_PUT(16, build_progress);
    SC64_PUT(1, bld->under_construction); SC64_PUT(7, bld->level);
    SC64_PUT(17, bld->shield); SC64_PUT(9, raw_type); SC64_PUT(9, raw_owner);
    SC64_PUT(16, bld->flags); SC64_PUT(2, bld->larva);
    SC64_PUT(9, bld->larva_timer); SC64_PUT(11, bld->shield_remainder);
#undef SC64_PUT
    if (bit != 216) return 0;
    *row = encoded;
    return 1;
}

static int
sc64_building_save_decode(Building *out, const Sc64PackedBuildingSave *row)
{
    unsigned bit = 0;
    Building decoded;
    u32 v, raw_type, raw_owner, build_progress;
    if (!out || !row) return 0;
    memset(&decoded, 0, sizeof(decoded));
    if (sc64_packed_building_is_zero(row)) { *out = decoded; return 1; }
#define SC64_GET(width, dst) do { if (!sc64_packed_building_get(row, &bit, (width), &v)) return 0; (dst) = (int)v; } while (0)
    SC64_GET(11, decoded.x); SC64_GET(11, decoded.y);
    if (!sc64_packed_building_get(row, &bit, 18, &v)) return 0;
    decoded.hp = (int)v - SC64_PACKED_BUILDING_HP_BIAS;
    SC64_GET(17, decoded.maxhp);
    SC64_GET(3, decoded.owner); SC64_GET(6, decoded.type); SC64_GET(1, decoded.alive);
    for (unsigned i = 0; i < 5; ++i) SC64_GET(6, decoded.queue[i]);
    SC64_GET(3, decoded.queue_head); SC64_GET(3, decoded.queue_count);
    SC64_GET(16, decoded.queue_progress);
    if (!sc64_packed_building_get(row, &bit, 16, &build_progress)) return 0;
    decoded.build_progress = (int)build_progress - 1;
    SC64_GET(1, decoded.under_construction); SC64_GET(7, decoded.level);
    SC64_GET(17, decoded.shield);
    if (!sc64_packed_building_get(row, &bit, 9, &raw_type) ||
        !sc64_packed_building_get(row, &bit, 9, &raw_owner)) return 0;
    decoded.raw_type = (int)raw_type - 1; decoded.raw_owner = (int)raw_owner - 1;
    SC64_GET(16, decoded.flags); SC64_GET(2, decoded.larva);
    SC64_GET(9, decoded.larva_timer); SC64_GET(11, decoded.shield_remainder);
#undef SC64_GET
    if (bit != 216 || decoded.alive > 1 || decoded.type >= SC_RB_COUNT ||
        (decoded.alive && decoded.hp < 0) || decoded.maxhp > 131071 ||
        decoded.owner > 4 || decoded.queue_head > 4 || decoded.queue_count > 5 ||
        decoded.level > 127 || decoded.raw_type > 255 ||
        decoded.raw_owner > 255 || decoded.larva > 3 ||
        decoded.shield_remainder > 1279 || decoded.build_progress < -1) return 0;
    /* Eight spare bits are reserved and must remain zero, as must any future
     * padding. This also rejects noncanonical encodings before live mutation. */
    for (; bit < SC64_PACKED_BUILDING_SAVE_BYTES * 8u; ++bit)
        if ((row->bytes[bit >> 3] >> (bit & 7u)) & 1u) return 0;
    for (unsigned i = 0; i < 5; ++i)
        if (decoded.queue[i] >= SC_RU_COUNT) return 0;
    *out = decoded;
    return 1;
}

#endif
