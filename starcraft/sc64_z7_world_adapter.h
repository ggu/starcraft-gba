#ifndef SC64_Z7_WORLD_ADAPTER_H
#define SC64_Z7_WORLD_ADAPTER_H

/* Z7's authored source-row executor. Unknown selectors, actors and opcodes
 * fail closed; this is not a general CHK interpreter. */
#include "sc64_z7_adapter.h"
#include "sc64_resource_actions.h"

static int sc64_z7_world_location_source(const Sc64Z7WorldContext *ctx,unsigned id,
                                          unsigned *left,unsigned *top,
                                          unsigned *right,unsigned *bottom)
{
    const Sc64Z7SourceLocation *l;
    if (!ctx || !id || id>SC64_Z7_SOURCE_LOCATION_COUNT || !left || !top ||
        !right || !bottom) return 0;
    if (id==SC64_Z7_SOURCE_LOCATION_COUNT) {
        *left=*top=0; *right=SC64_Z7_MAP_COLS*32u; *bottom=SC64_Z7_MAP_ROWS*32u;
        return 1;
    }
    l=&sc64_z7_source_locations[id-1u];
    *left=l->left; *top=l->top; *right=l->right; *bottom=l->bottom;
    return *left<=*right && *top<=*bottom &&
           *right<=(unsigned)race_cols*32u && *bottom<=(unsigned)race_rows*32u &&
           (l->flags==0);
}

static int sc64_z7_world_location(const Sc64Z7WorldContext *ctx,unsigned id,
                                   int *left,int *top,int *right,int *bottom)
{
    unsigned l,t,r,bm;
    if (!left || !top || !right || !bottom ||
        !sc64_z7_world_location_source(ctx,id,&l,&t,&r,&bm)) return 0;
    *left=(int)(l/2u); *top=(int)(t/2u);
    *right=(int)(r/2u); *bottom=(int)(bm/2u);
    return *left<=*right && *top<=*bottom;
}

static int sc64_z7_world_owner_matches(const Sc64Z7WorldContext *ctx,
                                       unsigned selector,int raw_owner)
{
    unsigned slot;
    if (!ctx || raw_owner<0 || raw_owner>=12) return -1;
    slot=(unsigned)raw_owner;
    if (!ctx->source_slot_active[slot]) return -1;
    if (selector<=11u) return selector==slot;
    if (selector==13u) return slot==ctx->source_current_player;
    if (selector==17u) return slot<8u;
    if (selector>=18u && selector<=21u)
        return slot<8u && ctx->source_force_for_slot[slot]==selector-18u;
    return -1;
}

static int sc64_z7_world_owner_resolve(const Sc64Z7WorldContext *ctx,
                                        unsigned selector,unsigned *slot)
{
    unsigned found=12;
    if (!ctx || !slot) return 0;
    if (selector<=11u) found=selector;
    else if (selector==13u) found=ctx->source_current_player;
    else if (selector>=18u && selector<=21u) {
        for (unsigned i=0;i<8;++i) if (ctx->source_slot_active[i] &&
            ctx->source_force_for_slot[i]==selector-18u) {
            if (found!=12u) return 0;
            found=i;
        }
    } else return 0;
    if (found>=12u || !ctx->source_slot_active[found]) return 0;
    *slot=found; return 1;
}

static int sc64_z7_world_type_match(unsigned wanted,unsigned raw,int building)
{
    if (wanted==229u) return 1; /* Any Unit, including buildings. */
    if (wanted==230u) {
        int type;
        if (building) return 0;
        type=sc64_race_unit_from_raw((int)raw);
        if (type<0) return -1;
        return (sc64_game_unit_actor_flags(type)&SC64_GAME_ACTOR_MEN)!=0;
    }
    if (wanted==231u) return building;
    if (wanted>=228u || wanted!=raw) return 0;
    if (building) return sc64_race_building_from_raw((int)raw)>=0;
    return sc64_race_unit_from_raw((int)raw)>=0;
}

/* CHK Bring uses actor rectangles, not actor centers. Imported runtime x/y
 * values are half the source's 32px coordinate scale; keep boundary tests
 * half-open just like the other campaign source adapters. Return exact
 * source-pixel rectangles so odd MRGN edges are not rounded independently. */
static int sc64_z7_world_actor_rect(unsigned raw,int x,int y,
                                     int *left,int *top,int *right,int *bottom)
{
    int type=sc64_race_unit_from_raw((int)raw),l,t,r,b;
    if(type<0 || x<0 || y<0) return 0;
    if(!sc64_zp_actor_bounds_source(type,&l,&t,&r,&b)) {
        /* BWAPI UnitType-pinned.cpp unitDimensions entries indexed by raw id. */
        switch(raw) {
        case 37:l=8;t=4;r=7;b=11;break; case 38:l=10;t=10;r=10;b=12;break;
        case 41:l=11;t=11;r=11;b=11;break; case 42:l=25;t=25;r=24;b=24;break;
        case 43:l=22;t=22;r=21;b=21;break; case 45:l=24;t=24;r=23;b=23;break;
        case 47:l=12;t=12;r=11;b=11;break;
        default:return 0;
        }
    }
    *left=x*2-l; *top=y*2-t;
    *right=x*2+r+1; *bottom=y*2+b+1;
    return 1;
}

static int sc64_z7_world_building_rect(unsigned raw,int x,int y,
                                        int *left,int *top,int *right,int *bottom)
{
    int type=sc64_race_building_from_raw((int)raw),l,t,r,b;
    if(type<0 || x<0 || y<0) return 0;
    if(!sc64_zp_building_bounds_source(type,&l,&t,&r,&b)) {
        /* BWAPI UnitType-pinned.cpp unitDimensions entries indexed by raw id. */
        switch(raw) {
        case 131:case 132:case 133:l=49;t=32;r=49;b=32;break;
        case 135:l=40;t=32;r=40;b=24;break;
        case 137:case 141:l=28;t=32;r=28;b=24;break;
        case 138:l=38;t=28;r=32;b=28;break;
        case 139:l=44;t=32;r=32;b=20;break;
        case 140:l=40;t=32;r=32;b=31;break;
        case 142:l=36;t=28;r=40;b=18;break;
        case 143:case 144:case 146:l=24;t=24;r=23;b=23;break;
        case 149:l=64;t=32;r=63;b=31;break;
        default:return 0;
        }
    }
    *left=x*2-l; *top=y*2-t;
    *right=x*2+r+1; *bottom=y*2+b+1;
    return 1;
}

static int sc64_z7_world_compare(unsigned comparison,u32 value,u32 amount)
{
    switch (comparison) {
    case 0: return value>=amount;
    case 1: return value<=amount;
    case 10: return value==amount;
    default: return -1;
    }
}

static int sc64_z7_world_count(const Sc64Z7WorldContext *ctx,unsigned selector,
                                unsigned wanted,unsigned location,u32 *out)
{
    unsigned sl=0,st=0,sr=(unsigned)WORLD_W*2u,sb=(unsigned)WORLD_H*2u;
    int l,t,r,bm;
    u32 count=0;
    if (!ctx || !out || wanted>231u || selector>21u ||
        (selector>13u && selector<17u) ||
        (location && !sc64_z7_world_location_source(ctx,location,&sl,&st,&sr,&sb))) return 0;
    l=(int)sl; t=(int)st; r=(int)sr; bm=(int)sb;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) {
        int raw=u[i].raw_type>=0?u[i].raw_type:sc64_race_raw_unit(u[i].type);
        int owner=sc64_z7_world_owner_matches(ctx,selector,u[i].raw_owner);
        int al,at,ar,ab;
        if (owner<0) return 0;
        if (!owner) continue;
        if (raw<0 || raw>227 || sc64_race_unit_from_raw(raw)<0) return 0;
        int type_match=sc64_z7_world_type_match(wanted,(unsigned)raw,0);
        if (type_match<0) return 0;
        if (!type_match) continue;
        if (location) {
            if (!sc64_z7_world_actor_rect((unsigned)raw,u[i].x,u[i].y,
                                           &al,&at,&ar,&ab)) return 0;
            if (l>=ar || al>=r || t>=ab || at>=bm) continue;
        }
        ++count;
    }
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive) {
        int raw=b[i].raw_type>=0?b[i].raw_type:sc64_race_raw_building(b[i].type);
        int owner=sc64_z7_world_owner_matches(ctx,selector,b[i].raw_owner);
        int al,at,ar,ab;
        if (owner<0) return 0;
        if (!owner) continue;
        if (raw<0 || raw>227 || sc64_race_building_from_raw(raw)<0) return 0;
        if (!sc64_z7_world_type_match(wanted,(unsigned)raw,1)) continue;
        if (location) {
            if (!sc64_z7_world_building_rect((unsigned)raw,b[i].x,b[i].y,
                                              &al,&at,&ar,&ab)) return 0;
            if (l>=ar || al>=r || t>=ab || at>=bm) continue;
        }
        ++count;
    }
    *out=count; return 1;
}

#if defined(SC64_Z7_COUNT_SNAPSHOT)
/* The five immutable source predicates that actually walk the actor census:
 * loss-any-unit, hostile-force buildings, their named-region building probe,
 * and the two human-force region probes. Keep this exact, source-row-bound,
 * bounded cache private to prevalidated ticks. */
#if defined(SC64_Z7_PRUNE_STARTED)
static int sc64_z7_count_snapshot_begin_full(Sc64Z7WorldContext *ctx)
{
    static const u8 trigger[5]={0,2,4,6,7};
    static const u8 opcode[5]={2,2,3,3,3};
    static const u8 selector[5]={13,19,19,18,18};
    static const u16 wanted[5]={229,231,231,229,229};
    static const u8 location[5]={0,0,4,3,5};
    static const u8 comparison[5]={1,1,1,0,0};
    static const u32 amount[5]={0,0,0,1,1};
    u32 counts[5]={0,0,0,0,0};
    unsigned left[5]={0},top[5]={0},right[5]={0},bottom[5]={0};
    if (!ctx) return 0;
    ctx->count_snapshot.active=0;
    for(unsigned q=0;q<5;++q) ctx->count_snapshot.count[q]=0;
    if (!ctx->source_tables_prevalidated) return 0;
    for (unsigned q=0;q<5;++q) {
        const Sc64Z7SourceCondition *c;
        if (!sc64_z7_condition_row(trigger[q],0,&c) ||
            c->opcode!=opcode[q] || c->player!=selector[q] ||
            c->unit!=wanted[q] || c->location!=location[q] ||
            c->comparison!=comparison[q] || c->amount!=amount[q] ||
            c->resource || c->flags!=16u || c->mask ||
            (location[q] && !sc64_z7_world_location_source(ctx,location[q],
                &left[q],&top[q],&right[q],&bottom[q]))) return 0;
    }
    for (int pass=0;pass<2;++pass) {
        int limit=pass?MAX_BUILDINGS:MAX_UNITS;
        for (int i=0;i<limit;++i) {
            int alive=pass?b[i].alive:u[i].alive;
            int raw_owner=pass?b[i].raw_owner:u[i].raw_owner;
            int x=pass?b[i].x:u[i].x, y=pass?b[i].y:u[i].y;
            int raw,owner13,owner19,owner18,actor_left=0,actor_top=0;
            int actor_right=0,actor_bottom=0,have_rect=0;
            if (!alive) continue;
            raw=pass?(b[i].raw_type>=0?b[i].raw_type:
                      sc64_race_raw_building(b[i].type)):
                     (u[i].raw_type>=0?u[i].raw_type:
                      sc64_race_raw_unit(u[i].type));
            owner13=sc64_z7_world_owner_matches(ctx,13u,raw_owner);
            owner19=sc64_z7_world_owner_matches(ctx,19u,raw_owner);
            owner18=sc64_z7_world_owner_matches(ctx,18u,raw_owner);
            if (owner13<0 || owner19<0 || owner18<0) return 0;
            if (!(owner13 || owner19 || owner18)) continue;
            if (raw<0 || raw>227 || (pass?
                sc64_race_building_from_raw(raw)<0:
                sc64_race_unit_from_raw(raw)<0)) return 0;
            if (owner13) ++counts[0];
            if (pass && owner19) ++counts[1];
            if (pass && owner19) {
                if (!sc64_z7_world_building_rect((unsigned)raw,x,y,&actor_left,
                    &actor_top,&actor_right,&actor_bottom)) return 0;
                have_rect=1;
                if ((int)left[2]<actor_right && actor_left<(int)right[2] &&
                    (int)top[2]<actor_bottom && actor_top<(int)bottom[2])
                    ++counts[2];
            }
            if (owner18) {
                if (!have_rect) {
                    if (pass) {
                        if (!sc64_z7_world_building_rect((unsigned)raw,x,y,
                            &actor_left,&actor_top,&actor_right,&actor_bottom)) return 0;
                    } else if (!sc64_z7_world_actor_rect((unsigned)raw,x,y,
                        &actor_left,&actor_top,&actor_right,&actor_bottom)) return 0;
                    have_rect=1;
                }
                if (owner18 && (int)left[3]<actor_right && actor_left<(int)right[3] &&
                    (int)top[3]<actor_bottom && actor_top<(int)bottom[3])
                    ++counts[3];
                if (owner18 && (int)left[4]<actor_right && actor_left<(int)right[4] &&
                    (int)top[4]<actor_bottom && actor_top<(int)bottom[4])
                    ++counts[4];
            }
        }
    }
    for (unsigned q=0;q<5;++q) ctx->count_snapshot.count[q]=counts[q];
    ctx->count_snapshot.active=31u;
    return 1;
}

static int sc64_z7_count_snapshot_begin_masked(Sc64Z7WorldContext *ctx,
                                               unsigned needed_mask,
                                               int actors_prevalidated)
{
    static const u8 trigger[5]={0,2,4,6,7};
    static const u8 opcode[5]={2,2,3,3,3};
    static const u8 selector[5]={13,19,19,18,18};
    static const u16 wanted[5]={229,231,231,229,229};
    static const u8 location[5]={0,0,4,3,5};
    static const u8 comparison[5]={1,1,1,0,0};
    static const u32 amount[5]={0,0,0,1,1};
    u32 counts[5]={0,0,0,0,0};
    unsigned left[5]={0},top[5]={0},right[5]={0},bottom[5]={0};
    if (!ctx) return 0;
    ctx->count_snapshot.active=0;
    for(unsigned q=0;q<5;++q) ctx->count_snapshot.count[q]=0;
    if (!ctx->source_tables_prevalidated || (needed_mask&~31u) ||
        (actors_prevalidated!=0 && actors_prevalidated!=1)) return 0;
    /* Preserve the full checked reference census when all predicates are
       needed. This common opening case avoids masked-loop overhead. */
    if (needed_mask==31u) return sc64_z7_count_snapshot_begin_full(ctx);
    /* Even omitted counters retain the full canonical source-row and region
       checks. Only their actor census work is pruned. */
    for (unsigned q=0;q<5;++q) {
        const Sc64Z7SourceCondition *c;
        if (!sc64_z7_condition_row(trigger[q],0,&c) ||
            c->opcode!=opcode[q] || c->player!=selector[q] ||
            c->unit!=wanted[q] || c->location!=location[q] ||
            c->comparison!=comparison[q] || c->amount!=amount[q] ||
            c->resource || c->flags!=16u || c->mask ||
            (location[q] && !sc64_z7_world_location_source(ctx,location[q],
                &left[q],&top[q],&right[q],&bottom[q]))) return 0;
    }
    /* Late Z7 often leaves only source player 13's loss check and force 1's
       hostile building check active. After race_z7_live_world_valid() has
       authenticated every actor, these predicates depend only on owner slots;
       neither uses type or geometry. */
    if (needed_mask==3u && actors_prevalidated) {
        unsigned current=ctx->source_current_player;
        if (current>=12u || !ctx->source_slot_active[current]) return 0;
        for (int i=0;i<MAX_UNITS;++i)
            if (u[i].alive && u[i].raw_owner==(int)current) ++counts[0];
        for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive) {
            int raw_owner=b[i].raw_owner;
            if (raw_owner==(int)current) ++counts[0];
            if (raw_owner>=0 && raw_owner<8 && ctx->source_slot_active[raw_owner] &&
                ctx->source_force_for_slot[raw_owner]==1u) ++counts[1];
        }
        for (unsigned q=0;q<5;++q) ctx->count_snapshot.count[q]=counts[q];
        ctx->count_snapshot.active=3u;
        return 1;
    }

    if (needed_mask) for (int pass=0;pass<2;++pass) {
        int limit=pass?MAX_BUILDINGS:MAX_UNITS;
        for (int i=0;i<limit;++i) {
            int alive=pass?b[i].alive:u[i].alive;
            int raw_owner=pass?b[i].raw_owner:u[i].raw_owner;
            int x=pass?b[i].x:u[i].x, y=pass?b[i].y:u[i].y;
            int need13=(needed_mask&1u)!=0;
            int need19=(needed_mask&6u)!=0;
            int need18=(needed_mask&24u)!=0;
            int raw,owner13=0,owner19=0,owner18=0;
            int actor_left=0,actor_top=0,actor_right=0,actor_bottom=0,have_rect=0;
            if (!alive) continue;
            if (need13) owner13=sc64_z7_world_owner_matches(ctx,13u,raw_owner);
            if (need19) owner19=sc64_z7_world_owner_matches(ctx,19u,raw_owner);
            if (need18) owner18=sc64_z7_world_owner_matches(ctx,18u,raw_owner);
            if ((!actors_prevalidated && (owner13<0 || owner19<0 || owner18<0)) ||
                (actors_prevalidated && ((need13&&owner13<0) ||
                                         (need19&&owner19<0) ||
                                         (need18&&owner18<0)))) return 0;
            if (!(owner13 || owner19 || owner18)) continue;
            raw=pass?(b[i].raw_type>=0?b[i].raw_type:sc64_race_raw_building(b[i].type)):
                     (u[i].raw_type>=0?u[i].raw_type:sc64_race_raw_unit(u[i].type));
            if (raw<0 || raw>227 || (pass?sc64_race_building_from_raw(raw)<0:
                                      sc64_race_unit_from_raw(raw)<0)) return 0;
            if ((needed_mask&1u) && owner13) ++counts[0];
            if (pass && (needed_mask&2u) && owner19) ++counts[1];
            if (pass && (needed_mask&4u) && owner19) {
                if (!sc64_z7_world_building_rect((unsigned)raw,x,y,&actor_left,
                    &actor_top,&actor_right,&actor_bottom)) return 0;
                have_rect=1;
                if ((int)left[2]<actor_right && actor_left<(int)right[2] &&
                    (int)top[2]<actor_bottom && actor_top<(int)bottom[2]) ++counts[2];
            }
            if (owner18 && (needed_mask&24u)) {
                if (!have_rect) {
                    if (pass) {
                        if (!sc64_z7_world_building_rect((unsigned)raw,x,y,
                            &actor_left,&actor_top,&actor_right,&actor_bottom)) return 0;
                    } else if (!sc64_z7_world_actor_rect((unsigned)raw,x,y,
                        &actor_left,&actor_top,&actor_right,&actor_bottom)) return 0;
                    have_rect=1;
                }
                if ((needed_mask&8u) && (int)left[3]<actor_right &&
                    actor_left<(int)right[3] && (int)top[3]<actor_bottom &&
                    actor_top<(int)bottom[3]) ++counts[3];
                if ((needed_mask&16u) && (int)left[4]<actor_right &&
                    actor_left<(int)right[4] && (int)top[4]<actor_bottom &&
                    actor_top<(int)bottom[4]) ++counts[4];
            }
        }
    }
    for (unsigned q=0;q<5;++q) ctx->count_snapshot.count[q]=counts[q];
    ctx->count_snapshot.active=(u8)needed_mask;
    return 1;
}

static int sc64_z7_count_snapshot_begin(Sc64Z7WorldContext *ctx)
{ return sc64_z7_count_snapshot_begin_full(ctx); }

/* Caller must be the game adapter after its per-tick actor validation. */
static int sc64_z7_count_snapshot_begin_pruned(Sc64Z7WorldContext *ctx,
                                               u16 started_mask)
{
    static const u8 trigger[5]={0,2,4,6,7};
    unsigned needed=0;
    if (!ctx) return 0;
    ctx->count_snapshot.active=0;
    for(unsigned q=0;q<5;++q) ctx->count_snapshot.count[q]=0;
    if (started_mask&~0xffu) return 0;
    for (unsigned q=0;q<5;++q)
        if (!(started_mask&(1u<<trigger[q]))) needed|=1u<<q;
    return sc64_z7_count_snapshot_begin_masked(ctx,needed,1);
}
#else
static int sc64_z7_count_snapshot_begin(Sc64Z7WorldContext *ctx)
{
    static const u8 trigger[5]={0,2,4,6,7};
    static const u8 opcode[5]={2,2,3,3,3};
    static const u8 selector[5]={13,19,19,18,18};
    static const u16 wanted[5]={229,231,231,229,229};
    static const u8 location[5]={0,0,4,3,5};
    static const u8 comparison[5]={1,1,1,0,0};
    static const u32 amount[5]={0,0,0,1,1};
    u32 counts[5]={0,0,0,0,0};
    unsigned left[5]={0},top[5]={0},right[5]={0},bottom[5]={0};
    if (!ctx) return 0;
    ctx->count_snapshot.active=0;
    for(unsigned q=0;q<5;++q) ctx->count_snapshot.count[q]=0;
    if (!ctx->source_tables_prevalidated) return 0;
    for (unsigned q=0;q<5;++q) {
        const Sc64Z7SourceCondition *c;
        if (!sc64_z7_condition_row(trigger[q],0,&c) ||
            c->opcode!=opcode[q] || c->player!=selector[q] ||
            c->unit!=wanted[q] || c->location!=location[q] ||
            c->comparison!=comparison[q] || c->amount!=amount[q] ||
            c->resource || c->flags!=16u || c->mask ||
            (location[q] && !sc64_z7_world_location_source(ctx,location[q],
                &left[q],&top[q],&right[q],&bottom[q]))) return 0;
    }
    for (int pass=0;pass<2;++pass) {
        int limit=pass?MAX_BUILDINGS:MAX_UNITS;
        for (int i=0;i<limit;++i) {
            int alive=pass?b[i].alive:u[i].alive;
            int raw_owner=pass?b[i].raw_owner:u[i].raw_owner;
            int x=pass?b[i].x:u[i].x, y=pass?b[i].y:u[i].y;
            int raw,owner13,owner19,owner18,actor_left=0,actor_top=0;
            int actor_right=0,actor_bottom=0,have_rect=0;
            if (!alive) continue;
            raw=pass?(b[i].raw_type>=0?b[i].raw_type:
                      sc64_race_raw_building(b[i].type)):
                     (u[i].raw_type>=0?u[i].raw_type:
                      sc64_race_raw_unit(u[i].type));
            owner13=sc64_z7_world_owner_matches(ctx,13u,raw_owner);
            owner19=sc64_z7_world_owner_matches(ctx,19u,raw_owner);
            owner18=sc64_z7_world_owner_matches(ctx,18u,raw_owner);
            if (owner13<0 || owner19<0 || owner18<0) return 0;
            if (!(owner13 || owner19 || owner18)) continue;
            if (raw<0 || raw>227 || (pass?
                sc64_race_building_from_raw(raw)<0:
                sc64_race_unit_from_raw(raw)<0)) return 0;
            if (owner13) ++counts[0];
            if (pass && owner19) ++counts[1];
            if (pass && owner19) {
                if (!sc64_z7_world_building_rect((unsigned)raw,x,y,&actor_left,
                    &actor_top,&actor_right,&actor_bottom)) return 0;
                have_rect=1;
                if ((int)left[2]<actor_right && actor_left<(int)right[2] &&
                    (int)top[2]<actor_bottom && actor_top<(int)bottom[2])
                    ++counts[2];
            }
            if (owner18) {
                if (!have_rect) {
                    if (pass) {
                        if (!sc64_z7_world_building_rect((unsigned)raw,x,y,
                            &actor_left,&actor_top,&actor_right,&actor_bottom)) return 0;
                    } else if (!sc64_z7_world_actor_rect((unsigned)raw,x,y,
                        &actor_left,&actor_top,&actor_right,&actor_bottom)) return 0;
                    have_rect=1;
                }
                if (owner18 && (int)left[3]<actor_right && actor_left<(int)right[3] &&
                    (int)top[3]<actor_bottom && actor_top<(int)bottom[3])
                    ++counts[3];
                if (owner18 && (int)left[4]<actor_right && actor_left<(int)right[4] &&
                    (int)top[4]<actor_bottom && actor_top<(int)bottom[4])
                    ++counts[4];
            }
        }
    }
    for (unsigned q=0;q<5;++q) ctx->count_snapshot.count[q]=counts[q];
    ctx->count_snapshot.active=1;
    return 1;
}
#endif

static int sc64_z7_count_snapshot_lookup(const Sc64Z7WorldContext *ctx,
    const Sc64Z7SourceCondition *c,u32 *count)
{
    static const u8 trigger[5]={0,2,4,6,7};
    if (!ctx || !count || !c || !ctx->source_tables_prevalidated ||
        !ctx->count_snapshot.active) return 0;
    for (unsigned q=0;q<5;++q) if (c->trigger==trigger[q]) {
        const Sc64Z7SourceCondition *canonical;
        if (c->index || !sc64_z7_condition_row(trigger[q],0,&canonical) ||
            canonical!=c) return 0;
#if defined(SC64_Z7_PRUNE_STARTED)
        if (!(ctx->count_snapshot.active&(1u<<q))) return 0;
#endif
        *count=ctx->count_snapshot.count[q];
        return 1;
    }
    return 0;
}

static void sc64_z7_count_snapshot_invalidate(Sc64Z7WorldContext *ctx)
{
    if (!ctx) return;
    ctx->count_snapshot.active=0;
    for(unsigned q=0;q<5;++q) ctx->count_snapshot.count[q]=0;
}
#endif

#if defined(SC64_Z7_PREVALIDATED_TICK)
static int sc64_z7_world_condition_impl(void *context,const Sc64Z7RuntimeState *state,
                                        const Sc64Z7SourceCondition *c,
                                        int source_metadata_checked)
{
    Sc64Z7WorldContext *ctx=(Sc64Z7WorldContext *)context;
    const Sc64Z7SourceCondition *source;
    u32 count=0;
    if (!ctx || !state || !c) return -1;
    if (!source_metadata_checked &&
        (!sc64_z7_condition_row(c->trigger,c->index,&source) || source!=c ||
         !sc64_z7_runtime_metadata_valid())) return -1;
    switch (c->opcode) {
    case 2: case 3:
        if (!source_metadata_checked &&
            (c->resource || c->flags!=16u || c->mask ||
             (c->comparison!=0u && c->comparison!=1u && c->comparison!=10u) ||
             (c->opcode==2 && c->location))) return -1;
#if defined(SC64_Z7_COUNT_SNAPSHOT)
        if (!(source_metadata_checked && sc64_z7_count_snapshot_lookup(ctx,c,&count)) &&
            !sc64_z7_world_count(ctx,c->player,c->unit,c->opcode==3?c->location:0,&count)) return -1;
#else
        if (!sc64_z7_world_count(ctx,c->player,c->unit,c->opcode==3?c->location:0,&count)) return -1;
#endif
        return sc64_z7_world_compare(c->comparison,count,c->amount);
    case 11:
        if (!source_metadata_checked &&
            (c->location || c->player || c->amount || c->unit || c->comparison<2u ||
             c->comparison>3u || c->resource!=10u || c->flags || c->mask)) return -1;
        return c->comparison==2u ? state->switch10 : !state->switch10;
    case 12:
        if (!source_metadata_checked &&
            (c->location || c->player || c->unit || c->resource || c->flags || c->mask ||
             (c->comparison!=0u && c->comparison!=1u && c->comparison!=10u))) return -1;
        return sc64_z7_world_compare(c->comparison,ctx->elapsed_frames/30u,c->amount);
    default: return -1;
    }
}

static int sc64_z7_world_condition(void *context,const Sc64Z7RuntimeState *state,
                                    const Sc64Z7SourceCondition *c)
{ return sc64_z7_world_condition_impl(context,state,c,0); }

/* Only the private authenticated adapter may call this after constructor or
 * rebind has validated the complete immutable scenario table. */
static int sc64_z7_world_condition_prevalidated(void *context,
                                                const Sc64Z7RuntimeState *state,
                                                const Sc64Z7SourceCondition *c)
{ return sc64_z7_world_condition_impl(context,state,c,1); }

static int sc64_z7_world_action_impl(Sc64Z7WorldContext *ctx,Sc64Z7RuntimeState *state,
                                      const Sc64Z7SourceAction *a,
                                      int source_metadata_checked)
{
    const Sc64Z7SourceAction *source;
    if (!ctx || !state || !a) return 0;
    if (!source_metadata_checked &&
        (!sc64_z7_source_action(a->trigger,a->index,&source) || source!=a ||
         !sc64_z7_runtime_metadata_valid())) return 0;
    switch (a->opcode) {
    case 1: ctx->victory_requested=1; return 1;
    case 2: ctx->defeat_requested=1; return 1;
    case 4: return 1; /* Scheduler owns the exact source wait. */
    case 12:
        ctx->objective_refresh_requested=1;
        ctx->last_string_id=(u16)a->string_id;
        return 1;
    case 13:
        if (!source_metadata_checked &&
            (a->group1 || a->group2!=10u || a->modifier!=4u || a->flags)) return 0;
        state->switch10=1; return 1;
    case 16: {
        unsigned bit;
        if (!source_metadata_checked &&
            (a->trigger!=SC64_Z7_TRIGGER_AI || a->index<1u || a->index>4u ||
             a->unit || a->group1 || a->modifier || a->flags)) return 0;
        if (a->index<1u || a->index>4u) return 0;
        bit=1u<<(a->index-1u);
        state->ai_script_mask|=(u8)bit;
        if (!state->ai_plan_ticks) state->ai_plan_ticks=SC64_Z7_AI_PERIOD_TICKS;
        return 1; /* The opaque source script is replaced by runtime policy. */
    }
    case 26: {
        unsigned slot; int owner; Sc64ResourceActionState values;
        if ((!source_metadata_checked &&
             (a->modifier!=7u || a->unit>2u || a->group2>65535u)) ||
            !sc64_z7_world_owner_resolve(ctx,a->group1,&slot)) return 0;
        owner=ctx->live_owner_for_source[slot];
        if (owner<SIDE || owner>4 || (a->group1==19u && (slot!=4u || owner!=ENEMY))) return 0;
        values.current_minerals=(u32)race_minerals[owner];
        values.current_gas=(u32)race_gas[owner];
        values.total_minerals_gathered=values.total_gas_gathered=0;
        if (!sc64_resource_action_apply_one_player(&values,(u16)a->unit,a->modifier,a->group2)) return 0;
        race_minerals[owner]=(int)values.current_minerals;
        race_gas[owner]=(int)values.current_gas;
        if (owner==SIDE) { minerals=race_minerals[owner]; gas=race_gas[owner]; }
        return 1;
    }
    default: return 0;
    }
}

static int sc64_z7_world_action(Sc64Z7WorldContext *ctx,Sc64Z7RuntimeState *state,
                                 const Sc64Z7SourceAction *a)
{ return sc64_z7_world_action_impl(ctx,state,a,0); }

static int sc64_z7_world_action_prevalidated(Sc64Z7WorldContext *ctx,
                                              Sc64Z7RuntimeState *state,
                                              const Sc64Z7SourceAction *a)
{ return sc64_z7_world_action_impl(ctx,state,a,1); }
#else
static int sc64_z7_world_condition(void *context,const Sc64Z7RuntimeState *state,
                                    const Sc64Z7SourceCondition *c)
{
    Sc64Z7WorldContext *ctx=(Sc64Z7WorldContext *)context;
    const Sc64Z7SourceCondition *source;
    u32 count=0;
    if (!ctx || !state || !c || !sc64_z7_condition_row(c->trigger,c->index,&source) ||
        source!=c || !sc64_z7_runtime_metadata_valid()) return -1;
    switch (c->opcode) {
    case 2: case 3:
        if (c->resource || c->flags!=16u || c->mask || (c->comparison!=0u && c->comparison!=1u && c->comparison!=10u) ||
            (c->opcode==2 && c->location)) return -1;
        if (!sc64_z7_world_count(ctx,c->player,c->unit,c->opcode==3?c->location:0,&count)) return -1;
        return sc64_z7_world_compare(c->comparison,count,c->amount);
    case 11:
        if (c->location || c->player || c->amount || c->unit || c->comparison<2u ||
            c->comparison>3u || c->resource!=10u || c->flags || c->mask) return -1;
        return c->comparison==2u ? state->switch10 : !state->switch10;
    case 12:
        if (c->location || c->player || c->unit || c->resource || c->flags || c->mask ||
            (c->comparison!=0u && c->comparison!=1u && c->comparison!=10u)) return -1;
        return sc64_z7_world_compare(c->comparison,ctx->elapsed_frames/30u,c->amount);
    default: return -1;
    }
}

static int sc64_z7_world_action(Sc64Z7WorldContext *ctx,Sc64Z7RuntimeState *state,
                                 const Sc64Z7SourceAction *a)
{
    const Sc64Z7SourceAction *source;
    if (!ctx || !state || !a || !sc64_z7_source_action(a->trigger,a->index,&source) ||
        source!=a || !sc64_z7_runtime_metadata_valid()) return 0;
    switch (a->opcode) {
    case 1: ctx->victory_requested=1; return 1;
    case 2: ctx->defeat_requested=1; return 1;
    case 4: return 1; /* Scheduler owns the exact source wait. */
    case 12:
        ctx->objective_refresh_requested=1;
        ctx->last_string_id=(u16)a->string_id;
        return 1;
    case 13:
        if (a->group1 || a->group2!=10u || a->modifier!=4u || a->flags) return 0;
        state->switch10=1; return 1;
    case 16: {
        unsigned bit;
        if (a->trigger!=SC64_Z7_TRIGGER_AI || a->index<1u || a->index>4u ||
            a->unit || a->group1 || a->modifier || a->flags) return 0;
        bit=1u<<(a->index-1u);
        state->ai_script_mask|=(u8)bit;
        if (!state->ai_plan_ticks) state->ai_plan_ticks=SC64_Z7_AI_PERIOD_TICKS;
        return 1; /* The opaque source script is replaced by runtime policy. */
    }
    case 26: {
        unsigned slot; int owner; Sc64ResourceActionState values;
        if (a->modifier!=7u || a->unit>2u || a->group2>65535u ||
            !sc64_z7_world_owner_resolve(ctx,a->group1,&slot)) return 0;
        owner=ctx->live_owner_for_source[slot];
        if (owner<SIDE || owner>4 || (a->group1==19u && (slot!=4u || owner!=ENEMY))) return 0;
        values.current_minerals=(u32)race_minerals[owner];
        values.current_gas=(u32)race_gas[owner];
        values.total_minerals_gathered=values.total_gas_gathered=0;
        if (!sc64_resource_action_apply_one_player(&values,(u16)a->unit,a->modifier,a->group2)) return 0;
        race_minerals[owner]=(int)values.current_minerals;
        race_gas[owner]=(int)values.current_gas;
        if (owner==SIDE) { minerals=race_minerals[owner]; gas=race_gas[owner]; }
        return 1;
    }
    default: return 0;
    }
}
#endif

#endif
