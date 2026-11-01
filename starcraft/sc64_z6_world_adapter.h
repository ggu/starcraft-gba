#ifndef SC64_Z6_WORLD_ADAPTER_H
#define SC64_Z6_WORLD_ADAPTER_H

/* Z6's source numeric rows resolved against the bounded live game arrays.
 * This is a mission adapter, not a general trigger interpreter. */
#include "sc64_z6_adapter.h"
#include "sc64_resource_actions.h"

typedef struct {
    u8 source_current_player;
    u8 source_slot_active[12];
    s8 live_owner_for_source[12];
    u8 source_force_for_slot[8];
    u32 elapsed_frames;
    u32 *deaths_by_source;
    /* MoveLocation mutates locations 1 and 2 in this mission. Persist these
       eight coordinates and the low two mask bits with the Z6 runtime. */
    u16 moved_location[2][4];
    u8 moved_locations_mask;
    u8 presentation_opcode;
    u8 victory_requested, defeat_requested, objective_refresh_requested;
    u16 last_string_id, last_wav_id;
    u16 center_x, center_y;
    u8 center_requested;
    /* Z6's first UPRP record requests a hallucinated actor. This engine has
       no native hallucination state, so the integration must provide an
       explicit policy callback or the create action fails closed. */
    void (*mark_hallucinated)(void *context, unsigned unit_slot);
    void *hallucination_context;
    /* Unit has no persistent energy register; any UPRP requesting energy
       needs this explicit integration callback as well. */
    void (*set_energy_percent)(void *context, unsigned unit_slot, u8 percent);
    void *energy_context;
    /* AI script IDs are opaque source data; there is no bytecode execution. */
    int (*run_ai_script)(void *context, unsigned script_id,
                         unsigned location_id, unsigned trigger_player);
    void *ai_context;
} Sc64Z6WorldContext;

static int sc64_z6_world_location_source(const Sc64Z6WorldContext *ctx,
                                          unsigned id, unsigned *left,
                                          unsigned *top, unsigned *right,
                                          unsigned *bottom)
{
    const Sc64Z6SourceLocation *loc;
    if (!ctx || !id || id > SC64_Z6_SOURCE_LOCATION_COUNT ||
        !left || !top || !right || !bottom) return 0;
    loc = &sc64_z6_source_locations[id - 1u];
    *left=loc->left;*top=loc->top;*right=loc->right;*bottom=loc->bottom;
    if (id<=2u && (ctx->moved_locations_mask & (1u << (id-1u)))) {
        *left=ctx->moved_location[id-1u][0];*top=ctx->moved_location[id-1u][1];
        *right=ctx->moved_location[id-1u][2];*bottom=ctx->moved_location[id-1u][3];
    }
    if (*left > *right || *top > *bottom || *right > (unsigned)race_cols * 32u ||
        *bottom > (unsigned)race_rows * 32u ||
        (loc->flags != 0 && loc->flags != 0x38u)) return 0;
    return 1;
}

static int sc64_z6_world_location(const Sc64Z6WorldContext *ctx, unsigned id,
                                  int *left, int *top, int *right, int *bottom)
{
    unsigned l,t,r,b;
    if(!left||!top||!right||!bottom||
       !sc64_z6_world_location_source(ctx,id,&l,&t,&r,&b))return 0;
    /* CHK uses source pixels, twice the runtime world-pixel scale. */
    *left=(int)(l/2u);*top=(int)(t/2u);*right=(int)(r/2u);*bottom=(int)(b/2u);
    return *left <= *right && *top <= *bottom;
}

static int sc64_z6_world_owner_matches(const Sc64Z6WorldContext *ctx,
                                        unsigned selector, int raw_owner,
                                        int live_owner)
{
    unsigned slot;
    if (!ctx || raw_owner < 0 || raw_owner >= 12) return -1;
    slot = (unsigned)raw_owner;
    (void)live_owner;
    if (!ctx->source_slot_active[slot]) return 0;
    if (selector <= 11) return selector == slot;
    if (selector == 12) return 0;
    if (selector == 13) return slot == ctx->source_current_player;
    if (selector == 17) return slot < 8;
    if (selector >= 18 && selector <= 21)
        return slot < 8 && ctx->source_force_for_slot[slot] == selector - 18u;
    return -1;
}

static int sc64_z6_world_owner_resolve(const Sc64Z6WorldContext *ctx,
                                        unsigned selector, unsigned *slot)
{
    unsigned found = 12;
    if (!ctx || !slot) return 0;
    if (selector <= 11) found = selector;
    else if (selector == 13) found = ctx->source_current_player;
    else if (selector >= 18 && selector <= 21) {
        for (unsigned i = 0; i < 8; ++i)
            if (ctx->source_slot_active[i] &&
                ctx->source_force_for_slot[i] == selector - 18u) {
                if (found != 12) return 0;
                found = i;
            }
    } else return 0; /* All Players is not one spawn owner. */
    if (found >= 12 || !ctx->source_slot_active[found]) return 0;
    *slot = found;
    return 1;
}

static int sc64_z6_world_compare(unsigned comparison, u32 value, u32 amount)
{
    switch (comparison) {
    case 0: return value >= amount;
    case 1: return value <= amount;
    case 10: return value == amount;
    default: return -1;
    }
}

static int sc64_z6_world_type_match(unsigned wanted, unsigned raw, int building)
{
    if (wanted == 229) return 1; /* Any Unit includes buildings. */
    if (wanted == 230) {
        int type;
        unsigned flags;
        if (building) return 0;
        type=sc64_race_unit_from_raw((int)raw);
        flags=type<0?0:sc64_game_unit_actor_flags(type);
        return !flags || (flags & SC64_GAME_ACTOR_MEN);
    }
    if (wanted == 231) return building; /* Buildings. */
    if (wanted >= 228 || wanted != raw) return 0;
    return building ? sc64_race_building_from_raw((int)raw) >= 0 :
                      sc64_race_unit_from_raw((int)raw) >= 0;
}

static int sc64_z6_world_count(const Sc64Z6WorldContext *ctx,
                                unsigned selector, unsigned raw,
                                unsigned location, u32 *out)
{
    int l = 0, t = 0, r = WORLD_W, btm = WORLD_H;
    u32 count = 0;
    if (!ctx || !out || selector > 21 || (selector > 13 && selector < 17) ||
        raw > 231 || (location && !sc64_z6_world_location(ctx, location,
                                                           &l,&t,&r,&btm))) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int type = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        int owner = sc64_z6_world_owner_matches(ctx, selector, u[i].raw_owner, u[i].owner);
        if (owner < 0) return 0;
        if (owner && type >= 0 && sc64_z6_world_type_match(raw,(unsigned)type,0) &&
            (!location || (u[i].x >= l && u[i].x < r && u[i].y >= t && u[i].y < btm)))
            ++count;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int type = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        int owner = sc64_z6_world_owner_matches(ctx, selector, b[i].raw_owner, b[i].owner);
        if (owner < 0) return 0;
        if (owner && type >= 0 && sc64_z6_world_type_match(raw,(unsigned)type,1) &&
            (!location || (b[i].x >= l && b[i].x < r && b[i].y >= t && b[i].y < btm)))
            ++count;
    }
    *out = count;
    return 1;
}

static int sc64_z6_world_condition(void *context,
                                    const Sc64Z6RuntimeState *state,
                                    const Sc64Z6SourceCondition *c)
{
    Sc64Z6WorldContext *ctx = (Sc64Z6WorldContext *)context;
    const Sc64Z6SourceCondition *source_row;
    u32 count = 0;
    if (!ctx || !state || !c ||
        !sc64_z6_condition_row(c->trigger,c->index,&source_row) || source_row!=c ||
        (c->flags != 0 && c->flags != 0x10u) || c->mask)
        return -1;
    switch (c->opcode) {
    case 2: case 3:
        if (c->resource || (c->opcode==2 && c->location) ||
            (c->comparison!=0 && c->comparison!=1 && c->comparison!=10)) return -1;
        if (!sc64_z6_world_count(ctx,c->player,c->unit,
                                 c->opcode == 3 ? c->location : 0,&count)) return -1;
        return sc64_z6_world_compare(c->comparison,count,c->amount);
    case 11:
        if (c->flags || c->location || c->player || c->amount || c->unit ||
            c->comparison < 2 || c->comparison > 3)
            return -1;
        count = sc64_z6_switch_is_set(state,c->resource) ? 1u : 0u;
        return c->comparison == 2 ? (int)count : !(int)count;
    case 12:
        if (c->location || c->player || c->unit || c->resource || c->flags ||
            (c->comparison!=0 && c->comparison!=1 && c->comparison!=10)) return -1;
        return sc64_z6_world_compare(c->comparison,ctx->elapsed_frames / 30u,c->amount);
    default: return -1;
    }
}

static int sc64_z6_world_clear_selected(Sc64Z6WorldContext *ctx,
                                         unsigned selector, unsigned raw,
                                         unsigned location)
{
    int l = 0, t = 0, r = WORLD_W, btm = WORLD_H, changed = 0;
    if (!ctx || raw > 231 ||
        (location && !sc64_z6_world_location(ctx,location,&l,&t,&r,&btm))) return 0;
    /* Validate all source owners before deleting anything. */
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
        sc64_z6_world_owner_matches(ctx,selector,u[i].raw_owner,u[i].owner) < 0) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
        sc64_z6_world_owner_matches(ctx,selector,b[i].raw_owner,b[i].owner) < 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int owner = sc64_z6_world_owner_matches(ctx,selector,u[i].raw_owner,u[i].owner);
        int type = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        if (!owner || type < 0 || !sc64_z6_world_type_match(raw,(unsigned)type,0) ||
            (location && (u[i].x < l || u[i].x >= r || u[i].y < t || u[i].y >= btm))) continue;
        if (ctx->deaths_by_source && u[i].raw_owner >= 0 && u[i].raw_owner < 12 &&
            ctx->deaths_by_source[u[i].raw_owner] != 0xffffffffu)
            ++ctx->deaths_by_source[u[i].raw_owner];
#ifdef SC64_TRANSPORT_RUNTIME
        /* The authored Kill Unit At Location can select a live Overlord.
         * Resolve its passengers before invalidating or reusing its slot. */
        if (race_transport_carrier_type(&u[i])) race_transport_destroy_cargo(i);
#endif
        u[i].alive=0; u[i].selected=0; u[i].target=-1; u[i].order=ORDER_IDLE;
        invincible_units[i]=0; changed=1;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int owner = sc64_z6_world_owner_matches(ctx,selector,b[i].raw_owner,b[i].owner);
        int type = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        if (!owner || type < 0 || !sc64_z6_world_type_match(raw,(unsigned)type,1) ||
            (location && (b[i].x < l || b[i].x >= r || b[i].y < t || b[i].y >= btm))) continue;
        if (ctx->deaths_by_source && b[i].raw_owner >= 0 && b[i].raw_owner < 12 &&
            ctx->deaths_by_source[b[i].raw_owner] != 0xffffffffu)
            ++ctx->deaths_by_source[b[i].raw_owner];
        b[i].alive=0; invincible_buildings[i]=0;
        if (selected_building == i) selected_building=-1;
        changed=1;
    }
    if (changed) {
        supply_used=race_owner_supply(SIDE); supply_cap=race_owner_supply_cap(SIDE);
        target_lists_valid=0; sc64_unit_bucket_active=0; sc64_target_index_end();
        invalidate_path_grid();
    }
    return 1;
}

static int sc64_z6_world_set_resources(const Sc64Z6WorldContext *ctx,
                                        const Sc64Z6SourceAction *a)
{
    u8 prepared[5]={0,0,0,0,0};
    u32 next_minerals[5]={0,0,0,0,0},next_gas[5]={0,0,0,0,0};
    int matched=0;
    if (!ctx || !a || a->opcode != 26 || a->location || a->flags ||
        a->group2 > 65535u || a->unit > SC64_RESOURCE_BOTH ||
        a->modifier < SC64_RESOURCE_MOD_SET || a->modifier > SC64_RESOURCE_MOD_SUBTRACT)
        return 0;
    for (unsigned slot=0;slot<12;slot++) {
        int selected=sc64_z6_world_owner_matches(ctx,a->group1,(int)slot,
                                                  ctx->live_owner_for_source[slot]);
        int owner, before_m, before_g;
        Sc64ResourceActionState value;
        if (selected<0) return 0;
        if (!selected) continue;
        ++matched; owner=ctx->live_owner_for_source[slot];
        if (owner<SIDE || owner>4) return 0;
        if (prepared[owner]) continue;
        before_m=owner==SIDE?minerals:race_minerals[owner];
        before_g=owner==SIDE?gas:race_gas[owner];
        if (before_m<0 || before_g<0) return 0;
        value=(Sc64ResourceActionState){(u32)before_m,(u32)before_g,0,0};
        if (!sc64_resource_action_apply_one_player(&value,(u16)a->unit,a->modifier,a->group2) ||
            value.current_minerals>0x7fffffffu || value.current_gas>0x7fffffffu) return 0;
        next_minerals[owner]=value.current_minerals;
        next_gas[owner]=value.current_gas;
        prepared[owner]=1;
    }
    if(!matched)return 0;
    for(int owner=SIDE;owner<=4;owner++)if(prepared[owner]) {
        if (a->unit==SC64_RESOURCE_MINERALS || a->unit==SC64_RESOURCE_BOTH) {
            race_minerals[owner]=(int)next_minerals[owner];
            if (owner==SIDE) minerals=(int)next_minerals[owner];
        }
        if (a->unit==SC64_RESOURCE_GAS || a->unit==SC64_RESOURCE_BOTH) {
            race_gas[owner]=(int)next_gas[owner];
            if (owner==SIDE) gas=(int)next_gas[owner];
        }
    }
    return 1;
}

static int sc64_z6_world_property_valid(unsigned number, unsigned raw)
{
    const unsigned char *p;
    if(number==0)return raw==101;
    if (number<1 || number>64 || !sc64_z6_source_property_used[number-1]) return 0;
    p=sc64_z6_source_properties[number-1];
    if (p[0]!=0x18 || p[1]!=0 || p[3]!=0 || p[4]!=0xff ||
        p[5]!=100 || p[8] || p[9] || p[10] || p[11] || p[12] || p[13] ||
        p[15] || p[16] || p[17] || p[18] || p[19]) return 0;
    if (number==1) return raw==79 && p[2]==0x0e && p[6]==100 && p[7]==100 && p[14]==8;
    if (number==2) return raw==101 && p[2]==0x02 && p[6]==0 && p[7]==0 && p[14]==0;
    return 0;
}

static int sc64_z6_world_create(const Sc64Z6WorldContext *ctx,
                                 const Sc64Z6SourceAction *a)
{
    unsigned owner_slot;
    int l,t,r,btm,type,owner,slot;
    if (!ctx || !a || a->opcode!=11 || a->modifier || a->unit>=228 ||
        a->flags!=(a->group2?0x18u:0x10u) ||
        a->trailing[0] || a->trailing[1] || a->trailing[2] ||
        !sc64_z6_world_property_valid(a->group2,a->unit) ||
        !sc64_z6_world_owner_resolve(ctx,a->group1,&owner_slot) ||
        !sc64_z6_world_location(ctx,a->location,&l,&t,&r,&btm)) return 0;
    type=sc64_race_unit_from_raw(a->unit); owner=ctx->live_owner_for_source[owner_slot];
    if (type<0 || owner<SIDE || owner>4 || find_unit_slot()<0 ||
        (a->group2==1 && (!ctx->mark_hallucinated || !ctx->set_energy_percent))) return 0;
    slot=spawn_unit(owner,type,(l+r)/2,(t+btm)/2);
    if (slot<0) return 0;
    u[slot].raw_type=(s16)a->unit; u[slot].raw_owner=(s16)owner_slot;
    if(a->group2) {
        u[slot].hp=u[slot].maxhp*sc64_z6_source_properties[a->group2-1u][5]/100;
        if(sc64_z6_source_properties[a->group2-1u][2]&0x04u)
            u[slot].shield=u[slot].shield*sc64_z6_source_properties[a->group2-1u][6]/100;
        if(sc64_z6_source_properties[a->group2-1u][2]&0x08u)
            ctx->set_energy_percent(ctx->energy_context,(unsigned)slot,
                                    sc64_z6_source_properties[a->group2-1u][7]);
        if (a->group2==1) ctx->mark_hallucinated(ctx->hallucination_context,(unsigned)slot);
    }
    return 1;
}

static int sc64_z6_world_move_location(Sc64Z6WorldContext *ctx,
                                        const Sc64Z6SourceAction *a)
{
    unsigned sl,st,sr,sb,tl,tt,tr,tb;
    int found=0;
    unsigned px=0,py=0;
    unsigned owner_selector;
    if (!ctx || !a || a->opcode!=38 || a->string_id || a->wav_id || a->time ||
        a->modifier || a->flags || a->trailing[0] || a->trailing[1] || a->trailing[2] ||
        a->group2<1 || a->group2>2 || a->unit>=228 ||
        !sc64_z6_world_location_source(ctx,a->location,&sl,&st,&sr,&sb) ||
        !sc64_z6_world_location_source(ctx,a->group2,&tl,&tt,&tr,&tb)) return 0;
    owner_selector=a->group1;
    for (int i=0;i<MAX_UNITS;i++) if (u[i].alive) {
        int owner=sc64_z6_world_owner_matches(ctx,owner_selector,u[i].raw_owner,u[i].owner);
        int raw=u[i].raw_type>=0?u[i].raw_type:sc64_race_raw_unit(u[i].type);
        if (owner<0) return 0;
        if (owner && raw==(int)a->unit && (unsigned)u[i].x*2u>=sl &&
            (unsigned)u[i].x*2u<sr && (unsigned)u[i].y*2u>=st &&
            (unsigned)u[i].y*2u<sb) {
            px=(unsigned)u[i].x*2u;py=(unsigned)u[i].y*2u;found=1;break;
        }
    }
    /* Native behavior falls back to the source rectangle center. Translate
       the target rectangle by the point's displacement from that center. */
    if(!found){px=(sl+sr)/2u;py=(st+sb)/2u;}
    {
        unsigned width=tr-tl,height=tb-tt;
        int left=(int)tl+(int)px-(int)((sl+sr)/2u);
        int top=(int)tt+(int)py-(int)((st+sb)/2u);
        int maxx=race_cols*32,maxy=race_rows*32;
        unsigned target=a->group2-1u;
        if(!width||!height||width>=(unsigned)maxx||height>=(unsigned)maxy)return 0;
        if(left<0)left=0;if(top<0)top=0;
        if((unsigned)left+width>=(unsigned)maxx)left=maxx-1-(int)width;
        if((unsigned)top+height>=(unsigned)maxy)top=maxy-1-(int)height;
        ctx->moved_location[target][0]=(u16)left;
        ctx->moved_location[target][1]=(u16)top;
        ctx->moved_location[target][2]=(u16)(left+(int)width);
        ctx->moved_location[target][3]=(u16)(top+(int)height);
        ctx->moved_locations_mask|=(u8)(1u<<target);
    }
    return 1;
}

static int sc64_z6_world_action(void *context, const Sc64Z6SourceAction *a)
{
    Sc64Z6WorldContext *ctx=(Sc64Z6WorldContext *)context;
    const Sc64Z6SourceAction *source_row;
    if (!ctx || !a || !sc64_z6_source_action(a->trigger,a->index,&source_row) ||
        source_row!=a) return 0;
    switch (a->opcode) {
    case 1: ctx->victory_requested=1;return 1;
    case 2: ctx->defeat_requested=1;return 1;
    case 4: case 13: return 1; /* Scheduler applies wait and switch state. */
    case 7: case 8: case 9:
        if(a->string_id>65535u||a->wav_id>65535u)return 0;
        ctx->presentation_opcode=a->opcode;
        ctx->last_string_id=(u16)a->string_id;ctx->last_wav_id=(u16)a->wav_id;
        return 1; /* Text/audio playback belongs to the presentation layer. */
    case 10: {
        int l,t,r,b;
        if(!sc64_z6_world_location(ctx,a->location,&l,&t,&r,&b))return 0;
        ctx->center_x=(u16)((l+r)/2);ctx->center_y=(u16)((t+b)/2);
        ctx->center_requested=1;return 1;
    }
    case 11: return sc64_z6_world_create(ctx,a);
    case 12: ctx->objective_refresh_requested=1;return 1;
    case 16:
        if (!ctx->run_ai_script || a->location>SC64_Z6_SOURCE_LOCATION_COUNT) return 0;
        return ctx->run_ai_script(ctx->ai_context,a->group2,a->location,
                                  ctx->source_current_player)==1;
    case 23: return sc64_z6_world_clear_selected(ctx,a->group1,a->unit,a->location);
    case 26: return sc64_z6_world_set_resources(ctx,a);
    case 38: return sc64_z6_world_move_location(ctx,a);
    default: return 0;
    }
}

#endif
