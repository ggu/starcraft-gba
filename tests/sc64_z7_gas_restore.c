/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Z7 Extractor-to-geyser restoration through the real building death path. */
#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_Z7_RUNTIME
#error "Compile this fixture with the complete Z7 runtime profile flags"
#endif

static unsigned held_keys;

static void press(unsigned key)
{
    game_update((u16)key,(u16)(key&~held_keys)); held_keys=key;
    game_update(0,0); held_keys=0;
}

static void launch_z7(void)
{
    game_init(); held_keys=0;
    press(KEY_A);
    for (unsigned page=0;page<3;++page) press(KEY_RIGHT);
    press(KEY_A); press(KEY_A);
    assert(screen==SCREEN_PLAY && race_z7_active());
    assert(race_z7_map==&sc64_race_maps[SC64_Z7_GLOBAL_ID]);
    assert(geyser_count==1);
}

static int source_extractor_at(int x,int y)
{
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].type==B_EXTRACTOR && b[i].raw_type==149 &&
            b[i].raw_owner==4 && b[i].x==x && b[i].y==y) return i;
    return -1;
}

static void destroy_building(int slot)
{
    int damage=b[slot].hp+sc64_race_building_armor(b[slot].type)+1;
    hit_target(MAX_UNITS+slot,damage,U_ZERGLING,SIDE);
    assert(!b[slot].alive);
}

static int geyser_position(int x,int y)
{
    for (int i=0;i<geyser_count;++i)
        if (geysers[i].x==x && geysers[i].y==y) return 1;
    return 0;
}

static void reject_without_mutation(const RaceSave *bad)
{
    Unit units[MAX_UNITS]; Building buildings[MAX_BUILDINGS];
    Geyser nodes[MAX_RACE_GEYSERS]; Sc64Z7PersistentState state=race_z7;
    int count=geyser_count,clock=frame;
    memcpy(units,u,sizeof(u)); memcpy(buildings,b,sizeof(b));
    memcpy(nodes,geysers,sizeof(geysers));
    save_write(bad,sizeof(*bad),SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(units,u,sizeof(u)) && !memcmp(buildings,b,sizeof(b)));
    assert(!memcmp(nodes,geysers,sizeof(geysers)) && geyser_count==count);
    assert(!memcmp(&state,&race_z7,sizeof(state)) && frame==clock);
}

int main(void)
{
    const Sc64RaceMap *map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];
    unsigned restored=0;
    launch_z7();

    /* Every source raw-149 position is individually authenticated, and each
     * genuine enemy Extractor death restores exactly that node. */
    for (unsigned row=0;row<map->unit_count;++row) {
        const Sc64ImportedUnit *src=&map->units[row];
        if (src->source_type!=149u) continue;
        assert(src->owner==4u);
        int slot=source_extractor_at(src->x_px,src->y_px);
        assert(slot>=0);
        int before=geyser_count;
        destroy_building(slot);
        assert(geyser_count==before+1 && geyser_position(src->x_px,src->y_px));
        ++restored;
        /* Duplicate notification is harmless, as is destroying a rebuilt
         * Extractor at the same source node. */
        race_z7_note_building_death(&b[slot]);
        assert(geyser_count==before+1);
        int rebuilt=spawn_build(SIDE,B_EXTRACTOR,src->x_px,src->y_px,0);
        assert(rebuilt>=0);
        b[rebuilt].raw_type=-1; b[rebuilt].raw_owner=-1;
        destroy_building(rebuilt);
        assert(geyser_count==before+1);
    }
    assert(restored==5u && geyser_count==6);

    /* The callback cannot turn an arbitrary Extractor coordinate into a
     * node, and another mission does not mutate the shared gas list. */
    {
        Building bogus={0}; bogus.type=B_EXTRACTOR; bogus.x=1; bogus.y=1;
        int before=geyser_count;
        race_z7_note_building_death(&bogus);
        assert(geyser_count==before);
        race_mode=1;
        race_z7_note_building_death(&bogus);
        assert(geyser_count==before);
        race_mode=0;
    }

    assert(race_save_game());
    assert(race_data.state[16]==6);
    assert(race_z7_save_candidate_valid(&race_data,&race_data.z7,
                                        SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    /* Candidate validation uses the immutable catalog and remains valid
     * before the active-map pointer has been rebound on HOME/load. */
    race_z7_map=0;
    assert(race_z7_save_candidate_valid(&race_data,&race_data.z7,
                                        SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    race_z7_map=map;
    geyser_count=1;
    memset(geysers,0,sizeof(geysers));
    assert(race_load_game());
    assert(geyser_count==6);
    for (unsigned row=0;row<map->unit_count;++row) {
        const Sc64ImportedUnit *src=&map->units[row];
        if (src->source_type==149u)
            assert(geyser_position(src->x_px,src->y_px));
    }

    {
        RaceSave bad=race_data;
        bad.geysers[2]=bad.geysers[1];
        assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                             SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
        reject_without_mutation(&bad);
        bad=race_data; bad.geysers[2].x=17; bad.geysers[2].y=19;
        assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                             SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
        reject_without_mutation(&bad);
        bad=race_data; bad.state[16]=MAX_RACE_GEYSERS+1;
        assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                             SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    }

    /* A full bounded list fails closed instead of writing past the array. */
    {
        int before=geyser_count;
        Geyser old_nodes[MAX_RACE_GEYSERS];
        memcpy(old_nodes,geysers,sizeof(old_nodes));
        geyser_count=MAX_RACE_GEYSERS;
        for (int i=0;i<MAX_RACE_GEYSERS;++i) {
            geysers[i].x=16+i*16; geysers[i].y=16;
        }
        Building source={0}; source.type=B_EXTRACTOR;
        source.x=map->units[0].x_px; source.y=map->units[0].y_px;
        for (unsigned row=0;row<map->unit_count;++row)
            if (map->units[row].source_type==149u) {
                source.x=map->units[row].x_px; source.y=map->units[row].y_px; break;
            }
        race_z7_note_building_death(&source);
        assert(geyser_count==MAX_RACE_GEYSERS &&
               race_z7.runtime.fault==SC64_Z7_RUNTIME_FAULT_ACTOR);
        geyser_count=before;
        memcpy(geysers,old_nodes,sizeof(old_nodes));
    }
    return 0;
}
