#ifndef SC64_RACE_MISSIONS_H
#define SC64_RACE_MISSIONS_H

/* Small saveable state for the authored campaigns (four Races maps plus
 * optional Terran missions in the unified build).
 * Timers are simulation frames (30 updates per game second). */
typedef struct {
    unsigned int elapsed_frames;
    unsigned int objective_flags;
    unsigned short rescue_unit;
    unsigned short chrysalis_unit;
    unsigned short carrier_unit;
    unsigned short enemy_units_sent;
    unsigned short message_ticks;
    unsigned char mission;
    unsigned char objective_stage;
    unsigned char phase;
    unsigned char message;
    unsigned char enemy_wave;
} Sc64RaceCampaignState;

enum {
    SC64_RACE_OBJECTIVE_PRIMARY = 1u << 0,
    SC64_RACE_OBJECTIVE_SECONDARY = 1u << 1,
    SC64_RACE_OBJECTIVE_FENIX_MET = 1u << 2,
    SC64_RACE_OBJECTIVE_FENIX_READY = 1u << 3,
    SC64_RACE_OBJECTIVE_CHRYSALIS_RECOVERED = 1u << 4,
    SC64_RACE_OBJECTIVE_CHRYSALIS_LOST = 1u << 5,
    SC64_RACE_OBJECTIVE_FENIX_LOST = 1u << 6,
    SC64_RACE_OBJECTIVE_CEREBRATE_DEAD = 1u << 7
};

enum {
    SC64_RACE_PHASE_SETUP,
    SC64_RACE_PHASE_ACTIVE,
    SC64_RACE_PHASE_REINFORCEMENTS,
    SC64_RACE_PHASE_COMPLETE,
    SC64_RACE_PHASE_FAILED
};

/* Included by game.c after Unit, Building, m, geysers, and engine helpers. */
static void race_campaign_init(void);
static void race_campaign_tick(void);
static const char *race_campaign_objective_text(void);
static const char *race_campaign_victory_text(void);
static const char *race_campaign_message_text(void);
static int race_campaign_won(void);
static int race_campaign_lost(void);

#endif
