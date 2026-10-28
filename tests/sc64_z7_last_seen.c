/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main z7_embedded_controller_main
#include "sc64_z7_campaign_run.c"
#undef main

static void reset_fixture(void)
{
    init_state();memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    race_mode=1;race_bots=1;race_selected_mission=0;race_human=1;
    race_cols=128;race_rows=128;frame=1;screen=SCREEN_PLAY;
    mode=MODE_NORMAL;menu_open=0;menu_last=MENU_MOVE;menu_cursor=MENU_MOVE;
    selected_building=-1;cx=cy=80;won=lost=0;
    b[0].alive=1;b[0].owner=SIDE;b[0].type=B_HATCHERY;b[0].raw_type=132;
    b[0].x=512;b[0].y=512;b[0].hp=b[0].maxhp=1500;
    b[1].alive=1;b[1].owner=ENEMY;b[1].type=B_HATCHERY;b[1].raw_type=132;
    b[1].x=1800;b[1].y=1800;b[1].hp=b[1].maxhp=1500;
    SC64_GRID_CLEAR(explored);SC64_GRID_CLEAR(visible);
    memset(z7_last_seen_buildings,0,sizeof(z7_last_seen_buildings));
    z7_last_seen_overflow=0;late_air_breakthrough_active=0;
    for(int cell=0;cell<MAP_COLS*MAP_ROWS;cell++) {
        SC64_GRID_SET(explored,cell,1);SC64_GRID_SET(visible,cell,1);
    }
}

static void set_visible_cell(int x,int y,int on)
{
    int cell=x/16+(y/16)*MAP_COLS;
    SC64_GRID_SET(visible,cell,on);
}

static void set_enemy_building(int slot,int x,int y,int raw_type)
{
    memset(&b[slot],0,sizeof(b[slot]));b[slot].alive=1;b[slot].owner=ENEMY;
    b[slot].raw_owner=4;b[slot].type=B_SPORE_COLONY;b[slot].raw_type=raw_type;
    b[slot].x=x;b[slot].y=y;b[slot].hp=b[slot].maxhp=400;
}

static void assert_ledger_empty(void)
{
    for(int i=0;i<Z7_LAST_SEEN_BUILDING_MAX;i++)assert(!z7_last_seen_buildings[i].used);
}

static void set_unit(int slot,int type,int order,int x,int y,int owner)
{
    memset(&u[slot],0,sizeof(u[slot]));u[slot].alive=1;u[slot].owner=owner;
    u[slot].type=type;u[slot].raw_type=type==U_MUTALISK?43:41;u[slot].raw_owner=owner;
    u[slot].hp=u[slot].maxhp=type==U_MUTALISK?120:40;u[slot].order=order;
    u[slot].x=u[slot].tx=x;u[slot].y=u[slot].ty=y;u[slot].target=-1;
}


static void set_group(void)
{
    set_unit(0,U_DRONE,ORDER_MOVE,400,400,SIDE);u[0].tx=1100;u[0].ty=400;
    set_unit(5,U_MUTALISK,ORDER_IDLE,800,800,SIDE);
    set_unit(6,U_MUTALISK,ORDER_IDLE,800,800,SIDE);
    set_unit(7,U_MUTALISK,ORDER_IDLE,800,800,SIDE);
    set_unit(10,U_MUTALISK,ORDER_ATTACK_MOVE,1900,1800,SIDE);u[10].tx=2000;u[10].ty=1000;
}

static void reset_escort_policy(void)
{
    z7_escort_pending_x=z7_escort_pending_y=-1;
    z7_escort_last_x=z7_escort_last_y=-1;
    z7_escort_retry_frame=z7_escort_same_target_until=0;
    z7_escort_attempts=0;
}

static void show_only_rally_cell(void)
{
    SC64_GRID_CLEAR(visible);
    SC64_GRID_SET(visible,792/16+(744/16)*MAP_COLS,1);
}

static void set_hidden_enemy(int slot,int x,int y)
{
    set_unit(slot,U_DRONE,ORDER_IDLE,x,y,ENEMY);
    u[slot].raw_owner=ENEMY;u[slot].raw_type=41;
}

static void assert_hidden_location_same_choices(int ex,int ey,int *rx,int *ry)
{
    reset_fixture();set_group();show_only_rally_cell();set_hidden_enemy(20,ex,ey);
    assert(!currently_visible(ex,ey));
    assert(z7_escort_find_rally((int[]){5,6,7},(int[]){43,43,43},
                                (int[]){SIDE,SIDE,SIDE},3,rx,ry));
    assert(*rx==792&&*ry==744);
    assert(campaign_muta_native_rally_box_attackmove(1500,1200,600,2));
    for(int i=5;i<=7;i++)assert(u[i].order==ORDER_ATTACK_MOVE&&
        abs(u[i].tx-1500)<=16&&abs(u[i].ty-1200)<=16);
}


int main(void)
{
    reset_fixture();set_group();
    int left=760,top=760,right=856,bottom=856;
    int slots[3]={5,6,7};
    assert(z7_escort_box_pure(left,top,right,bottom,slots,3));
    set_unit(1,U_DRONE,ORDER_IDLE,808,808,SIDE);
    assert(!z7_escort_box_pure(left,top,right,bottom,slots,3));
    u[1].alive=0;
    set_unit(11,U_MUTALISK,ORDER_ATTACK_MOVE,808,808,SIDE);
    assert(!z7_escort_box_pure(left,top,right,bottom,slots,3));
    u[11].alive=0;
    b[0].alive=1;b[0].owner=SIDE;b[0].type=B_HATCHERY;b[0].x=808;b[0].y=808;
    assert(!z7_escort_box_pure(left,top,right,bottom,slots,3));

    /* Actual native rally, box selection, and one simultaneous Attack-Move. */
    reset_fixture();set_group();
    unsigned before=host_ticks;
    int rallied=campaign_muta_native_rally_box_attackmove(1500,1200,600,2);
    fprintf(stderr,"fixture rally result%d host%u->%u selected%d mode%d menu%d\n",rallied,before,host_ticks,selected_count(),mode,menu_open);
    for(int i=5;i<=7;i++)fprintf(stderr,"fixture slot%d pos%d,%d target%d,%d order%d selected%d\n",i,u[i].x,u[i].y,u[i].tx,u[i].ty,u[i].order,u[i].selected);
    assert(rallied);
    assert(host_ticks>before&&selected_count()==3);
    for(int i=5;i<=7;i++) {
        assert(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_MUTALISK);
        assert(u[i].order==ORDER_ATTACK_MOVE&&abs(u[i].tx-1500)<=16&&abs(u[i].ty-1200)<=16);
    }
    assert(u[0].order==ORDER_MOVE&&u[0].tx==1100&&u[0].ty==400&&!u[0].selected);
    assert(u[10].order==ORDER_ATTACK_MOVE&&u[10].tx==2000&&u[10].ty==1000&&!u[10].selected);
    puts("PASS native rally box selected exactly three idle Mutalisks; one witnessed group Attack-Move left worker and active group unchanged");

    /* A completed Hatchery selection is cleared with the field's real SELECT
       action before the native group procedure proceeds. */
    reset_fixture();set_group();selected_building=0;
    assert(campaign_muta_native_rally_box_attackmove(1500,1200,600,2));
    assert(selected_building<0&&selected_count()==3);
    for(int i=5;i<=7;i++)assert(u[i].order==ORDER_ATTACK_MOVE);
    puts("PASS selected Hatchery was genuinely deselected before native rally and group order");

    /* Reject out-of-contract budgets without any UI input, including wrap-sized input. */
    reset_fixture();set_group();before=host_ticks;
    assert(!campaign_muta_native_rally_box_attackmove(1500,1200,601,2));
    assert(!campaign_muta_native_rally_box_attackmove(1500,1200,UINT_MAX,2));
    assert(host_ticks==before&&u[0].order==ORDER_MOVE&&u[10].order==ORDER_ATTACK_MOVE);
    puts("PASS over-limit and UINT_MAX waits rejected before any input");

    /* Hidden enemy relocation must not change rally ranking or resulting
       native group choices; visible-enemy occupancy remains a blocker. */
    int hidden_rx=-1,hidden_ry=-1,relocated_rx=-1,relocated_ry=-1;
    assert_hidden_location_same_choices(776,744,&hidden_rx,&hidden_ry);
    assert_hidden_location_same_choices(1800,400,&relocated_rx,&relocated_ry);
    assert(hidden_rx==relocated_rx&&hidden_ry==relocated_ry);
    reset_fixture();set_group();show_only_rally_cell();
    set_unit(11,U_DRONE,ORDER_IDLE,776,744,ENEMY);u[11].raw_owner=ENEMY;
    SC64_GRID_SET(visible,776/16+(744/16)*MAP_COLS,1);
    before=host_ticks;
    assert(!campaign_muta_native_rally_box_attackmove(1500,1200,600,2));
    assert(host_ticks==before);
    puts("PASS visible enemy in sole candidate rally box blocks; hidden relocation invariance preserved");

    /* Only an observed own scout loss at its recorded destination queues a
       one-shot escort. Accepted group orders clear it; same-target retries
       are cooldown-bounded. */
    reset_fixture();set_group();reset_escort_policy();
    z7_scout_track_move(5,1500,1200,-1);
    assert(!z7_escort_record_scout_loss(5,1500,1200));
    u[5].alive=0;
    assert(z7_escort_record_scout_loss(5,1500,1200));
    assert(!z7_escort_record_scout_loss(5,1500,1200));
    assert(campaign_muta_escort_after_scout_loss());
    assert(z7_escort_pending_x<0&&z7_escort_pending_y<0);
    for(int i=6;i<=7;i++)assert(u[i].order==ORDER_ATTACK_MOVE&&
        abs(u[i].tx-1500)<=16&&abs(u[i].ty-1200)<=16);
    unsigned escort_done_ticks=host_ticks;
    assert(!campaign_muta_escort_after_scout_loss()&&host_ticks==escort_done_ticks);
    z7_scout_track_move(6,1500,1200,-1);u[6].alive=0;
    assert(!z7_escort_record_scout_loss(6,1500,1200));
    puts("PASS observed dead own scout queues one bounded escort; accepted order clears it and same-target repeat is cooled");

    /* A matching native Move is left in place; failed availability retries
       honor their deadline and can later succeed once a second idle Muta exists. */
    reset_fixture();set_group();reset_escort_policy();
    z7_scout_track_move(5,1500,1200,-1);u[5].alive=0;
    assert(z7_escort_record_scout_loss(5,1500,1200));
    u[6].order=ORDER_MOVE;u[6].tx=1500;u[6].ty=1200;
    before=host_ticks;
    assert(campaign_muta_escort_after_scout_loss());
    assert(host_ticks==before&&u[6].order==ORDER_MOVE&&u[6].tx==1500&&u[6].ty==1200);
    assert(z7_escort_pending_x==1500&&z7_escort_pending_y==1200);
    u[6].order=ORDER_IDLE;u[7].order=ORDER_ATTACK_MOVE;
    assert(!campaign_muta_escort_after_scout_loss());
    assert(z7_escort_attempts==1&&z7_escort_retry_frame>(unsigned)frame);
    before=host_ticks;
    assert(!campaign_muta_escort_after_scout_loss()&&host_ticks==before);
    u[7].order=ORDER_IDLE;frame=z7_escort_retry_frame;
    assert(campaign_muta_escort_after_scout_loss());
    for(int i=6;i<=7;i++)assert(u[i].order==ORDER_ATTACK_MOVE&&
        abs(u[i].tx-1500)<=16&&abs(u[i].ty-1200)<=16);
    puts("PASS matching scout Move preserved; unavailable group backs off and retries after finite deadline");

    /* Forced zero-frame wait tests a bounded timeout: native Move orders may
       gather the group, but no Attack-Move may issue before all arrive. */
    reset_fixture();set_group();
    assert(!campaign_muta_native_rally_box_attackmove(1500,1200,0,2));
    for(int i=5;i<=7;i++)assert(u[i].order!=ORDER_ATTACK_MOVE);
    assert(u[0].order==ORDER_MOVE&&u[0].tx==1100&&u[0].ty==400&&!u[0].selected);
    assert(u[10].order==ORDER_ATTACK_MOVE&&u[10].tx==2000&&u[10].ty==1000);
    puts("PASS rally timeout aborted before group Attack-Move and preserved worker/active orders");

    /* With the only visible rally cell contaminated, native staging refuses
       before issuing any input or order to a worker or active unit. */
    reset_fixture();set_group();
    int rally_x=-1,rally_y=-1,raw[3]={43,43,43},raw_owner[3]={SIDE,SIDE,SIDE};
    assert(z7_escort_find_rally(slots,raw,raw_owner,3,&rally_x,&rally_y));
    SC64_GRID_CLEAR(visible);
    SC64_GRID_SET(visible,800/16+(800/16)*MAP_COLS,1);
    SC64_GRID_SET(visible,rally_x/16+(rally_y/16)*MAP_COLS,1);
    set_unit(1,U_DRONE,ORDER_IDLE,rally_x,rally_y,SIDE);
    before=host_ticks;
    assert(!campaign_muta_native_rally_box_attackmove(1500,1200,600,2));
    assert(host_ticks==before);
    for(int i=5;i<=7;i++)assert(u[i].order==ORDER_IDLE);
    assert(u[0].order==ORDER_MOVE&&u[1].order==ORDER_IDLE);
    assert(u[10].order==ORDER_ATTACK_MOVE&&u[10].tx==2000&&u[10].ty==1000);
    puts("PASS worker-contaminated sole visible rally box rejected before orders");

    reset_fixture();set_group();
    assert(z7_escort_find_rally(slots,raw,raw_owner,3,&rally_x,&rally_y));
    SC64_GRID_CLEAR(visible);
    SC64_GRID_SET(visible,800/16+(800/16)*MAP_COLS,1);
    SC64_GRID_SET(visible,rally_x/16+(rally_y/16)*MAP_COLS,1);
    set_unit(11,U_MUTALISK,ORDER_ATTACK_MOVE,rally_x,rally_y,SIDE);
    before=host_ticks;
    assert(!campaign_muta_native_rally_box_attackmove(1500,1200,600,2));
    assert(host_ticks==before);
    for(int i=5;i<=7;i++)assert(u[i].order==ORDER_IDLE);
    assert(u[11].order==ORDER_ATTACK_MOVE&&!u[11].selected);
    puts("PASS active-Mutalisk-contaminated sole visible rally box rejected without retasking it");

    /* The building ledger is sampled after both held and released native
       updates, retains coordinates in fog, and never follows hidden slot data. */
    reset_fixture();SC64_GRID_CLEAR(visible);set_enemy_building(1,900,900,144);
    set_visible_cell(900,900,1);step(KEY_RIGHT);
    int seen=z7_last_seen_find(900,900);
    assert(seen>=0&&z7_last_seen_buildings[seen].raw_type==144&&
           z7_last_seen_buildings[seen].owner==ENEMY);
    set_visible_cell(900,900,0);b[1].x=1200;b[1].y=1200;b[1].raw_type=146;
    step(0);
    seen=z7_last_seen_find(900,900);
    assert(seen>=0&&z7_last_seen_buildings[seen].raw_type==144&&
           z7_last_seen_buildings[seen].owner==ENEMY);
    puts("PASS post-update ledger retains the last observed type and coordinate through fog/hidden relocation");

    /* A visible morph at the same coordinate updates by coordinate, while a
       reused slot at a new visible coordinate clears the old memory. */
    reset_fixture();SC64_GRID_CLEAR(visible);set_enemy_building(1,900,900,144);
    set_visible_cell(900,900,1);z7_last_seen_observe();
    set_enemy_building(1,900,900,146);z7_last_seen_observe();
    seen=z7_last_seen_find(900,900);
    assert(seen>=0&&z7_last_seen_buildings[seen].raw_type==146);
    set_visible_cell(1000,900,1);set_enemy_building(1,1000,900,146);z7_last_seen_observe();
    assert(z7_last_seen_find(900,900)<0&&z7_last_seen_find(1000,900)>=0);
    puts("PASS visible morph updates the coordinate record; slot reuse at a new visible site clears the old one");

    /* Emptying is authoritative only when the old coordinate is visible. */
    reset_fixture();SC64_GRID_CLEAR(visible);set_enemy_building(1,900,900,144);
    set_visible_cell(900,900,1);z7_last_seen_observe();
    seen=z7_last_seen_find(900,900);assert(seen>=0);
    z7_last_seen_buildings[seen].last_order_frame=123;
    z7_last_seen_buildings[seen].next_attempt_frame=60000;
    set_visible_cell(900,900,0);b[1].alive=0;z7_last_seen_observe();
    assert(z7_last_seen_find(900,900)>=0);
    set_visible_cell(900,900,1);z7_last_seen_observe();
    assert(z7_last_seen_find(900,900)<0);
    set_enemy_building(1,900,900,144);z7_last_seen_observe();
    seen=z7_last_seen_find(900,900);
    assert(seen>=0&&z7_last_seen_buildings[seen].last_order_frame==0&&
           z7_last_seen_buildings[seen].next_attempt_frame==0);
    puts("PASS visible-empty clear and coordinate reallocation reset remembered attack cooldown");

    /* A UI sequence can run observation updates that clear/reuse a ledger
       slot. Post-order bookkeeping must find by the snapshotted coordinate,
       never mutate whichever record now occupies the old slot. */
    reset_fixture();SC64_GRID_CLEAR(visible);set_enemy_building(1,900,900,144);
    set_visible_cell(900,900,1);z7_last_seen_observe();seen=z7_last_seen_find(900,900);
    assert(seen>=0);z7_last_seen_buildings[seen].used=0;
    int reused=z7_last_seen_alloc();assert(reused==seen);
    z7_last_seen_buildings[reused].x=1000;z7_last_seen_buildings[reused].y=900;
    z7_last_seen_buildings[reused].raw_type=146;z7_last_seen_buildings[reused].owner=ENEMY;
    z7_last_seen_buildings[reused].seen_frame=frame;z7_last_seen_buildings[reused].used=1;
    assert(!z7_last_seen_note_order(900,900,1));
    assert(z7_last_seen_buildings[reused].x==1000&&
           z7_last_seen_buildings[reused].last_order_frame==0&&
           z7_last_seen_buildings[reused].next_attempt_frame==0);
    puts("PASS post-input order bookkeeping ignores a reallocated record slot unless coordinate key still matches");

    /* Candidate ranking depends on the controller ledger and own state; moving
       an unseen live building cannot alter its result. */
    reset_fixture();SC64_GRID_CLEAR(visible);set_enemy_building(1,900,900,144);
    set_enemy_building(2,1000,900,146);set_visible_cell(900,900,1);set_visible_cell(1000,900,1);
    z7_last_seen_observe();SC64_GRID_CLEAR(visible);
    int candidate=z7_last_seen_choose_candidate();assert(candidate>=0);
    int candidate_x=z7_last_seen_buildings[candidate].x,candidate_y=z7_last_seen_buildings[candidate].y;
    b[1].x=1400;b[1].y=1400;b[1].raw_type=99;b[1].owner=SIDE;
    b[2].x=1500;b[2].y=1500;b[2].raw_type=77;b[2].owner=SIDE;
    z7_last_seen_observe();candidate=z7_last_seen_choose_candidate();
    assert(candidate>=0&&z7_last_seen_buildings[candidate].x==candidate_x&&
           z7_last_seen_buildings[candidate].y==candidate_y);
    puts("PASS hidden actor relocation/ownership/type changes do not affect last-seen candidate ranking");

    /* The production fallback requires four idle Mutalisks, rallies them with
       native controls, then sends one witnessed group Attack-Move. */
    reset_fixture();set_group();set_unit(8,U_MUTALISK,ORDER_IDLE,808,800,SIDE);
    u[10].x=200;u[10].y=1800;u[10].tx=2000;u[10].ty=1000;
    SC64_GRID_CLEAR(visible);set_enemy_building(1,1800,400,144);
    set_visible_cell(1800,400,1);z7_last_seen_observe();set_visible_cell(1800,400,0);
    show_only_rally_cell();late_air_breakthrough_active=1;before=host_ticks;
    assert(campaign_last_seen_building_attack_step());
    int attack_group=0;
    for(int i=5;i<=8;i++)if(u[i].order==ORDER_ATTACK_MOVE&&
       abs(u[i].tx-1800)<=32&&abs(u[i].ty-400)<=32)++attack_group;
    assert(attack_group==4&&host_ticks>before);
    assert(u[0].order==ORDER_MOVE&&u[0].tx==1100&&u[0].ty==400&&!u[0].selected);
    assert(u[10].order==ORDER_ATTACK_MOVE&&u[10].tx==2000&&u[10].ty==1000&&!u[10].selected);
    unsigned group_done=host_ticks;
    assert(!campaign_last_seen_building_attack_step()&&host_ticks==group_done);
    puts("PASS remembered candidate receives one native 4-Muta group order; worker, active group, and no-spam cooldown hold");

    /* Eight idle actors remain within the bounded group contract; non-idle
       scouts/assault actors and workers never join the native group. */
    reset_fixture();set_unit(0,U_DRONE,ORDER_MOVE,400,400,SIDE);u[0].tx=1100;u[0].ty=400;
    for(int i=5;i<13;i++)set_unit(i,U_MUTALISK,ORDER_IDLE,800+(i-5)*3,800,SIDE);
    set_unit(14,U_MUTALISK,ORDER_ATTACK_MOVE,200,1800,SIDE);u[14].tx=2000;u[14].ty=1000;
    assert(campaign_muta_native_rally_box_attackmove(1500,1200,600,4));
    int eight=0;for(int i=5;i<13;i++)if(u[i].order==ORDER_ATTACK_MOVE&&
        abs(u[i].tx-1500)<=32&&abs(u[i].ty-1200)<=32)eight++;
    assert(eight==8&&u[0].order==ORDER_MOVE&&u[14].order==ORDER_ATTACK_MOVE&&
           !u[0].selected&&!u[14].selected);
    puts("PASS native group grows to eight idle Mutalisks without selecting worker or active attacker");

    /* A first isolated idle actor cannot starve a later valid cluster. Four
       globally gathered idle actors can rally even though none share a
       64-pixel neighborhood. */
    reset_fixture();set_unit(0,U_DRONE,ORDER_MOVE,400,400,SIDE);u[0].tx=1100;u[0].ty=400;
    set_unit(5,U_MUTALISK,ORDER_IDLE,300,300,SIDE);
    set_unit(6,U_MUTALISK,ORDER_IDLE,800,800,SIDE);
    set_unit(7,U_MUTALISK,ORDER_IDLE,900,800,SIDE);
    set_unit(8,U_MUTALISK,ORDER_IDLE,1000,800,SIDE);
    set_unit(9,U_MUTALISK,ORDER_IDLE,1100,800,SIDE);
    set_unit(14,U_MUTALISK,ORDER_ATTACK_MOVE,200,1800,SIDE);u[14].tx=2000;u[14].ty=1000;
    int slots4[4],raw4[4],owners4[4];
    assert(z7_escort_collect_idle_cluster(1500,1200,slots4,raw4,owners4)==4);
    for(int i=0;i<4;i++)assert(slots4[i]>=6&&slots4[i]<=9);
    for(int i=0;i<4;i++)for(int j=i+1;j<4;j++)assert(slots4[i]!=slots4[j]);
    assert(campaign_muta_native_rally_box_attackmove(1500,1200,600,4));
    for(int i=6;i<=9;i++)assert(u[i].order==ORDER_ATTACK_MOVE&&
        abs(u[i].tx-1500)<=32&&abs(u[i].ty-1200)<=32);
    assert(u[5].order==ORDER_IDLE&&u[14].order==ORDER_ATTACK_MOVE&&
           u[0].order==ORDER_MOVE&&!u[5].selected&&!u[14].selected&&!u[0].selected);
    puts("PASS isolated first idle Muta skipped; four actors farther than64 pixels apart gathered with native orders");

    reset_fixture();set_group();before=host_ticks;
    assert(!campaign_muta_native_rally_box_attackmove(1500,1200,600,4));
    assert(host_ticks==before);
    for(int i=5;i<=7;i++)assert(u[i].order==ORDER_IDLE);
    puts("PASS three-idle-Muta group is rejected without weakening the four-unit minimum");
    return 0;
}
