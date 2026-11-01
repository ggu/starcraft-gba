/* SPDX-License-Identifier: GPL-3.0-or-later
 * In-process split-record journal oracle; no game assets or guest injection. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"

static const u32 tags[] = {0x10203040u, 0x30405060u};
static u8 payload[SAVE_PAYLOAD_MAX], scratch[SAVE_PAYLOAD_MAX];
static u8 expected[32768], original[32768], committed[SAVE_SLOT_SIZE];

static void reset(void) { memset(host_save,0,sizeof(host_save)); }
static int validate(const void *data,u32 len,u32 tag,void *context) {
    (void)tag; (void)context;
    return len>=2 && ((const u8*)data)[0]==0xa5;
}
static void seed(int slot,u32 tag,u32 sequence,int acceptable) {
    u8 bytes[]={acceptable?0xa5:0,0x12,0x34,0x56};
    volatile u8 *p=host_save+(unsigned)slot*SAVE_SLOT_SIZE;
    sram_put32(p,0); sram_put32(p+4,sizeof(bytes));
    sram_put32(p+8,hash_bytes(bytes,sizeof(bytes))); sram_put32(p+12,sequence);
    for(unsigned i=0;i<sizeof(bytes);++i)p[16+i]=bytes[i];
    sram_put32(p,tag);
}
static GbaSaveSelection selected(void) {
    GbaSaveSelection result={0};
    assert(save_select_validated(scratch,sizeof(scratch),tags,2,validate,0,&result));
    return result;
}
static void compare_fresh(u32 length,u32 split) {
    reset(); assert(save_write_first(payload,length,tags[1]));
    memcpy(expected,host_save,sizeof(expected));
    reset(); assert(save_write_parts_first(payload,split,payload+split,length-split,tags[1]));
    assert(!memcmp(expected,host_save,sizeof(expected)));
    GbaSaveSelection result=selected();
    assert(result.slot==0 && result.sequence==1 && result.length==length);
    assert(!memcmp(scratch,payload,length));
}
static void compare_selected(u32 length,u32 split,int slot,u32 sequence) {
    reset(); seed(slot,tags[0],sequence,1);
    /* The selected semantic predecessor need not be the checksum-newest one. */
    seed(1-slot,tags[1],sequence+3u?sequence+3u:1u,0);
    GbaSaveSelection prior=selected(); assert(prior.slot==slot);
    memcpy(original,host_save,sizeof(original));
    assert(save_write_selected(payload,length,tags[1],&prior,tags,2));
    memcpy(expected,host_save,sizeof(expected));
    memcpy(host_save,original,sizeof(original));
    assert(save_write_parts_selected(payload,split,payload+split,length-split,
                                      tags[1],&prior,tags,2));
    assert(!memcmp(expected,host_save,sizeof(expected)));
    assert(!memcmp(original+(unsigned)slot*SAVE_SLOT_SIZE,
                   host_save+(unsigned)slot*SAVE_SLOT_SIZE,SAVE_SLOT_SIZE));
    GbaSaveSelection next=selected();
    assert(next.slot==1-slot && next.sequence==(sequence==0xffffffffu?1:sequence+1u));
    assert(next.length==length && !memcmp(scratch,payload,length));
}
static void reject(const void *data,u32 len,const void *tail,u32 tail_len,u32 tag,
                   const GbaSaveSelection *prior,const u32 *allowed,unsigned count) {
    memcpy(original,host_save,sizeof(original));
    assert(!save_write_parts_selected(data,len,tail,tail_len,tag,prior,allowed,count));
    assert(!memcmp(original,host_save,sizeof(original)));
}
static void rejected_requests(void) {
    reset(); seed(0,tags[0],7,1); GbaSaveSelection prior=selected();
    reject(0,1,payload,1,tags[1],&prior,tags,2);
    reject(payload,1,0,1,tags[1],&prior,tags,2);
    reject(payload,0,payload,1,tags[1],&prior,tags,2);
    reject(payload,1,payload,0,tags[1],&prior,tags,2);
    reject(payload,SAVE_PAYLOAD_MAX,payload,1,tags[1],&prior,tags,2);
    reject(payload,0xffffffffu,payload,1,tags[1],&prior,tags,2);
    reject(payload,1,payload,0xffffffffu,tags[1],&prior,tags,2);
    reject(payload,1,payload,1,0,&prior,tags,2);
    reject(payload,1,payload,1,tags[1],0,tags,2);
    reject(payload,1,payload,1,tags[1],&prior,0,2);
    reject(payload,1,payload,1,tags[1],&prior,tags,0);
    reject(payload,1,payload,1,tags[1],&prior,tags,4);
    const u32 duplicate[]={tags[0],tags[1],tags[0]},zero[]={tags[0],tags[1],0};
    reject(payload,1,payload,1,tags[1],&prior,duplicate,3);
    reject(payload,1,payload,1,tags[1],&prior,zero,3);
    for(int field=0;field<5;++field) {
        GbaSaveSelection stale=prior;
        if(field==0)stale.slot=2;
        if(field==1)stale.tag=tags[1];
        if(field==2)++stale.length;
        if(field==3)++stale.sequence;
        if(field==4)++stale.checksum;
        reject(payload,1,payload,1,tags[1],&stale,tags,2);
    }
    seed(1,0x55667788u,9,1);
    reject(payload,1,payload,1,tags[1],&prior,tags,2);
    host_save[SAVE_SLOT_SIZE+16]^=1;
    assert(save_write_parts_selected(payload,1,payload+1,3,tags[1],&prior,tags,2));
    memcpy(original,host_save,sizeof(original));
    assert(!save_write_parts_first(payload,1,payload+1,3,tags[1]));
    assert(!memcmp(original,host_save,sizeof(original)));
    reset(); seed(1,0x55667788u,1,1); memcpy(original,host_save,sizeof(original));
    assert(!save_write_parts_first(payload,1,payload+1,3,tags[1]));
    assert(!memcmp(original,host_save,sizeof(original)));
}
static void fresh_bounds(void) {
    reset(); memcpy(original,host_save,sizeof(original));
    assert(!save_write_parts_first(0,1,payload,1,tags[1]));
    assert(!save_write_parts_first(payload,1,0,1,tags[1]));
    assert(!save_write_parts_first(payload,0,payload,1,tags[1]));
    assert(!save_write_parts_first(payload,1,payload,0,tags[1]));
    assert(!save_write_parts_first(payload,SAVE_PAYLOAD_MAX,payload,1,tags[1]));
    assert(!save_write_parts_first(payload,0xffffffffu,payload,1,tags[1]));
    assert(!save_write_parts_first(payload,1,payload,0xffffffffu,tags[1]));
    assert(!save_write_parts_first(payload,1,payload,1,0));
    assert(!memcmp(original,host_save,sizeof(original)));
}
static void interrupted_record_model(void) {
    const u32 length=33,split=17,cuts[]={0,16,17,18,32,33};
    reset(); seed(0,tags[0],7,1); GbaSaveSelection prior=selected();
    memcpy(original,host_save,sizeof(original));
    assert(save_write_parts_selected(payload,split,payload+split,length-split,
                                      tags[1],&prior,tags,2));
    memcpy(committed,host_save+SAVE_SLOT_SIZE,sizeof(committed));
    for(unsigned i=0;i<sizeof(cuts)/sizeof(cuts[0]);++i) {
        memcpy(host_save,original,sizeof(original));
        /* Model every relevant side of the span boundary before the final
         * volatile tag commit; the old slot must remain selectable. */
        memcpy(host_save+SAVE_SLOT_SIZE+4,committed+4,12);
        memcpy(host_save+SAVE_SLOT_SIZE+16,committed+16,cuts[i]);
        assert(selected().slot==0);
        assert(!memcmp(original,host_save,SAVE_SLOT_SIZE));
        if(cuts[i]==length) {
            memcpy(host_save+SAVE_SLOT_SIZE,committed,4);
            assert(selected().slot==1);
        }
    }
}
int main(void) {
    gba_init();
    for(unsigned i=0;i<sizeof(payload);++i)payload[i]=(u8)(i*73u+i/7u);
    payload[0]=0xa5;
    const u32 lengths[]={2,4,17,137,SAVE_PAYLOAD_MAX};
    for(unsigned i=0;i<sizeof(lengths)/sizeof(lengths[0]);++i) {
        u32 n=lengths[i],splits[]={1,n/2,n-1};
        for(unsigned j=0;j<3;++j) {
            compare_fresh(n,splits[j]);
            for(int slot=0;slot<2;++slot) {
                compare_selected(n,splits[j],slot,7);
                compare_selected(n,splits[j],slot,0xffffffffu);
            }
        }
    }
    rejected_requests(); fresh_bounds(); interrupted_record_model();
    puts("split selected/fresh journal parity, bounds, predecessor and tag-commit model passed");
    return 0;
}
