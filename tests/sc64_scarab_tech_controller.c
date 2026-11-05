/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST controller: one real custom opponent, buttons and a constructed bank.
 * No prerequisite building, Reaver or target is spawned by this fixture. */
#ifndef SC64_SCARAB_TECH_PATH_HOST_PROBE
#error "Use the explicit full-service HOST tech-path probe"
#endif
static void observe_scarab_hit(int target,int damage,int source_type,int source_owner);
#define SC64_TEST_HIT_OBSERVER(target,damage,type,owner) observe_scarab_hit(target,damage,type,owner)
#define SC64_CATALOG_BUTTONS_ENTRY sc64_catalog_buttons_unused_main
#include "sc64_skirmish_catalog_buttons.c"
#undef SC64_CATALOG_BUTTONS_ENTRY

static int hit_slot=-1,hit_hp,hit_raw,hit_owner;
static void observe_scarab_hit(int target,int damage,int source_type,int source_owner)
{
    if (hit_slot<0 && target>=0 && target<MAX_UNITS && damage>0 &&
        source_type==U_REAVER && source_owner==SIDE && u[target].alive) {
        hit_slot=target;hit_hp=u[target].hp;hit_raw=u[target].raw_type;hit_owner=u[target].owner;
    }
}

extern void guardian_test_snapshot(void *);
extern void guardian_test_restore(const void *);
static unsigned char controller_sram[32768];

static void setup_options(void)
{
    option_map=0;option_human=R_PROTOSS;option_bots=1;option_bot_races[0]=R_ZERG;
    match_limit=90000;gba_init();assert(launch_match());
}

static int sole_reaver(void)
{
    int found=-1;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==SIDE &&
                                      sc64_race_raw_unit(u[i].type)==83) {
        assert(found<0);found=i;
    }
    return found;
}

static void stock_by_buttons(int row)
{
    if(menu_open)tap(KEY_B);
    tap(KEY_B);
    for(int guard=0;menu_open && menu_cursor!=row && guard<32;++guard) {
#ifdef SC64_COMMAND_ICONS
        if(card_fits()) {
            int slot=card_command_slot(menu_cursor),target_col=card_command_slot(row)%CARD_COLUMNS;
            int length=field_menu_count()-(slot/CARD_COLUMNS)*CARD_COLUMNS;
            if(length>CARD_COLUMNS)length=CARD_COLUMNS;
            if(slot%CARD_COLUMNS!=target_col)tap(target_col>=length?KEY_UP:KEY_RIGHT);
            else tap(KEY_DOWN);
        } else
#endif
            tap(KEY_DOWN);
    }
    assert(menu_open && menu_cursor==row);
    int before=minerals;
    input(KEY_A); /* Menu input pauses simulation; observe the actual debit. */
    assert(!menu_open && minerals==before-15);
    input(0);
}

static void pause_export(const char *path)
{
    tap(KEY_START);assert(screen==SCREEN_PAUSE);tap(KEY_B);
    assert((pause_notice & 0xc0)==UI_NOTICE_SAVED);
    guardian_test_snapshot(controller_sram);
    FILE *f=fopen(path,"wb");assert(f && fwrite(controller_sram,1,sizeof(controller_sram),f)==sizeof(controller_sram));
    assert(!fclose(f));
}

static void write_controller_progress(const char *path)
{
    setup_options();
    /* Explicit setup only: ample synthetic funds and initially idle workers.
     * All subsequent world changes pass through button input/game_update. */
    minerals=gas=5000;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==SIDE &&
                                      sc64_race_is_worker(u[i].type))u[i].order=ORDER_IDLE;
    assert(build_by_buttons(156,0)>=0); /* Pylon */
    assert(build_by_buttons(160,0)>=0); /* Gateway */
    assert(build_by_buttons(164,0)>=0); /* Cybernetics Core */
    int facility=build_by_buttons(155,0);assert(facility>=0);
    assert(build_by_buttons(171,0)>=0); /* Robotics Support Bay */
    produce_by_buttons(facility,83);
    int parent=sole_reaver();assert(parent>=0 && select_unit(parent));
    Sc64ScarabParent p;Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(parent,0,&p,&counts) && !p.queued && !counts.inside);
    int row=race_context_command_base(RACE_CONTEXT_SCARAB);
    assert(!strcmp(race_menu_label(row),"BUILD SCARAB"));stock_by_buttons(row);
    assert(race_scarab_parent_read(parent,0,&p,&counts) && p.queued==1);
    for(int i=0;i<1500 && !counts.inside;++i) {
        assert(screen==SCREEN_PLAY && !won && !lost);input(0);
        assert(race_scarab_parent_read(parent,0,&p,&counts));
    }
    assert(counts.inside==1 && !p.queued);
    pause_export(path);
    printf("buttons built tech, produced Reaver and stocked Scarab; saved at tick %u\n",host_ticks);
}

static void read_controller_progress(const char *path)
{
    FILE *f=fopen(path,"rb");assert(f && fread(controller_sram,1,sizeof(controller_sram),f)==sizeof(controller_sram));
    assert(!fclose(f));setup_options();guardian_test_restore(controller_sram);
    tap(KEY_START);assert(screen==SCREEN_PAUSE);tap(KEY_L);assert(screen==SCREEN_PLAY);
    assert(race_mode && race_human==R_PROTOSS && race_bots==1);
    assert(find_raw_building(156)>=0 && find_raw_building(160)>=0 &&
           find_raw_building(164)>=0 && find_raw_building(155)>=0 && find_raw_building(171)>=0);
    int parent=sole_reaver();Sc64ScarabParent p;Sc64ScarabCounts counts;
    assert(parent>=0 && race_scarab_parent_read(parent,0,&p,&counts));
    assert(counts.inside==1 && !p.queued);
    for(int i=0;i<60;++i) { assert(screen==SCREEN_PLAY && !won && !lost);input(0); }
    assert(race_scarab_parent_read(parent,0,&p,&counts) && counts.inside==1);
    puts("fresh HOST process loaded tech and paid ammunition and continued");
}

static int pending_scarab(void)
{
    for(unsigned i=0;i<race_impact_count;++i)
        if(race_impact_rows[i].source_type==SC64_SCARAB_IMPACT_MARKER)return (int)i;
    return -1;
}

static void attack_and_export(const char *path,const char *impact_path)
{
    read_controller_progress(path);
    int parent=sole_reaver();assert(parent>=0 && select_unit(parent));
    assert(choose_menu_command(1) && mode==MODE_ATTACK);
    move_cursor(race_base_x[1],race_base_y[1]);
    assert(screen==SCREEN_PLAY && !won && !lost && mode==MODE_ATTACK);
    tap(KEY_A);assert(mode==MODE_NORMAL);
    for(int i=0;i<20000 && pending_scarab()<0;++i) {
        if(screen!=SCREEN_PLAY || won || lost || !u[parent].alive)break;
        input(0);
    }
    if(pending_scarab()<0)
        fprintf(stderr,"attack timeout: tick=%u frame=%d screen=%d won=%d lost=%d parent_alive=%d order=%d pos=%d,%d target=%d impacts=%u\n",
                host_ticks,frame,screen,won,lost,u[parent].alive,u[parent].order,
                u[parent].x,u[parent].y,u[parent].target,race_impact_count);
    int index=pending_scarab();Sc64ScarabImpact packet;
    assert(index>=0 && sc64_scarab_impact_unpack(&race_impact_rows[index],&packet) && packet.ticks>0);
    pause_export(impact_path);
    printf("button attack created pending Scarab target %d with %u ticks; saved at tick %u\n",packet.target,packet.ticks,host_ticks);
}

static void resume_impact(const char *path)
{
    FILE *f=fopen(path,"rb");assert(f && fread(controller_sram,1,sizeof(controller_sram),f)==sizeof(controller_sram));
    assert(!fclose(f));setup_options();guardian_test_restore(controller_sram);
    tap(KEY_START);assert(screen==SCREEN_PAUSE);tap(KEY_L);assert(screen==SCREEN_PLAY);
    int index=pending_scarab();Sc64ScarabImpact packet;
    assert(index>=0 && sc64_scarab_impact_unpack(&race_impact_rows[index],&packet) && packet.ticks>0);
    hit_slot=-1;int damaged=0;
    for(int i=0;i<200 && pending_scarab()>=0;++i) {
        assert(screen==SCREEN_PLAY && !won && !lost);input(0);
        if (hit_slot>=0 && (!u[hit_slot].alive ||
            (u[hit_slot].raw_type==hit_raw && u[hit_slot].owner==hit_owner && u[hit_slot].hp<hit_hp))) damaged=1;
    }
    assert(pending_scarab()<0 && damaged);
    printf("fresh HOST process resumed saved Scarab impact and damaged enemy slot %d\n",hit_slot);
}

/* Component regression, separate from the ordinary-input run: move an
 * already-stocked parent and exercise the launch transaction directly. */
static void check_launch_origin(const char *path)
{
    read_controller_progress(path);
    int parent=sole_reaver(),child=-1;unsigned phase;int linked;
    for(int i=0;i<MAX_UNITS;++i)
        if(race_scarab_child_lookup(i,&linked,&phase) && linked==parent && phase==SC64_SCARAB_CHILD_INSIDE) child=i;
    assert(child>=0);
    u[parent].x=0;u[parent].tx=-10;u[parent].ty=u[parent].y;
    Unit old_parent=u[parent],old_child=u[child];int bank=minerals;
    assert(!race_scarab_launch_child(parent,0,child));
    assert(!memcmp(&u[parent],&old_parent,sizeof(Unit)) && !memcmp(&u[child],&old_child,sizeof(Unit)) && minerals==bank);
    u[parent].x=310;u[parent].tx=310;u[parent].ty=u[parent].y+60;
    int py=u[parent].y;
    assert(race_scarab_launch_child(parent,0,child));
    assert(u[child].x==310 && u[child].y==py+12 &&
           (((unsigned)u[child].carry>>19)&511u)==256u &&
           u[child].owner==u[parent].owner && u[child].raw_owner==u[parent].raw_owner && minerals==bank);
    race_scarab_control_dispatch(child);
    assert((((unsigned)u[child].carry>>19)&511u)==256u);
    puts("moved-parent launch preserved source fraction and atomic rejection");
}

int main(int argc,char **argv)
{
    assert(argc>=3);
    if(!strcmp(argv[1],"write")) {assert(argc==3);write_controller_progress(argv[2]);}
    else if(!strcmp(argv[1],"read")) {assert(argc==3);read_controller_progress(argv[2]);}
    else if(!strcmp(argv[1],"attack")) {assert(argc==4);attack_and_export(argv[2],argv[3]);}
    else if(!strcmp(argv[1],"launch-check")) {assert(argc==3);check_launch_origin(argv[2]);}
    else {assert(argc==3 && !strcmp(argv[1],"resume"));resume_impact(argv[2]);}
    return 0;
}
