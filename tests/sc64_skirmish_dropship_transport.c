/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native custom-game Dropship service through real input, update and SRAM paths. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void start_skirmish(void)
{
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();
    assert(screen==SCREEN_PLAY && race_mode);
}
static void fixture_clear_selection(void)
{
    for(int i=0;i<MAX_UNITS;i++) u[i].selected=0;
    selected_building=-1;menu_open=0;mode=MODE_NORMAL;
}
static void tick(void)
{
    game_update(0,0);
    assert(screen==SCREEN_PLAY);
}
static int spawn_here(int owner,int type,int x,int y)
{
    int slot=spawn_unit(owner,type,x,y); assert(slot>=0); return slot;
}
static void board_via_input(int passenger,int ship)
{
    fixture_clear_selection();u[passenger].selected=1;
    cx=u[ship].x;cy=u[ship].y;interact();
    assert(u[passenger].order==ORDER_BOARD && u[passenger].target==ship);
    for(int i=0;i<90 && !SC64_TRANSPORT_LOADED(&u[passenger]);i++) tick();
    assert(SC64_TRANSPORT_LOADED(&u[passenger]) && u[passenger].target==ship);
    assert(!u[passenger].selected && u[passenger].x==u[ship].x && u[passenger].y==u[ship].y);
}
static RaceSave valid_save, bad_save;
static Unit before_units[MAX_UNITS];
static Building before_buildings[MAX_BUILDINGS];
static void encode_unit(RaceSave *save,int slot,const Unit *q)
{
#if defined(SC64_ZP_FULL_STORAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
    assert(sc64_p7_typed_unit_encode(&save->units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#elif defined(SC64_ZP_FULL_STORAGE) && defined(SC64_Z7_DERIVED_SAVE)
    assert(sc64_derived_unit_encode(&save->units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#elif defined(SC64_ZP_FULL_STORAGE)
    assert(sc64_zp_unit_save_encode(&save->units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#elif defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    assert(sc64_unit_save_encode(&save->units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#else
    for(unsigned i=0;i<RACE_UNIT_WORDS;i++) {
        int n;memcpy(&n,(const u8 *)q+i*sizeof(int),sizeof(n));save->units[slot][i]=(s16)n;
    }
#endif
}
static void invalid_cargo_rejected(int ship,int passenger)
{
    valid_save=race_data;
    memcpy(before_units,u,sizeof(u));memcpy(before_buildings,b,sizeof(b));
    int old_minerals=minerals,old_gas=gas;
    for(int mutation=0;mutation<6;mutation++) {
        bad_save=valid_save;
        int slot=mutation==4?ship:passenger;
        Unit actor;
        assert(race_skirmish_transport_decode(&bad_save,slot,race_cols,race_rows,&actor));
        switch(mutation) {
        case 0:actor.selected=1;break;
        case 1:actor.target=-1;break;
        case 2:actor.x=(actor.x+1)%(race_cols*16);break;
        case 3:actor.owner=ENEMY;break;
        case 4:actor.raw_type=42;break;
        case 5:actor.type=U_TANK;actor.raw_type=5;
               actor.maxhp=sc64_race_unit_rule(U_TANK)->hp;actor.hp=actor.maxhp;
               actor.armor=sc64_race_unit_armor(U_TANK);break;
        }
        encode_unit(&bad_save,slot,&actor);
        save_write(&bad_save,sizeof(bad_save),SAVE_VERSION);
        save_write(&bad_save,sizeof(bad_save),SAVE_VERSION);
        assert(!race_load_game());
        assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)));
        assert(minerals==old_minerals && gas==old_gas);
    }
    save_write(&valid_save,sizeof(valid_save),SAVE_VERSION);
    save_write(&valid_save,sizeof(valid_save),SAVE_VERSION);
    puts("malformed cargo links rejected atomically PASS");
}
int main(void)
{
    setbuf(stdout,NULL);gba_init();start_skirmish();
    if(sc64_race_unit_from_raw(11)!=U_DROPSHIP) {
        puts("SKIP profile lacks native Dropship identity");return 0;
    }
    int x=0,y=0;
    for(int row=4;row<race_rows-4 && !x;row++) for(int col=4;col<race_cols-4 && !x;col++) {
        int open=1;for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++)
            if(terrain_blocked(col+dx,row+dy))open=0;
        int px=col*16+8,py=row*16+8;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && dist2(px,py,u[i].x,u[i].y)<96*96)open=0;
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive && dist2(px,py,b[i].x,b[i].y)<96*96)open=0;
        if(open){x=px;y=py;}
    }
    assert(x && y);
    int ship=spawn_here(SIDE,U_DROPSHIP,x,y),passengers[9];
    /* spawn_unit is deliberately used without assigning source metadata. */
    assert(race_transport_carrier_valid(ship));
    assert(u[ship].raw_type==-1 || u[ship].raw_type==11);
    for(int i=0;i<9;i++) passengers[i]=spawn_here(SIDE,U_MARINE,x+(i==8?64:0),y);
    int enemy=spawn_here(ENEMY,U_MARINE,x+120,y),flying=spawn_here(SIDE,U_WRAITH,x+100,y);
    int overlord=spawn_here(SIDE,U_OVERLORD,x+120,y+32);
    u[overlord].raw_type=42;
    assert(!race_transport_carrier_valid(overlord));
    assert(!race_transport_request_board(enemy,ship));
    assert(!race_transport_request_board(flying,ship));
    assert(!race_transport_request_board(ship,ship));
    int passenger_raw=u[passengers[0]].raw_type;
    u[passengers[0]].raw_type=7; /* Explicit SCV identity contradicts Marine. */
    assert(!race_transport_request_board(passengers[0],ship));
    u[passengers[0]].raw_type=passenger_raw;
    u[passengers[0]].flags|=2;assert(!race_transport_request_board(passengers[0],ship));
    u[passengers[0]].flags&=~2;
    int saved_raw=u[ship].raw_type;u[ship].raw_type=7;
    assert(!race_transport_carrier_valid(ship));u[ship].raw_type=saved_raw;
    for(int i=0;i<8;i++) board_via_input(passengers[i],ship);
    assert(race_transport_cargo_space(ship)==8);
    assert(!race_transport_request_board(passengers[0],ship));
    assert(!race_transport_request_board(passengers[8],ship));
    assert(race_save_game());
#ifdef SC64_T5_RUNTIME
    /* Build an actual custom Control Tower with the normal add-on command;
     * custom slots use geometry and can lack campaign source metadata. */
    int starport=-1,tower=-1;
    minerals=gas=5000;
    for(int yy=96;yy<WORLD_H-100 && starport<0;yy+=32)
        for(int xx=96;xx<WORLD_W-120 && starport<0;xx+=32)
            if(can_place(B_STARPORT,xx,yy) && can_place(B_CONTROL_TOWER,xx+48,yy+8))
                starport=spawn_build(SIDE,B_STARPORT,xx,yy,0);
    assert(starport>=0);
    selected_building=starport;
    const Sc64SkirmishRecord *tower_row=sc64_skirmish_menu_record(R_TER,115);
    assert(tower_row && race_skirmish_choose_record(tower_row));
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(b[i].alive && b[i].type==B_CONTROL_TOWER && b[i].x==b[starport].x+48 && b[i].y==b[starport].y+8) tower=i;
    assert(tower>=0 && b[tower].under_construction);
    assert(race_save_game());
    valid_save=race_data;bad_save=valid_save;
    Building bad_tower=b[tower];bad_tower.level=MAX_BUILDINGS+1;
#ifdef SC64_T6_STORAGE
    assert(sc64_building_save_encode(&bad_save.buildings[tower],&bad_tower,
                                     (unsigned)race_cols*16u,(unsigned)race_rows*16u));
#else
    for(unsigned j=0;j<RACE_BUILD_WORDS;j++) {
        int n;memcpy(&n,(const u8 *)&bad_tower+j*sizeof(int),sizeof(n));bad_save.buildings[tower][j]=(s16)n;
    }
#endif
    memcpy(before_units,u,sizeof(u));memcpy(before_buildings,b,sizeof(b));
    save_write(&bad_save,sizeof(bad_save),SAVE_VERSION);
    save_write(&bad_save,sizeof(bad_save),SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)));
    save_write(&valid_save,sizeof(valid_save),SAVE_VERSION);
    save_write(&valid_save,sizeof(valid_save),SAVE_VERSION);
    race_data=valid_save;
#endif
    invalid_cargo_rejected(ship,passengers[0]);
    assert(race_load_game());
    assert(race_transport_cargo_space(ship)==8);
    for(int i=0;i<8;i++)assert(SC64_TRANSPORT_LOADED(&u[passengers[i]]) && u[passengers[i]].target==ship);
    fixture_clear_selection();u[ship].selected=1;
    assert(race_skirmish_dropship_menu_available());
    int row=field_menu_count()-1;
    assert(!strcmp(race_menu_label(row),"UNLOAD CARGO"));
    choose_command(row);assert(mode==MODE_UNLOAD);
    cx=x;cy=y;interact();assert(u[ship].order==ORDER_UNLOAD && mode==MODE_NORMAL);
    for(int i=0;i<240 && race_transport_cargo_space(ship)>0;i++) tick();
    assert(race_transport_cargo_space(ship)==0);
    for(int i=0;i<8;i++) {
        Unit *q=&u[passengers[i]];
        assert(q->alive && !SC64_TRANSPORT_LOADED(q) && q->order==ORDER_IDLE);
        assert(q->x>=0 && q->x<WORLD_W && q->y>=0 && q->y<WORLD_H);
        assert(!terrain_blocked(q->x/16,q->y/16));
    }
    board_via_input(passengers[0],ship);
    assert(race_transport_cargo_space(ship)==1);
    hit_target(ship,10000,U_MARINE,ENEMY);
    tick();
    assert(!u[ship].alive && !u[passengers[0]].alive);
    assert(!race_transport_carrier_valid(ship));
    puts("native Dropship boarding, save, capacity, unload and death PASS");return 0;
}
