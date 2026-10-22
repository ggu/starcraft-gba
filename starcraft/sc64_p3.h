#ifndef SC64_P3_H
#define SC64_P3_H

/* Source-only Higher Ground facts for asset-free fixtures and future runtime
 * work. These constants describe the CHK; they do not execute triggers or
 * make catalog mission 22 playable. Values are from audited BOLT 008/020. */
enum {
    SC64_P3_GLOBAL_ID = 22,
    SC64_P3_WIDTH_TILES = 128,
    SC64_P3_HEIGHT_TILES = 96,
    SC64_P3_HUMAN_SLOT = 7,
    SC64_P3_FORCE2_SLOT = 0,
    SC64_P3_FORCE3_SLOT = 6,
    SC64_P3_NEUTRAL_SLOT = 11,
    SC64_P3_HUMAN_MINERALS = 300,
    SC64_P3_HUMAN_GAS = 100,
    SC64_P3_FORCE2_COMBINED_RESOURCES = 5000,
    SC64_P3_FORCE3_COMBINED_RESOURCES = 8000,
    SC64_P3_FOES_ZERO_BUILDINGS_WAIT_MS = 2000,
    SC64_P3_FOES_ZERO_BUILDINGS_POST_TX_WAIT_MS = 3000,
    SC64_P3_FOES_ZERO_BUILDINGS_TRANSMISSIONS = 2,
    SC64_P3_FORCE2_SCRIPT_DWORD = 1194553936,
    SC64_P3_FORCE3_SCRIPT_DWORD = 1379103312,
    SC64_P3_TRIGGER_CURRENT_PLAYER = 13,
    SC64_P3_TRIGGER_FOES = 14,
    SC64_P3_TRIGGER_FORCE2 = 19,
    SC64_P3_TRIGGER_FORCE3 = 20,
    SC64_P3_RAW_ANY_UNIT = 229,
    SC64_P3_RAW_BUILDINGS = 231,
    SC64_P3_CONDITION_COMMAND = 2,
    SC64_P3_CONDITION_ELAPSED_TIME = 12,
    SC64_P3_CONDITION_AT_LEAST = 0,
    SC64_P3_CONDITION_AT_MOST = 1,
    SC64_P3_ACTION_DEFEAT = 2,
    SC64_P3_ACTION_WAIT = 4,
    SC64_P3_ACTION_TRANSMISSION = 7,
    SC64_P3_ACTION_VICTORY = 1,
    SC64_P3_ACTION_SET_RESOURCES = 26,
    SC64_P3_ACTION_SET_OBJECTIVES = 12,
    SC64_P3_ACTION_RUN_AI_SCRIPT_AT_LOCATION = 16
};

typedef struct Sc64P3SourceAction {
    unsigned char opcode, location, string_id, wav_id;
    unsigned short time_ms;
    unsigned int group1, group2;
    unsigned short unit_type, modifier, flags;
} Sc64P3SourceAction;

typedef struct Sc64P3SourceCondition {
    unsigned char opcode, player_group, amount, unit_type;
    unsigned char comparison, resource, flags;
} Sc64P3SourceCondition;

/* Raw execution facts for the loss condition, mission-start reward/objective,
 * delayed victory, and the two separate Zerg-force initialization triggers.
 * Action rows contain raw fields; timings are not normalized into frame time. */
static const Sc64P3SourceCondition sc64_p3_source_conditions[5] = {
    {2, 13, 0, 229, 1, 0, 16}, /* Current Player has <=0 Any Unit */
    {12, 0, 0, 0, 0, 0, 0},    /* Elapsed Time >=0 */
    {2, 14, 0, 231, 1, 0, 16}, /* Foes have <=0 Buildings */
    {2, 20, 1, 229, 0, 0, 16}, /* Force 3 has >=1 Any Unit */
    {2, 19, 1, 229, 0, 0, 16}  /* Force 2 has >=1 Any Unit */
};

static const Sc64P3SourceAction sc64_p3_source_rewards[2] = {
    {26, 0, 0, 0, 0, 13, 300, 0, 7, 0},
    {26, 0, 0, 0, 0, 13, 100, 1, 7, 0}
};

static const Sc64P3SourceAction sc64_p3_source_foes_zero_actions[5] = {
    {4, 0, 0, 0, 2000, 0, 0, 0, 0, 0},
    {7, 64, 2, 14, 0, 0, 0, 77, 9, 16},
    {7, 1, 11, 14, 0, 0, 0, 87, 9, 16},
    {4, 0, 0, 0, 3000, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static const Sc64P3SourceAction sc64_p3_source_force3_actions[2] = {
    {26, 0, 0, 0, 0, 20, 8000, 2, 7, 0},
    {16, 3, 0, 0, 0, 0, 1379103312u, 0, 0, 0}
};

static const Sc64P3SourceAction sc64_p3_source_force2_actions[2] = {
    {26, 0, 0, 0, 0, 19, 5000, 2, 7, 0},
    {16, 4, 0, 0, 0, 0, 1194553936u, 0, 0, 0}
};

#endif
