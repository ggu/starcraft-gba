/* SPDX-License-Identifier: GPL-3.0-or-later
 * Authored HOST-only journals: compare the old selector with prepared plans.
 * These invalid fixtures must never be used as guest SRAM. */
#define main original_selection_regressions
#include "gba_save_selection.c"
#undef main

typedef struct { unsigned calls; int plan, live; } Prepared;
static int prepare_once(const void *data,u32 len,u32 tag,void *context) {
    Prepared *p=context;
    int accepted=validate(data,len,tag,0);
    ++p->calls;p->plan=accepted?(int)(tag>>24):-1;
    memset((void*)data,0x3c,len); /* Prepared storage differs from raw wire bytes. */
    return accepted;
}
static void compare_selectors(void) {
    GbaSaveSelection old={11,22,33,44,55},next=old,untouched=old;
    Prepared plan={0,-7,99};
    memcpy(before,host_save,sizeof(before));
    int expected=save_select_validated(scratch,sizeof(scratch),tags,3,validate,0,&old);
    int actual=save_select_prepared(scratch,sizeof(scratch),tags,3,prepare_once,&plan,&next);
    assert(actual==expected && !memcmp(&old,&next,sizeof(old)));
    assert(plan.live==99 && !memcmp(before,host_save,sizeof(before)));
    if(actual) {
        assert(plan.plan==(int)(next.tag>>24));
        for(unsigned i=0;i<next.length;i++)assert(scratch[i]==0x3c);
    } else assert(!memcmp(&next,&untouched,sizeof(next)));
}
static Prepared prepared_success(int slot,unsigned calls) {
    GbaSaveSelection selected={0};Prepared plan={0,-7,99};
    assert(save_select_prepared(scratch,sizeof(scratch),tags,3,prepare_once,&plan,&selected));
    assert(selected.slot==slot && plan.calls==calls && plan.live==99);
    assert(plan.plan==(int)(selected.tag>>24) && scratch[0]==0x3c);
    return plan;
}
static u32 random_state=0x8a2bcf11u;
static u32 random_word(void) { random_state=random_state*1664525u+1013904223u;return random_state; }
static int alter_record_after_prepare(const void *data,u32 len,u32 tag,void *context) {
    int accepted=validate(data,len,tag,0);
    /* Deliberately violate the callback contract in this HOST-only fixture
     * to prove the final committed-record check still rejects a stale token. */
    if(*(int*)context)sram_put32(host_save+12,8);
    else host_save[18]^=1;
    return accepted;
}
int main(void) {
    /* Preserve the existing raw-selector/transaction contract, including the
     * deliberate failing third preflight. The new API does not replace it. */
    assert(original_selection_regressions()==0);
    reset();record(0,tags[0],10,0xa5);prepared_success(0,1);
    reset();record(0,tags[0],10,0xa5);record(1,tags[2],11,0xa5);prepared_success(1,1);
    reset();record(0,tags[2],11,0xa5);record(1,tags[0],10,0xa5);prepared_success(0,1);
    reset();record(0,tags[0],10,0xa5);record(1,tags[2],11,0);prepared_success(0,2);
    reset();record(0,tags[0],7,0xa5);record(1,tags[2],7,0xa5);prepared_success(0,1);
    /* Half-range ordering is undefined only when both payloads validate. A
     * rejected second row destroys the first plan and requires its rebuild. */
    reset();record(0,tags[0],1,0xa5);record(1,tags[2],0x80000001u,0);prepared_success(0,3);
    reset();record(0,tags[0],1,0);record(1,tags[2],0x80000001u,0xa5);prepared_success(1,2);
    static const u32 sequences[]={0,1,7,0x7fffffffu,0x80000000u,0x80000001u,0xffffffffu};
    for(unsigned a=0;a<3;a++)for(unsigned z=0;z<3;z++)
      for(unsigned s=0;s<sizeof(sequences)/sizeof(sequences[0]);s++)
       for(unsigned t=0;t<sizeof(sequences)/sizeof(sequences[0]);t++)
        for(unsigned mask=0;mask<4;mask++) {
            reset();record(0,tags[a],sequences[s],mask&1?0xa5:0);
            record(1,tags[z],sequences[t],mask&2?0xa5:0);compare_selectors();
        }
    /* Independent raw-selector oracle for torn headers, unknown namespaces,
     * checksum errors and semantically invalid, otherwise committed records. */
    for(unsigned trial=0;trial<8192;trial++) {
        reset();
        for(unsigned slot=0;slot<2;slot++) {
            u32 r=random_word(),tag=(r&7)<6?tags[(r>>4)%3]:0x55667788u;
            record((int)slot,tag,random_word(),r&0x100?0xa5:0);
            volatile u8 *wire=host_save+slot*SAVE_SLOT_SIZE;
            switch((r>>12)&7) {
            case 0:wire[18]^=1;break;
            case 1:sram_put32(wire,0);break;
            case 2:sram_put32(wire+4,sizeof(scratch)+1);break;
            case 3:sram_put32(wire+12,0);break;
            default:break;
            }
        }
        compare_selectors();
    }
    /* The prepared token protects the exact predecessor during publication. */
    reset();record(0,tags[0],5,0xa5);record(1,tags[2],6,0xa5);
    GbaSaveSelection selected={0};Prepared plan={0,-7,99};
    assert(save_select_prepared(scratch,sizeof(scratch),tags,3,prepare_once,&plan,&selected));
    u8 saved[SAVE_SLOT_SIZE];memcpy(saved,host_save+SAVE_SLOT_SIZE,sizeof(saved));
    const u8 payload[]={0x30,0xa5,4,5};
    assert(save_write_selected(payload,sizeof(payload),tags[2],&selected,tags,3));
    assert(!memcmp(saved,host_save+SAVE_SLOT_SIZE,sizeof(saved)));
    /* Invalid API arguments never publish a selection token or call prepare. */
    const u32 duplicate[]={tags[0],tags[0]},zero[]={0};
    GbaSaveSelection sentinel={11,22,33,44,55},token=sentinel;
    plan=(Prepared){0,-7,99};
    assert(!save_select_prepared(scratch,sizeof(scratch),duplicate,2,prepare_once,&plan,&token));
    assert(!save_select_prepared(scratch,sizeof(scratch),zero,1,prepare_once,&plan,&token));
    assert(!save_select_prepared(scratch,0,tags,3,prepare_once,&plan,&token));
    assert(!save_select_prepared(0,sizeof(scratch),tags,3,prepare_once,&plan,&token));
    assert(!save_select_prepared(scratch,sizeof(scratch),tags,0,prepare_once,&plan,&token));
    assert(!save_select_prepared(scratch,sizeof(scratch),tags,3,0,&plan,&token));
    assert(!plan.calls && plan.live==99 && !memcmp(&token,&sentinel,sizeof(token)));
    for(int change_header=0;change_header<2;change_header++) {
        reset();record(0,tags[0],7,0xa5);token=sentinel;
        assert(!save_select_prepared(scratch,sizeof(scratch),tags,3,alter_record_after_prepare,&change_header,&token));
        assert(!memcmp(&token,&sentinel,sizeof(token)));
    }
    {u8 large[SAVE_PAYLOAD_MAX+64];reset();record(0,tags[0],7,0xa5);
     memset(large,0xcc,sizeof(large));plan=(Prepared){0,-7,99};
     assert(save_select_prepared(large,sizeof(large),tags,3,prepare_once,&plan,&token));
     assert(token.length==4 && large[0]==0x3c && large[4]==0xcc && large[sizeof(large)-1]==0xcc);}
#ifdef SC64_SHARED_ACTOR_FLIGHT_HOST_STAGE
    /* The flight candidate has six distinct historic pairs. Validate both
     * fallback and publication while preserving the six-tag bound. */
    {
        const u32 six[]={0x10203040u,0x20304050u,0x30405060u,
                         0x40506070u,0x50607080u,0x60708090u};
        const u32 seven[]={0x10203040u,0x20304050u,0x30405060u,
                           0x40506070u,0x50607080u,0x60708090u,0x708090a0u};
        const u32 duplicate_six[]={0x10203040u,0x20304050u,0x30405060u,
                                   0x40506070u,0x50607080u,0x50607080u};
        reset();record(0,six[0],10,0xa5);record(1,six[5],11,0);
        plan=(Prepared){0,-7,99};token=sentinel;
        assert(save_select_prepared(scratch,sizeof(scratch),six,6,prepare_once,&plan,&token));
        assert(token.tag==six[0] && token.slot==0 && plan.calls==2);
        const u8 next[]={0x50,0xa5,4,5};
        assert(save_write_selected(next,sizeof(next),six[4],&token,six,6));
        token=sentinel;plan=(Prepared){0,-7,99};
        assert(!save_select_prepared(scratch,sizeof(scratch),seven,7,prepare_once,&plan,&token));
        assert(!save_select_prepared(scratch,sizeof(scratch),duplicate_six,6,prepare_once,&plan,&token));
        assert(!plan.calls && !memcmp(&token,&sentinel,sizeof(token)));
    }
#endif
    puts("prepared journal selection: raw-selector oracle, mutated plans, half-range fallback and predecessor checks passed");
    return 0;
}
