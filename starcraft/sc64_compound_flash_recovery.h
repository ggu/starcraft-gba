/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_COMPOUND_FLASH_RECOVERY_H
#define SC64_COMPOUND_FLASH_RECOVERY_H
/* This marker is reserved to v3 compound writes. It is not cryptographic. */
#define SC64_CFR_MARKER_OFFSET 16u
#define SC64_CFR_MARKER_SIZE 8u
static const u8 sc64_cfr_marker[SC64_CFR_MARKER_SIZE]={0x53,0x43,0x36,0x34,0x43,0x46,0x33,0xa5};
enum {SC64_CFR_FOREIGN=-1,SC64_CFR_BLANK=0,SC64_CFR_MARKER_PARTIAL=1,
      SC64_CFR_OPEN=2,SC64_CFR_ERASE=3,SC64_CFR_COMMITTED=4};
typedef int (*Sc64CfrProof)(void *,const GbaFlashJournalIo *,unsigned);
/* One byte is programmed at a time. A cut may leave any bitwise partial
 * value in that byte; all preceding bytes are complete and later bytes FF. */
static int sc64_cfr_marker_prefix(const u8 *bytes)
{
    unsigned i=0;while(i<SC64_CFR_MARKER_SIZE && bytes[i]==sc64_cfr_marker[i])++i;
    if(i==SC64_CFR_MARKER_SIZE)return 1;
    if((bytes[i]&sc64_cfr_marker[i])!=sc64_cfr_marker[i])return 0;
    for(++i;i<SC64_CFR_MARKER_SIZE;++i)if(bytes[i]!=0xff)return 0;
    return 1;
}
static int sc64_cfr_erased_data(const GbaFlashJournalIo *io,unsigned index)
{
    u8 bytes[64];u32 base=gba_fj_base(index);
    for(u32 offset=GBA_FJ_SECTOR;offset<GBA_FJ_SLOT;offset+=sizeof bytes) {
        if(!io->read(io->context,base+offset,bytes,sizeof bytes))return -1;
        for(unsigned i=0;i<sizeof bytes;++i)if(bytes[i]!=0xff)return 0;
    }
    return 1;
}
/* Proof is checksum + complete semantic validation of the same slot, even if
 * its commit word is torn. It is called only before outgoing data is built. */
static int sc64_cfr_classify(const GbaFlashJournalIo *io,unsigned index,u32 tag,
                              Sc64CfrProof proof,void *ctx)
{
    u8 sector[64],metadata[4];u32 base=gba_fj_base(index),raw,inner;int full,blank,p;
    if(index<2 || index>3 || !io || !io->read || !proof)return SC64_CFR_FOREIGN;
    if(!io->read(io->context,base,sector,sizeof sector) ||
       !io->read(io->context,gba_fj_metadata(index),metadata,4))return SC64_CFR_FOREIGN;
    raw=gba_fj_u32(sector);inner=gba_fj_u32(metadata);
    for(unsigned i=4;i<sizeof sector;++i)
        if((i<SC64_CFR_MARKER_OFFSET || i>=SC64_CFR_MARKER_OFFSET+SC64_CFR_MARKER_SIZE) && sector[i]!=0xff)return SC64_CFR_FOREIGN;
    for(u32 off=sizeof sector;off<GBA_FJ_SECTOR;off+=sizeof sector) {
        u8 bytes[64];if(!io->read(io->context,base+off,bytes,sizeof bytes))return SC64_CFR_FOREIGN;
        for(unsigned i=0;i<sizeof bytes;++i)if(bytes[i]!=0xff)return SC64_CFR_FOREIGN;
    }
    full=!memcmp(sector+SC64_CFR_MARKER_OFFSET,sc64_cfr_marker,SC64_CFR_MARKER_SIZE);
    /* Matching committed foreign metadata is protected regardless of CRC. */
    if(raw!=0xffffffffu && raw==inner && raw!=tag)return SC64_CFR_FOREIGN;
    if(raw==tag && inner==tag) {
        p=proof(ctx,io,index);if(p!=1)return SC64_CFR_FOREIGN;
        for(unsigned i=0;i<SC64_CFR_MARKER_SIZE;++i)
            if((sector[SC64_CFR_MARKER_OFFSET+i]&sc64_cfr_marker[i])!=sc64_cfr_marker[i])return SC64_CFR_FOREIGN;
        return SC64_CFR_COMMITTED;
    }
    if(raw!=0xffffffffu) {
        if((raw&tag)!=tag || inner!=tag)return SC64_CFR_FOREIGN;
        p=proof(ctx,io,index);if(p!=1)return SC64_CFR_FOREIGN;
        for(unsigned i=0;i<SC64_CFR_MARKER_SIZE;++i)
            if((sector[SC64_CFR_MARKER_OFFSET+i]&sc64_cfr_marker[i])!=sc64_cfr_marker[i])return SC64_CFR_FOREIGN;
        return SC64_CFR_ERASE;
    }
    if(full) {
        /* A complete pending payload can be finished safely if it is the
         * only surviving copy. Incomplete data remain marker-owned. */
        p=proof(ctx,io,index);if(p<0)return SC64_CFR_FOREIGN;
        return SC64_CFR_OPEN;
    }
    blank=sc64_cfr_erased_data(io,index);if(blank<0)return SC64_CFR_FOREIGN;
    p=blank?0:proof(ctx,io,index);if(p<0)return SC64_CFR_FOREIGN;
    if(!blank && p!=1)return SC64_CFR_FOREIGN;
    if(sc64_cfr_marker_prefix(sector+SC64_CFR_MARKER_OFFSET))return SC64_CFR_MARKER_PARTIAL;
    /* A partial erase of an old verified owned marker need not form a
     * programming prefix; only 0->1 transitions from that marker are valid. */
    if(p==1) {
        for(unsigned i=0;i<SC64_CFR_MARKER_SIZE;++i)
            if((sector[SC64_CFR_MARKER_OFFSET+i]&sc64_cfr_marker[i])!=sc64_cfr_marker[i])return SC64_CFR_FOREIGN;
        return SC64_CFR_ERASE;
    }
    return SC64_CFR_FOREIGN;
}
#endif
