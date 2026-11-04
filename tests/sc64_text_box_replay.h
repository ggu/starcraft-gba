/* SPDX-License-Identifier: GPL-3.0-or-later */
/* For key-tape players. Include before game.c, and call TEXT_BOXES_DISMISS after
 * each tape frame. A box takes no simulation step, so its frames are inserted
 * between tape ticks and the tape keeps its own numbering. Dismissal is ordinary
 * input through game_update(), and it leaves the previous keys as the tape left
 * them. A tape holding A across a box dismisses with Start, keeping that A down. */
#ifndef SC64_TEXT_BOX_REPLAY_H
#define SC64_TEXT_BOX_REPLAY_H

#ifdef SC64_TEXT_BOXES
#define SC64_TEXT_BOX_HOST_MODAL
static u32 text_boxes_acknowledged;
#define TEXT_BOXES_DISMISS(frame_fn) do { \
        while (sc64_text_box_count()) { \
            u16 tape_keys_now = previous_held; \
            u16 key = (tape_keys_now & KEY_A) ? KEY_START : KEY_A; \
            frame_fn((u16)(tape_keys_now | key)); \
            frame_fn(tape_keys_now); \
            ++text_boxes_acknowledged; \
        } \
    } while (0)
#else
#define TEXT_BOXES_DISMISS(frame_fn) ((void)0)
#endif

#endif
