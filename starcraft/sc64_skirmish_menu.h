/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SKIRMISH_MENU_H
#define SC64_SKIRMISH_MENU_H

#include "sc64_skirmish_catalog.h"

/* Row kinds consumed by the race field-command menu. */
enum {
    SC64_SK_MENU_MOVE = 0,
    SC64_SK_MENU_ATTACK,
    SC64_SK_MENU_WEAPONS,
    SC64_SK_MENU_ARMOR,
    SC64_SK_MENU_STOP,
    SC64_SK_MENU_BUILD,
    SC64_SK_MENU_ADDON,
    SC64_SK_MENU_MORPH
};

typedef struct {
    unsigned char kind;
    const Sc64SkirmishRecord *record;
    const char *label;
} Sc64SkirmishMenuRow;

static const unsigned char sc64_skirmish_menu_basics[SC64_SK_RACE_COUNT][5] = {
    { 111, 109, 110, 112, 122 },
    { 142, 131, 149, 135, 139 },
    { 160, 156, 157, 164, 166 }
};

static inline int sc64_skirmish_menu_is_basic(unsigned race, unsigned raw)
{
    unsigned i;
    if (race >= SC64_SK_RACE_COUNT) return 0;
    for (i = 0; i < 5; ++i)
        if (sc64_skirmish_menu_basics[race][i] == raw) return 1;
    return 0;
}

static inline const Sc64SkirmishRecord *sc64_skirmish_menu_record(unsigned race,
                                                                  unsigned raw)
{
    unsigned i;
    if (race >= SC64_SK_RACE_COUNT) return (const Sc64SkirmishRecord *)0;
    for (i = 0; i < sc64_skirmish_build_count(race); ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_build_at(race, i);
        if (r->raw_id == raw &&
            (r->kind == SC64_SK_WORKER_BUILD || r->kind == SC64_SK_ADDON ||
             r->kind == SC64_SK_BUILDING_MORPH)) return r;
    }
    return (const Sc64SkirmishRecord *)0;
}

static inline unsigned sc64_skirmish_menu_count(unsigned race)
{
    unsigned i, n = 10;
    if (race >= SC64_SK_RACE_COUNT) return 0;
    for (i = 0; i < sc64_skirmish_build_count(race); ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_build_at(race, i);
        if (!sc64_skirmish_menu_is_basic(race, r->raw_id)) ++n;
    }
    return n;
}

static inline int sc64_skirmish_menu_at(unsigned race, unsigned index,
                                         Sc64SkirmishMenuRow *out)
{
    static const unsigned char basic_kind[5] = {
        SC64_SK_MENU_BUILD, SC64_SK_MENU_BUILD, SC64_SK_MENU_BUILD,
        SC64_SK_MENU_BUILD, SC64_SK_MENU_BUILD
    };
    static const char *const basic_label[5][5] = {
        { "BARRACKS", "SUPPLY DEPOT", "REFINERY", "ACADEMY", "ENGINEERING BAY" },
        { "SPAWNING POOL", "HATCHERY", "EXTRACTOR", "HYDRALISK DEN", "EVOLUTION CHAMBER" },
        { "GATEWAY", "PYLON", "ASSIMILATOR", "CYBERNETICS CORE", "FORGE" }
    };
    static const char *const fixed_label[5] = { "MOVE", "ATTACK", "WEAPONS", "ARMOR", "STOP / HOLD" };
    unsigned i, n = 10;
    if (out) { out->kind = 0; out->record = 0; out->label = 0; }
    if (!out || race >= SC64_SK_RACE_COUNT || index >= sc64_skirmish_menu_count(race)) return 0;
    if (index < 2) {
        out->kind = index == 0 ? SC64_SK_MENU_MOVE : SC64_SK_MENU_ATTACK;
        out->label = fixed_label[index];
        return 1;
    }
    if (index < 7) {
        i = index - 2;
        out->kind = basic_kind[i];
        out->record = sc64_skirmish_menu_record(race, sc64_skirmish_menu_basics[race][i]);
        out->label = basic_label[race][i];
        return out->record != 0;
    }
    if (index < 10) {
        out->kind = index == 7 ? SC64_SK_MENU_WEAPONS :
                    index == 8 ? SC64_SK_MENU_ARMOR : SC64_SK_MENU_STOP;
        out->label = fixed_label[index - 5];
        return 1;
    }
    for (i = 0; i < sc64_skirmish_build_count(race); ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_build_at(race, i);
        if (sc64_skirmish_menu_is_basic(race, r->raw_id)) continue;
        if (n++ != index) continue;
        out->record = r;
        out->label = r->name;
        out->kind = r->kind == SC64_SK_ADDON ? SC64_SK_MENU_ADDON :
                    r->kind == SC64_SK_BUILDING_MORPH ? SC64_SK_MENU_MORPH :
                    SC64_SK_MENU_BUILD;
        return 1;
    }
    return 0;
}

typedef int (*Sc64SkirmishProductAvailable)(const Sc64SkirmishRecord *record,
                                            void *context);

/* Return the next available TRAIN record for this producer, wrapping once.
 * With current_raw == SC64_SK_NONE selection begins at the first product. */
static inline int sc64_skirmish_menu_next_product(unsigned producer_raw,
                                                   unsigned current_raw,
                                                   Sc64SkirmishProductAvailable available,
                                                   void *context)
{
    unsigned i, count = sc64_skirmish_product_count(producer_raw), start = 0;
    int found_current = 0;
    for (i = 0; i < count; ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_product_at(producer_raw, i);
        if (r->raw_id == current_raw) {
            start = (i + 1) % count;
            found_current = 1;
            break;
        }
    }
    if (!found_current) start = 0;
    for (i = 0; i < count; ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_product_at(producer_raw,
                                                                (start + i) % count);
        if (!available || available(r, context)) return r->raw_id;
    }
    return SC64_SK_NONE;
}

#endif
