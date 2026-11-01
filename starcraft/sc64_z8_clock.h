/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_Z8_CLOCK_H
#define SC64_Z8_CLOCK_H

/* The engine frame is incremented once before each active Z8 update and is
 * already saved. Its residue therefore supplies the 4/5 source-clock phase
 * without changing the save format. The first engine update does not advance the
 * scheduler; the next four do. */
static inline int sc64_z8_native_step(unsigned engine_frame)
{
    return engine_frame != 0u && engine_frame % 5u != 1u;
}

#endif
