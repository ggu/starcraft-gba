/* SPDX-License-Identifier: GPL-3.0-or-later
 * Experimental HOST Z10-only compound Flash integration. */
#ifndef SC64_COMPOUND_FLASH_HOST_ADAPTER_H
#define SC64_COMPOUND_FLASH_HOST_ADAPTER_H
#include "sc64_compound_flash_envelope.h"
_Static_assert(MAX_UNITS==240 && MAX_BUILDINGS==160,"compound Z10 actor shape");
#define SC64_CF_SIDE (2u*(MAX_UNITS+MAX_BUILDINGS)+MAX_UNITS+4u*MAX_UNITS)
#define SC64_CF_FLIGHT_OFFSET (2u*(MAX_UNITS+MAX_BUILDINGS)+MAX_UNITS)
typedef struct {GbaFlashJournalRecord records[4];u8 seen[4],accepted[4];RaceLoadPlan *plan;} Sc64CfSnapshot;
static Sc64CfSnapshot sc64_cf_snapshot;
static GbaFlashJournalRecord sc64_cf_previous;
static int sc64_cf_has_previous;
#ifdef SC64_CF_TEST_DIAGNOSTICS
static int sc64_cf_test_stage;
#define SC64_CF_TEST_STAGE(n) (sc64_cf_test_stage=(n))
#else
#define SC64_CF_TEST_STAGE(n) ((void)0)
#endif
static Sc64CompoundFlashHeader sc64_cf_expected(u32 base)
{ return (Sc64CompoundFlashHeader){1,0,SC64_Z10_GLOBAL_ID,SC64_CF_FEATURES,base,1,MAX_UNITS+MAX_BUILDINGS,MAX_UNITS+MAX_BUILDINGS,MAX_UNITS,MAX_UNITS*4u}; }
/* Until genuine predecessors have a qualified decoder, require an entirely
 * blank legacy area. Never reinterpret a historical or incomplete record. */
static int sc64_cf_ownership(const GbaFlashJournalIo *io)
{
    u8 bytes[64];
    if(!io || !io->read)return 0;
    for(u32 p=0;p<32768;p+=sizeof bytes) {
        if(!io->read(io->context,p,bytes,sizeof bytes))return 0;
        for(unsigned j=0;j<sizeof bytes;++j)if(bytes[j]!=0xff)return 0;
    }
    for(unsigned i=2;i<4;++i) {
        u8 commit[4],metadata[16],header[SC64_CF_HEADER];
        if(!io->read(io->context,gba_fj_base(i),commit,4) ||
           !io->read(io->context,gba_fj_metadata(i),metadata,sizeof metadata))return 0;
        u32 a=gba_fj_u32(commit),b=gba_fj_u32(metadata);
        if((a!=0xffffffffu && a!=SC64_COMPOUND_FLASH_TAG) ||
           (b!=0xffffffffu && b!=SC64_COMPOUND_FLASH_TAG))return 0;
        /* Check raw ownership even when probe will filter a bad CRC or
         * length. Unknown/v1 tuples must never be erased as empty slots. */
        if(a==SC64_COMPOUND_FLASH_TAG && b!=SC64_COMPOUND_FLASH_TAG)return 0;
        if(b==SC64_COMPOUND_FLASH_TAG) {
            Sc64CompoundFlashHeader parsed,expected=sc64_cf_expected(0);
            if(!io->read(io->context,gba_fj_metadata(i)+16,header,sizeof header) ||
               !sc64_cf_parse(header,gba_fj_u32(metadata+4),&expected,&parsed))return 0;
        }
    }
    return 1;
}
static int sc64_cf_town(const Sc64ZlMission *m,unsigned controller)
{
    int found=-1;
    for(int i=0;i<SC64_ZL_AI_TOWNS;++i) if(m->town_slot[i]==controller) {
        if(found>=0)return -1;
        found=i;
    }
    return found;
}
static int sc64_cf_validate(RaceLoadPlan *p)
{
    const Sc64ZlMission *m=race_zl_mission_for(SC64_Z10_GLOBAL_ID);
    const Sc64StageSaveAuth *guard_auth=&p->compound_auth;
    if(!m || race_data.state[0]!=0 || race_data.state[1]!=R_ZERG ||
       race_data.state[2]!=SC64_Z10_GLOBAL_ID || m->global_id!=SC64_Z10_GLOBAL_ID)return 0;
    for(unsigned i=0;i<MAX_UNITS;++i) {
        Unit q;const Sc64StockUnitProfile *stock;int raw;u8 c=p->controller_units[i],payer=p->scarab_payers[i];
        if(!SC64_GUARD_DECODE(&race_data,i,2048,2048,&q))return 0;
        if(!q.alive) {if(c!=255 || payer || p->q8_debt[i])return 0;continue;}
        if(q.hp<=0 || q.maxhp<=0 || q.hp>q.maxhp)return 0;
        if(c!=255 && (c>=8 || m->live_owner[c]!=q.owner))return 0;
        if(q.owner==SIDE && c!=SC64_ZL_HUMAN_SLOT)return 0;
        raw=q.raw_type>=0?q.raw_type:sc64_race_raw_unit(q.type);
        stock=sc64_stock_unit_profile(raw);if(!stock)return 0;
        if(stock->shields && (q.shield<0 || (unsigned)q.shield>stock->shields ||
           q.shield_remainder>=1280 || ((unsigned)q.shield==stock->shields && q.shield_remainder)))return 0;
        if(raw==83) {
            Sc64ScarabParentFields f;int child_count=0;
            if(q.owner!=ENEMY || c>=8 || sc64_cf_town(m,c)<0 ||
               !sc64_scarab_parent_fields_unpack(q.carry,q.work_timer,&f))return 0;
            for(unsigned j=0;j<MAX_UNITS;++j) {
                Unit child;Sc64ScarabChildFields fields;
                if(!SC64_GUARD_DECODE(&race_data,j,2048,2048,&child))return 0;
                if(!child.alive || child.type!=U_SCARAB)continue;
                if(!race_scarab_save_child_fields(&child,&fields))return 0;
                if(fields.parent_slot!=i)continue;
                if(p->controller_units[j]!=c)return 0;
                ++child_count;
            }
            if(payer && payer!=(u8)(c+6))return 0;
            if((f.queued || child_count) && payer!=(u8)(c+6))return 0;
        } else if(payer)return 0;
        if(q.type==U_SCARAB) {
            Sc64ScarabChildFields fields;
            if(!race_scarab_save_child_fields(&q,&fields) || fields.parent_slot>=MAX_UNITS || c>=8)return 0;
        }
    }
    for(unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building q;const Sc64StockUnitProfile *stock;int raw;u8 c=p->controller_buildings[i];
        if(!race_save_decode_building(&race_data,i,&q))return 0;
        if(!q.alive) {if(c!=255 || p->q8_debt[MAX_UNITS+i])return 0;continue;}
        if(q.hp<=0 || q.maxhp<=0 || q.hp>q.maxhp ||
           (c!=255 && (c>=8 || m->live_owner[c]!=q.owner)) ||
           (q.owner==SIDE && c!=SC64_ZL_HUMAN_SLOT))return 0;
        raw=q.raw_type>=0?q.raw_type:sc64_race_raw_building(q.type);
        stock=sc64_stock_unit_profile(raw);if(!stock)return 0;
        if(race_building_can_burn(&q) && p->q8_debt[MAX_UNITS+i])return 0;
        if(stock->shields && (q.shield<0 || (unsigned)q.shield>stock->shields ||
           q.shield_remainder<0 || q.shield_remainder>=1280 ||
           ((unsigned)q.shield==stock->shields && q.shield_remainder)))return 0;
    }
    return 1;
}
static int sc64_cf_prepare(RaceLoadPlan *p,const GbaFlashJournalIo *io,const GbaFlashJournalRecord *r)
{
    u8 header[SC64_CF_HEADER],flight[MAX_UNITS*4u];Sc64CompoundFlashHeader got,expected=sc64_cf_expected(0);u32 at;
    if(r->index<2 || r->tag!=SC64_COMPOUND_FLASH_TAG)return 0;
    if(gba_fj_read_payload(io,r,0,header,sizeof header)!=GBA_FJ_OK)return -1;
    if(!sc64_cf_parse(header,r->length,&expected,&got) || got.base_length>sizeof(race_impact_record))return 0;
    at=SC64_CF_HEADER+got.base_length+got.debts+got.controllers+got.payers;
    if(gba_fj_read_payload(io,r,at,flight,sizeof flight)!=GBA_FJ_OK)return -1;
    if(gba_fj_read_payload(io,r,SC64_CF_HEADER,&race_impact_record,got.base_length)!=GBA_FJ_OK)return -1;
    Sc64CfPrepareContext context={p,flight};
    if(!race_worker_cargo_prepare_selected(&race_impact_record,got.base_length,SC64_COMPOUND_FLASH_TAG,&context))return 0;
    at=SC64_CF_HEADER+got.base_length;
    if(gba_fj_read_payload(io,r,at,p->q8_debt,sizeof p->q8_debt)!=GBA_FJ_OK)return -1;
    at+=sizeof p->q8_debt;
    if(gba_fj_read_payload(io,r,at,p->controller_units,sizeof p->controller_units)!=GBA_FJ_OK)return -1;
    at+=sizeof p->controller_units;
    if(gba_fj_read_payload(io,r,at,p->controller_buildings,sizeof p->controller_buildings)!=GBA_FJ_OK)return -1;
    at+=sizeof p->controller_buildings;
    if(gba_fj_read_payload(io,r,at,p->scarab_payers,sizeof p->scarab_payers)!=GBA_FJ_OK)return -1;
    if(!sc64_cf_validate(p))return 0;
    GbaFlashJournalRecord checked;int status=gba_fj_probe(io,r->index,&checked);
    if(status<0)return -1;
    return status==GBA_FJ_OK && gba_fj_same_record(r,&checked);
}
static int sc64_cf_capture(void *ctx,const GbaFlashJournalIo *io,const GbaFlashJournalRecord *r)
{
    Sc64CfSnapshot *s=ctx;int ok;
    if(!s || r->index>=4 || s->seen[r->index])return -1;
    if(r->index<2 || r->tag!=SC64_COMPOUND_FLASH_TAG)return 0;
    ok=sc64_cf_prepare(s->plan,io,r);if(ok<0)return -1;
    s->seen[r->index]=1;s->records[r->index]=*r;s->accepted[r->index]=(u8)ok;return ok;
}
static int sc64_cf_reselect(void *ctx,const GbaFlashJournalIo *io,const GbaFlashJournalRecord *r)
{
    Sc64CfSnapshot *s=ctx;(void)io;
    if(!s || r->index>=4 || !s->seen[r->index] || !gba_fj_same_record(&s->records[r->index],r))return -1;
    return s->accepted[r->index];
}
static int race_q8_flash_begin(void)
{
    SC64_CF_TEST_STAGE(1);
    const GbaFlashJournalIo *io=sc64_q8_flash_host_io();RaceLoadPlan scratch;int status;
    memset(&sc64_cf_snapshot,0,sizeof sc64_cf_snapshot);
    if(!sc64_cf_ownership(io))return 0;
    sc64_cf_snapshot.plan=&scratch;
    status=gba_fj_select(io,sc64_cf_capture,&sc64_cf_snapshot,&sc64_cf_previous);
    sc64_cf_snapshot.plan=0;
    if(status<0)return 0;
    for(unsigned i=0;i<4;++i)if(sc64_cf_snapshot.seen[i] && !sc64_cf_snapshot.accepted[i])return 0;
    sc64_cf_has_previous=status==GBA_FJ_OK;return 1;
}
static int race_q8_flash_select(RaceLoadPlan *plan)
{
    const GbaFlashJournalIo *io=sc64_q8_flash_host_io();
    Sc64CfSnapshot snapshot={0};GbaFlashJournalRecord selected,checked;
    if(!plan || !sc64_cf_ownership(io))return 0;
    snapshot.plan=plan;
    int status=gba_fj_select(io,sc64_cf_capture,&snapshot,&selected);
    if(status!=GBA_FJ_OK || sc64_cf_prepare(plan,io,&selected)!=1)return 0;
    status=gba_fj_probe(io,selected.index,&checked);
    return status==GBA_FJ_OK && gba_fj_same_record(&selected,&checked);
}
typedef struct {const GbaFlashJournalIo *inner;int checked;} Sc64CfGuard;
static int sc64_cf_guard_read(void *v,u32 p,void *out,u32 n)
{Sc64CfGuard *g=v;return g->inner->read(g->inner->context,p,out,n);}
static int sc64_cf_guard_program(void *v,u32 p,const void *data,u32 n)
{Sc64CfGuard *g=v;return g->inner->program(g->inner->context,p,data,n);}
static int sc64_cf_guard_erase(void *v,u32 p)
{Sc64CfGuard *g=v;if(!g->checked){if(!sc64_cf_ownership(g->inner))return 0;g->checked=1;}return g->inner->erase_sector(g->inner->context,p);}
static int race_q8_flash_write(const void *data,u32 length)
{
    SC64_CF_TEST_STAGE(2);
    const GbaFlashJournalIo *io=sc64_q8_flash_host_io();
    u8 header[SC64_CF_HEADER],side[SC64_CF_SIDE];RaceLoadPlan plan;
    u8 suffix[SC64_BC_SAVE_EXTRA_BYTES+sizeof(Sc64ImpactSaveHeader)+
              RACE_IMPACT_SAVE_CAPACITY*sizeof(Sc64ImpactEvent)+SC64_TRANSPORT_TRAILER_BYTES];
    u8 padding[MAX_BUILDINGS];unsigned hp,shield;u32 at=0;
    const u32 suffix_start=(u32)sizeof(RaceSave)-SC64_BC_SAVE_EXTRA_BYTES;
    u32 suffix_length,original_hash;int valid;
    if(!io || !io->read || !io->program || !io->erase_sector ||
       race_mode || race_selected_mission!=SC64_Z10_GLOBAL_ID || race_human!=R_ZERG ||
       data!=(const void *)&race_impact_record || length<suffix_start ||
       length>sizeof(race_impact_record) || length-suffix_start>sizeof suffix)return 0;
    for(int i=0;i<MAX_UNITS;++i) {
        if(!u[i].alive) {if(sc64_pr_hp_debt[i])return 0;}
        else if(!sc64_pr_q8_actor_hp(i,&hp) || !hp || !sc64_pr_q8_actor_shield(i,&shield))return 0;
        side[at++]=sc64_pr_hp_debt[i];
    }
    for(int i=0;i<MAX_BUILDINGS;++i) {
        if(!b[i].alive) {if(sc64_pr_hp_debt[MAX_UNITS+i])return 0;}
        else if(!sc64_pr_q8_actor_hp(MAX_UNITS+i,&hp) || !hp ||
                !sc64_pr_q8_actor_shield(MAX_UNITS+i,&shield))return 0;
        side[at++]=sc64_pr_hp_debt[MAX_UNITS+i];
    }
    for(int i=0;i<MAX_UNITS;++i)side[at++]=u[i].alive?sc64_current_unit[i]:255;
    for(int i=0;i<MAX_BUILDINGS;++i)side[at++]=b[i].alive?sc64_current_building[i]:255;
    for(int i=0;i<MAX_UNITS;++i)side[at++]=u[i].alive?sc64_scarab_stage_payer[i]:0;
    for(int i=0;i<MAX_UNITS;++i) {
        unsigned x=0,y=0;
        if(u[i].alive && u[i].type==U_SCARAB && u[i].raw_type==85 &&
           (((unsigned)u[i].carry>>8)&3u)>=2u) {
            x=u[i].path_timer;y=u[i].path_retry;
            if(x>32767 || y>32767)return 0;
        }
        side[at++]=(u8)x;side[at++]=(u8)(x>>8);side[at++]=(u8)y;side[at++]=(u8)(y>>8);
    }
    if(at!=sizeof side)return 0;
    suffix_length=length-suffix_start;
    memcpy(suffix,(const u8 *)data+suffix_start,suffix_length);
    original_hash=gba_fj_hash(2166136261u,data,length);
    for(unsigned i=0;i<MAX_BUILDINGS;++i)
        padding[i]=((u8 *)race_data.buildings)[i*SC64_PR_ROW_WIDTH+SC64_PR_ROW_WIDTH-1];
    Sc64CfPrepareContext context={&plan,side+SC64_CF_FLIGHT_OFFSET};
    valid=race_worker_cargo_prepare_selected(data,length,SC64_COMPOUND_FLASH_TAG,&context);
    SC64_CF_TEST_STAGE(valid?3:4);
    if(valid) {
        u32 cursor=0;
        memcpy(plan.q8_debt,side+cursor,sizeof plan.q8_debt);cursor+=sizeof plan.q8_debt;
        memcpy(plan.controller_units,side+cursor,sizeof plan.controller_units);cursor+=sizeof plan.controller_units;
        memcpy(plan.controller_buildings,side+cursor,sizeof plan.controller_buildings);cursor+=sizeof plan.controller_buildings;
        memcpy(plan.scarab_payers,side+cursor,sizeof plan.scarab_payers);
        valid=sc64_cf_validate(&plan);
        SC64_CF_TEST_STAGE(valid?5:6);
    }
    for(unsigned i=0;i<MAX_BUILDINGS;++i)
        ((u8 *)race_data.buildings)[i*SC64_PR_ROW_WIDTH+SC64_PR_ROW_WIDTH-1]=padding[i];
    memcpy((u8 *)&race_impact_record+suffix_start,suffix,suffix_length);
    if(!valid || gba_fj_hash(2166136261u,data,length)!=original_hash)return 0;
    Sc64CompoundFlashHeader envelope=sc64_cf_expected(length);sc64_cf_write(header,&envelope);
    Sc64CfGuard guard={io,0};GbaFlashJournalIo guarded={&guard,sc64_cf_guard_read,sc64_cf_guard_erase,sc64_cf_guard_program};
    GbaFlashJournalRecord written;
    return gba_fj_write_three(&guarded,header,sizeof header,data,length,side,sizeof side,
         SC64_COMPOUND_FLASH_TAG,sc64_cf_reselect,&sc64_cf_snapshot,
         sc64_cf_has_previous?&sc64_cf_previous:0,&written)==GBA_FJ_OK;
}

#endif
