/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define MAX_UNITS 255
#define MAX_BUILDINGS 160
#define MAX_RACE_GEYSERS 12
#define SC64_SOURCE_GAS 1
#define SC64_GAS_SERVICE_TRIAL 1
typedef uint16_t u16;
typedef uint32_t u32;
typedef struct { int x, y; } Geyser;
#include "../starcraft/sc64_source_gas_save.h"
#include "../starcraft/sc64_gas_service_save.h"
typedef struct { Sc64GasQueueSaveRow row[MAX_UNITS]; int valid[MAX_UNITS]; } Candidate;
static int read_row(void *c, unsigned i, Sc64GasQueueSaveRow *r) { *r=((Candidate *)c)->row[i]; return 1; }
static int write_row(void *c, unsigned i, int carry) { ((Candidate *)c)->row[i].carry=carry; return 1; }
static int eligible(void *c, unsigned i, unsigned site, const Sc64GasQueueSaveRow *r)
{ return ((Candidate *)c)->valid[i] && r->target==(int)site && r->carry==0; }
static unsigned cap(int mode,int map,int mission,unsigned i)
{ (void)mode;(void)map;(void)mission; return i<2 ? 50000u : 0u; }
int main(void)
{
    Candidate c={0}, original, before; Sc64GasServiceQueue q={0}, restored, prior;
    for (unsigned i=0;i<MAX_UNITS;i++) {
        c.row[i]=(Sc64GasQueueSaveRow){1,1,1,159,0,(int)(i%61)}; c.valid[i]=1;
        assert(sc64_gas_service_join(&q,159,i));
    }
    original=c;
    assert(sc64_gas_queue_save_encode(&c,&q,read_row,write_row,eligible));
    assert(c.row[254].carry == (255<<4)); /* fits signed 16- and 17-bit rows */
    memset(&restored,0xa5,sizeof restored);
    assert(sc64_gas_queue_save_decode(&c,&restored,read_row,write_row,eligible));
    assert(!memcmp(&c,&original,sizeof c)); assert(!memcmp(&q,&restored,sizeof q));
    assert(sc64_gas_queue_save_encode(&c,&q,read_row,write_row,eligible));
    before=c; prior=restored;
    c.row[3].carry=c.row[2].carry; before=c;
    assert(!sc64_gas_queue_save_decode(&c,&restored,read_row,write_row,eligible));
    assert(!memcmp(&c,&before,sizeof c) && !memcmp(&restored,&prior,sizeof prior));
    c=original; assert(sc64_gas_queue_save_encode(&c,&q,read_row,write_row,eligible));
    c.row[3].carry=0; before=c; /* Missing middle rank. */
    assert(!sc64_gas_queue_save_decode(&c,&restored,read_row,write_row,eligible));
    assert(!memcmp(&c,&before,sizeof c) && !memcmp(&restored,&prior,sizeof prior));
    c=original; assert(sc64_gas_queue_save_encode(&c,&q,read_row,write_row,eligible));
    c.row[3].target=MAX_BUILDINGS; before=c;
    assert(!sc64_gas_queue_save_decode(&c,&restored,read_row,write_row,eligible));
    assert(!memcmp(&c,&before,sizeof c) && !memcmp(&restored,&prior,sizeof prior));
    c=original; assert(sc64_gas_queue_save_encode(&c,&q,read_row,write_row,eligible));
    c.row[3].carry=9; before=c; /* Neither plain cargo nor a valid rank. */
    assert(!sc64_gas_queue_save_decode(&c,&restored,read_row,write_row,eligible));
    assert(!memcmp(&c,&before,sizeof c) && !memcmp(&restored,&prior,sizeof prior));
    c=original; c.row[3].timer=61; before=c;
    assert(!sc64_gas_queue_save_encode(&c,&q,read_row,write_row,eligible));
    assert(!memcmp(&c,&before,sizeof c));
    c=original; c.valid[3]=0; before=c;
    assert(!sc64_gas_queue_save_encode(&c,&q,read_row,write_row,eligible));
    assert(!memcmp(&c,&before,sizeof c));
    c=original; q.next[254]=1; before=c;
    assert(!sc64_gas_queue_save_encode(&c,&q,read_row,write_row,eligible));
    assert(!memcmp(&c,&before,sizeof c));
    Geyser rows[MAX_RACE_GEYSERS]={{0}}; u16 used[2*MAX_RACE_GEYSERS]={0};
    assert(sc64_gas_save_count_valid(SC64_GAS_QUEUE_SAVE_MARKER|3));
    assert(sc64_gas_save_count(SC64_GAS_QUEUE_SAVE_MARKER|3)==3);
    assert(sc64_gas_save_debts_valid(rows,SC64_GAS_QUEUE_SAVE_MARKER,0,0,0,cap));
    used[0]=8; assert(sc64_gas_save_pack(rows,used,0,0,0,cap));
    assert(sc64_gas_save_debts_valid(rows,SC64_GAS_SAVE_MARKER|SC64_GAS_QUEUE_SAVE_MARKER,0,0,0,cap));
    assert(!sc64_gas_save_count_valid(SC64_GAS_QUEUE_SAVE_MARKER|0x40000));
    puts("gas service save: compact ranks, markers, malformed queues and detached restore pass");
}
