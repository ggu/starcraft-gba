/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_UNIT_SLOT_FREE_H
#define SC64_UNIT_SLOT_FREE_H
#ifndef SC64_UNIT_SLOT_FREE
#ifdef SC64_SHARED_DEFILER_EXECUTOR
#error "Defiler field reservations require the owning world's slot predicate"
#else
#define SC64_UNIT_SLOT_FREE(i) (!u[i].alive)
#endif
#endif
#endif
