/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include "../common/gba.h"
#include "../starcraft/sc64_scarab_steer_motion.h"
int main(int argc,char **argv) {
    if(argc!=4)return 2;
    Sc64ScarabSteer s={.x=(s32)strtol(argv[1],0,10),.y=(s32)strtol(argv[2],0,10),.heading=(u8)strtoul(argv[3],0,10),.velocity=(u8)strtoul(argv[3],0,10),.turning=1,.age=1};
    int wx,wy;
    while(scanf("%d %d",&wx,&wy)==2) {
        if(!sc64_scarab_steer_tick(&s,wx,wy,1))return 1;
        printf("%d %d %u %u\n",s.x,s.y,s.heading,s.velocity);
    }
    return 0;
}
