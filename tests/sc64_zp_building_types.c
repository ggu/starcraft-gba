/* Layout/correctness probe for the proposed compact ZP runtime Building. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int16_t s16;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint32_t u32;
enum { SC_RU_COUNT = 64, SC_RB_COUNT = 64 };

typedef struct {
    int x, y, hp, maxhp, owner, type, alive;
    int queue[5], queue_head, queue_count, queue_progress;
    int build_progress, under_construction, level;
    int shield, raw_type, raw_owner, flags, larva, larva_timer, shield_remainder;
} LegacyBuilding;
typedef LegacyBuilding Building;
#define SC64_RACES 1
#include "../starcraft/sc64_compact_building_save.h"

#include "../starcraft/sc64_zp_building_types.h"

_Static_assert(sizeof(LegacyBuilding) == 25 * sizeof(int), "legacy model layout");
_Static_assert(sizeof(Sc64ZpCompactBuilding) == 48, "compact layout");

/* Validate every source-width field before assigning any narrowed member.
 * This mirrors the required migration discipline at the live-state boundary. */
static int compact_from_legacy(Sc64ZpCompactBuilding *out,
                               const LegacyBuilding *in)
{
    Sc64ZpCompactBuilding candidate;
    if (!out || !in || in->x < 0 || in->x > 2047 || in->y < 0 || in->y > 2047 ||
        in->owner < 0 || in->owner > 4 || in->type < 0 || in->type > 63 ||
        (in->alive != 0 && in->alive != 1) || in->queue_head < 0 ||
        in->queue_head > 4 || in->queue_count < 0 || in->queue_count > 5 ||
        in->queue_progress < 0 || in->queue_progress > 65535 ||
        in->under_construction < 0 || in->under_construction > 1 ||
        in->level < 0 || in->level > 127 || in->raw_type < -1 ||
        in->raw_type > 255 || in->raw_owner < -1 || in->raw_owner > 255 ||
        in->flags < 0 || in->flags > 65535 || in->larva < 0 || in->larva > 3 ||
        in->larva_timer < 0 || in->larva_timer > 511 ||
        in->shield_remainder < 0 || in->shield_remainder > 1279) return 0;
    for (unsigned i = 0; i < 5; ++i)
        if (in->queue[i] < 0 || in->queue[i] >= SC_RU_COUNT) return 0;

    memset(&candidate, 0, sizeof(candidate));
    candidate.hp = in->hp; candidate.maxhp = in->maxhp;
    candidate.shield = in->shield; candidate.build_progress = in->build_progress;
    candidate.x = (s16)in->x; candidate.y = (s16)in->y;
    candidate.raw_type = (s16)in->raw_type; candidate.raw_owner = (s16)in->raw_owner;
    candidate.queue_progress = (u16)in->queue_progress;
    candidate.flags = (u16)in->flags; candidate.larva_timer = (u16)in->larva_timer;
    candidate.shield_remainder = (u16)in->shield_remainder;
    candidate.owner = (u8)in->owner; candidate.type = (u8)in->type;
    candidate.alive = (u8)in->alive;
    for (unsigned i = 0; i < 5; ++i) candidate.queue[i] = (u8)in->queue[i];
    candidate.queue_head = (u8)in->queue_head;
    candidate.queue_count = (u8)in->queue_count;
    candidate.under_construction = (u8)in->under_construction;
    candidate.level = (u8)in->level; candidate.larva = (u8)in->larva;
    *out = candidate;
    return 1;
}

static void legacy_from_compact(LegacyBuilding *out,
                                const Sc64ZpCompactBuilding *in)
{
    memset(out, 0, sizeof(*out));
    out->x = in->x; out->y = in->y; out->hp = in->hp;
    out->maxhp = in->maxhp; out->owner = in->owner; out->type = in->type;
    out->alive = in->alive;
    for (unsigned i = 0; i < 5; ++i) out->queue[i] = in->queue[i];
    out->queue_head = in->queue_head; out->queue_count = in->queue_count;
    out->queue_progress = in->queue_progress;
    out->build_progress = in->build_progress;
    out->under_construction = in->under_construction; out->level = in->level;
    out->shield = in->shield; out->raw_type = in->raw_type;
    out->raw_owner = in->raw_owner; out->flags = in->flags;
    out->larva = in->larva; out->larva_timer = in->larva_timer;
    out->shield_remainder = in->shield_remainder;
}

static void edge_roundtrip(void)
{
    LegacyBuilding src = {0}, restored;
    Sc64ZpCompactBuilding compact;
    Sc64PackedBuildingSave packed;
    src.x = 2047; src.y = 2046; src.hp = src.maxhp = 100000;
    src.owner = 4; src.type = 63; src.alive = 1;
    for (int i = 0; i < 5; ++i) src.queue[i] = 63 - i;
    src.queue_head = 4; src.queue_count = 5; src.queue_progress = 65535;
    src.build_progress = 65534; src.under_construction = 1; src.level = 127;
    src.shield = 100000; src.raw_type = 255; src.raw_owner = -1;
    src.flags = 65535; src.larva = 3; src.larva_timer = 511;
    src.shield_remainder = 1279;
    assert(compact_from_legacy(&compact, &src));
    legacy_from_compact(&restored, &compact);
    assert(!memcmp(&src, &restored, sizeof(src)));
    assert(sc64_building_save_encode(&packed, &src, 2048, 2048));
    memset(&restored, 0, sizeof(restored));
    assert(sc64_building_save_decode(&restored, &packed));
    assert(!memcmp(&src, &restored, sizeof(src)));

    src.alive = 0; src.hp = -131072; src.maxhp = 131071; src.shield = 131071;
    src.build_progress = -1; src.raw_type = -1; src.raw_owner = 255;
    src.queue_count = 0; src.under_construction = 0; src.flags = 0;
    assert(compact_from_legacy(&compact, &src));
    legacy_from_compact(&restored, &compact);
    assert(!memcmp(&src, &restored, sizeof(src)));
    assert(sc64_building_save_encode(&packed, &src, 2048, 2048));
    memset(&restored, 0, sizeof(restored));
    assert(sc64_building_save_decode(&restored, &packed));
    assert(!memcmp(&src, &restored, sizeof(src)));
}

static void reject_before_narrowing(void)
{
    LegacyBuilding src = {0};
    Sc64ZpCompactBuilding out, sentinel;
    memset(&out, 0xa5, sizeof(out)); sentinel = out;
    src.owner = 1; src.type = 2;
#define REJECT(field, value) do { \
    src.field = (value); \
    assert(!compact_from_legacy(&out, &src)); \
    assert(!memcmp(&out, &sentinel, sizeof(out))); \
    src.field = 0; \
} while (0)
    REJECT(x, 32768); REJECT(y, -1); REJECT(owner, 256); REJECT(type, 256);
    REJECT(alive, 2); REJECT(queue_head, 5); REJECT(queue_count, 6);
    REJECT(queue_progress, 65536); REJECT(under_construction, 256);
    REJECT(level, 256); REJECT(raw_type, 32768); REJECT(raw_owner, -32769);
    REJECT(flags, 65536); REJECT(larva, 4); REJECT(larva_timer, 65536);
    REJECT(shield_remainder, 65536);
#undef REJECT
    src.queue[2] = 256;
    assert(!compact_from_legacy(&out, &src));
    assert(!memcmp(&out, &sentinel, sizeof(out)));
    /* These remain int fields, retaining the exact legacy values. */
    src.queue[2] = 0; src.hp = 100000; src.maxhp = 131071;
    src.shield = 100000; src.build_progress = 65534;
    assert(compact_from_legacy(&out, &src));
    assert(out.hp == 100000 && out.maxhp == 131071 && out.shield == 100000 &&
           out.build_progress == 65534);
}

static void capacity_accounting(void)
{
    const unsigned unit_runtime = 240u * 56u;
    const unsigned old_building_runtime = 160u * 100u;
    const unsigned compact_building_runtime = 160u * 48u;
    const unsigned mineral_runtime = 144u * 16u;
    const unsigned geyser_runtime = 12u * 8u;
    const unsigned unit_rows = 240u * 35u;
    const unsigned building_rows = 160u * 28u;
    const unsigned mineral_rows = 144u * 5u;
    const unsigned invincibility_bits = (240u + 160u + 7u) / 8u;
    assert(unit_runtime == 13440u);
    assert(old_building_runtime == 16000u && compact_building_runtime == 7680u);
    assert(mineral_runtime == 2304u && geyser_runtime == 96u);
    assert(unit_rows == 8400u && building_rows == 4480u && mineral_rows == 720u);
    assert(invincibility_bits == 50u);
    assert(unit_rows + building_rows + mineral_rows + 12u * 8u +
           invincibility_bits == 13746u);
}

int main(void)
{
    edge_roundtrip(); reject_before_narrowing(); capacity_accounting();
    puts("compact ZP Building layout tests passed (48-byte runtime record)");
    return 0;
}
