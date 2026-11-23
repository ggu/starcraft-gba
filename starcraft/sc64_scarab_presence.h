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
/* Two invariants let the IWRAM actor loops skip the Scarab test (each test costs 12 bytes
 * there, and the replay ROMs have little IWRAM to spare).
 *
 * 1. A collision bucket chain never holds a live flying Scarab. sc64_unit_buckets_begin()
 *    leaves the row out when it rebuilds the chains; sc64_unit_bucket_move() never links it
 *    and switches the buckets off instead; spawn_unit() calls sc64_unit_bucket_move() for a
 *    birth while the buckets are active, so a Scarab that reuses a dead linked slot switches
 *    them off before any later query; every other writer of the chain head, next and prev
 *    arrays is sc64_unit_bucket_insert(), reached only from those two functions. A loop that
 *    walks a chain uses SC64_SCARAB_NONCOLLIDING_IN_CHAIN, which tests nothing on the ROM.
 * 2. A Scarab is never the mover of a collision query: its flight (sc64_psc_flight_tick) moves
 *    it by itself and no order, route or steering code is given the row.
 *    SC64_SCARAB_NONCOLLIDING_MOVER guards the movers.
 * Host builds (sanitized controllers, tests) trap on a violation of either; loops over the
 * whole actor table, and the insertion functions themselves, keep SC64_SCARAB_NONCOLLIDING. */
/* 3. The only caller that hands a Scarab to sc64_unit_bucket_move() is spawn_unit() at its
 *    birth, and it tests SC64_SCARAB_BIRTH first and switches the buckets off itself; no
 *    Scarab is passed to sc64_target_index_move(), which never indexes one. The two
 *    insertion functions therefore carry no Scarab test either (SC64_SCARAB_BUCKET_ROW,
 *    SC64_SCARAB_INDEX_ROW). */
#define SC64_SCARAB_BIRTH(q) ((q)->type == U_SCARAB)
#ifdef GBA_HOST
#define SC64_SCARAB_NONCOLLIDING_IN_CHAIN(q) ((q)->type == U_SCARAB ? (__builtin_trap(), 1) : 0)
#define SC64_SCARAB_NONCOLLIDING_MOVER(q) ((q)->type == U_SCARAB ? (__builtin_trap(), 1) : 0)
#define SC64_SCARAB_BUCKET_ROW(q) ((q)->type == U_SCARAB ? (__builtin_trap(), 1) : 0)
#define SC64_SCARAB_INDEX_ROW(q) ((q)->type == U_SCARAB ? (__builtin_trap(), 1) : 0)
#else
#define SC64_SCARAB_NONCOLLIDING_IN_CHAIN(q) 0
#define SC64_SCARAB_NONCOLLIDING_MOVER(q) 0
#define SC64_SCARAB_BUCKET_ROW(q) 0
#define SC64_SCARAB_INDEX_ROW(q) 0
#endif
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
#define SC64_SCARAB_NONCOLLIDING_IN_CHAIN(q) SC64_SCARAB_NONCOLLIDING(q)
#define SC64_SCARAB_NONCOLLIDING_MOVER(q) SC64_SCARAB_NONCOLLIDING(q)
#define SC64_SCARAB_BIRTH(q) 0
#define SC64_SCARAB_BUCKET_ROW(q) SC64_SCARAB_NONCOLLIDING(q)
#define SC64_SCARAB_INDEX_ROW(q) SC64_SCARAB_HIDDEN(q)
#else
#define SC64_SCARAB_CHILD(q) 0
#define SC64_SCARAB_HIDDEN(q) 0
#define SC64_SCARAB_NONCOLLIDING(q) 0
#define SC64_SCARAB_DRAW_HIDDEN(q) 0
#define SC64_SCARAB_NONCOLLIDING_IN_CHAIN(q) 0
#define SC64_SCARAB_NONCOLLIDING_MOVER(q) 0
#define SC64_SCARAB_BIRTH(q) 0
#define SC64_SCARAB_BUCKET_ROW(q) 0
#define SC64_SCARAB_INDEX_ROW(q) 0
#endif
#endif
