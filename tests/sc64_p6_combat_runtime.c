/* Real-engine array checks for the optional P6 combat helpers. These test
 * bounded actor interactions, not a full controller win or native animation. */
#include <assert.h>
#include "../starcraft/game.c"

#ifndef SC64_P6_COMBAT_ENABLED
#error "Compile this fixture with the P6 combat profile flags"
#endif

static void reset_combat(void)
{
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    memset(tracers,0,sizeof(tracers));
    memset(invincible_units,0,sizeof(invincible_units));
    memset(invincible_buildings,0,sizeof(invincible_buildings));
    memset(&race_p6,0,sizeof(race_p6));
    memset(race_p6_doodad_bindings,0xff,sizeof(race_p6_doodad_bindings));
    race_mode=0; race_selected_mission=SC64_P6_GLOBAL_ID;
    sc64_p6_runtime_init(&race_p6.runtime);
    race_p6_world.source_current_player=7;
    race_p6_world.map_cols=128; race_p6_world.map_rows=128;
}

static void bind_enabled_doodad(unsigned raw,int slot)
{
    for(unsigned row=0;row<SC64_P6_DOODAD_SOURCE_COUNT;++row) {
        if(sc64_p6_doodads[row].raw_type!=raw || sc64_p6_doodads[row].sprite_only) continue;
        race_p6_doodad_bindings[row].building_slot=(u8)slot;
        race_p6.doodad_enabled[row>>3]|=(u8)(1u<<(row&7u));
        return;
    }
    assert(!"fixture raw doodad row exists");
}

static void disable_doodad(unsigned raw)
{
    for(unsigned row=0;row<SC64_P6_DOODAD_SOURCE_COUNT;++row) {
        if(sc64_p6_doodads[row].raw_type!=raw || sc64_p6_doodads[row].sprite_only) continue;
        race_p6.doodad_enabled[row>>3]&=(u8)~(1u<<(row&7u));
        return;
    }
    assert(!"fixture raw doodad row exists");
}

static void source_box_geometry(void)
{
    Sc64Z5DetectBox a={100,100,8,9,8,10};
    Sc64Z5DetectBox b0={100+8+8,100,7,10,7,11};
    Sc64Z5DetectBox b1={100+8+8+1,100,7,10,7,11};
    assert(sc64_z5_box_distance(&a,&b0)==0);
    assert(sc64_z5_box_distance(&a,&b1)==1);
    assert(race_p6_combat_common_bounds(32,&a.left,&a.up,&a.right,&a.down));
    assert(a.left==11 && a.up==7 && a.right==11 && a.down==14);
    assert(race_p6_combat_common_bounds(65,&a.left,&a.up,&a.right,&a.down));
    assert(a.left==11 && a.up==5 && a.right==11 && a.down==13);
    assert(race_p6_combat_common_bounds(66,&a.left,&a.up,&a.right,&a.down));
    assert(a.left==15 && a.up==15 && a.right==16 && a.down==16);
    assert(race_p6_combat_common_bounds(79,&a.left,&a.up,&a.right,&a.down));
    assert(a.left==12 && a.up==10 && a.right==11 && a.down==13);
    assert(!race_p6_combat_common_bounds(999,&a.left,&a.up,&a.right,&a.down));
}

static void properties_and_saved_state(void)
{
    Sc64P6WorldUnitProperties p={0x1a,2,100,2,117};
    reset_combat();
    u[0].alive=1;u[0].owner=ENEMY;u[0].raw_owner=3;
    u[0].type=sc64_race_unit_from_raw(37);u[0].raw_type=37;
    u[0].maxhp=100;u[0].hp=41;u[0].target=7;
    assert(race_p6_apply_source_properties(0,&p)==1);
    assert(u[0].hp==100 && (u[0].flags&2u) && u[0].order==ORDER_IDLE && u[0].target==-1);
    assert(race_p6_combat_unit_burrowed(0));
    assert(race_p6_combat_saved_actor_valid(37,u[0].flags,u[0].order,u[0].target,0));
    assert(!race_p6_combat_saved_actor_valid(37,u[0].flags,ORDER_ATTACK_MOVE,1,0));
    p.requested_order=116;
    assert(!race_p6_apply_source_properties(0,&p));
    assert(race_p6_combat_saved_actor_valid(74,0,ORDER_IDLE,-1,0));
    assert(race_p6_combat_saved_actor_valid(75,0,ORDER_IDLE,-1,0));
    assert(race_p6_combat_saved_actor_valid(210,0,ORDER_IDLE,-1,28));
    assert(!race_p6_combat_saved_actor_valid(210,0,ORDER_IDLE,-1,29));
}

static void all_source_detector_props_use_boxes(void)
{
    static const unsigned raws[]={203,209,211,213,210};
    static const unsigned ranges[]={192,192,96,96,192};
    for(unsigned n=0;n<sizeof(raws)/sizeof(raws[0]);++n) {
        int slot=0;
        reset_combat();
        u[0].alive=1;u[0].owner=ENEMY;u[0].raw_owner=3;
        u[0].type=sc64_race_unit_from_raw(74);u[0].raw_type=74;
        u[0].x=300+(int)ranges[n]+100;u[0].y=300;
        b[slot].alive=1;b[slot].owner=SIDE;b[slot].raw_owner=7;
        b[slot].raw_type=(s16)raws[n];
        b[slot].type=sc64_race_building_from_raw((int)raws[n]);
        assert(b[slot].type>=0);
        b[slot].x=300;b[slot].y=300;
        Sc64Z5DetectBox target_box;
        assert(race_p6_combat_unit_box(&u[0],&target_box));
        Sc64Z5DetectBox detector_box;
        assert(race_p6_combat_building_box(&b[slot],&detector_box));
        assert(sc64_z5_box_distance(&target_box,&detector_box)>ranges[n]);
        assert(!race_p6_combat_owner_detects(SIDE,&target_box));
        u[0].x=300+(int)ranges[n]/2;
        assert(race_p6_combat_unit_box(&u[0],&target_box));
        assert(sc64_z5_box_distance(&target_box,&detector_box)<=ranges[n]);
        assert(!race_p6_combat_owner_detects(SIDE,&target_box));
        assert(!race_p6_combat_target_detected(0,SIDE));
        bind_enabled_doodad(raws[n],slot);
        assert(race_p6_combat_owner_detects(SIDE,&target_box));
        assert(race_p6_combat_target_detected(0,SIDE));
        disable_doodad(raws[n]);
        assert(!race_p6_combat_owner_detects(SIDE,&target_box));
        assert(!race_p6_combat_target_detected(0,SIDE));
    }
}

static void explosion_uses_nearest_bbox_octagonal_distance_and_deaths(void)
{
    reset_combat();
    u[0].alive=1;u[0].owner=ENEMY;u[0].raw_owner=3;
    u[0].type=U_P6_INFESTED_TERRAN;u[0].raw_type=50;u[0].x=100;u[0].y=100;
    u[0].maxhp=60;u[0].hp=60;
    u[1].alive=1;u[1].owner=SIDE;u[1].raw_owner=7;
    u[1].type=U_MARINE;u[1].raw_type=0;u[1].x=110;u[1].y=100;
    u[1].hp=100;u[1].maxhp=100;u[1].armor=0;
    assert(race_p6_combat_infested_explode(0)==2);
    assert(tracers[0].life>0 &&
        sc64_weapon_vfx_style(tracers[0].color)==SC64_WEAPON_VFX_STYLE_BURST);
    assert(!u[0].alive && !u[1].alive);
    assert(race_p6.deaths_by_source[7]==1);
    assert(sc64_p6_infested_splash_tier(20)==SC64_P6_SPLASH_INNER);

    reset_combat();
    u[0].alive=1;u[0].owner=ENEMY;u[0].raw_owner=3;
    u[0].type=U_P6_INFESTED_TERRAN;u[0].raw_type=50;u[0].x=100;u[0].y=100;
    u[1].alive=1;u[1].owner=SIDE;u[1].raw_owner=7;
    u[1].type=U_ZERGLING;u[1].raw_type=37;u[1].x=130;u[1].y=100;
    u[1].hp=100;u[1].maxhp=100;u[1].armor=0;u[1].flags|=2u;
    (void)race_p6_combat_infested_explode(0);
    assert(u[1].alive); /* Burrowed target outside inner band is immune. */
}

static void normal_attack_entry_and_trap_cooldown(void)
{
    reset_combat();
    u[0].alive=1;u[0].owner=ENEMY;u[0].raw_owner=3;
    u[0].type=U_P6_INFESTED_TERRAN;u[0].raw_type=50;u[0].x=100;u[0].y=100;
    u[1].alive=1;u[1].owner=SIDE;u[1].raw_owner=7;
    u[1].type=U_MARINE;u[1].raw_type=0;u[1].x=110;u[1].y=100;
    u[1].hp=100;u[1].maxhp=100;
    tick_combat(&u[0],1);
    assert(!u[0].alive && !u[1].alive); /* Actual generic tick entry dispatches the source event. */

    reset_combat();
    b[0].alive=1;b[0].owner=ENEMY;b[0].raw_owner=3;
    b[0].raw_type=210;b[0].type=sc64_race_building_from_raw(210);
    b[0].x=100;b[0].y=100;
    bind_enabled_doodad(210,0);
    u[0].alive=1;u[0].owner=SIDE;u[0].raw_owner=7;
    u[0].type=U_MARINE;u[0].raw_type=0;u[0].x=110;u[0].y=100;
    u[0].hp=100;u[0].maxhp=100;
    assert(race_p6_combat_tick_traps()==1);
    assert(tracers[0].life>0 &&
        sc64_weapon_vfx_style(tracers[0].color)==SC64_WEAPON_VFX_STYLE_MISSILE);
    int after_first=u[0].hp;
    assert(after_first<100 && b[0].shield_remainder==28);
    for(int i=0;i<27;++i) assert(race_p6_combat_tick_traps()==0);
    assert(u[0].hp==after_first && b[0].shield_remainder==1);
    assert(race_p6_combat_tick_traps()==1);
    assert(u[0].hp<after_first);

    reset_combat();
    b[0].alive=1;b[0].owner=ENEMY;b[0].raw_owner=3;
    b[0].raw_type=210;b[0].type=sc64_race_building_from_raw(210);
    bind_enabled_doodad(210,0);
    u[0].alive=1;u[0].owner=SIDE;u[0].raw_owner=7;
    u[0].type=U_ZERGLING;u[0].raw_type=37;u[0].x=100;u[0].y=100;
    u[0].hp=100;u[0].maxhp=100;u[0].flags|=2u;
    assert(race_p6_combat_tick_traps()==0);
    assert(u[0].hp==100 && b[0].shield_remainder==0);
}

int main(void)
{
    source_box_geometry();
    properties_and_saved_state();
    all_source_detector_props_use_boxes();
    explosion_uses_nearest_bbox_octagonal_distance_and_deaths();
    normal_attack_entry_and_trap_cooldown();
    return 0;
}
