/* SPDX-License-Identifier: GPL-3.0-or-later */
/* How mission runtimes post an in-mission text box. A build without text boxes
 * compiles these out, which also leaves the runtimes usable from fixtures that
 * do not include game.c. Rows absent from the generated table are ignored, so
 * disabled source actions need no check here. */
#ifndef SC64_TEXT_BOX_HOOK_H
#define SC64_TEXT_BOX_HOOK_H

#ifdef SC64_TEXT_BOXES
#include "sc64_text_box.h"
static int race_text_box_post_string(unsigned long string_id);
static int race_text_box_post_row(unsigned trigger, unsigned action);
#define RACE_TEXT_BOX_TRY_ROW(trigger, action) race_text_box_post_row((unsigned)(trigger), (unsigned)(action))
#define RACE_TEXT_BOX_TRY_STRING(string_id) race_text_box_post_string((unsigned long)(string_id))
#define RACE_TEXT_BOX_TRY_ACTION(opcode, string_id) \
    (((opcode) == 7u || (opcode) == 9u) ? RACE_TEXT_BOX_TRY_STRING(string_id) : 0)
#define RACE_TEXT_BOX_ROW(trigger, action) race_text_box_post_row((unsigned)(trigger), (unsigned)(action))
#define RACE_TEXT_BOX_STRING(string_id) race_text_box_post_string((unsigned long)(string_id))
/* For runtimes that share one case among Transmission, DisplayText and others. */
#define RACE_TEXT_BOX_ACTION(opcode, string_id) do { \
        if ((opcode) == 7u || (opcode) == 9u) RACE_TEXT_BOX_STRING(string_id); \
    } while (0)
#else
#define RACE_TEXT_BOX_TRY_ROW(trigger, action) 0
#define RACE_TEXT_BOX_TRY_STRING(string_id) 0
#define RACE_TEXT_BOX_TRY_ACTION(opcode, string_id) 0
#define RACE_TEXT_BOX_ROW(trigger, action) ((void)0)
#define RACE_TEXT_BOX_STRING(string_id) ((void)0)
#define RACE_TEXT_BOX_ACTION(opcode, string_id) ((void)0)
#endif

#endif
