/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The wire has 24 independent source-ordinal debts even when the world has
 * fewer than 12 neutral geysers (some sites begin with a refinery). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint16_t u16;
typedef uint32_t u32;
typedef struct { int x, y; } Geyser;
#define MAX_RACE_GEYSERS 12
#define SC64_SOURCE_GAS 1
#include "../starcraft/sc64_source_gas_save.h"
static unsigned capacity(int mode, int map, int mission, unsigned ordinal)
{
    assert(mode == 0 && map == 1 && mission == 29);
    return ordinal < 16 ? 50003u : 0u;
}
int main(void)
{
    Geyser rows[MAX_RACE_GEYSERS], before[MAX_RACE_GEYSERS], out[MAX_RACE_GEYSERS];
    u16 used[2*MAX_RACE_GEYSERS] = {0}, decoded[2*MAX_RACE_GEYSERS];
    for (unsigned i=0;i<MAX_RACE_GEYSERS;i++) rows[i]=(Geyser){(int)(i*16), (int)(i*24)};
    memcpy(before, rows, sizeof rows);
    assert(sc64_gas_save_debts_valid(rows,0,0,1,29,capacity));
    assert(sc64_gas_save_pack(rows,used,0,1,29,capacity));
    assert(!memcmp(rows,before,sizeof rows)); /* zero debt is byte-identical */
    sc64_gas_save_publish(rows,out,decoded);
    assert(!memcmp(rows,out,sizeof rows));
    for (unsigned i=0;i<2*MAX_RACE_GEYSERS;i++) assert(decoded[i]==0);
    used[0]=8; used[1]=32768; used[15]=50003;
    assert(sc64_gas_save_pack(rows,used,0,1,29,capacity));
    assert(sc64_gas_save_debts_valid(rows,SC64_GAS_SAVE_MARKER,0,1,29,capacity));
    assert(sc64_gas_save_debts_valid(rows,SC64_GAS_SAVE_MARKER,0,1,29,capacity)); /* no mutation on retry */
    assert(!sc64_gas_save_debts_valid(rows,0,0,1,29,capacity)); /* marker required */
    assert(!sc64_gas_save_debts_valid(before,SC64_GAS_SAVE_MARKER,0,1,29,capacity));
    assert(!sc64_gas_save_count_valid(SC64_GAS_SAVE_MARKER | (MAX_RACE_GEYSERS+1)));
    assert((int)SC64_GAS_SAVE_MARKER > MAX_RACE_GEYSERS); /* old preflight rejects */
    sc64_gas_save_publish(rows,out,decoded);
    assert(!memcmp(before,out,sizeof out)); assert(!memcmp(used,decoded,sizeof used));
    Geyser bad[MAX_RACE_GEYSERS]; memcpy(bad,rows,sizeof bad);
    u32 invalid = (u32)sc64_gas_save_coordinate(bad[0].x) | (50004u << 16);
    memcpy(&bad[0].x,&invalid,sizeof invalid);
    assert(!sc64_gas_save_debts_valid(bad,SC64_GAS_SAVE_MARKER,0,1,29,capacity));
    memcpy(bad,before,sizeof bad); used[0]=9;
    assert(!sc64_gas_save_pack(bad,used,0,1,29,capacity));
    used[0]=8; used[16]=8;
    assert(!sc64_gas_save_pack(bad,used,0,1,29,capacity));
    assert(!memcmp(bad,before,sizeof bad)); /* failed write is atomic */
    used[16]=0; bad[3].x=-1; memcpy(out,bad,sizeof bad);
    assert(!sc64_gas_save_pack(bad,used,0,1,29,capacity));
    assert(!memcmp(bad,out,sizeof bad));
    puts("source gas save: legacy, 16 ordinals, retries and malformed debts pass");
}
