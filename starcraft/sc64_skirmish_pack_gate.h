/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SKIRMISH_PACK_GATE_H
#define SC64_SKIRMISH_PACK_GATE_H
/* This include must use the same search path as game.c. In particular the
 * cover bank may precede the standard skirmish bank. */
#include "sc64_skirmish_generated.h"
#if !defined(SC64_SKIRMISH_PACK_REVISION) || SC64_SKIRMISH_PACK_REVISION != 2
#error "Stale skirmish pack: regenerate standard and cover headers with tools/sc64_skirmish_pack.py (see docs/DEVELOPMENT.md)"
#endif
#endif
