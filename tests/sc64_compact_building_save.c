/* Asset-free test of the versioned compact building codec. The parallel
 * engine/save fixture exercises this same header through RaceSave. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint32_t u32;
enum { SC_RU_COUNT = 48, SC_RB_COUNT = 45 };
typedef struct {
    int x, y, hp, maxhp, owner, type, alive;
    int queue[5], queue_head, queue_count, queue_progress;
    int build_progress, under_construction, level;
    int shield, raw_type, raw_owner, flags, larva, larva_timer, shield_remainder;
} Building;
#include "../starcraft/sc64_compact_building_save.h"

_Static_assert(sizeof(Building) == 25 * sizeof(int), "fixture matches live Building");

static void fill_full(Building *b)
{
    memset(b, 0, sizeof(*b));
    b->x = 2047; b->y = 2046;
    b->hp = b->maxhp = 100000;
    b->owner = 4; b->type = 44; b->alive = 1;
    for (int i = 0; i < 5; ++i) b->queue[i] = i;
    b->queue_head = 4; b->queue_count = 5;
    b->queue_progress = 65535; b->build_progress = 65534;
    b->under_construction = 1; b->level = 80;
    b->shield = 131071; b->raw_type = 255; b->raw_owner = 255;
    b->flags = 65535; b->larva = 3; b->larva_timer = 511;
    b->shield_remainder = 1279;
}

static void exact_roundtrip(void)
{
    Building before, after;
    Sc64PackedBuildingSave row;
    fill_full(&before);
    assert(sc64_building_save_encode(&row, &before, 2048, 2048));
    assert(sc64_building_save_decode(&after, &row));
    assert(!memcmp(&before, &after, sizeof(before)));

    before.raw_type = -1; before.raw_owner = -1; before.build_progress = -1;
    before.queue_count = 0;
    assert(sc64_building_save_encode(&row, &before, 2048, 2048));
    assert(sc64_building_save_decode(&after, &row));
    assert(!memcmp(&before, &after, sizeof(before)));
}

static void dead_slots_preserve_fields(void)
{
    Building stale, after;
    Sc64PackedBuildingSave row;
    fill_full(&stale);
    stale.alive = 0;
    stale.hp = -321; /* hit_target leaves overkill on dead building records */
    assert(sc64_building_save_encode(&row, &stale, 2048, 2048));
    assert(sc64_building_save_decode(&after, &row));
    assert(!memcmp(&stale, &after, sizeof(after)));
    assert(after.hp == -321);
}

static void rejects_without_mutating_outputs(void)
{
    Building b, after, sentinel;
    Sc64PackedBuildingSave row, original;
    fill_full(&b);
    memset(&row, 0xa5, sizeof(row)); original = row;
#define REJECT_FIELD(field, value) do { \
    b.field = (value); \
    assert(!sc64_building_save_encode(&row, &b, 2048, 2048)); \
    assert(!memcmp(&row, &original, sizeof(row))); \
    fill_full(&b); \
} while (0)
    REJECT_FIELD(x, 2048);
    REJECT_FIELD(hp, 131072);
    REJECT_FIELD(hp, -131073);
    REJECT_FIELD(hp, -1);
    REJECT_FIELD(maxhp, 131072);
    REJECT_FIELD(type, 64);
    REJECT_FIELD(queue[0], 64);
    REJECT_FIELD(queue_head, 5);
    REJECT_FIELD(queue_count, 6);
    REJECT_FIELD(queue_progress, 65536);
    REJECT_FIELD(build_progress, 65535);
    REJECT_FIELD(under_construction, 2);
    REJECT_FIELD(level, 128);
    REJECT_FIELD(shield, 131072);
    REJECT_FIELD(raw_type, 256);
    REJECT_FIELD(raw_owner, 256);
    REJECT_FIELD(flags, 65536);
    REJECT_FIELD(larva_timer, 512);
    REJECT_FIELD(shield_remainder, 1280);
#undef REJECT_FIELD
    assert(!sc64_building_save_encode(&row, &b, 2049, 2048));

    assert(sc64_building_save_encode(&row, &b, 2048, 2048));
    memset(&sentinel, 0x3c, sizeof(sentinel)); after = sentinel;
    row.bytes[27] |= 0x80; /* Reserved high bit must remain zero. */
    assert(!sc64_building_save_decode(&after, &row));
    assert(!memcmp(&after, &sentinel, sizeof(after)));
    assert(sc64_building_save_encode(&row, &b, 2048, 2048));
    row.bytes[8] &= (u8)~0x04; /* alive is encoded at bit 66; dead still decodes. */
    after = sentinel;
    assert(sc64_building_save_decode(&after, &row));
    assert(after.alive == 0 && after.type == b.type);
}

int main(void)
{
    assert(sizeof(Sc64PackedBuildingSave) == 28);
    exact_roundtrip();
    dead_slots_preserve_fields();
    rejects_without_mutating_outputs();
    puts("compact building codec ok (28 bytes, lossless live fields)");
    return 0;
}
