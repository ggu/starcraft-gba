/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Detached, non-installing v36 row witness for the existing Z7 rules. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../starcraft/game.c"
#include "../../starcraft/sc64_zp_unit_save.h"
#include "../../starcraft/sc64_compact_building_save.h"
#include "../../starcraft/sc64_z7_legacy_envelope_probe.h"
#include "../../starcraft/sc64_z7_legacy_rows_probe.h"
static _Alignas(Unit) unsigned char old_slot[SC64_Z7_V36_SLOT_BYTES];
static unsigned char old_copy[SC64_Z7_V36_SLOT_BYTES], forged_copy[SC64_Z7_V36_SLOT_BYTES];
static int z7_map(const void *map) { return race_z7_save_map_valid((const Sc64RaceMap *)map); }
static void set_unit_wire_type(unsigned slot,unsigned type)
{
    unsigned start=16u+4u+slot*SC64_ZP_PACKED_UNIT_SAVE_BYTES;
    for(unsigned bit=0;bit<6;++bit) {
        unsigned wire_bit=60u+bit,at=start+(wire_bit>>3),mask=1u<<(wire_bit&7u);
        old_slot[at]=(uint8_t)((old_slot[at]&~mask)|(((type>>bit)&1u)?mask:0u));
    }
}
static void reseal(void)
{
    uint32_t length=sc64_z7_v36_read32(old_slot+4), hash=UINT32_C(2166136261);
    for(unsigned i=0;i<length;++i) hash=(hash^old_slot[16+i])*UINT32_C(16777619);
    for(unsigned i=0;i<4;++i) old_slot[8+i]=(uint8_t)(hash>>(8*i));
}
int main(int argc,char **argv)
{
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fread(old_slot,1,sizeof(old_slot),f)==sizeof(old_slot));assert(!fclose(f));
    memcpy(old_copy,old_slot,sizeof(old_slot));
    const Sc64RaceMap *map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];
    const Sc64Z7LegacyMapAuthority authority={map,z7_map};Sc64Z7LegacyEnvelopeProbe meta;
    assert(sc64_z7_v36_probe_context(old_slot,sizeof(old_slot),map,&authority,&meta));
    unsigned alive=0,eggs=0,encode_units=0,encode_buildings=0,weapon_nonzero=0;
    int sample=-1,building_sample=-1;Unit original={0};Building original_building={0};
    unsigned dead_slots[12],dead_count=0;
    for(unsigned i=0;i<MAX_UNITS;i++) {
        Sc64ZpPackedUnitSave row;Unit q;Sc64DerivedUnitSave encoded;
        memcpy(&row,old_slot+16+4+i*sizeof(row),sizeof(row));
        assert(sc64_z7_v36_probe_unit(&meta,map,&authority,i,&q));
        Unit direct;assert(sc64_zp_unit_save_decode(&direct,&row,2048,2048));
        assert(!memcmp(&q,&direct,sizeof(q)));
        if(q.alive) {++alive;if(q.weapon)++weapon_nonzero;if(q.type==U_Z7_EGG)++eggs;
            if(sample<0 && q.raw_owner>=0 && q.raw_type>=0 && q.raw_type!=36){sample=(int)i;original=q;}}
        else if(dead_count<12) dead_slots[dead_count++]=i;
        if(sc64_derived_unit_encode_for(&encoded,&q,2048,2048,0))++encode_units;
    }
    for(unsigned i=0;i<MAX_BUILDINGS;i++) {
        Sc64PackedBuildingSave row;Building q;Sc64DerivedBuildingSave encoded;
        memcpy(&row,old_slot+16+8404+i*sizeof(row),sizeof(row));
        assert(sc64_z7_v36_probe_building(&meta,map,&authority,i,&q));
        Building direct;assert(sc64_building_save_decode(&direct,&row));
        assert(!memcmp(&q,&direct,sizeof(q)));
        if(building_sample<0 && q.alive && q.raw_owner>=0 && q.raw_type>=0)
            {building_sample=(int)i;original_building=q;}
        if(sc64_derived_building_encode(&encoded,&q,2048,2048))++encode_buildings;
    }
    assert(sample>=0 && building_sample>=0 && dead_count==12);
    assert(!sc64_z7_v36_probe_unit(&meta,map,&authority,(unsigned)sample,(Unit *)(void *)old_slot));
    assert(!memcmp(old_slot,old_copy,sizeof(old_slot)));
    assert(sc64_z7_v36_probe_transport(&meta,map,&authority));
    Sc64Z7LegacyCommitmentProbe baseline_commitments,one_commitments,many_commitments;
    assert(sc64_z7_v36_probe_commitments(&meta,map,&authority,&baseline_commitments));
    /* Change an actual row, recompute the valid outer checksum and establish
     * that the detached source-identity check rejects without output writes. */
    for(int which=0;which<2;++which) {
        Unit bad=original,unchanged,output;Sc64ZpPackedUnitSave wire;
        if(which==0) bad.raw_type=85; else bad.raw_owner=8;
        assert(sc64_zp_unit_save_encode(&wire,&bad,2048,2048));
        memcpy(old_slot+16+4+(unsigned)sample*sizeof(wire),&wire,sizeof(wire));
        memset(&output,0xa5,sizeof(output));unchanged=output;
        assert(!sc64_z7_v36_probe_unit(&meta,map,&authority,(unsigned)sample,&output));
        assert(!memcmp(&output,&unchanged,sizeof(output))); /* stale checksum */
        reseal();
        assert(sc64_z7_v36_probe_context(old_slot,sizeof(old_slot),map,&authority,&meta));
        memset(&output,0xa5,sizeof(output));unchanged=output;memcpy(forged_copy,old_slot,sizeof(old_slot));
        assert(!sc64_z7_v36_probe_unit(&meta,map,&authority,(unsigned)sample,&output));
        assert(!memcmp(&output,&unchanged,sizeof(output)) && !memcmp(old_slot,forged_copy,sizeof(old_slot)));
        memcpy(old_slot,old_copy,sizeof(old_slot));
    }
    /* No outer-tag rewrite may turn a legacy row into an appended staged actor. */
    for(unsigned kind=0;kind<2;++kind) {
        Unit output,unchanged;set_unit_wire_type((unsigned)sample,kind?U_GUARDIAN_COCOON:U_SCARAB);reseal();
        assert(sc64_z7_v36_probe_context(old_slot,sizeof(old_slot),map,&authority,&meta));
        memset(&output,0xa5,sizeof(output));unchanged=output;memcpy(forged_copy,old_slot,sizeof(old_slot));
        assert(!sc64_z7_v36_probe_unit(&meta,map,&authority,(unsigned)sample,&output));
        assert(!memcmp(&output,&unchanged,sizeof(output)) && !memcmp(old_slot,forged_copy,sizeof(old_slot)));
        memcpy(old_slot,old_copy,sizeof(old_slot));
    }
    /* Source-owner forgery on a real retained building is also rejected. */
    { Building bad=original_building,output,unchanged;Sc64PackedBuildingSave wire;
      bad.raw_owner=8;assert(sc64_building_save_encode(&wire,&bad,2048,2048));
      memcpy(old_slot+16+8404+(unsigned)building_sample*sizeof(wire),&wire,sizeof(wire));reseal();
      assert(sc64_z7_v36_probe_context(old_slot,sizeof(old_slot),map,&authority,&meta));
      memset(&output,0xa5,sizeof(output));unchanged=output;
      assert(!sc64_z7_v36_probe_building(&meta,map,&authority,(unsigned)building_sample,&output));
      assert(!memcmp(&output,&unchanged,sizeof(output)));
      memcpy(old_slot,old_copy,sizeof(old_slot)); }
    /* Paid pairs consume one Egg slot and reserve one additional body. This
     * is a synthetic census, not proof that these jobs were actually paid. */
    { Unit egg=original;Sc64ZpPackedUnitSave wire;
      egg.alive=1;egg.selected=0;egg.owner=SIDE;egg.type=U_Z7_EGG;egg.raw_type=36;egg.raw_owner=3;
      egg.hp=200;egg.maxhp=200;egg.armor=10;egg.shield=0;egg.shield_remainder=0;
      egg.carry=0;egg.cd=0;egg.order=ORDER_IDLE;egg.build=-1;egg.target=U_ZERGLING;
      egg.tx=egg.x;egg.ty=egg.y;egg.path_timer=0;egg.path_retry=0;egg.flags=0;egg.weapon=0;egg.work_timer=1;
      assert(sc64_zp_unit_save_encode(&wire,&egg,2048,2048));
      for(unsigned i=0;i<12;++i) {
          memcpy(old_slot+16+4+dead_slots[i]*sizeof(wire),&wire,sizeof(wire));
          if(i==0 || i==11) {
              reseal();assert(sc64_z7_v36_probe_context(old_slot,sizeof(old_slot),map,&authority,&meta));
              if(i==0) assert(sc64_z7_v36_probe_commitments(&meta,map,&authority,&one_commitments));
              else assert(sc64_z7_v36_probe_commitments(&meta,map,&authority,&many_commitments));
          }
      }
      assert(one_commitments.live==baseline_commitments.live+1 &&
             one_commitments.egg_extra==baseline_commitments.egg_extra+1 &&
             one_commitments.free_slots+1==baseline_commitments.free_slots);
      assert(!baseline_commitments.overcommitted && many_commitments.overcommitted);
      memcpy(old_slot,old_copy,sizeof(old_slot)); }
    /* Valid metadata does not excuse a malformed pending transport job. */
    old_slot[16+16312-16]=1;reseal();
    assert(sc64_z7_v36_probe_context(old_slot,sizeof(old_slot),map,&authority,&meta));
    assert(!sc64_z7_v36_probe_transport(&meta,map,&authority));
    memcpy(old_slot,old_copy,sizeof(old_slot));
    assert(sc64_z7_v36_probe_context(old_slot,sizeof(old_slot),map,&authority,&meta));
    /* A legacy Overlord/raw36 remains an Overlord through the v36 wire codec.
     * Z7's Egg-specific source policy rejects it in this mission; this must
     * not be generalized into custom-game retagging. */
    Unit over=original,decoded;Sc64ZpPackedUnitSave over_wire;
    over.type=U_OVERLORD;over.raw_type=36;
    assert(sc64_zp_unit_save_encode(&over_wire,&over,2048,2048));
    assert(sc64_zp_unit_save_decode(&decoded,&over_wire,2048,2048));
    assert(decoded.type==U_OVERLORD && decoded.raw_type==36);
    assert(!race_z7_save_egg_valid(map,&decoded,SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    memcpy(old_slot+16+4+(unsigned)sample*sizeof(over_wire),&over_wire,sizeof(over_wire));reseal();
    assert(sc64_z7_v36_probe_context(old_slot,sizeof(old_slot),map,&authority,&meta));
    assert(!sc64_z7_v36_probe_unit(&meta,map,&authority,(unsigned)sample,&decoded));
    memcpy(old_slot,old_copy,sizeof(old_slot));
    assert(!memcmp(old_slot,old_copy,sizeof(old_slot)));
    printf("decoded units=%d alive=%u eggs=%u buildings=%d; derived-encodable units=%u buildings=%u; live weapon nonzero=%u\n",
           MAX_UNITS,alive,eggs,MAX_BUILDINGS,encode_units,encode_buildings,weapon_nonzero);
    printf("commitments: live=%u free=%u queued=%u egg_extra=%u scripted=%u; synthetic overcommit reported=%d\n",
           baseline_commitments.live,baseline_commitments.free_slots,baseline_commitments.queued,
           baseline_commitments.egg_extra,baseline_commitments.scripted,many_commitments.overcommitted);
    puts("detached row and reservation census only; full world unvalidated");
    return 0;
}
