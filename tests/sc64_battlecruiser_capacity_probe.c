/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/game.c"
#include <stdio.h>
int main(void) { printf("{\"race_save\":%zu,\"save_free\":%zu,\"unit\":%zu,\"building\":%zu}\n",sizeof(RaceSave),(size_t)16368-sizeof(RaceSave),sizeof(Unit),sizeof(Building)); return 0; }
