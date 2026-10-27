/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_game_actor_facts.h"
#include "../starcraft/sc64_fact_tables.h"
#include "../starcraft/sc64_fact_tables.h"
#include "../starcraft/sc64_p7_geometry.h"
#ifndef SC64_P7_RECT_TABLES
#error "Candidate must use the generated rectangle table"
#endif
int p7_rect_candidate(unsigned raw,int x,int y,int building,
                      int *left,int *top,int *right,int *bottom)
{ return sc64_p7_actor_rect_source(raw,x,y,building,left,top,right,bottom); }
