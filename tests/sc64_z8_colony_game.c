/* Ordinary Z8 colony menu, paid build, morph and valid save continuation. */
#define main sc64_z8_existing_fixture_main
#include "sc64_z8_game.c"
#undef main

static void open_row(int row)
{
    int guard=0;
    press(KEY_B);
    assert(menu_open);
    while (menu_cursor!=row && guard++<field_menu_count()) press(KEY_DOWN);
    assert(menu_cursor==row);
    press(KEY_A);
}

static int closest_site(int type,int wx,int wy,int require_creep)
{
    int best=-1, best_distance=0x7fffffff;
    for (int y=32;y<WORLD_H-32;y+=8) for (int x=32;x<WORLD_W-32;x+=8) {
        int d=dist2(x,y,wx,wy);
        if (d>=best_distance || !can_place(type,x,y) ||
            (require_creep && !race_can_site(SIDE,type,x,y))) continue;
        best=x+y*WORLD_W; best_distance=d;
    }
    return best;
}

int main(void)
{
    static RaceSave saved;
    int worker=-1, hatch=-1, chamber=-1, colony=-1, site;
    launch_z8_from_normal_menu();
    for (int i=0;i<120;++i) input(0);
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].owner==SIDE && u[i].type==U_DRONE) { worker=i; break; }
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive && b[i].owner==SIDE) {
        if (b[i].type==B_HATCHERY) hatch=i;
        if (b[i].type==B_EVOLUTION_CHAMBER && building_complete(i)) chamber=i;
    }
    assert(worker>=0 && hatch>=0);
    assert(race_z8_colony_product_available(SIDE,u[worker].raw_owner,B_CREEP_COLONY));
    assert(!race_z8_colony_product_available(ENEMY,u[worker].raw_owner,B_CREEP_COLONY));
    site=closest_site(B_CREEP_COLONY,u[worker].x,u[worker].y,1);
    assert(site>=0);
    for (int i=0;i<MAX_UNITS;++i) u[i].selected=0;
    u[worker].selected=1; selected_building=-1;
    assert(race_z7_creep_command_available());
    open_row(MENU_Z7_CREEP);
    assert(mode==MODE_BUILD && menu==B_CREEP_COLONY);
    cx=site%WORLD_W; cy=site/WORLD_W;
    press(KEY_A);
    assert(u[worker].order==ORDER_BUILD_PENDING || !u[worker].alive);
    for (int tick=0;tick<2000 && colony<0;++tick) {
        input(0);
        for (int j=0;j<MAX_BUILDINGS;++j)
            if (b[j].alive && b[j].owner==SIDE && b[j].type==B_CREEP_COLONY &&
                b[j].x==site%WORLD_W && b[j].y==site/WORLD_W) { colony=j; break; }
    }
    assert(colony>=0 && b[colony].raw_owner==SC64_Z8_HUMAN_SLOT);
    for (int tick=0;tick<500 && !building_complete(colony);++tick) input(0);
    assert(building_complete(colony));
    if (chamber<0) {
        assert(!race_colony_morph_available_for(colony,B_SPORE_COLONY));
        site=closest_site(B_EVOLUTION_CHAMBER,b[hatch].x,b[hatch].y,0);
        assert(site>=0);
        chamber=spawn_build(SIDE,B_EVOLUTION_CHAMBER,site%WORLD_W,site/WORLD_W,0);
        assert(chamber>=0 && building_complete(chamber));
    }
    for (int i=0;i<MAX_UNITS;++i) u[i].selected=0;
    selected_building=colony;
    assert(race_colony_morph_available_for(colony,B_SPORE_COLONY));
    b[colony].raw_owner=1;
    assert(!race_colony_morph_available_for(colony,B_SPORE_COLONY));
    b[colony].raw_owner=SC64_Z8_HUMAN_SLOT;
    int before=minerals;
    open_row(MENU_STOP+2);
    assert(minerals==before-50 && b[colony].under_construction &&
           (b[colony].flags&SC64_Z7_SPORE_MORPH_FLAG));
    for (int tick=0;tick<50;++tick) input(0);
    assert(b[colony].under_construction && b[colony].build_progress>0);
    int progress=b[colony].build_progress;
    press(KEY_START); assert(screen==SCREEN_PAUSE);
    press(KEY_B); fixture_capture_save(&saved);
    assert(race_z8_save_candidate_world_valid(&saved,race_cols,race_rows));
    game_init(); held_keys=0; press(KEY_L);
    assert(screen==SCREEN_PLAY && race_z8_active());
    assert(b[colony].under_construction && b[colony].build_progress>=progress &&
           (b[colony].flags&SC64_Z7_SPORE_MORPH_FLAG));
    for (int tick=0;tick<500 && b[colony].under_construction;++tick) input(0);
    assert(building_complete(colony) && b[colony].type==B_SPORE_COLONY &&
           b[colony].raw_type==144 && b[colony].raw_owner==SC64_Z8_HUMAN_SLOT);
    /* An enemy airborne actor receives the actual delayed building impact.
     * The ground control is closer, so a ground-targeting regression is visible. */
    int ground=spawn_unit(ENEMY,U_ZERGLING,b[colony].x+8,b[colony].y+8);
    int air=spawn_unit(ENEMY,U_MUTALISK,b[colony].x+20,b[colony].y+20);
    assert(ground>=0 && air>=0);
    u[ground].raw_owner=1; u[air].raw_owner=1;
    int ground_hp=u[ground].hp, air_hp=u[air].hp;
    for (int tick=0;tick<120 && u[air].alive && u[air].hp==air_hp;++tick) input(0);
    assert(u[air].hp<air_hp && u[ground].hp==ground_hp);
    puts("Z8 colony menu, build, morph, valid save continuation and air impact passed");
    return 0;
}
