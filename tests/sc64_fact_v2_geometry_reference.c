/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_p7_geometry.h"
int p7_rect_reference(unsigned raw,int x,int y,int building,
                      int *left,int *top,int *right,int *bottom)
{ return sc64_p7_actor_rect_source(raw,x,y,building,left,top,right,bottom); }
