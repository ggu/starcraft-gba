/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Authored metadata contract for the authenticated Z8 source scenario.
 *
 * This header contains identity/count checks only. The canonical numeric
 * descriptor is private generated data under .local/build; no CHK tables or
 * source-derived rows belong in the ROM source tree. Passing this contract
 * does not enable Z8 gameplay or claim that its trigger/actor semantics are
 * implemented.
 *
 * Still-unimplemented semantic domains include the 39-trigger scheduler and
 * PreserveTrigger lifecycle, Kill conditions, native AI FourCC behavior,
 * property-aware actor creation, MoveLocation/SetInvincibility, Observer
 * detection/cloak, and linked Nydus transport.
 */
#ifndef SC64_Z8_SOURCE_H
#define SC64_Z8_SOURCE_H

#include <stddef.h>

#define SC64_Z8_SOURCE_SCHEMA 1u
#define SC64_Z8_SOURCE_GLOBAL_ID 19u
#define SC64_Z8_SOURCE_MAP_PATH "008/01B"
#define SC64_Z8_SOURCE_ROM_SHA1 "a36d91b5aac7bb95df0d7de5f9814f5270e25c2b"
#define SC64_Z8_SOURCE_CHK_SHA256 \
    "ff6d0177a6a76fc68ef98f0962661857853ea11c50de9060b90ff934826cfd8e"
#define SC64_Z8_SOURCE_DESCRIPTOR_SHA256 \
    "f80b5f9e9419c324a6b86b9a11bbe3dab0ec545d20817fb0f26509e82c85a986"
#define SC64_Z8_SOURCE_EXTRACTOR_COMMIT \
    "09b57f787a247107d1de82b8c5eab2734667ea1d"

#define SC64_Z8_SOURCE_MAP_COLS 128u
#define SC64_Z8_SOURCE_MAP_ROWS 128u
#define SC64_Z8_SOURCE_TRIGGER_COUNT 39u
#define SC64_Z8_SOURCE_ACTION_COUNT 122u
#define SC64_Z8_SOURCE_CONDITION_COUNT 55u
#define SC64_Z8_SOURCE_LOCATION_COUNT 64u
#define SC64_Z8_SOURCE_UNIT_RECORD_COUNT 198u
#define SC64_Z8_SOURCE_PRESERVE_ACTION_COUNT 13u
#define SC64_Z8_SOURCE_CREATE_ACTION_COUNT 16u
#define SC64_Z8_SOURCE_NYDUS_RECORD_COUNT 4u

typedef struct {
    unsigned int schema;
    unsigned int global_id;
    unsigned int map_cols;
    unsigned int map_rows;
    unsigned int trigger_count;
    unsigned int action_count;
    unsigned int condition_count;
    unsigned int location_count;
    unsigned int unit_record_count;
    unsigned int preserve_action_count;
    unsigned int create_action_count;
    unsigned int nydus_record_count;
    const char *map_path;
    const char *source_rom_sha1;
    const char *source_chk_sha256;
    const char *descriptor_sha256;
    const char *extractor_commit;
} Sc64Z8SourceIdentity;

/* Local freestanding equality helper: do not pull in a libc string symbol. */
static inline int sc64_z8_source_text_equal(const char *left, const char *right)
{
    if (left == NULL || right == NULL)
        return 0;
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

/* Fail closed on source identity or row-census drift. This intentionally does
 * not validate runtime state or interpret any source trigger/action. */
static inline int sc64_z8_source_identity_valid(const Sc64Z8SourceIdentity *id)
{
    return id != NULL &&
        id->schema == SC64_Z8_SOURCE_SCHEMA &&
        id->global_id == SC64_Z8_SOURCE_GLOBAL_ID &&
        id->map_cols == SC64_Z8_SOURCE_MAP_COLS &&
        id->map_rows == SC64_Z8_SOURCE_MAP_ROWS &&
        id->trigger_count == SC64_Z8_SOURCE_TRIGGER_COUNT &&
        id->action_count == SC64_Z8_SOURCE_ACTION_COUNT &&
        id->condition_count == SC64_Z8_SOURCE_CONDITION_COUNT &&
        id->location_count == SC64_Z8_SOURCE_LOCATION_COUNT &&
        id->unit_record_count == SC64_Z8_SOURCE_UNIT_RECORD_COUNT &&
        id->preserve_action_count == SC64_Z8_SOURCE_PRESERVE_ACTION_COUNT &&
        id->create_action_count == SC64_Z8_SOURCE_CREATE_ACTION_COUNT &&
        id->nydus_record_count == SC64_Z8_SOURCE_NYDUS_RECORD_COUNT &&
        id->map_path != NULL &&
        sc64_z8_source_text_equal(id->map_path, SC64_Z8_SOURCE_MAP_PATH) &&
        id->source_rom_sha1 != NULL &&
        sc64_z8_source_text_equal(id->source_rom_sha1, SC64_Z8_SOURCE_ROM_SHA1) &&
        id->source_chk_sha256 != NULL &&
        sc64_z8_source_text_equal(id->source_chk_sha256, SC64_Z8_SOURCE_CHK_SHA256) &&
        id->descriptor_sha256 != NULL &&
        sc64_z8_source_text_equal(id->descriptor_sha256, SC64_Z8_SOURCE_DESCRIPTOR_SHA256) &&
        id->extractor_commit != NULL &&
        sc64_z8_source_text_equal(id->extractor_commit, SC64_Z8_SOURCE_EXTRACTOR_COMMIT);
}

#endif /* SC64_Z8_SOURCE_H */
