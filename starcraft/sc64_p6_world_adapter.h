#ifndef SC64_P6_WORLD_ADAPTER_H
#define SC64_P6_WORLD_ADAPTER_H

#include <string.h>
/* Bounded P6 source-world callbacks. This is intentionally mission-specific:
 * opaque AI, door state, and actor-specific properties require explicit
 * host policies instead of silently succeeding. */
#include "sc64_p6_adapter.h"
#include "sc64_p6_actor_rules.h"

typedef struct {
    u8 valid_special, valid_fields, hp_percent, special_value;
    u8 requested_order;
} Sc64P6WorldUnitProperties;

typedef struct {
    int internal_type;
    unsigned implemented_capabilities;
} Sc64P6WorldActorBinding;

typedef struct {
    unsigned raw_type, source_owner;
    int internal_type, live_owner, x, y;
    u8 count;
    u8 has_properties;
    Sc64P6WorldUnitProperties properties;
} Sc64P6WorldCreateRequest;

typedef struct {
    u8 source_current_player;
    u8 source_slot_active[12];
    s8 live_owner_for_source[12];
    u8 source_force_for_slot[8];
    u16 map_cols, map_rows;
    u32 elapsed_frames;
    u32 *deaths_by_source;
    u8 victory_requested, defeat_requested, objective_refresh_requested;
    u8 presentation_opcode, speech_muted;
    u16 last_string_id, last_wav_id, center_x, center_y;
    int (*resolve_actor)(void *context, unsigned raw,
                         Sc64P6WorldActorBinding *binding);
    int (*create_actor)(void *context, const Sc64P6WorldCreateRequest *request);
    int (*is_men_raw)(void *context, unsigned raw);
    int (*run_ai)(void *context, const Sc64P6SourceAction *action);
    int (*set_doodad_state)(void *context, const Sc64P6SourceAction *action);
    int (*actors_removed)(void *context);
    void *callback_context;
} Sc64P6WorldContext;

static int sc64_p6_world_location(const Sc64P6WorldContext *ctx, unsigned id,
                                   int *left, int *top, int *right, int *bottom)
{
    if (!ctx || !left || !top || !right || !bottom ||
        !ctx->map_cols || !ctx->map_rows) return 0;
    if (!sc64_p6_location_runtime(id, ctx->map_cols, ctx->map_rows,
                                  left, top, right, bottom)) return 0;
    return *left < *right && *top < *bottom;
}

static int sc64_p6_world_owner_matches(const Sc64P6WorldContext *ctx,
                                        unsigned selector, int raw_owner,
                                        int live_owner)
{
    unsigned slot;
    if (!ctx || raw_owner < 0 || raw_owner >= 12 ||
        !sc64_p6_selector_valid(selector)) return -1;
    slot = (unsigned)raw_owner;
    if (!ctx->source_slot_active[slot]) return 0;
    if (selector <= 11) return selector == slot;
    if (selector == 13) return live_owner == SIDE;
    if (selector == 17) return slot < 8;
    return slot < 8 && ctx->source_force_for_slot[slot] == selector - 18u;
}

static int sc64_p6_world_owner_resolve(const Sc64P6WorldContext *ctx,
                                        unsigned selector, unsigned *slot_out)
{
    unsigned found = 12;
    if (!ctx || !slot_out) return 0;
    if (selector <= 11) found = selector;
    else if (selector == 13) found = ctx->source_current_player;
    else if (selector >= 18 && selector <= 21) {
        for (unsigned i = 0; i < 8; ++i) {
            if (ctx->source_slot_active[i] &&
                ctx->source_force_for_slot[i] == selector - 18u) {
                if (found != 12) return 0;
                found = i;
            }
        }
    } else return 0; /* All Players is not one spawn owner. */
    if (found >= 12 || !ctx->source_slot_active[found]) return 0;
    *slot_out = found;
    return 1;
}

static int sc64_p6_world_type_match(const Sc64P6WorldContext *ctx,
                                     unsigned wanted, unsigned raw,
                                     int building)
{
    if (wanted == SC64_P6_RAW_ANY_UNIT) return 1;
    if (wanted == SC64_P6_RAW_ANY_NONBUILDING) {
        int men;
        if (building) return 0;
        if (!ctx->is_men_raw) return -1;
        men = ctx->is_men_raw(ctx->callback_context, raw);
        return men == 0 || men == 1 ? men : -1;
    }
    if (wanted == 231u) return building;
    if (wanted >= 228u) return 0;
    return raw == wanted;
}

static int sc64_p6_world_count(const Sc64P6WorldContext *ctx,
                                unsigned selector, unsigned wanted,
                                unsigned location, u32 *out)
{
    int l=0,t=0,r=0,btm=0;
    u32 count=0;
    if (!ctx || !out || !sc64_p6_selector_valid(selector) || wanted > 231u ||
        (location && !sc64_p6_world_location(ctx,location,&l,&t,&r,&btm))) return 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) {
        int raw=u[i].raw_type>=0?u[i].raw_type:sc64_race_raw_unit(u[i].type);
        int owner=sc64_p6_world_owner_matches(ctx,selector,u[i].raw_owner,u[i].owner);
        int type;
        if(owner<0)return 0;
        if(!owner||raw<0)continue;
        type=sc64_p6_world_type_match(ctx,wanted,(unsigned)raw,0);
        if(type<0)return 0;
        if(type&&(!location||(u[i].x>=l&&u[i].x<r&&u[i].y>=t&&u[i].y<btm)))++count;
    }
    for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive){
        int raw=b[i].raw_type>=0?b[i].raw_type:sc64_race_raw_building(b[i].type);
        int owner=sc64_p6_world_owner_matches(ctx,selector,b[i].raw_owner,b[i].owner);
        int type;
        if(owner<0)return 0;
        if(!owner||raw<0)continue;
        type=sc64_p6_world_type_match(ctx,wanted,(unsigned)raw,1);
        if(type<0)return 0;
        if(type&&(!location||(b[i].x>=l&&b[i].x<r&&b[i].y>=t&&b[i].y<btm)))++count;
    }
    *out=count;
    return 1;
}

static int sc64_p6_world_condition(void *context,
                                    const Sc64P6RuntimeState *state,
                                    const Sc64P6SourceCondition *c)
{
    Sc64P6WorldContext *ctx=context;
    const Sc64P6SourceCondition *source;
    u32 count=0;
    if(!ctx||!state||!c||!sc64_p6_condition_row(c->trigger,c->index,&source)||
       source!=c||(c->flags!=0&&c->flags!=0x10u)||c->mask)return -1;
    switch(c->opcode){
    case SC64_P6_CONDITION_COMMAND: case SC64_P6_CONDITION_BRING:
        if(c->resource||!sc64_p6_selector_valid(c->player)||
           (c->opcode==SC64_P6_CONDITION_COMMAND&&(!c->unit||c->location))||
           (c->opcode==SC64_P6_CONDITION_BRING&&!c->location))return -1;
        if(!sc64_p6_world_count(ctx,c->player,c->unit,
             c->opcode==SC64_P6_CONDITION_BRING?c->location:0,&count))return -1;
        return sc64_p6_count_comparison(count,c->comparison,c->amount);
    case SC64_P6_CONDITION_SWITCH:
        if(c->location||c->player||c->amount||c->unit||
           (c->comparison!=2&&c->comparison!=3)||c->flags)return -1;
        return sc64_p6_switch_condition(state,c);
    case SC64_P6_CONDITION_ELAPSED_TIME:
        if(c->location||c->player||c->amount||c->unit||c->comparison||
           c->resource||c->flags)return -1;
        /* P6's authenticated row is ElapsedTime >= 0. The world clock is
         * unsigned, so every representable elapsed frame satisfies it. */
        (void)ctx->elapsed_frames;
        return sc64_p6_count_comparison(ctx->elapsed_frames/30u,
                                       c->comparison,c->amount);
    case SC64_P6_CONDITION_ALWAYS:
        if(c->location||c->player||c->amount||c->unit||c->comparison||
           c->resource||c->flags)return -1;
        return 1;
    default:return -1;
    }
}

static int sc64_p6_world_properties(unsigned id,
                                     Sc64P6WorldUnitProperties *out)
{
    const unsigned char *p;
    if(!out||id<1||id>SC64_P6_PROPERTY_COUNT||
       !sc64_p6_source_property(id,&p)||!p)return 0;
    /* All thirteen P6 UPRPs are byte-identical. Only these fields are
     * interpreted here; special value 2 is passed as an explicit order-117
     * request for the host policy to accept or reject. */
    if(p[0]!=0x1au||p[1]!=0||p[2]!=2u||p[3]!=0||p[4]!=0xffu||p[5]!=100u||
       p[6]||p[7]||p[8]||p[9]||p[10]||p[11]||p[12]||p[13]||p[14]!=2u||
       p[15]||p[16]||p[17]||p[18]||p[19])return 0;
    out->valid_special=p[0];out->valid_fields=p[2];out->hp_percent=p[5];
    out->special_value=p[14];out->requested_order=117;
    return 1;
}

static int sc64_p6_world_create(Sc64P6WorldContext *ctx,
                                 const Sc64P6SourceAction *a)
{
    unsigned owner_slot, property_index;
    int l,t,r,btm,live_owner;
    Sc64P6WorldActorBinding binding;
    Sc64P6WorldCreateRequest request;
    const Sc64P6ActorPolicyRequirement *requirement;
    if(!ctx||!a||a->opcode!=SC64_P6_ACTION_CREATE_UNIT||a->modifier||
       a->unit>=228u||a->flags!=(a->group2?0x18u:0x10u)||
       a->trailing[0]||a->trailing[1]||a->trailing[2]||
       !sc64_p6_world_owner_resolve(ctx,a->group1,&owner_slot)||
       !sc64_p6_world_location(ctx,a->location,&l,&t,&r,&btm)||
       !ctx->resolve_actor||!ctx->create_actor)return 0;
    property_index=a->group2;
    memset(&request,0,sizeof(request));
    if(property_index){
        if(!sc64_p6_world_properties(property_index,&request.properties))return 0;
        request.has_properties=1;
    }
    memset(&binding,0,sizeof(binding));
    if(ctx->resolve_actor(ctx->callback_context,a->unit,&binding)!=1||
       binding.internal_type<0)return 0;
    requirement=sc64_p6_actor_policy_requirement(a->unit);
    if(requirement&&
       (binding.implemented_capabilities&requirement->required_capabilities)!=
        requirement->required_capabilities)return 0;
    if(request.has_properties&&request.properties.requested_order==117u&&
       !(binding.implemented_capabilities&SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY))
        return 0;
    live_owner=ctx->live_owner_for_source[owner_slot];
    if(live_owner<SIDE||live_owner>4)return 0;
    request.raw_type=a->unit;request.source_owner=owner_slot;
    request.internal_type=binding.internal_type;request.live_owner=live_owner;
    request.x=(l+r)/2;request.y=(t+btm)/2;request.count=1;
    return ctx->create_actor(ctx->callback_context,&request)==1;
}

static int sc64_p6_world_kill_current_hero(Sc64P6WorldContext *ctx,
                                            const Sc64P6SourceAction *a)
{
    if(!ctx||!a||a->group1!=13u||a->group2||a->unit!=101u||a->location||
       a->modifier||a->flags!=0x10u||a->trailing[0]||a->trailing[1]||
       a->trailing[2]||!ctx->actors_removed)return 0;
    /* Check ownership errors before the first mutation. */
    for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&
       sc64_p6_world_owner_matches(ctx,13u,u[i].raw_owner,u[i].owner)<0)return 0;
    if(!ctx->actors_removed(ctx->callback_context))return 0;
    for(int i=0;i<MAX_UNITS;++i)if(u[i].alive){
        int raw=u[i].raw_type>=0?u[i].raw_type:sc64_race_raw_unit(u[i].type);
        if(raw!=101||!sc64_p6_world_owner_matches(ctx,13u,u[i].raw_owner,u[i].owner))
            continue;
        int slot=u[i].raw_owner;
        u[i].alive=0;u[i].selected=0;u[i].target=-1;u[i].order=0;
        if(slot>=0&&slot<12&&ctx->deaths_by_source&&
           ctx->deaths_by_source[slot]!=0xffffffffu)++ctx->deaths_by_source[slot];
    }
    return 1;
}

static int sc64_p6_world_action(void *context,const Sc64P6SourceAction *a)
{
    Sc64P6WorldContext *ctx=context;
    const Sc64P6SourceAction *source;
    if(!ctx||!a||!sc64_p6_source_action(a->trigger,a->index,&source)||source!=a)
        return 0;
    switch(a->opcode){
    case SC64_P6_ACTION_VICTORY:
        if(a->location||a->group1||a->group2||a->unit||a->time)return 0;
        ctx->victory_requested=1;return 1;
    case SC64_P6_ACTION_DEFEAT:
        if(a->location||a->group1||a->group2||a->unit||a->time)return 0;
        ctx->defeat_requested=1;return 1;
    case SC64_P6_ACTION_WAIT: case SC64_P6_ACTION_SET_SWITCH:
        return 1; /* The scheduler accounts for waits and mutates switches. */
    case SC64_P6_ACTION_TRANSMISSION: case SC64_P6_ACTION_PLAY_WAV:
    case SC64_P6_ACTION_DISPLAY_TEXT:
        if(a->string_id>65535u||a->wav_id>65535u)return 0;
        ctx->presentation_opcode=a->opcode;
        ctx->last_string_id=(u16)a->string_id;ctx->last_wav_id=(u16)a->wav_id;
        return 1;
    case SC64_P6_ACTION_CENTER_VIEW:{
        int l,t,r,btm;
        if(!sc64_p6_world_location(ctx,a->location,&l,&t,&r,&btm))return 0;
        ctx->center_x=(u16)((l+r)/2);ctx->center_y=(u16)((t+btm)/2);return 1;
    }
    case SC64_P6_ACTION_CREATE_UNIT:return sc64_p6_world_create(ctx,a);
    case SC64_P6_ACTION_OBJECTIVES:ctx->objective_refresh_requested=1;return 1;
    case SC64_P6_ACTION_RUN_AI:
        return ctx->run_ai&&ctx->run_ai(ctx->callback_context,a)==1;
    case SC64_P6_ACTION_KILL_UNIT:return sc64_p6_world_kill_current_hero(ctx,a);
    case SC64_P6_ACTION_MUTE_SPEECH: case SC64_P6_ACTION_UNMUTE_SPEECH:
        if(a->location||a->string_id||a->wav_id||a->time||a->group1||
           a->group2||a->unit||a->modifier||a->flags!=0x02u)return 0;
        ctx->speech_muted=(u8)(a->opcode==SC64_P6_ACTION_MUTE_SPEECH);return 1;
    case SC64_P6_ACTION_SET_DOODAD:
        if(!a->location||a->group1!=17u||a->group2||
           (a->unit<205u||a->unit>208u)||a->modifier||a->flags!=0x10u||
           !ctx->set_doodad_state)return 0;
        {int l,t,r,btm;
         if(!sc64_p6_world_location(ctx,a->location,&l,&t,&r,&btm))return 0;}
        return ctx->set_doodad_state(ctx->callback_context,a)==1;
    default:return 0;
    }
}

#endif
