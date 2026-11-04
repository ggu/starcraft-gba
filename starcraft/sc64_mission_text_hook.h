/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_MISSION_TEXT_HOOK_H
#define SC64_MISSION_TEXT_HOOK_H

/* A mission interpreter reports each row it runs. Adapters are also compiled
 * alone by their own tests, where there is no box to open. */
#ifndef SC64_MISSION_TEXT_POST
#define SC64_MISSION_TEXT_POST(opcode, string_id, unit) ((void)0)
#define race_mission_text_post_source(trigger, action) ((void)0)
#endif

#endif
