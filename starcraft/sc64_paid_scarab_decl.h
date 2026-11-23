/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_PAID_SCARAB_DECL_H
#define SC64_PAID_SCARAB_DECL_H
/* What game.c needs before the paid Scarab service (sc64_paid_scarab.h) is
 * defined: forward declarations and the hook macros, all empty (and the
 * label the Interceptor one) when SC64_PAID_SCARAB is off. */
/* The target-loss hook of the staged probe is the paid service's too. */
#if defined(SC64_PAID_SCARAB) || defined(SC64_SCARAB_WEAPON_PROBE)
#define SC64_PSC_TARGET_HOOK 1
#else
#define SC64_PSC_TARGET_HOOK 0
#endif
#ifdef SC64_PAID_SCARAB
#ifndef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
#error "Paid Scarabs use the Protoss hangar command and projectile services"
#endif
static void race_paid_scarab_trained(int slot);
static int race_paid_scarab_menu_available(void);
static int race_paid_scarab_stock(int slot);
static int race_paid_scarab_queued(int slot);
static int race_paid_scarab_queue_owner(int slot);
static int race_paid_scarab_queue_selected(void);
static void race_paid_scarab_launch(Unit *q, int target);
static u8 sc64_psc_hint;
static void race_paid_scarab_world_tick(void);
static void race_paid_scarab_clear_slot(int slot);
static void race_paid_scarab_end_frame(void);
static void race_scarab_probe_target_invalidate(int ref);
static int race_paid_scarab_candidate_valid(const Unit *actors, int count);
static const char *race_carrier_menu_label(void);
#define PSC_LABEL() race_carrier_menu_label()
/* A loaded world may hold a flight or a Reaver with work: wake the service once the actors are in
 * place. A new world starts with it asleep. */
#define SC64_PSC_WORLD_LOADED() (sc64_psc_hint = 1)
#define SC64_PSC_WORLD_RESET() (sc64_psc_hint = 0)
/* The Reaver's Scarab build progress lives in work_timer, which a Shield Battery order must not wipe,
 * and which the recharge save check must accept for a Reaver. */
#define SC64_PSC_BATTERY_WORK(q) do { if ((q)->type != U_REAVER) (q)->work_timer = 0; } while (0)
#define SC64_PSC_RECHARGE_WORK_OK(a) ((a)->type == U_REAVER)
/* A dead cargo Reaver sheds its state like any other death. */
#define SC64_PSC_CLEAR_SLOT(i) race_paid_scarab_clear_slot(i)
/* STOP keeps a Reaver's Scarabs and orders. */
#define SC64_PSC_KEEP_STATE(q) if ((q).type != U_REAVER)
/* In reach, a Reaver fights only with a Scarab it holds: never the generic hit. */
#define SC64_PSC_FIRE(q, target) if ((q)->type == U_REAVER) { race_paid_scarab_launch((q), (target)); return; }
/* Source 24 Hz ticks, only while something is ordered, cooling down or flying. */
#define SC64_PSC_WORLD_TICK() \
    do { if (sc64_psc_hint && (unsigned)frame % 5u != 0u) race_paid_scarab_world_tick(); } while (0)
#else
#define SC64_PSC_WORLD_LOADED() ((void)0)
#define SC64_PSC_WORLD_RESET() ((void)0)
#define SC64_PSC_BATTERY_WORK(q) ((q)->work_timer = 0)
#define SC64_PSC_RECHARGE_WORK_OK(a) 0
#define SC64_PSC_CLEAR_SLOT(i) ((void)0)
#define PSC_LABEL() "BUILD INTERCEPTOR"
#define SC64_PSC_KEEP_STATE(q)
#define SC64_PSC_FIRE(q, target)
#define SC64_PSC_WORLD_TICK()
#endif
#endif
