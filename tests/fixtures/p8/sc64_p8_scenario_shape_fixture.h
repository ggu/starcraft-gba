/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored shape-only descriptor fixture for the P8 world adapter tests. */
#ifndef TEST_SC64_P8_SCENARIO_GENERATED_H
#define TEST_SC64_P8_SCENARIO_GENERATED_H

#define SC64_P8_SOURCE_GLOBAL_ID 27u
#define SC64_P8_SOURCE_MAP_COLS 128u
#define SC64_P8_SOURCE_MAP_ROWS 128u
#define SC64_P8_SOURCE_TRIGGER_COUNT 8u
#define SC64_P8_SOURCE_ACTION_COUNT 68u
#define SC64_P8_SOURCE_CONDITION_COUNT 9u
#define SC64_P8_SOURCE_LOCATION_COUNT 64u

typedef struct {
    unsigned int location, string_id, wav_id, time, group1, group2;
    unsigned short unit;
    unsigned char trigger, index, opcode, modifier, flags, trailing[3];
} Sc64P8SourceAction;
typedef struct {
    unsigned int location, player, amount;
    unsigned short unit, mask;
    unsigned char trigger, index, comparison, opcode, resource, flags;
} Sc64P8SourceCondition;
typedef struct {
    unsigned short first_action, action_count, first_condition, condition_count;
    unsigned int execution_flags;
    unsigned char initial_cursor, enabled_players[27];
} Sc64P8SourceTrigger;
typedef struct {
    unsigned int left, top, right, bottom;
    unsigned short name_id, flags;
} Sc64P8SourceLocation;

static const Sc64P8SourceAction sc64_p8_source_actions[68] = {{0}};
static const Sc64P8SourceCondition sc64_p8_source_conditions[9] = {{0}};
static const Sc64P8SourceTrigger sc64_p8_source_triggers[8] = {{0}};
static const Sc64P8SourceLocation sc64_p8_source_locations[64] = {{0}};

#endif
