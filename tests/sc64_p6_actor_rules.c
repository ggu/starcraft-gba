#include <assert.h>
#include "../starcraft/sc64_p6_actor_rules.h"
#ifdef SC64_P6_SOURCE_AUDIT
#include "../.local/sc64/zp-stage/generated/scenarios/sc64_p6_scenario_generated.h"
#endif

static void test_added_source_facts(void)
{
    const Sc64P6AddedActorFact *f;
    f=sc64_p6_added_actor_fact(SC64_P6_RAW_WALL_MISSILE_TRAP);
    assert(f && f->raw==210 && f->hp==50 && f->shields==0 && f->armor==0);
    assert(f->sight_tiles==6 && f->tile_width==1 && f->tile_height==1);
    assert(f->left==16 && f->up==16 && f->right==15 && f->down==15);
    assert(!f->flying && f->size==2);
    assert(f->ground_weapon==97 && f->ground_damage==10 && f->ground_bonus==1);
    assert(f->ground_cooldown==22 && f->ground_range==160 && f->ground_hits==1);
    assert(f->ground_damage_type==SC64_P6_WEAPON_DAMAGE_EXPLOSIVE);
    assert(f->ground_target_flags==SC64_P6_WEAPON_TARGET_GROUND);
    assert(!f->ground_splash_inner && !f->ground_splash_middle &&
           !f->ground_splash_outer);
    assert(!f->air_weapon && !f->air_damage && !f->air_bonus &&
           !f->air_cooldown && !f->air_range && !f->air_hits);
    assert(f->flags==(SC64_P6_SOURCE_DETECTOR|SC64_P6_SOURCE_HUGE_UNIT|
                      SC64_P6_SOURCE_MECHANICAL));
    /* Source defines a weapon but lacks both Attack and AutoAttackAndMove. */
    assert(!(f->flags&(SC64_P6_SOURCE_ATTACK|SC64_P6_SOURCE_AUTO_ATTACK)));

    f=sc64_p6_added_actor_fact(50);
    assert(f && f->hp==60 && f->shields==0 && f->armor==0 && f->sight_tiles==5);
    assert(f->minerals==100 && f->gas==50 && f->build_frames==600);
    assert(f->tile_width==1 && f->tile_height==1);
    assert(f->left==8 && f->up==9 && f->right==8 && f->down==10);
    assert(!f->flying && f->size==0);
    assert(f->ground_weapon==54 && f->ground_damage==500 && f->ground_bonus==0);
    assert(f->ground_cooldown==1 && f->ground_range==3 && f->ground_hits==1);
    assert(f->ground_damage_type==SC64_P6_WEAPON_DAMAGE_EXPLOSIVE);
    assert(f->ground_target_flags==(SC64_P6_WEAPON_TARGET_GROUND|
                                    SC64_P6_WEAPON_RADIAL_SPLASH));
    assert(f->air_weapon==0 && f->air_hits==0);
    assert((f->flags&(SC64_P6_SOURCE_REGENERATES_HP|SC64_P6_SOURCE_BURROWABLE))==
           (SC64_P6_SOURCE_REGENERATES_HP|SC64_P6_SOURCE_BURROWABLE));

    f=sc64_p6_added_actor_fact(74);
    assert(f && f->hp==40 && f->shields==80 && f->armor==0 && f->sight_tiles==7);
    assert(f->minerals==150 && f->gas==150 && f->build_frames==750);
    assert(f->left==12 && f->up==6 && f->right==11 && f->down==19);
    assert(!f->flying && f->size==0);
    assert(f->ground_weapon==86 && f->ground_damage==45 && f->ground_bonus==1);
    assert(f->ground_cooldown==30 && f->ground_range==15 && f->ground_hits==1);
    assert(f->ground_target_flags==SC64_P6_WEAPON_TARGET_GROUND);
    assert(f->flags&SC64_P6_SOURCE_PERMANENT_CLOAK);
    assert(!(f->flags&SC64_P6_SOURCE_HERO));

    f=sc64_p6_added_actor_fact(75);
    assert(f && f->hp==60 && f->shields==400 && f->armor==0 && f->sight_tiles==7);
    assert(f->minerals==100 && f->gas==300 && f->build_frames==1500);
    assert(f->left==12 && f->up==6 && f->right==11 && f->down==19);
    assert(!f->flying && f->size==0);
    assert(f->ground_weapon==85 && f->ground_damage==100 && f->ground_bonus==1);
    assert(f->ground_cooldown==22 && f->ground_range==15 && f->ground_hits==1);
    assert(f->flags&(SC64_P6_SOURCE_HERO|SC64_P6_SOURCE_PERMANENT_CLOAK));
    assert(!f->air_weapon && !f->air_hits);
    assert(!sc64_p6_added_actor_fact(1) && !sc64_p6_added_actor_fact(195));
}

static void test_inherited_source_facts(void)
{
    const Sc64Z5ActorFact *ghost=sc64_p6_shared_actor_fact(1);
    const Sc64Z5BuildingFact *fact;
    assert(ghost && ghost->raw==1 && ghost->hp==45 &&
           (ghost->flags&SC64_Z5_SOURCE_CLOAKABLE));
    assert(!sc64_p6_shared_actor_fact(50));
    fact=sc64_p6_shared_building_fact(195);
    assert(fact && fact->raw==195 && fact->width_tiles==3 && fact->height_tiles==2);
    assert(fact->hp==100000 && (fact->flags&SC64_Z5_SOURCE_INVINCIBLE));
    assert(sc64_p6_source_is_building(195));
    for (int raw=205;raw<=208;++raw) {
        fact=sc64_p6_shared_building_fact(raw);
        assert(fact && fact->raw==raw && fact->width_tiles==3 && fact->height_tiles==2);
        assert(fact->hp==100000 && (fact->flags&SC64_Z5_SOURCE_INVINCIBLE));
        assert(!sc64_p6_source_is_building(raw));
    }
    assert(sc64_p6_source_is_building(196));
    assert(!sc64_p6_shared_building_fact(196));
    assert(!sc64_p6_source_is_building(SC64_P6_RAW_WALL_MISSILE_TRAP));
    assert(!sc64_p6_shared_building_fact(SC64_P6_RAW_WALL_MISSILE_TRAP));
}

static void test_policy_requirements(void)
{
    const Sc64P6ActorPolicyRequirement *p;
    p=sc64_p6_actor_policy_requirement(SC64_P6_RAW_WALL_MISSILE_TRAP);
    assert(p && p->required_capabilities==
           SC64_P6_POLICY_NEEDS_WALL_TRAP_ART_AND_RUNTIME);
    assert(p->implemented_capabilities==0);

    p=sc64_p6_actor_policy_requirement(50);
    assert(p && p->required_capabilities==
           (SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY|
            SC64_P6_POLICY_NEEDS_SUICIDE_EXPLOSION));
    assert(p->implemented_capabilities==0);
    for (int raw=37;raw<=38;++raw) {
        p=sc64_p6_actor_policy_requirement(raw);
        assert(p && p->required_capabilities==SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY);
        assert(p->implemented_capabilities==0);
    }
    for (int raw=74;raw<=75;++raw) {
        p=sc64_p6_actor_policy_requirement(raw);
        assert(p && p->required_capabilities==
               SC64_P6_POLICY_NEEDS_PERMANENT_CLOAK_AND_DETECTORS);
        assert(p->implemented_capabilities==0);
    }
    assert(!sc64_p6_actor_policy_requirement(1));
}

#ifdef SC64_P6_SOURCE_AUDIT
static void test_p6_properties(void)
{
    enum { P6_USED_PROPERTIES = 13 };
    unsigned referenced=0, create_by_type[3]={0,0,0};
    for (unsigned i=0;i<SC64_P6_SOURCE_ACTION_COUNT;++i) {
        const Sc64P6SourceAction *a=&sc64_p6_source_actions[i];
        if (a->opcode!=11 || !a->group2) continue;
        assert(a->group2<=P6_USED_PROPERTIES);
        assert(a->unit==37 || a->unit==38 || a->unit==50);
        assert(!(referenced&(1u<<(a->group2-1u))));
        referenced|=1u<<(a->group2-1u);
        if (a->unit==37) ++create_by_type[0];
        else if (a->unit==38) ++create_by_type[1];
        else ++create_by_type[2];
    }
    assert(referenced==((1u<<P6_USED_PROPERTIES)-1u));
    assert(create_by_type[0]==8 && create_by_type[1]==3 && create_by_type[2]==2);
    for (unsigned i=0;i<64;++i) {
        assert(sc64_p6_source_property_used[i]==(i<P6_USED_PROPERTIES));
        if (i>=P6_USED_PROPERTIES) {
            for (unsigned j=0;j<20;++j) assert(sc64_p6_source_properties[i][j]==0);
            continue;
        }
        const unsigned char *r=sc64_p6_source_properties[i];
        unsigned valid_special=(unsigned)r[0]|((unsigned)r[1]<<8);
        unsigned valid_fields=(unsigned)r[2]|((unsigned)r[3]<<8);
        unsigned special=(unsigned)r[14]|((unsigned)r[15]<<8);
        assert(valid_special==0x001a && valid_fields==0x0002);
        assert(r[4]==0xff && r[5]==100 && r[6]==0 && r[7]==0);
        assert(r[8]==0 && r[9]==0 && r[10]==0 && r[11]==0);
        assert(r[12]==0 && r[13]==0 && special==0x0002);
        assert(r[16]==0 && r[17]==0 && r[18]==0 && r[19]==0);
        /* Values do not set either implemented OpenBW hallucination (0x8)
         * or invincibility (0x10) special bits. Native N64 handling of bit
         * 0x2 has separate T6 evidence for an order-117 request; P6 needs
         * runtime validation before claiming that behavior here. */
        assert((special&0x18)==0);
    }
}

#endif

int main(void)
{
    test_added_source_facts();
    test_inherited_source_facts();
    test_policy_requirements();

#ifdef SC64_P6_SOURCE_AUDIT
    test_p6_properties();
#endif
    return 0;
}
