/* SPDX-License-Identifier: GPL-3.0-or-later
 * Isolated HOST-only bridge from the real P10 custom codec to the Flash model.
 * The normal SRAM journal and ARM builds never include this file. */
#ifndef SC64_Q8_FLASH_HOST_ADAPTER_H
#define SC64_Q8_FLASH_HOST_ADAPTER_H
#define SC64_Q8_FLASH_TAG SC64_SAVE_TAG_Q8_FLASH(SC64_TRANSPORT_SAVE_VERSION)
#define SC64_Q8_FLASH_TAIL_MAGIC 0x31465051u /* QPF1 */
#define SC64_Q8_FLASH_TAIL_HEADER 8u
#define SC64_Q8_FLASH_DEBTS (MAX_UNITS+MAX_BUILDINGS)
#define SC64_Q8_FLASH_TAIL_BYTES (SC64_Q8_FLASH_TAIL_HEADER+SC64_Q8_FLASH_DEBTS)
_Static_assert(SC64_Q8_FLASH_TAG!=SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION &&
               SC64_Q8_FLASH_TAG!=SC64_RESEARCH_TRANSPORT_SAVE_VERSION &&
               SC64_Q8_FLASH_TAG!=SC64_TRANSPORT_SAVE_VERSION,"Q8 Flash tag collision");

typedef struct {
    GbaFlashJournalRecord record[4];
    u8 seen[4], accepted[4];
    RaceLoadPlan *plan;
} Sc64Q8FlashSnapshot;
static Sc64Q8FlashSnapshot sc64_q8_flash_snapshot;
static GbaFlashJournalRecord sc64_q8_flash_previous;
static int sc64_q8_flash_has_previous;

/* Detached rows only: no references to live actor state or wallets. Old
 * shield phases are accepted only when zero, where 320- and 1280-unit
 * interpretations agree. Non-shield aliases are not interpreted here. */
static int race_q8_flash_candidate_fractions(RaceLoadPlan *plan,int old)
{
    if (!plan || race_data.state[0]!=1 || race_data.state[1]!=R_PROTOSS) return 0;
    for (int i=0;i<MAX_UNITS;++i) {
        Unit q;const Sc64StockUnitProfile *stock;int raw;
        if (!sc64_p7_typed_unit_decode(&q,&race_data.units[i],2048,2048)) return 0;
        if (!q.alive) {if(plan->q8_debt[i])return 0;continue;}
        if (q.hp<=0 || q.order==ORDER_REPAIR) return 0;
        raw=q.raw_type>=0?q.raw_type:sc64_race_raw_unit(q.type);
        stock=sc64_stock_unit_profile(raw);if(!stock)return 0;
        if (stock->shields && (q.shield<0 || (unsigned)q.shield>stock->shields ||
            q.shield_remainder>=1280 || (old && q.shield_remainder) ||
            ((unsigned)q.shield==stock->shields && q.shield_remainder))) return 0;
    }
    for (int i=0;i<MAX_BUILDINGS;++i) {
        Building q;const Sc64StockUnitProfile *stock;int raw;
        if (!race_save_decode_building(&race_data,(unsigned)i,&q)) return 0;
        if (!q.alive) {if(plan->q8_debt[MAX_UNITS+i])return 0;continue;}
        if(q.hp<=0) return 0;
        raw=q.raw_type>=0?q.raw_type:sc64_race_raw_building(q.type);
        stock=sc64_stock_unit_profile(raw);if(!stock)return 0;
        if (race_building_can_burn(&q) && plan->q8_debt[MAX_UNITS+i]) return 0;
        if (stock->shields && (q.shield<0 || (unsigned)q.shield>stock->shields ||
            q.shield_remainder>=1280 || (old && q.shield_remainder) ||
            ((unsigned)q.shield==stock->shields && q.shield_remainder))) return 0;
    }
    return 1;
}

static int race_q8_flash_prepare_record(RaceLoadPlan *plan,const GbaFlashJournalIo *io,
                                         const GbaFlashJournalRecord *record)
{
    u32 tag=record->tag,base_length=record->length;u8 header[SC64_Q8_FLASH_TAIL_HEADER];
    int is_new=record->index>=2;
    if (is_new) {
        if(tag!=SC64_Q8_FLASH_TAG || base_length<=SC64_Q8_FLASH_TAIL_BYTES) return 0;
        base_length-=SC64_Q8_FLASH_TAIL_BYTES;
    } else if(tag!=SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION &&
              tag!=SC64_TRANSPORT_SAVE_VERSION && tag!=SC64_RESEARCH_TRANSPORT_SAVE_VERSION &&
              tag!=SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION) return 0;
    if (base_length>sizeof(race_impact_record)) return 0;
    if (gba_fj_read_payload(io,record,0,&race_impact_record,base_length)!=GBA_FJ_OK) return -1;
    /* 0x48 explicitly wraps the 0x45 game representation. Only this local
     * decoder dispatch is translated; the stored tag is never rewritten. */
    if (!race_worker_cargo_prepare_selected(&race_impact_record,base_length,
           is_new?SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION:tag,plan)) return 0;
    if (is_new) {
        if (gba_fj_read_payload(io,record,base_length,header,sizeof header)!=GBA_FJ_OK ||
            gba_fj_read_payload(io,record,base_length+sizeof header,plan->q8_debt,SC64_Q8_FLASH_DEBTS)!=GBA_FJ_OK) return -1;
        if (gba_fj_u32(header)!=SC64_Q8_FLASH_TAIL_MAGIC ||
            gba_fj_u32(header+4)!=SC64_Q8_FLASH_DEBTS) return 0;
    }
    if (!race_q8_flash_candidate_fractions(plan,!is_new)) return 0;
    GbaFlashJournalRecord checked;
    int status=gba_fj_probe(io,record->index,&checked);
    if(status<0)return -1;
    return status==GBA_FJ_OK && gba_fj_same_record(record,&checked);
}
static int race_q8_flash_capture(void *context,const GbaFlashJournalIo *io,
                                  const GbaFlashJournalRecord *record)
{
    Sc64Q8FlashSnapshot *s=context;int accepted;
    if (!s || record->index>=4 || s->seen[record->index]) return -1;
    /* Never erase an unrelated committed new-layout record. */
    if(record->index>=2 && record->tag!=SC64_Q8_FLASH_TAG) return -1;
    accepted=race_q8_flash_prepare_record(s->plan,io,record);
    if(accepted<0)return -1;
    s->seen[record->index]=1;s->record[record->index]=*record;
    s->accepted[record->index]=(u8)accepted;return accepted;
}
static int race_q8_flash_snapshot_accept(void *context,const GbaFlashJournalIo *io,
                                          const GbaFlashJournalRecord *record)
{
    Sc64Q8FlashSnapshot *s=context;(void)io;
    if (!s || record->index>=4 || !s->seen[record->index] ||
        !gba_fj_same_record(&s->record[record->index],record)) return -1;
    return s->accepted[record->index];
}
static int race_q8_flash_begin(void)
{
    const GbaFlashJournalIo *io=sc64_q8_flash_host_io();RaceLoadPlan scratch;
    int result;memset(&sc64_q8_flash_snapshot,0,sizeof sc64_q8_flash_snapshot);
    sc64_q8_flash_snapshot.plan=&scratch;
    result=gba_fj_select(io,race_q8_flash_capture,&sc64_q8_flash_snapshot,&sc64_q8_flash_previous);
    sc64_q8_flash_snapshot.plan=0;
    if(result<0)return 0;
    /* A checksum-valid but unsupported predecessor must not silently become
     * a new epoch: the user has not authorized losing its selection. */
    if(result==GBA_FJ_NONE)
        for(unsigned i=0;i<4;++i) if(sc64_q8_flash_snapshot.seen[i]) return 0;
    sc64_q8_flash_has_previous=result==GBA_FJ_OK;
    return 1;
}
static int race_q8_flash_write(const void *data,u32 length)
{
    const GbaFlashJournalIo *io=sc64_q8_flash_host_io();
    u8 tail[SC64_Q8_FLASH_TAIL_BYTES];GbaFlashJournalRecord written;
    unsigned ignored;
    if (!io || !data || !length || race_mode!=1 || race_human!=R_PROTOSS) return 0;
    gba_fj_put32(tail,SC64_Q8_FLASH_TAIL_MAGIC);gba_fj_put32(tail+4,SC64_Q8_FLASH_DEBTS);
    for(int i=0;i<MAX_UNITS;++i) {
        unsigned debt=sc64_pr_hp_debt[i];
        if(!u[i].alive) {if(debt)return 0;}
        else if(u[i].order==ORDER_REPAIR || !sc64_pr_q8_actor_hp(i,&ignored) ||
                !sc64_pr_q8_actor_shield(i,&ignored)) return 0;
        tail[SC64_Q8_FLASH_TAIL_HEADER+i]=(u8)debt;
    }
    for(int i=0;i<MAX_BUILDINGS;++i) {
        unsigned debt=sc64_pr_hp_debt[MAX_UNITS+i];
        if(!b[i].alive) {if(debt)return 0;}
        else if((race_building_can_burn(&b[i]) && debt) ||
                !sc64_pr_q8_actor_hp(MAX_UNITS+i,&ignored) ||
                !sc64_pr_q8_actor_shield(MAX_UNITS+i,&ignored)) return 0;
        tail[SC64_Q8_FLASH_TAIL_HEADER+MAX_UNITS+i]=(u8)debt;
    }
    return gba_fj_write_parts(io,data,length,tail,sizeof tail,SC64_Q8_FLASH_TAG,
        race_q8_flash_snapshot_accept,&sc64_q8_flash_snapshot,
        sc64_q8_flash_has_previous?&sc64_q8_flash_previous:0,&written)==GBA_FJ_OK;
}
static int race_q8_flash_select(RaceLoadPlan *plan)
{
    const GbaFlashJournalIo *io=sc64_q8_flash_host_io();
    Sc64Q8FlashSnapshot snapshot={0};GbaFlashJournalRecord selected,checked;
    snapshot.plan=plan;
    int status=gba_fj_select(io,race_q8_flash_capture,&snapshot,&selected);
    if(status!=GBA_FJ_OK || race_q8_flash_prepare_record(plan,io,&selected)!=1) return 0;
    status=gba_fj_probe(io,selected.index,&checked);
    return status==GBA_FJ_OK && gba_fj_same_record(&selected,&checked);
}
#endif
