/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_PRESENCE_H
#define SC64_SCARAB_PRESENCE_H

/* Fields are meaningful only after the actor adapter validates/initializes
 * the row: phases 0/1 are training/inside, 2/3 outside/self-destruct. Parent
 * 255 on an outside child is detached, never a live slot reference. */
#if defined(SC64_PAID_SCARAB) && defined(SC64_SCARAB_RUNTIME)
#error "The paid Scarab product and the staged Scarab runtime are alternatives"
#endif
#ifdef SC64_PAID_SCARAB
/* The paid service (sc64_paid_scarab.h) keeps ammunition as a count on the
 * Reaver; the only Scarab actor is one in flight. It is drawn, but nothing
 * can target, see through, collide with or select it. */
#ifndef SC64_RACES
#error "Scarab actors require the imported race engine"
#endif
#define SC64_SCARAB_CHILD(q) ((q)->type == U_SCARAB)
#define SC64_SCARAB_HIDDEN(q) ((q)->type == U_SCARAB)
#define SC64_SCARAB_NONCOLLIDING(q) ((q)->type == U_SCARAB)
#define SC64_SCARAB_DRAW_HIDDEN(q) 0
#elif defined(SC64_SCARAB_RUNTIME)
#ifndef SC64_RACES
#error "Scarab actors require the imported race engine"
#endif
#define SC64_SCARAB_CHILD(q) ((q)->type == U_SCARAB)
#define SC64_SCARAB_HIDDEN(q) (SC64_SCARAB_CHILD(q) && \
    (((unsigned)(q)->carry >> 8) & 3u) < 2u)
#define SC64_SCARAB_NONCOLLIDING(q) (SC64_SCARAB_HIDDEN(q) || \
    (SC64_SCARAB_CHILD(q) && (((unsigned)(q)->work_timer >> 7) & 7u)))
#define SC64_SCARAB_DRAW_HIDDEN(q) SC64_SCARAB_HIDDEN(q)
#else
#define SC64_SCARAB_CHILD(q) 0
#define SC64_SCARAB_HIDDEN(q) 0
#define SC64_SCARAB_NONCOLLIDING(q) 0
#define SC64_SCARAB_DRAW_HIDDEN(q) 0
#endif
#endif
