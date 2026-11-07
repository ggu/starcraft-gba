/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba_flash_journal.h"

typedef struct {
    uint8_t bytes[GBA_FJ_BYTES];
    int budget,mutations,fail_read,bad_commit_erase;
} Model;
static Model model;
static uint8_t snapshot[GBA_FJ_BYTES],legacy[32768],payload[GBA_FJ_PAYLOAD_MAX],tail[400];
static int read_bytes(void *context,uint32_t offset,void *out,uint32_t n)
{
    Model *m=context;
    if(m->fail_read||offset>GBA_FJ_BYTES||n>GBA_FJ_BYTES-offset)return 0;
    memcpy(out,m->bytes+offset,n);return 1;
}
static int mutation(Model *m)
{
    if(!m->budget)return 0;
    if(m->budget>0)--m->budget;
    ++m->mutations;return 1;
}
static int erase_sector(void *context,uint32_t offset)
{
    Model *m=context;
    assert(offset>=32768&&offset<98304&&!(offset&4095));
    if(!mutation(m))return 0;
    if(m->bad_commit_erase&&(offset==32768||offset==65536))return 1;
    memset(m->bytes+offset,0xff,4096);return 1;
}
static int program(void *context,uint32_t offset,const void *data,uint32_t n)
{
    Model *m=context;const uint8_t *p=data;
    assert(offset>=32768&&offset<=98304&&n<=98304-offset);
    for(uint32_t i=0;i<n;++i){
        assert((m->bytes[offset+i]&p[i])==p[i]); /* No 0->1 programming. */
        if(!mutation(m))return 0;
        m->bytes[offset+i]&=p[i];
    }
    return 1;
}
static GbaFlashJournalIo io={&model,read_bytes,erase_sector,program};
static void reset(void)
{
    memset(&model,0,sizeof model);memset(model.bytes,0xff,sizeof model.bytes);
    model.budget=-1;
}
static void legacy_record(unsigned index,uint32_t sequence)
{
    uint32_t base=index*16384;uint8_t *p=model.bytes+base;
    for(unsigned i=0;i<32;++i)p[16+i]=(uint8_t)(i+index);
    gba_fj_put32(p,0x56435a60u);gba_fj_put32(p+4,32);
    gba_fj_put32(p+8,gba_fj_hash(2166136261u,p+16,32));gba_fj_put32(p+12,sequence);
}
static int accept_payload(void *unused,const GbaFlashJournalIo *device,
                            const GbaFlashJournalRecord *record)
{
    uint8_t first;(void)unused;
    if(gba_fj_read_payload(device,record,0,&first,1)!=GBA_FJ_OK)return GBA_FJ_IO_ERROR;
    return first!=0xee;
}
static void assert_legacy_unchanged(void)
{ assert(!memcmp(model.bytes,legacy,sizeof legacy)); }
static void ordinary_and_full(void)
{
    GbaFlashJournalRecord previous,written,selected;
    reset();legacy_record(0,41);legacy_record(1,42);memcpy(legacy,model.bytes,sizeof legacy);
    assert(gba_fj_select(&io,accept_payload,0,&previous)==GBA_FJ_OK&&previous.index==1);
    assert(gba_fj_write_parts(&io,payload,16368,tail,sizeof tail,0x56465a60,accept_payload,0,&previous,&written)==GBA_FJ_OK);
    assert(written.index==2&&written.sequence==43&&written.length==16768);
    uint8_t got[400];assert(gba_fj_read_payload(&io,&written,16368,got,sizeof got)==GBA_FJ_OK);
    assert(!memcmp(got,tail,sizeof got));assert_legacy_unchanged();
    previous=written;
    assert(gba_fj_write_parts(&io,payload,sizeof payload,0,0,0x56465a60,accept_payload,0,&previous,&written)==GBA_FJ_OK);
    assert(written.index==3&&written.sequence==44&&written.length==GBA_FJ_PAYLOAD_MAX);
    assert(gba_fj_select(&io,accept_payload,0,&selected)==GBA_FJ_OK&&gba_fj_same_record(&selected,&written));
    assert_legacy_unchanged();
    for(unsigned i=98304;i<GBA_FJ_BYTES;++i)assert(model.bytes[i]==0xff);
}
static void power_cuts(void)
{
    GbaFlashJournalRecord previous,written,selected;
    uint8_t changed[256],changed_tail[400];
    for(unsigned i=0;i<sizeof changed;++i)changed[i]=payload[i]^0x55;
    for(unsigned i=0;i<sizeof changed_tail;++i)changed_tail[i]=tail[i]^0xa5;
    reset();legacy_record(0,41);legacy_record(1,42);memcpy(legacy,model.bytes,sizeof legacy);
    assert(gba_fj_write_parts(&io,payload,256,tail,sizeof tail,0x56465a60,accept_payload,0,0,&previous)==GBA_FJ_OK);
    assert(gba_fj_write_parts(&io,payload,256,tail,sizeof tail,0x56465a60,accept_payload,0,&previous,&written)==GBA_FJ_OK);
    previous=written;memcpy(snapshot,model.bytes,sizeof snapshot);
    int operations=8+16+256+(int)sizeof tail+4;
    for(int cut=0;cut<=operations;++cut){
        memcpy(model.bytes,snapshot,sizeof snapshot);model.budget=cut;model.mutations=0;
        memset(&written,0xa5,sizeof written);GbaFlashJournalRecord untouched=written;
        int status=gba_fj_write_parts(&io,changed,sizeof changed,changed_tail,sizeof changed_tail,0x56465a60,accept_payload,0,&previous,&written);
        model.budget=-1;
        assert(gba_fj_select(&io,accept_payload,0,&selected)==GBA_FJ_OK);
        if(status==GBA_FJ_OK){
            uint8_t got[256];
            assert(selected.sequence==45&&selected.index==2);
            assert(gba_fj_read_payload(&io,&selected,0,got,sizeof got)==GBA_FJ_OK);
            assert(!memcmp(got,changed,sizeof got));
        }
        else{
            assert(status==GBA_FJ_WRITE_ERROR&&!memcmp(&written,&untouched,sizeof written));
            assert(gba_fj_same_record(&selected,&previous));
        }
        assert_legacy_unchanged();
        assert(!memcmp(model.bytes+65536,snapshot+65536,GBA_FJ_SLOT));
    }
    /* Partial erase may restore arbitrary subsets of tag bits. External
     * metadata/sequence stay unchanged, so a surviving old tag stays stale. */
    for(unsigned byte=0;byte<4;++byte)for(unsigned mask=0;mask<256;++mask){
        memcpy(model.bytes,snapshot,sizeof snapshot);model.bytes[32768+byte]|=(uint8_t)mask;
        assert(gba_fj_select(&io,accept_payload,0,&selected)==GBA_FJ_OK);
        assert(gba_fj_same_record(&selected,&previous));
    }
    memcpy(model.bytes,snapshot,sizeof snapshot);model.bad_commit_erase=1;model.mutations=0;
    assert(gba_fj_write_parts(&io,payload,256,0,0,0x56465a60,accept_payload,0,&previous,&written)==GBA_FJ_WRITE_ERROR);
    assert(model.mutations==1&&!memcmp(model.bytes,snapshot,sizeof snapshot));model.bad_commit_erase=0;
    printf("power-cut boundaries %d; partial-tag erasures 1024: PASS\n",operations+1);
}
static void erased_tag_bytes(void)
{
    GbaFlashJournalRecord previous,written,selected;
    uint8_t changed[32];memset(changed,0x69,sizeof changed);
    reset();legacy_record(0,41);memcpy(legacy,model.bytes,sizeof legacy);
    assert(gba_fj_write_parts(&io,payload,64,0,0,0x56465a60,accept_payload,0,0,&previous)==GBA_FJ_OK);
    memcpy(snapshot,model.bytes,sizeof snapshot);
    /* A tag containing erased bytes can become fully valid before the last
     * redundant FF byte is visited. An interrupted call may then report an
     * error while a complete new record is already committed. Either the
     * previous record or that exact complete successor is an atomic result. */
    for(int cut=0;cut<=8+16+(int)sizeof changed+4;++cut){
        memcpy(model.bytes,snapshot,sizeof snapshot);model.budget=cut;
        memset(&written,0xa5,sizeof written);GbaFlashJournalRecord untouched=written;
        int status=gba_fj_write_parts(&io,changed,sizeof changed,0,0,0xffffff00u,accept_payload,0,&previous,&written);
        model.budget=-1;
        assert(gba_fj_select(&io,accept_payload,0,&selected)==GBA_FJ_OK);
        if(!gba_fj_same_record(&selected,&previous)){
            assert(selected.index==3&&selected.sequence==43&&selected.tag==0xffffff00u&&
                   selected.length==sizeof changed&&
                   selected.checksum==gba_fj_hash(2166136261u,changed,sizeof changed));
        }
        if(status!=GBA_FJ_OK)assert(!memcmp(&written,&untouched,sizeof written));
        assert_legacy_unchanged();
    }
}
static void stale_winner_and_semantic_fallback(void)
{
    GbaFlashJournalRecord old,current,written,selected;
    reset();legacy_record(0,41);memcpy(legacy,model.bytes,sizeof legacy);
    assert(gba_fj_select(&io,accept_payload,0,&old)==GBA_FJ_OK);
    assert(gba_fj_write_parts(&io,payload,64,0,0,0x56465a60,accept_payload,0,&old,&current)==GBA_FJ_OK);
    memcpy(snapshot,model.bytes,sizeof snapshot);model.mutations=0;
    memset(&written,0xa5,sizeof written);GbaFlashJournalRecord sentinel=written;
    assert(gba_fj_write_parts(&io,payload,64,0,0,0x56465a60,accept_payload,0,&old,&written)==GBA_FJ_STALE);
    assert(!model.mutations&&!memcmp(model.bytes,snapshot,sizeof snapshot)&&
           !memcmp(&written,&sentinel,sizeof written));
    old=current;
    assert(gba_fj_write_parts(&io,payload,64,0,0,0x56465a60,accept_payload,0,&old,&current)==GBA_FJ_OK);
    memcpy(snapshot,model.bytes,sizeof snapshot);model.mutations=0;
    assert(gba_fj_write_parts(&io,payload,64,0,0,0x56465a60,accept_payload,0,&old,&written)==GBA_FJ_STALE);
    assert(!model.mutations&&!memcmp(model.bytes,snapshot,sizeof snapshot));
    /* A checksum-valid but semantically rejected newer slot is replaceable,
     * using exactly the policy that selected the older admissible record. */
    uint32_t metadata=gba_fj_metadata(current.index);
    model.bytes[metadata+16]=0xee;
    gba_fj_put32(model.bytes+metadata+8,gba_fj_hash(2166136261u,model.bytes+metadata+16,current.length));
    assert(gba_fj_select(&io,accept_payload,0,&selected)==GBA_FJ_OK&&gba_fj_same_record(&selected,&old));
    assert(gba_fj_write_parts(&io,payload,64,0,0,0x56465a60,accept_payload,0,&selected,&written)==GBA_FJ_OK);
    assert(written.index==current.index);assert_legacy_unchanged();
    /* The no-token route must apply semantic policy too. */
    metadata=gba_fj_metadata(written.index);model.bytes[metadata+16]=0xee;
    gba_fj_put32(model.bytes+metadata+8,gba_fj_hash(2166136261u,model.bytes+metadata+16,written.length));
    assert(gba_fj_write_parts(&io,payload,64,0,0,0x56465a60,accept_payload,0,0,&written)==GBA_FJ_OK);
    assert(written.index==current.index);assert_legacy_unchanged();
}
static void fallback_and_arguments(void)
{
    GbaFlashJournalRecord previous,written,selected,sentinel;
    reset();legacy_record(0,0xffffffffu);memcpy(legacy,model.bytes,sizeof legacy);
    assert(gba_fj_write_parts(&io,payload,256,0,0,0x56465a60,accept_payload,0,0,&previous)==GBA_FJ_OK);
    assert(previous.sequence==1); /* Wrap skips zero. */
    legacy_record(0,0x80000001u); /* Ancient legacy counter cannot regain priority. */
    memcpy(legacy,model.bytes,sizeof legacy);
    assert(gba_fj_select(&io,accept_payload,0,&selected)==GBA_FJ_OK&&
           gba_fj_same_record(&selected,&previous));
    assert(gba_fj_write_parts(&io,payload,256,0,0,0x56465a60,accept_payload,0,&previous,&written)==GBA_FJ_OK);
    memcpy(snapshot,model.bytes,sizeof snapshot);
    uint32_t metadata=gba_fj_metadata(written.index);
    model.bytes[metadata+16]^=1;
    assert(gba_fj_select(&io,accept_payload,0,&selected)==GBA_FJ_OK&&gba_fj_same_record(&selected,&previous));
    memcpy(model.bytes,snapshot,sizeof snapshot);model.bytes[metadata+16]=0xee;
    gba_fj_put32(model.bytes+metadata+8,gba_fj_hash(2166136261u,model.bytes+metadata+16,written.length));
    assert(gba_fj_select(&io,accept_payload,0,&selected)==GBA_FJ_OK&&gba_fj_same_record(&selected,&previous));
    memcpy(model.bytes,snapshot,sizeof snapshot);model.mutations=0;
    GbaFlashJournalRecord stale=previous;stale.sequence++;
    assert(gba_fj_write_parts(&io,payload,16,0,0,0x56465a60,accept_payload,0,&stale,&selected)==GBA_FJ_STALE);
    assert(!model.mutations&&!memcmp(model.bytes,snapshot,sizeof snapshot));
    memset(&sentinel,0xa5,sizeof sentinel);selected=sentinel;model.fail_read=1;
    assert(gba_fj_select(&io,accept_payload,0,&selected)==GBA_FJ_IO_ERROR);
    assert(!memcmp(&selected,&sentinel,sizeof selected));model.fail_read=0;
    assert(gba_fj_write_parts(&io,payload,GBA_FJ_PAYLOAD_MAX,tail,1,0x56465a60,accept_payload,0,&previous,&selected)==GBA_FJ_ARGUMENT);
    assert(gba_fj_write_parts(&io,payload,1,0,0,0xffffffffu,accept_payload,0,&previous,&selected)==GBA_FJ_ARGUMENT);
    assert(!model.mutations&&!memcmp(model.bytes,snapshot,sizeof snapshot));assert_legacy_unchanged();
}
int main(void)
{
    for(unsigned i=0;i<sizeof payload;++i)payload[i]=(uint8_t)(i*37u+3u);
    for(unsigned i=0;i<sizeof tail;++i)tail[i]=(uint8_t)(i*13u+7u);
    ordinary_and_full();power_cuts();erased_tag_bytes();stale_winner_and_semantic_fallback();fallback_and_arguments();
    {
        GbaFlashJournalRecord written;uint8_t header[32],saved[96];
        memset(header,0x31,sizeof header);reset();
        assert(gba_fj_write_three(&io,header,sizeof header,payload,48,tail,16,
                                  0x56465a60,accept_payload,0,0,&written)==GBA_FJ_OK);
        assert(written.length==sizeof saved && gba_fj_read_payload(&io,&written,0,saved,sizeof saved)==GBA_FJ_OK);
        assert(!memcmp(saved,header,32) && !memcmp(saved+32,payload,48) && !memcmp(saved+80,tail,16));
        assert(gba_fj_write_three(&io,header,32,payload,GBA_FJ_PAYLOAD_MAX,tail,16,
                                  0x56465a60,accept_payload,0,0,&written)==GBA_FJ_ARGUMENT);
    }
    puts("isolated Flash journal model: full payload, legacy preservation and fallback PASS");
    return 0;
}
