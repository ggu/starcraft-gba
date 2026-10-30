/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Records the held keys a host controller passes to game_update().
 *
 * Include immediately after the controller's game.c include; every later
 * game_update() call then goes through the recorder. SC64_TAPE_RECORD names
 * the output: one "tick held" line per change of held keys, then "end N".
 * A controller whose pressed argument is not the ordinary rising edge of held
 * cannot be replayed from held keys alone, so the recorder rejects it.
 * tools/sc64_impact_tape.py turns the file into a replay header. */
#ifndef SC64_INPUT_TAPE_RECORD_H
#define SC64_INPUT_TAPE_RECORD_H
#include <stdio.h>
#include <stdlib.h>

static FILE *tape_record_file;
static unsigned tape_record_tick;
static u16 tape_record_previous;

static void
tape_record_close(void)
{
    if (!tape_record_file) return;
    fprintf(tape_record_file, "end %u\n", tape_record_tick);
    fclose(tape_record_file);
    tape_record_file = 0;
}

static void
tape_record_update(u16 held, u16 pressed)
{
    if (!tape_record_file) {
        const char *path = getenv("SC64_TAPE_RECORD");
        if (!path || !(tape_record_file = fopen(path, "w"))) abort();
        atexit(tape_record_close);
    }
    if (pressed != (u16)(held & (u16)~tape_record_previous)) {
        fprintf(stderr, "tape: non-edge pressed keys at tick %u\n", tape_record_tick + 1u);
        abort();
    }
    ++tape_record_tick;
    if (held != tape_record_previous)
        fprintf(tape_record_file, "%u %u\n", tape_record_tick, (unsigned)held);
    tape_record_previous = held;
    game_update(held, pressed);
}
#define game_update tape_record_update
#endif
