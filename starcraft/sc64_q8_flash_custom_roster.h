/* SPDX-License-Identifier: GPL-3.0-or-later
 * Custom current controllers, independent of any actor's retained raw origin. */
#ifndef SC64_Q8_FLASH_CUSTOM_ROSTER_H
#define SC64_Q8_FLASH_CUSTOM_ROSTER_H
static inline int sc64_q8_flash_custom_roster_valid(int human,int bots,
                                                    const int players[5],const int selected[3])
{
    static const int owners[4]={SIDE,ENEMY,RESCUABLE,4};
    if(!players || !selected || human<R_TER || human>R_PROTOSS ||
       bots<1 || bots>3 || players[SIDE]!=human) return 0;
    for(int i=0;i<bots;++i)
        if(selected[i]<R_TER || selected[i]>R_PROTOSS ||
           players[owners[i+1]]!=selected[i]) return 0;
    return 1;
}
#endif
