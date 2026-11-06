/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GBA_FLASH_JOURNAL_H
#define GBA_FLASH_JOURNAL_H
#include <stdint.h>
#include <string.h>

/* Experimental transport-independent layout. It does not enable Flash in a
 * normal ROM. A backend must enforce bank bounds, erase/program completion,
 * 1->0 writes and a sticky failure after uncertain hardware completion.
 * Source spans must remain immutable RAM/ROM throughout a write.
 *
 * Keep the original 32 KiB journal read-only. Two new 32 KiB slots begin at
 * 0x8000 and 0x10000. Each reserves its first 4 KiB sector for a commit tag;
 * metadata and payload live in the remaining seven sectors. Partial erasure
 * of the commit sector therefore cannot change an old record's sequence or
 * checksum while its old tag remains readable. */
enum { GBA_FJ_BYTES=131072, GBA_FJ_SECTOR=4096, GBA_FJ_SLOT=32768,
       GBA_FJ_PAYLOAD_MAX=28656, GBA_FJ_LEGACY_PAYLOAD_MAX=16368 };
enum { GBA_FJ_IO_ERROR=-1, GBA_FJ_ARGUMENT=-2, GBA_FJ_STALE=-3,
       GBA_FJ_WRITE_ERROR=-4, GBA_FJ_NONE=0, GBA_FJ_OK=1 };
typedef struct {
    void *context;
    int (*read)(void *,uint32_t,void *,uint32_t);
    int (*erase_sector)(void *,uint32_t);
    int (*program)(void *,uint32_t,const void *,uint32_t);
} GbaFlashJournalIo;
typedef struct {
    uint32_t index,tag,length,checksum,sequence;
} GbaFlashJournalRecord;
/* A validator may read candidate bytes through this IO but must neither write
 * the device nor publish live game state. A negative result aborts selection. */
typedef int (*GbaFlashJournalAccept)(void *,const GbaFlashJournalIo *,
                                    const GbaFlashJournalRecord *);

static uint32_t gba_fj_base(uint32_t index)
{ return index<2 ? index*16384u : index==2 ? 32768u : 65536u; }
static uint32_t gba_fj_metadata(uint32_t index)
{ return gba_fj_base(index)+(index<2 ? 0u : GBA_FJ_SECTOR); }
static uint32_t gba_fj_capacity(uint32_t index)
{ return index<2 ? GBA_FJ_LEGACY_PAYLOAD_MAX : GBA_FJ_PAYLOAD_MAX; }
static uint32_t gba_fj_u32(const uint8_t *p)
{ return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static void gba_fj_put32(uint8_t *p,uint32_t value)
{ for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(i*8)); }
static uint32_t gba_fj_hash(uint32_t hash,const uint8_t *p,uint32_t length)
{ while(length--){hash^=*p++;hash*=16777619u;} return hash; }
static int gba_fj_newer(uint32_t a,uint32_t b)
{ return a!=b && a-b<0x80000000u; }

static int gba_fj_read_payload(const GbaFlashJournalIo *io,
    const GbaFlashJournalRecord *record,uint32_t offset,void *out,uint32_t length)
{
    if(!io||!io->read||!record||record->index>=4||(!out&&length)||
       record->length>gba_fj_capacity(record->index)||offset>record->length||
       length>record->length-offset)return GBA_FJ_ARGUMENT;
    if(!length)return GBA_FJ_OK;
    return io->read(io->context,gba_fj_metadata(record->index)+16+offset,out,length)
           ? GBA_FJ_OK : GBA_FJ_IO_ERROR;
}

/* On success alone, publish a fully checksummed descriptor. Invalid/torn
 * records are NONE; a transport failure is never treated as an empty slot. */
static int gba_fj_probe(const GbaFlashJournalIo *io,uint32_t index,
                         GbaFlashJournalRecord *out)
{
    uint8_t header[16],bytes[64],commit[4];
    GbaFlashJournalRecord record;
    uint32_t hash=2166136261u;
    if(!io||!io->read||!out||index>=4)return GBA_FJ_ARGUMENT;
    if(!io->read(io->context,gba_fj_metadata(index),header,16))return GBA_FJ_IO_ERROR;
    record=(GbaFlashJournalRecord){index,gba_fj_u32(header),gba_fj_u32(header+4),
                                 gba_fj_u32(header+8),gba_fj_u32(header+12)};
    if(!record.tag||record.tag==0xffffffffu||!record.length||
       record.length>gba_fj_capacity(index)||!record.sequence)return GBA_FJ_NONE;
    if(index>=2){
        if(!io->read(io->context,gba_fj_base(index),commit,4))return GBA_FJ_IO_ERROR;
        if(gba_fj_u32(commit)!=record.tag)return GBA_FJ_NONE;
    }
    for(uint32_t offset=0;offset<record.length;){
        uint32_t n=record.length-offset;if(n>sizeof bytes)n=sizeof bytes;
        if(gba_fj_read_payload(io,&record,offset,bytes,n)!=GBA_FJ_OK)return GBA_FJ_IO_ERROR;
        hash=gba_fj_hash(hash,bytes,n);offset+=n;
    }
    if(hash!=record.checksum)return GBA_FJ_NONE;
    *out=record;return GBA_FJ_OK;
}

static int gba_fj_select(const GbaFlashJournalIo *io,GbaFlashJournalAccept accept,
                          void *context,GbaFlashJournalRecord *out)
{
    GbaFlashJournalRecord best,candidate;int have=0;
    if(!io||!io->read||!out)return GBA_FJ_ARGUMENT;
    for(uint32_t index=0;index<4;++index){
        int status=gba_fj_probe(io,index,&candidate);if(status<0)return status;
        if(status!=GBA_FJ_OK)continue;
        if(accept){status=accept(context,io,&candidate);if(status<0)return status;if(!status)continue;}
        /* This is one-way migration. Once an admissible new-layout record
         * exists, its epoch precedes comparisons against permanently retained
         * legacy counters; ancient counters must not win after sequence wrap. */
        int newer_epoch=have&&candidate.index>=2&&best.index<2;
        int same_epoch=!have||((candidate.index>=2)==(best.index>=2));
        if(!have||newer_epoch||(same_epoch&&
           (gba_fj_newer(candidate.sequence,best.sequence)||
            (candidate.sequence==best.sequence&&candidate.index>best.index)))){
            best=candidate;have=1;
        }
    }
    if(!have)return GBA_FJ_NONE;
    *out=best;return GBA_FJ_OK;
}

/* Recheck the selected record immediately before the first mutation. This
 * descriptor is a consistency token, not proof of a historical payment. */
static int gba_fj_same_record(const GbaFlashJournalRecord *a,
                              const GbaFlashJournalRecord *b)
{
    return a->index==b->index&&a->tag==b->tag&&a->length==b->length&&
           a->checksum==b->checksum&&a->sequence==b->sequence;
}
static int gba_fj_write_parts(const GbaFlashJournalIo *io,
    const void *data,uint32_t length,const void *tail,uint32_t tail_length,
    uint32_t tag,GbaFlashJournalAccept accept,void *context,
    const GbaFlashJournalRecord *previous,GbaFlashJournalRecord *out)
{
    GbaFlashJournalRecord selected,check,written;
    uint8_t header[16],commit[4],bytes[64];
    uint32_t sequence=1,index=2,hash=2166136261u,verify=2166136261u;
    int status;
    if(!io||!io->read||!io->erase_sector||!io->program||!data||!length||
       (!tail&&tail_length)||length>GBA_FJ_PAYLOAD_MAX||
       tail_length>GBA_FJ_PAYLOAD_MAX-length||!tag||tag==0xffffffffu||!out)
        return GBA_FJ_ARGUMENT;
    /* The same semantic policy used by the caller's selection is required.
     * An intact old slot is not a current consistency token after a later
     * successful write. Preserve the selected winner before choosing a target. */
    status=gba_fj_select(io,accept,context,&selected);
    if(status<0)return status;
    if(previous&&(status!=GBA_FJ_OK||!gba_fj_same_record(previous,&selected)))
        return GBA_FJ_STALE;
    if(previous||status==GBA_FJ_OK){
        sequence=selected.sequence+1u;if(!sequence)sequence=1;
        index=selected.index==2 ? 3u : 2u;
    }
    hash=gba_fj_hash(hash,data,length);
    if(tail_length)hash=gba_fj_hash(hash,tail,tail_length);
    written=(GbaFlashJournalRecord){index,tag,length+tail_length,hash,sequence};
    gba_fj_put32(header,tag);gba_fj_put32(header+4,written.length);
    gba_fj_put32(header+8,hash);gba_fj_put32(header+12,sequence);
    /* Erase the isolated commit sector before touching any old metadata. */
    uint32_t base=gba_fj_base(index),meta=gba_fj_metadata(index);
    if(!io->erase_sector(io->context,base))return GBA_FJ_WRITE_ERROR;
    for(uint32_t offset=0;offset<GBA_FJ_SECTOR;offset+=sizeof bytes){
        if(!io->read(io->context,base+offset,bytes,sizeof bytes))return GBA_FJ_IO_ERROR;
        for(unsigned i=0;i<sizeof bytes;++i)
            if(bytes[i]!=0xff)return GBA_FJ_WRITE_ERROR;
    }
    for(uint32_t sector=GBA_FJ_SECTOR;sector<GBA_FJ_SLOT;sector+=GBA_FJ_SECTOR)
        if(!io->erase_sector(io->context,base+sector))return GBA_FJ_WRITE_ERROR;
    if(!io->program(io->context,meta,header,sizeof header)||
       !io->program(io->context,meta+16,data,length)||
       (tail_length&&!io->program(io->context,meta+16+length,tail,tail_length)))
        return GBA_FJ_WRITE_ERROR;
    if(!io->read(io->context,meta,bytes,sizeof header))return GBA_FJ_IO_ERROR;
    if(memcmp(bytes,header,sizeof header))return GBA_FJ_WRITE_ERROR;
    for(uint32_t offset=0;offset<written.length;){
        uint32_t n=written.length-offset;if(n>sizeof bytes)n=sizeof bytes;
        if(gba_fj_read_payload(io,&written,offset,bytes,n)!=GBA_FJ_OK)return GBA_FJ_IO_ERROR;
        verify=gba_fj_hash(verify,bytes,n);offset+=n;
    }
    if(verify!=hash)return GBA_FJ_WRITE_ERROR;
    gba_fj_put32(commit,tag);
    if(!io->program(io->context,base,commit,4))return GBA_FJ_WRITE_ERROR;
    status=gba_fj_probe(io,index,&check);
    if(status<0)return status;
    if(status!=GBA_FJ_OK||!gba_fj_same_record(&written,&check))return GBA_FJ_WRITE_ERROR;
    *out=written;return GBA_FJ_OK;
}
#endif
