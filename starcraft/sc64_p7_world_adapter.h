#ifndef SC64_P7_WORLD_ADAPTER_H
#define SC64_P7_WORLD_ADAPTER_H
#include "sc64_unit_slot_free.h"

/* Source-specific callbacks for P7. This intentionally implements only the
 * opcodes present in the authenticated P7 table; it is not a CHK executor. */
#include "sc64_p7_state.h"
#include "sc64_p7_geometry.h"
#include "sc64_resource_actions.h"

#ifdef SC64_P7_COUNT_SNAPSHOT
#if !defined(SC64_P7_PREVALIDATED_TICK)
#error "P7 count snapshot requires the prevalidated tick path"
#endif
typedef struct { u32 count[8]; u8 active; } Sc64P7CountSnapshot;
#endif

enum {
    SC64_P7_OWNER_SLOTS = 8,
    SC64_P7_MAP_UNITS = 180,
    SC64_P7_MAP_BUILDINGS = 112,
    SC64_P7_MAP_MINERALS = 64,
    SC64_P7_MAP_GEYSERS = 1,
    SC64_P7_CREATION_COUNT = 3,
    SC64_P7_RAW_ANY_UNIT = 229,
    SC64_P7_RAW_ANY_MEN = 230,
    SC64_P7_RUNTIME_FAULT_MAP = 0x60,
    SC64_P7_RUNTIME_FAULT_ACTOR = 0x61,
    SC64_P7_RUNTIME_FAULT_AI = 0x62
};

/* FORC stores 0=Force1, 1=Force2, 2=Force3, 3=Force4 in the first eight
 * player entries. The map's human slot is Force3; two computer forces share
 * the engine's ENEMY combat bucket, so their source wallets remain separate. */
static const u8 sc64_p7_force_for_slot[8] = {1,3,3,3,0,3,3,2};
/* All eight canonical player slots remain addressable. Participation is not
 * inferred from the presence of initial placement rows. */
static const u8 sc64_p7_active_slots[8] = {1,1,1,1,1,1,1,1};
static const u8 sc64_p7_owner_modes_expected[8] = {1,3,3,2,1,2,2,0};
static const u8 sc64_p7_owner_races_expected[8] = {2,2,2,1,2,2,1,2};

typedef struct {
    const Sc64RaceMap *map;
    Sc64P7PersistentState *persistent;
    u8 source_current_player;
    u8 source_slot_active[12];
    s8 live_owner_for_source[12];
    u8 source_force_for_slot[8];
    u32 elapsed_frames;
    u8 victory_requested, defeat_requested, objective_refresh_requested;
    u8 presentation_opcode;
    u16 last_string_id, last_wav_id, center_x, center_y;
#ifdef SC64_P7_PREVALIDATED_TICK
    u8 prevalidated_tick;
#endif
#ifdef SC64_P7_COUNT_SNAPSHOT
    Sc64P7CountSnapshot count_snapshot;
#endif
} Sc64P7WorldContext;

#ifdef SC64_P7_COUNT_SNAPSHOT
static int sc64_p7_world_owner_matches(const Sc64P7WorldContext *,unsigned,int,int);
static int sc64_p7_world_type_match(unsigned,unsigned,int);
/* The eight authenticated Command rows, indexed by trigger. Build counts in
 * one Unit+Building census while preserving the reference callback's
 * fail-closed owner and raw-type checks. */
static int sc64_p7_count_snapshot_begin(Sc64P7WorldContext *ctx)
{
    Sc64P7CountSnapshot next = {{0},0};
    static const u8 selector[8] = {13,17,17,17,19,18,18,19};
    static const u8 wanted[8] = {229,78,79,75,229,229,154,154};
    if (!ctx) return 0;
    ctx->count_snapshot.active=0;
    for (unsigned q=0;q<8;++q) {
        unsigned trigger = q==0 ? 0 : q<=3 ? q : q+2;
        const Sc64P7SourceCondition *c;
        if (!sc64_p7_condition_row(trigger,0,&c) ||
            c->opcode!=SC64_P7_CONDITION_COMMAND || c->player!=selector[q] ||
            c->unit!=wanted[q] || c->location || c->resource || c->mask ||
            c->flags!=0x10u) return 0;
    }
    for (int pass=0;pass<2;++pass) {
        int limit=pass ? MAX_BUILDINGS : MAX_UNITS;
        for (int i=0;i<limit;++i) {
            int alive=pass ? b[i].alive : u[i].alive;
            int raw_owner=pass ? b[i].raw_owner : u[i].raw_owner;
            int live_owner=pass ? b[i].owner : u[i].owner;
            if (!alive) continue;
            int raw=pass ? (b[i].raw_type>=0 ? b[i].raw_type : sc64_race_raw_building(b[i].type)) :
                           (u[i].raw_type>=0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type));
            int owner13=sc64_p7_world_owner_matches(ctx,13,raw_owner,live_owner);
            int owner17=sc64_p7_world_owner_matches(ctx,17,raw_owner,live_owner);
            int owner19=sc64_p7_world_owner_matches(ctx,19,raw_owner,live_owner);
            int owner18=sc64_p7_world_owner_matches(ctx,18,raw_owner,live_owner);
            if (owner13<0 || owner17<0 || owner19<0 || owner18<0) return 0;
            if (!(owner13 || owner17 || owner19 || owner18)) continue;
            if (raw<0 || raw>227) return 0;
            if (owner13) ++next.count[0];
            if (owner17) {
                for (unsigned q=1;q<=3;++q) {
                    if ((unsigned)raw == wanted[q]) {
                        int match=sc64_p7_world_type_match(wanted[q],(unsigned)raw,pass);
                        if (match<0) return 0;
                        if (match) ++next.count[q];
                    }
                }
            }
            if (owner19) ++next.count[4];
            if (owner18) ++next.count[5];
            if ((owner18 || owner19) && raw == 154) {
                int match=sc64_p7_world_type_match(154,(unsigned)raw,pass);
                if (match<0) return 0;
                if (match && owner18) ++next.count[6];
                if (match && owner19) ++next.count[7];
            }
        }
    }
    next.active=1;
    ctx->count_snapshot=next;
    return 1;
}

static void sc64_p7_count_snapshot_end(Sc64P7WorldContext *ctx)
{ if (ctx) ctx->count_snapshot.active=0; }

static int sc64_p7_count_snapshot_lookup(const Sc64P7WorldContext *ctx,
                                          const Sc64P7SourceCondition *c,
                                          u32 *count)
{
    unsigned q;
    if (!ctx || !count || ctx->prevalidated_tick!=1 ||
        ctx->count_snapshot.active!=1 || !c || c->index ||
        c->opcode!=SC64_P7_CONDITION_COMMAND) return 0;
    q=c->trigger==0 ? 0 : c->trigger<=3 ? c->trigger : c->trigger==6 ? 4 :
      c->trigger==7 ? 5 : c->trigger==8 ? 6 : c->trigger==9 ? 7 : 8;
    if (q>=8) return 0;
    *count=ctx->count_snapshot.count[q];
    return 1;
}
#endif

static int sc64_p7_world_location_source(const Sc64P7WorldContext *ctx,
                                          unsigned id, unsigned *left,
                                          unsigned *top, unsigned *right,
                                          unsigned *bottom)
{
    const Sc64P7SourceLocation *loc;
    if (!ctx || !ctx->map || !left || !top || !right || !bottom ||
        id == 0 || id > SC64_P7_SOURCE_LOCATION_COUNT) return 0;
    if (id == 3u && ctx->persistent &&
        ctx->persistent->moved_location3_valid) {
        *left=ctx->persistent->moved_location3[0];
        *top=ctx->persistent->moved_location3[1];
        *right=ctx->persistent->moved_location3[2];
        *bottom=ctx->persistent->moved_location3[3];
        return *left < *right && *top < *bottom;
    }
    if (id == SC64_P7_SOURCE_LOCATION_COUNT) {
        *left=*top=0;
        *right=(unsigned)ctx->map->width*32u;
        *bottom=(unsigned)ctx->map->height*32u;
        return 1;
    }
    loc=&sc64_p7_source_locations[id-1u];
    *left=loc->left; *top=loc->top; *right=loc->right; *bottom=loc->bottom;
    return loc->flags == 0 && *left <= *right && *top <= *bottom &&
           *right <= (unsigned)ctx->map->width*32u &&
           *bottom <= (unsigned)ctx->map->height*32u;
}

static int sc64_p7_world_location(const Sc64P7WorldContext *ctx,unsigned id,
                                   int *left,int *top,int *right,int *bottom)
{
    unsigned l,t,r,b;
    if (!left || !top || !right || !bottom ||
        !sc64_p7_world_location_source(ctx,id,&l,&t,&r,&b)) return 0;
    *left=(int)(l/2u); *top=(int)(t/2u);
    *right=(int)(r/2u); *bottom=(int)(b/2u);
    return *left <= *right && *top <= *bottom;
}

static int sc64_p7_world_owner_matches(const Sc64P7WorldContext *ctx,
                                        unsigned selector,int raw_owner,
                                        int live_owner)
{
    unsigned slot;
    if (!ctx || raw_owner < 0 || raw_owner >= 12) return -1;
    slot=(unsigned)raw_owner;
    if (!ctx->source_slot_active[slot]) return -1;
    if (selector <= 11u) return selector == slot;
    if (selector == 13u) return live_owner == SIDE;
    if (selector == 17u) return slot < 8u;
    if (selector >= 18u && selector <= 21u)
        return slot < 8u && ctx->source_force_for_slot[slot] == selector-18u;
    return -1;
}

static int sc64_p7_world_owner_resolve(const Sc64P7WorldContext *ctx,
                                       unsigned selector,unsigned *slot_out)
{
    unsigned found=12;
    if (!ctx || !slot_out) return 0;
    if (selector <= 11u) found=selector;
    else if (selector == 13u) found=ctx->source_current_player;
    else if (selector >= 18u && selector <= 21u) {
        for (unsigned i=0;i<8;++i) if (ctx->source_slot_active[i] &&
            ctx->source_force_for_slot[i] == selector-18u) {
            if (found != 12u) return 0; /* A spawn requires one source slot. */
            found=i;
        }
    } else return 0;
    if (found >= 12u || !ctx->source_slot_active[found]) return 0;
    *slot_out=found;
    return 1;
}

static int sc64_p7_world_compare(unsigned comparison,u32 value,u32 amount)
{
    switch (comparison) {
    case 0: return value >= amount;       /* At least */
    case 1: return value <= amount;       /* At most */
    case 10: return value == amount;      /* Exactly */
    default: return -1;
    }
}

static int sc64_p7_world_type_match(unsigned wanted,unsigned raw,int building)
{
    int type;
    if (wanted == SC64_P7_RAW_ANY_UNIT) return 1;
    if (wanted == SC64_P7_RAW_ANY_MEN) {
        if (building) return 0;
        type=sc64_race_unit_from_raw((int)raw);
        if (type < 0) return -1;
        return (sc64_game_unit_actor_flags(type) & SC64_GAME_ACTOR_MEN) != 0;
    }
    if (wanted >= 228u || wanted != raw) return 0;
    if (building) return sc64_race_building_from_raw((int)raw) >= 0;
    return sc64_race_unit_from_raw((int)raw) >= 0;
}

/* P7's authenticated predicates are Command/Exactly? no: they use native
 * AtLeast/AtMost comparisons and no Bring rows. Keep this generic source-pixel
 * overlap helper for location-bound actions and future audited Bring rows. */
static int sc64_p7_world_actor_rect_source(unsigned raw,int x,int y,
                                            int building,int *left,int *top,
                                            int *right,int *bottom)
{
#ifdef SC64_P7_ACTORS
    return sc64_p7_actor_rect_source(raw,x,y,building,left,top,right,bottom);
#else
    (void)raw; (void)x; (void)y; (void)building;
    (void)left; (void)top; (void)right; (void)bottom;
    return 0;
#endif
}

static int sc64_p7_world_count(const Sc64P7WorldContext *ctx,unsigned selector,
                                unsigned wanted,unsigned location,u32 *out)
{
    unsigned l=0,t=0,r=0,bottom=0;
    u32 count=0;
    if (!ctx || !out || wanted > 231u ||
        (selector > 13u && selector < 17u) || selector > 21u ||
        (location && !sc64_p7_world_location_source(ctx,location,&l,&t,&r,&bottom)))
        return 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) {
        int raw=u[i].raw_type>=0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        int owner=sc64_p7_world_owner_matches(ctx,selector,u[i].raw_owner,u[i].owner);
        int match;
        if (owner < 0) return 0;
        if (!owner) continue;
        if (raw < 0 || raw > 227) return 0;
        match=sc64_p7_world_type_match(wanted,(unsigned)raw,0);
        if (match < 0) return 0;
        if (!match) continue;
        if (location) {
            int al,at,ar,ab;
            if (!sc64_p7_world_actor_rect_source((unsigned)raw,u[i].x,u[i].y,
                                                 0,&al,&at,&ar,&ab)) return 0;
            if ((int)l >= ar || al >= (int)r || (int)t >= ab || at >= (int)bottom)
                continue;
        }
        ++count;
    }
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive) {
        int raw=b[i].raw_type>=0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        int owner=sc64_p7_world_owner_matches(ctx,selector,b[i].raw_owner,b[i].owner);
        int match;
        if (owner < 0) return 0;
        if (!owner) continue;
        if (raw < 0 || raw > 227) return 0;
        match=sc64_p7_world_type_match(wanted,(unsigned)raw,1);
        if (match < 0) return 0;
        if (!match) continue;
        if (location) {
            int al,at,ar,ab;
            if (!sc64_p7_world_actor_rect_source((unsigned)raw,b[i].x,b[i].y,
                                                 1,&al,&at,&ar,&ab)) return 0;
            if ((int)l >= ar || al >= (int)r || (int)t >= ab || at >= (int)bottom)
                continue;
        }
        ++count;
    }
    *out=count;
    return 1;
}

static int sc64_p7_world_condition(void *context,const Sc64P7RuntimeState *state,
                                    const Sc64P7SourceCondition *c)
{
    Sc64P7WorldContext *ctx=(Sc64P7WorldContext *)context;
    const Sc64P7SourceCondition *source;
    u32 count=0;
    if (!ctx || !state || !c ||
        !sc64_p7_condition_row(c->trigger,c->index,&source) || source!=c
#ifdef SC64_P7_PREVALIDATED_TICK
        || (!ctx->prevalidated_tick && !sc64_p7_runtime_metadata_valid())
#else
        || !sc64_p7_runtime_metadata_valid()
#endif
        ) return -1;
    switch(c->opcode) {
    case SC64_P7_CONDITION_COMMAND:
#ifdef SC64_P7_COUNT_SNAPSHOT
        if (c->location || c->resource || c->mask || c->flags!=0x10u) return -1;
        if (!sc64_p7_count_snapshot_lookup(ctx,c,&count) &&
            !sc64_p7_world_count(ctx,c->player,c->unit,0,&count)) return -1;
#else
        if (c->location || c->resource || c->mask || c->flags!=0x10u ||
            !sc64_p7_world_count(ctx,c->player,c->unit,0,&count)) return -1;
#endif
        return sc64_p7_world_compare(c->comparison,count,c->amount);
    case SC64_P7_CONDITION_SWITCH:
        if (c->location || c->player || c->amount || c->unit ||
            c->resource!=SC64_P7_SWITCH_VICTORY_GATE || c->flags || c->mask ||
            c->comparison!=2u) return -1;
        return (state->switch_bits & SC64_P7_SWITCH_MASK) != 0;
    case SC64_P7_CONDITION_ELAPSED_TIME:
        if (c->location || c->player || c->unit || c->resource || c->flags ||
            c->mask || c->comparison>10u) return -1;
        return sc64_p7_world_compare(c->comparison,ctx->elapsed_frames/30u,
                                     c->amount);
    default:return -1;
    }
}

static int sc64_p7_world_move_location(Sc64P7WorldContext *ctx,
                                        const Sc64P7SourceAction *a)
{
    unsigned sl,st,sr,sb,tl,tt,tr,tb;
    unsigned px=0,py=0,owner_selector;
    int found=0;
    if (!ctx || !ctx->persistent || !a || a->opcode!=SC64_P7_ACTION_MOVE_LOCATION ||
        a->group2!=3u || a->unit>=228u || a->modifier || a->flags ||
        a->string_id || a->wav_id || a->time || a->trailing[0] ||
        a->trailing[1] || a->trailing[2] ||
        !sc64_p7_world_location_source(ctx,a->location,&sl,&st,&sr,&sb) ||
        !sc64_p7_world_location_source(ctx,a->group2,&tl,&tt,&tr,&tb)) return 0;
    owner_selector=a->group1;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) {
        int owner=sc64_p7_world_owner_matches(ctx,owner_selector,u[i].raw_owner,u[i].owner);
        int raw=u[i].raw_type>=0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        if (owner<0) return 0;
        if (owner && raw==(int)a->unit && (unsigned)u[i].x*2u>=sl &&
            (unsigned)u[i].x*2u<sr && (unsigned)u[i].y*2u>=st &&
            (unsigned)u[i].y*2u<sb) {
            px=(unsigned)u[i].x*2u; py=(unsigned)u[i].y*2u; found=1; break;
        }
    }
    if (!found) { px=(sl+sr)/2u; py=(st+sb)/2u; }
    {
        unsigned width=tr-tl,height=tb-tt;
        int left=(int)tl+(int)px-(int)((sl+sr)/2u);
        int top=(int)tt+(int)py-(int)((st+sb)/2u);
        int maxx=(int)ctx->map->width*32,maxy=(int)ctx->map->height*32;
        if (!width || !height || width >= (unsigned)maxx || height >= (unsigned)maxy)
            return 0;
        if (left<0) left=0; if (top<0) top=0;
        if ((unsigned)left+width >= (unsigned)maxx) left=maxx-1-(int)width;
        if ((unsigned)top+height >= (unsigned)maxy) top=maxy-1-(int)height;
        ctx->persistent->moved_location3[0]=(u16)left;
        ctx->persistent->moved_location3[1]=(u16)top;
        ctx->persistent->moved_location3[2]=(u16)(left+(int)width);
        ctx->persistent->moved_location3[3]=(u16)(top+(int)height);
        ctx->persistent->moved_location3_valid=1;
    }
    return 1;
}

static int sc64_p7_world_selector_resources(Sc64P7WorldContext *ctx,
                                             unsigned selector,u32 amount,
                                             unsigned resource,unsigned modifier)
{
    unsigned matched=0;
    if (!ctx || !ctx->persistent || resource>SC64_RESOURCE_BOTH || amount>65535u ||
        (modifier!=SC64_RESOURCE_MOD_SET && modifier!=SC64_RESOURCE_MOD_ADD &&
         modifier!=SC64_RESOURCE_MOD_SUBTRACT)) return 0;
    for (unsigned slot=0;slot<8;++slot) {
        Sc64ResourceActionState v;
        if (!ctx->source_slot_active[slot]) continue;
        int selected=sc64_p7_world_owner_matches(ctx,selector,(int)slot,
                                       ctx->live_owner_for_source[slot]);
        if (selected<0) return 0;
        if (!selected) continue;
        ++matched;
        int human=ctx->live_owner_for_source[slot]==SIDE;
        if (!human && slot!=0u && slot!=4u) return 0;
        v.current_minerals=human ? (u32)race_minerals[SIDE] :
                                   ctx->persistent->source_minerals[slot];
        v.current_gas=human ? (u32)race_gas[SIDE] :
                              ctx->persistent->source_gas[slot];
        v.total_minerals_gathered=v.total_gas_gathered=0;
        if (!sc64_resource_action_apply_one_player(&v,(u16)resource,
                                                    (u8)modifier,amount) ||
            v.current_minerals>65535u || v.current_gas>65535u) return 0;
        if (human) {
            minerals=race_minerals[SIDE]=(int)v.current_minerals;
            gas=race_gas[SIDE]=(int)v.current_gas;
        } else {
            ctx->persistent->source_minerals[slot]=v.current_minerals;
            ctx->persistent->source_gas[slot]=v.current_gas;
        }
    }
    return matched != 0;
}

/* Bounded authored replacement for the three opaque Protoss AI script IDs.
 * It issues ordinary AttackMove orders to that source force toward the
 * requested source location. Native script equivalence and production policy
 * are not claimed; no units/resources are granted by this replacement. */
static int sc64_p7_world_run_ai(Sc64P7WorldContext *ctx,
                                 const Sc64P7SourceAction *a)
{
    unsigned force,loc;
    int l,t,r,bm,tx,ty,ordered=0;
    if (!ctx || !a || a->opcode!=SC64_P7_ACTION_RUN_AI_AT_LOCATION ||
        a->group1 || a->unit || a->time || a->modifier || a->flags ||
        a->string_id || a->wav_id || a->trailing[0] || a->trailing[1] ||
        a->trailing[2]) return 0;
    if (a->trigger==6 && a->index==1 && a->group2==0x376f7250u) {
        force=1; loc=4;
    } else if (a->trigger==6 && a->index==2 && a->group2==0x42377250u) {
        force=1; loc=5;
    } else if (a->trigger==7 && a->index==1 && a->group2==0x53377250u) {
        force=0; loc=6;
    } else return 0;
    if (a->location!=loc || !sc64_p7_world_location(ctx,loc,&l,&t,&r,&bm)) return 0;
    tx=(l+r)/2; ty=(t+bm)/2;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) {
        int slot=u[i].raw_owner;
        int raw=u[i].raw_type>=0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        if (slot<0 || slot>=8 || !ctx->source_slot_active[slot] || raw<0 ||
            sc64_race_unit_from_raw(raw)<0) return 0;
        if (ctx->source_force_for_slot[slot]!=force) continue;
        u[i].order=ORDER_ATTACK_MOVE; u[i].target=-1;
        u[i].tx=tx; u[i].ty=ty; u[i].path_timer=0; u[i].path_retry=0;
        ordered++;
    }
    return ordered>0;
}

static int sc64_p7_world_create(Sc64P7WorldContext *ctx,
                                 const Sc64P7SourceAction *a)
{
    unsigned slot;
    int l,t,r,bm,type,owner,free_slots=0;
    int bit;
    if (!ctx || !ctx->persistent || !a || a->opcode!=SC64_P7_ACTION_CREATE_UNIT ||
        a->modifier || a->group2 || a->unit>=228u || a->flags!=0x10u ||
        a->trailing[0] || a->trailing[1] || a->trailing[2] ||
        a->trigger!=5 || a->index<8 || a->index>10 ||
        !sc64_p7_world_owner_resolve(ctx,a->group1,&slot) ||
        !sc64_p7_world_location(ctx,a->location,&l,&t,&r,&bm)) return 0;
    bit=(int)a->index-8;
    if (ctx->persistent->created_mask & (u8)(1u<<bit)) return 0;
    type=sc64_race_unit_from_raw(a->unit);
    owner=ctx->live_owner_for_source[slot];
    if (type<0 || owner<SIDE || owner>4) return 0; /* No actor aliases. */
    for (int i=0;i<MAX_UNITS;++i) free_slots += SC64_UNIT_SLOT_FREE(i);
    if (!free_slots) return 0;
    int created=spawn_unit(owner,type,(l+r)/2,(t+bm)/2);
    if (created<0) return 0;
    u[created].raw_type=(s16)a->unit;
    u[created].raw_owner=(s16)slot;
    ctx->persistent->created_mask|=(u8)(1u<<bit);
    return 1;
}

static int sc64_p7_world_action(void *context,const Sc64P7SourceAction *a)
{
    Sc64P7WorldContext *ctx=(Sc64P7WorldContext *)context;
    const Sc64P7SourceAction *source;
    if (!ctx || !a || !sc64_p7_source_action(a->trigger,a->index,&source) ||
        source!=a
#ifdef SC64_P7_PREVALIDATED_TICK
        || (!ctx->prevalidated_tick && !sc64_p7_runtime_metadata_valid())
#else
        || !sc64_p7_runtime_metadata_valid()
#endif
        ) return 0;
    switch (a->opcode) {
    case SC64_P7_ACTION_VICTORY:
        if (a->location || a->group1 || a->group2 || a->unit || a->time) return 0;
        ctx->victory_requested=1; return 1;
    case SC64_P7_ACTION_DEFEAT:
        if (a->location || a->group1 || a->group2 || a->unit || a->time) return 0;
        ctx->defeat_requested=1; return 1;
    case SC64_P7_ACTION_WAIT: case SC64_P7_ACTION_SET_SWITCH:
        return 1; /* Scheduler applies timer/switch state. */
    case SC64_P7_ACTION_TRANSMISSION: case SC64_P7_ACTION_PLAY_WAV:
    case SC64_P7_ACTION_DISPLAY_TEXT:
        if (a->string_id>65535u || a->wav_id>65535u) return 0;
        ctx->presentation_opcode=a->opcode;
        ctx->last_string_id=(u16)a->string_id; ctx->last_wav_id=(u16)a->wav_id;
        return 1;
    case SC64_P7_ACTION_CENTER_VIEW: {
        int l,t,r,bm;
        if (!sc64_p7_world_location(ctx,a->location,&l,&t,&r,&bm)) return 0;
        ctx->center_x=(u16)((l+r)/2); ctx->center_y=(u16)((t+bm)/2); return 1;
    }
    case SC64_P7_ACTION_CREATE_UNIT: return sc64_p7_world_create(ctx,a);
    case SC64_P7_ACTION_OBJECTIVES:
        if (a->location || a->group1 || a->group2 || a->unit || a->time) return 0;
        ctx->objective_refresh_requested=1; return 1;
    case SC64_P7_ACTION_RUN_AI_AT_LOCATION:
        return sc64_p7_world_run_ai(ctx,a);
    case SC64_P7_ACTION_SET_RESOURCES:
        if (!ctx->persistent || a->location || a->flags || a->unit>2u ||
            a->group2>65535u || a->modifier!=SC64_RESOURCE_MOD_SET ||
            !((a->trigger==4 && a->index==1) ||
              ((a->trigger==6 || a->trigger==7) && a->index==0))) return 0;
        return sc64_p7_world_selector_resources(ctx,a->group1,a->group2,
                                                 a->unit,a->modifier);
    case SC64_P7_ACTION_MOVE_LOCATION:
        return sc64_p7_world_move_location(ctx,a);
    default:return 0;
    }
}

#endif
