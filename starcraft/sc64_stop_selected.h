/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_STOP_SELECTED_H
#define SC64_STOP_SELECTED_H
/* The Stop command for the selected units. Included by game.c at the point
 * it was defined (moved out to keep game.c under the source size limit). */
static void
stop_selected(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && u[i].selected) {
#ifdef SC64_SHARED_NUKE_PROBE
            if (sc64_race_ordinary_ghost(u[i].type) && race_nuke_ghost_locked(i)) continue;
#endif
            SC64_CMD_FORGET(i); /* no pending Move lands after a Stop */
#ifdef SC64_WORKER_CARGO_KIND
#ifdef SC64_T6_RUNTIME
            int was_repair = u[i].order == ORDER_REPAIR;
#else
            int was_repair = 0;
#endif
#endif
#ifdef SC64_EGG_RUNTIME
            /* STOP is not Egg cancellation: retain its paid product/timer so
             * the contextual CANCEL EGG action remains the only cancel path. */
            if (u[i].type == SC64_EGG_ACTOR) continue;
#endif
#ifdef SC64_SCARAB_HUMAN_CONTROL_PROBE
            if (sc64_race_raw_unit(u[i].type)==83) (void)race_scarab_stop_signal(i,0);
#endif
            if (IS_WORKER(u[i].type) &&
                (u[i].order == ORDER_MINE || u[i].order == ORDER_GAS))
                u[i].work_timer = 0;
	    u[i].order = ORDER_IDLE;
#ifdef SC64_WORKER_CARGO_KIND
            sc64_worker_gather_pending_set(&u[i], 0);
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
            if (sc64_race_raw_unit(u[i].type)==12) u[i].work_timer&=~7;
#endif
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
            if (sc64_race_raw_unit(u[i].type) != 12)
#endif
#ifdef SC64_SCARAB_HUMAN_CONTROL_PROBE
            if (sc64_race_raw_unit(u[i].type) != 83)
#endif
            SC64_PSC_KEEP_STATE(u[i])
#ifdef SC64_Z5_CLOAK_ENABLED
            if (!race_z5_cloak_actor(&u[i]))
#endif
#ifdef SC64_P6_STORM_ENABLED
            if (!(race_p6_active() && u[i].raw_type == 79 && u[i].raw_owner == 2))
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
            if (!race_z6_cloak_actor(&u[i]))
#endif
#ifdef SC64_WORKER_CARGO_KIND
            if (!IS_WORKER(u[i].type) || was_repair) u[i].carry = 0;
            if (was_repair) {
                sc64_worker_cargo_clear(&u[i]);
                sc64_worker_gather_pending_set(&u[i], 0);
            }
#else
	    u[i].carry = 0;
#endif
	    u[i].target = -1;
	}
    mode = MODE_NORMAL;
    menu_open = 0;
}

#endif
