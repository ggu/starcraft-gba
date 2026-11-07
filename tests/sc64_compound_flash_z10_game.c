/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual Z10 game codec through a strict authored HOST Flash model. */
#define main sc64_unused_flash_model_main
#include "gba_flash1m_model.c"
#undef main
#include "../common/gba.c"
#define main sc64_unused_z10_live_main
#include "sc64_zl_scarab_controller_game.c"
#undef main
static Model device;
static GbaFlash1m flash={&device,rd,wr,20,0,0};
static int erases,programs,fail_prefix,inject_reselect,injected,fault_triggered;
static const char *fault_phase;
static u8 external_after[131072];
static int cf_read(void *c,uint32_t p,void *v,uint32_t n)
{
#ifdef SC64_CF_TEST_DIAGNOSTICS
    if(inject_reselect && !injected && sc64_cf_test_stage==5) {
        device.bytes[gba_fj_base(2)+SC64_CFR_MARKER_OFFSET+1]^=1;
        memcpy(external_after,device.bytes,sizeof external_after);injected=1;
    }
#endif
    if(fault_phase && !strcmp(fault_phase,"verify") && !fault_triggered && programs>=12 &&
       p==gba_fj_metadata(2) && n==16){fault_triggered=1;return 0;}
    return gba_flash1m_read(c,p,v,n);
}
static int cf_program(void *c,uint32_t p,const void *v,uint32_t n)
{
    ++programs;
    if(fail_prefix && n==SC64_CF_HEADER && (p==gba_fj_metadata(2)+16 || p==gba_fj_metadata(3)+16))return 0;
    if(fault_phase && !fault_triggered) {
        u32 meta=gba_fj_metadata(2),base=gba_fj_base(2),partial=0;
        if(!strcmp(fault_phase,"marker") && p==base+SC64_CFR_MARKER_OFFSET && n==1)partial=1;
        else if(!strcmp(fault_phase,"meta") && p==meta && n==16)partial=8;
        else if(!strcmp(fault_phase,"header") && p==meta+16 && n==SC64_CF_HEADER)partial=16;
        else if(!strcmp(fault_phase,"base") && p==meta+16+SC64_CF_HEADER && n>64)partial=64;
        else if(!strcmp(fault_phase,"tail") && n==SC64_CF_SIDE)partial=64;
        else if(!strcmp(fault_phase,"tag") && p==base && n==4)partial=2;
        if(partial){fault_triggered=1;assert(gba_flash1m_program(c,p,v,partial));return 0;}
    }
    return gba_flash1m_program(c,p,v,n);
}
static int cf_erase(void *c,uint32_t p)
{
    ++erases;
    if(fault_phase && !fault_triggered &&
       ((!strcmp(fault_phase,"commit-erase") && p==gba_fj_base(2)) ||
        (!strcmp(fault_phase,"data-erase") && p==gba_fj_base(2)+GBA_FJ_SECTOR))) {
        fault_triggered=1;assert(gba_flash1m_erase_sector(c,p));return 0;
    }
    return gba_flash1m_erase_sector(c,p);
}
static GbaFlashJournalIo io={&flash,cf_read,cf_erase,cf_program};
const GbaFlashJournalIo *sc64_q8_flash_host_io(void){return &io;}
static void copy_file(const char *path,int writing)
{
    FILE *f=fopen(path,writing?"wb":"rb");assert(f);
    if(writing)assert(fwrite(device.bytes,1,sizeof device.bytes,f)==sizeof device.bytes);
    else assert(fread(device.bytes,1,sizeof device.bytes,f)==sizeof device.bytes && fgetc(f)==EOF);
    assert(!fclose(f));
}
typedef struct {
    Unit units[MAX_UNITS];Building buildings[MAX_BUILDINGS];
    u8 controller_units[MAX_UNITS],controller_buildings[MAX_BUILDINGS],payers[MAX_UNITS],debts[MAX_UNITS+MAX_BUILDINGS];
    Sc64ProtossResearchLevels levels;Sc64ImpactEvent impacts[RACE_IMPACT_CAPACITY];Sc64BcYamatoShot shots[SC64_BC_SHOT_LIMIT];
    u32 town_minerals[SC64_ZL_AI_TOWNS],town_gas[SC64_ZL_AI_TOWNS];
    int impact_count,minerals,gas,frame,screen,mission,mode;
} LiveSnapshot;
static LiveSnapshot before_live;
static void live_capture(LiveSnapshot *s)
{
    memset(s,0,sizeof *s);memcpy(s->units,u,sizeof s->units);memcpy(s->buildings,b,sizeof s->buildings);
    memcpy(s->controller_units,sc64_current_unit,sizeof s->controller_units);memcpy(s->controller_buildings,sc64_current_building,sizeof s->controller_buildings);
    memcpy(s->payers,sc64_scarab_stage_payer,sizeof s->payers);memcpy(s->debts,sc64_pr_hp_debt,sizeof s->debts);
    s->levels=sc64_experimental_research_levels;memcpy(s->impacts,race_impact_rows,sizeof s->impacts);memcpy(s->shots,sc64_bc_shots,sizeof s->shots);
    memcpy(s->town_minerals,race_zl_ai_minerals,sizeof s->town_minerals);memcpy(s->town_gas,race_zl_ai_gas,sizeof s->town_gas);
    s->impact_count=race_impact_count;s->minerals=minerals;s->gas=gas;s->frame=frame;s->screen=screen;s->mission=race_selected_mission;s->mode=race_mode;
}
static void live_unchanged(void)
{
    static LiveSnapshot after;live_capture(&after);assert(!memcmp(&before_live,&after,sizeof after));
}
typedef struct {
    int parent,child,target,parent_carry,parent_timer,parent_owner,parent_origin,parent_controller,payer;
    int carry,timer,order,target_ref,x,y,path_timer,path_retry,path_x,path_y,weapon,cd,child_controller;
    int target_hp,target_debt,frame,minerals,gas,town0,town1,town2,impacts;
} FlightSnapshot;
typedef struct {int frame,child_alive,target_alive,target_hp,target_debt,town0,town1,town2,minerals,gas,impacts,controller,payer;} FlightOutcome;
static void side_file(const char *base,const char *suffix,void *data,size_t n,int writing)
{
    char path[1024];assert(snprintf(path,sizeof path,"%s%s",base,suffix)>0);
    FILE *f=fopen(path,writing?"wb":"rb");assert(f);
    if(writing)assert(fwrite(data,1,n,f)==n);
    else assert(fread(data,1,n,f)==n && fgetc(f)==EOF);
    assert(!fclose(f));
}
static FlightSnapshot flight_snapshot(int parent)
{
    FlightSnapshot s={0};s.parent=parent;s.child=-1;
    for(int i=0;i<MAX_UNITS;++i){int owner;unsigned phase;
        if(race_scarab_child_lookup(i,&owner,&phase) && owner==parent && phase==SC64_SCARAB_CHILD_OUTSIDE){s.child=i;break;}}
    assert(s.child>=0);Unit *q=&u[s.child];s.target=q->target;
    assert(s.target>=0 && s.target<MAX_UNITS && u[s.target].alive);
    s.parent_carry=u[parent].carry;s.parent_timer=u[parent].work_timer;s.parent_owner=u[parent].owner;
    s.parent_origin=u[parent].raw_owner;s.parent_controller=sc64_controller_unit(parent);s.payer=sc64_scarab_stage_payer[parent];
    s.carry=q->carry;s.timer=q->work_timer;s.order=q->order;s.target_ref=q->target;s.x=q->x;s.y=q->y;
    s.path_timer=q->path_timer;s.path_retry=q->path_retry;s.path_x=q->path_x;s.path_y=q->path_y;
    s.weapon=q->weapon;s.cd=q->cd;s.child_controller=sc64_controller_unit(s.child);
    s.target_hp=u[s.target].hp;s.target_debt=sc64_pr_hp_debt[s.target];s.frame=frame;
    s.minerals=minerals;s.gas=gas;s.town0=race_zl_ai_minerals[0];s.town1=race_zl_ai_minerals[1];s.town2=race_zl_ai_minerals[2];s.impacts=race_impact_count;
    return s;
}
static FlightOutcome flight_finish(const FlightSnapshot *s)
{
    int initial_hp=u[s->target].hp,damage=0;
    for(int i=0;i<150;++i) {
        ++frame;if(u[s->child].alive && sc64_battlecruiser_source_tick_due())race_scarab_control_dispatch(s->child);
        race_impact_tick();if(!u[s->target].alive || u[s->target].hp<initial_hp)damage=1;
        if(damage && !u[s->child].alive && !race_impact_count)break;
    }
    assert(damage && !u[s->child].alive && !race_impact_count);
    FlightOutcome o={frame,u[s->child].alive,u[s->target].alive,u[s->target].hp,sc64_pr_hp_debt[s->target],
        (int)race_zl_ai_minerals[0],(int)race_zl_ai_minerals[1],(int)race_zl_ai_minerals[2],minerals,gas,race_impact_count,
        sc64_controller_unit(s->parent),sc64_scarab_stage_payer[s->parent]};
    return o;
}
/* Small strict 1->0 model exercises the actual recovery writer primitives.
 * It is not a substitute for the separate real-game continuation fixture. */
typedef struct {u8 bytes[GBA_FJ_BYTES];int budget,read_budget,mutations,reads;} TinyDevice;
static TinyDevice tiny;static u8 tiny_initial[GBA_FJ_BYTES],tiny_winner[GBA_FJ_SLOT];
static int tiny_read(void *v,u32 p,void *out,u32 n)
{TinyDevice *t=v;if(p>GBA_FJ_BYTES || n>GBA_FJ_BYTES-p || t->read_budget==0)return 0;if(t->read_budget>0)--t->read_budget;++t->reads;memcpy(out,t->bytes+p,n);return 1;}
static int tiny_mutation(TinyDevice *t)
{if(t->budget==0)return 0;if(t->budget>0)--t->budget;++t->mutations;return 1;}
static int tiny_erase(void *v,u32 p)
{TinyDevice *t=v;if(p<32768 || p>=98304 || (p&4095) || !tiny_mutation(t))return 0;memset(t->bytes+p,255,4096);return 1;}
static int tiny_program(void *v,u32 p,const void *src,u32 n)
{
    TinyDevice *t=v;const u8 *b=src;if(p<32768 || p>98304 || n>98304-p)return 0;
    for(u32 i=0;i<n;++i){if((t->bytes[p+i]&b[i])!=b[i] || !tiny_mutation(t))return 0;t->bytes[p+i]&=b[i];}return 1;
}
static GbaFlashJournalIo tiny_io={&tiny,tiny_read,tiny_erase,tiny_program};
static int tiny_proof(void *ctx,const GbaFlashJournalIo *device,unsigned index)
{
    Sc64CfSnapshot *s=ctx;u8 metadata[16],prefix[2],chunk[64];u32 hash=2166136261u;
    if(!device->read(device->context,gba_fj_metadata(index),metadata,16))return -1;
    GbaFlashJournalRecord r={index,gba_fj_u32(metadata),gba_fj_u32(metadata+4),gba_fj_u32(metadata+8),gba_fj_u32(metadata+12)};
    if(r.tag!=SC64_COMPOUND_FLASH_TAG || r.length<2 || r.length>GBA_FJ_PAYLOAD_MAX || !r.sequence)return 0;
    if(gba_fj_read_payload(device,&r,0,prefix,2)!=GBA_FJ_OK)return -1;
    if(prefix[0]!=0xc3 || prefix[1]!=0x73)return 0;
    for(u32 off=0;off<r.length;){u32 n=r.length-off;if(n>sizeof chunk)n=sizeof chunk;
        if(gba_fj_read_payload(device,&r,off,chunk,n)!=GBA_FJ_OK)return -1;
        hash=gba_fj_hash(hash,chunk,n);off+=n;}
    if(hash!=r.checksum)return 0;s->proven[index]=r;s->has_proof[index]=1;return 1;
}
static int tiny_begin(void)
{
    memset(&sc64_cf_snapshot,0,sizeof sc64_cf_snapshot);
    for(unsigned i=2;i<4;++i) {
        int state=sc64_cfr_classify(&tiny_io,i,SC64_COMPOUND_FLASH_TAG,tiny_proof,&sc64_cf_snapshot);
        if(state==SC64_CFR_FOREIGN)return 0;sc64_cf_snapshot.classes[i]=state;
        GbaFlashJournalRecord r;int status=gba_fj_probe(&tiny_io,i,&r);
        if(status<0)return 0;
        if(status==GBA_FJ_OK) {
            if(tiny_proof(&sc64_cf_snapshot,&tiny_io,i)!=1)return 0;
            sc64_cf_snapshot.seen[i]=sc64_cf_snapshot.accepted[i]=1;sc64_cf_snapshot.records[i]=r;
        }
    }
    int status=gba_fj_select(&tiny_io,sc64_cf_reselect,&sc64_cf_snapshot,&sc64_cf_previous);
    if(status<0)return 0;sc64_cf_has_previous=status==GBA_FJ_OK;
    return sc64_cf_digest(&tiny_io,&sc64_cf_snapshot.digest_a,&sc64_cf_snapshot.digest_b);
}
static int tiny_write(const u8 *prefix,const u8 *body,const u8 *tail)
{return sc64_cf_recovery_write(&tiny_io,prefix,16,body,48,tail,16);}
static void tiny_assert_legacy(void){for(unsigned i=0;i<32768;++i)assert(tiny.bytes[i]==255);}
static void tiny_primitive_cuts(void)
{
    u8 prefix[16],body[48],tail[16],prefix_copy[16],body_copy[48],tail_copy[16];
    for(unsigned i=0;i<sizeof prefix;++i)prefix[i]=(u8)(i+0x31);prefix[0]=0xc3;prefix[1]=0x73;
    for(unsigned i=0;i<sizeof body;++i)body[i]=(u8)(i*3+1);
    for(unsigned i=0;i<sizeof tail;++i)tail[i]=(u8)(i*7+5);
    memcpy(prefix_copy,prefix,sizeof prefix);memcpy(body_copy,body,sizeof body);memcpy(tail_copy,tail,sizeof tail);
    for(int scenario=0;scenario<3;++scenario) {
        memset(&tiny,0,sizeof tiny);memset(tiny.bytes,255,sizeof tiny.bytes);tiny.budget=tiny.read_budget=-1;
        if(scenario>=1)assert(tiny_begin() && tiny_write(prefix,body,tail));
        if(scenario==2)assert(tiny_begin() && tiny_write(prefix,body,tail));
        assert(tiny_begin());int had_winner=sc64_cf_has_previous;GbaFlashJournalRecord winner=sc64_cf_previous;
        memcpy(tiny_initial,tiny.bytes,sizeof tiny_initial);
        if(had_winner)memcpy(tiny_winner,tiny.bytes+gba_fj_base(winner.index),sizeof tiny_winner);
        tiny.mutations=tiny.reads=0;assert(tiny_write(prefix,body,tail));int operations=tiny.mutations,reads=tiny.reads;
        for(int cut=0;cut<=operations;++cut) {
            memcpy(tiny.bytes,tiny_initial,sizeof tiny_initial);tiny.budget=tiny.read_budget=-1;assert(tiny_begin());
            tiny.budget=cut;(void)tiny_write(prefix,body,tail);tiny.budget=-1;
            if(had_winner)assert(!memcmp(tiny.bytes+gba_fj_base(winner.index),tiny_winner,sizeof tiny_winner));tiny_assert_legacy();
            assert(tiny_begin() && tiny_write(prefix,body,tail));tiny_assert_legacy();
            assert(!memcmp(prefix,prefix_copy,sizeof prefix) && !memcmp(body,body_copy,sizeof body) && !memcmp(tail,tail_copy,sizeof tail));
        }
        for(int cut=0;cut<=reads;++cut) {
            memcpy(tiny.bytes,tiny_initial,sizeof tiny_initial);tiny.budget=tiny.read_budget=-1;assert(tiny_begin());
            tiny.read_budget=cut;(void)tiny_write(prefix,body,tail);tiny.read_budget=-1;
            if(had_winner)assert(!memcmp(tiny.bytes+gba_fj_base(winner.index),tiny_winner,sizeof tiny_winner));tiny_assert_legacy();
            assert(tiny_begin() && tiny_write(prefix,body,tail));
        }
        printf("tiny scenario=%d program/erase cuts=%d read cuts=%d PASS\n",scenario,operations+1,reads+1);
    }
    /* Every subset in one marker byte while all prior bytes are complete:
     * unfinished bytes and the entire data area remain erased. */
    unsigned marker_cases=0,tag_cases=0,erase_marker_cases=0;
    for(unsigned byte=0;byte<SC64_CFR_MARKER_SIZE;++byte)for(unsigned mask=0;mask<256;++mask) {
        memset(&tiny,0,sizeof tiny);memset(tiny.bytes,255,sizeof tiny.bytes);tiny.budget=tiny.read_budget=-1;
        u32 mark=gba_fj_base(2)+SC64_CFR_MARKER_OFFSET;
        for(unsigned k=0;k<byte;++k)tiny.bytes[mark+k]=sc64_cfr_marker[k];
        tiny.bytes[mark+byte]=(u8)(sc64_cfr_marker[byte]|mask);
        assert(tiny_begin() && tiny_write(prefix,body,tail));tiny_assert_legacy();++marker_cases;
    }
    /* An old v3 target may suffer arbitrary 0->1 subsets within any tag or
     * marker byte during isolated-sector erase. The other winner is intact. */
    memset(&tiny,0,sizeof tiny);memset(tiny.bytes,255,sizeof tiny.bytes);tiny.budget=tiny.read_budget=-1;
    assert(tiny_begin() && tiny_write(prefix,body,tail));
    assert(tiny_begin() && tiny_write(prefix,body,tail));
    assert(tiny_begin() && sc64_cf_previous.index==3);
    memcpy(tiny_initial,tiny.bytes,sizeof tiny_initial);memcpy(tiny_winner,tiny.bytes+gba_fj_base(3),sizeof tiny_winner);
    for(unsigned byte=0;byte<4;++byte)for(unsigned mask=0;mask<256;++mask) {
        memcpy(tiny.bytes,tiny_initial,sizeof tiny_initial);tiny.budget=tiny.read_budget=-1;
        tiny.bytes[gba_fj_base(2)+byte]|=(u8)mask;
        assert(tiny_begin() && tiny_write(prefix,body,tail));
        assert(!memcmp(tiny.bytes+gba_fj_base(3),tiny_winner,sizeof tiny_winner));tiny_assert_legacy();++tag_cases;
    }
    for(unsigned byte=0;byte<SC64_CFR_MARKER_SIZE;++byte)for(unsigned mask=0;mask<256;++mask) {
        memcpy(tiny.bytes,tiny_initial,sizeof tiny_initial);tiny.budget=tiny.read_budget=-1;
        memset(tiny.bytes+gba_fj_base(2),255,4);
        tiny.bytes[gba_fj_base(2)+SC64_CFR_MARKER_OFFSET+byte]|=(u8)mask;
        assert(tiny_begin() && tiny_write(prefix,body,tail));
        assert(!memcmp(tiny.bytes+gba_fj_base(3),tiny_winner,sizeof tiny_winner));tiny_assert_legacy();++erase_marker_cases;
    }
    printf("marker-program=%u partial-tag-erase=%u partial-marker-erase=%u PASS\n",marker_cases,tag_cases,erase_marker_cases);
    /* Interrupted first save A must never be promoted during a distinct B
     * retry. Test every B mutation cut after each incomplete A tag byte. */
    static u8 partial[GBA_FJ_BYTES];u8 body_b[48],got[48];unsigned distinct_cases=0;
    for(unsigned i=0;i<sizeof body_b;++i)body_b[i]=(u8)(body[i]^0x5a);
    memset(&tiny,0,sizeof tiny);memset(tiny.bytes,255,sizeof tiny.bytes);tiny.budget=tiny.read_budget=-1;
    memcpy(tiny_initial,tiny.bytes,sizeof tiny_initial);assert(tiny_begin());tiny.mutations=0;
    assert(tiny_write(prefix,body,tail));int first_operations=tiny.mutations;
    for(int tag_bytes=0;tag_bytes<4;++tag_bytes) {
        memcpy(tiny.bytes,tiny_initial,sizeof tiny_initial);tiny.budget=tiny.read_budget=-1;assert(tiny_begin());
        tiny.budget=first_operations-4+tag_bytes;assert(!tiny_write(prefix,body,tail));tiny.budget=-1;
        GbaFlashJournalRecord r;assert(gba_fj_probe(&tiny_io,2,&r)==GBA_FJ_NONE);
        memcpy(partial,tiny.bytes,sizeof partial);assert(tiny_begin() && !sc64_cf_has_previous);
        tiny.mutations=0;assert(tiny_write(prefix,body_b,tail));int retry_operations=tiny.mutations;
        for(int cut=0;cut<=retry_operations;++cut) {
            memcpy(tiny.bytes,partial,sizeof partial);tiny.budget=tiny.read_budget=-1;assert(tiny_begin() && !sc64_cf_has_previous);
            tiny.budget=cut;(void)tiny_write(prefix,body_b,tail);tiny.budget=-1;
            for(unsigned slot=2;slot<4;++slot)if(gba_fj_probe(&tiny_io,slot,&r)==GBA_FJ_OK) {
                assert(gba_fj_read_payload(&tiny_io,&r,16,got,sizeof got)==GBA_FJ_OK && !memcmp(got,body_b,sizeof got));
            }
            assert(tiny_begin() && tiny_write(prefix,body_b,tail));tiny_assert_legacy();++distinct_cases;
        }
    }
    printf("distinct-A-B retry cuts=%u PASS\n",distinct_cases);
}
int main(int argc,char **argv)
{
    assert(argc==3);gba_init();
    if(!strcmp(argv[1],"primitive-cuts")) {tiny_primitive_cuts();
    } else if(!strncmp(argv[1],"write-",6)) {
        int parent=-1,target=-1;launch_z10_from_normal_menu();
        for(int i=0;i<MAX_UNITS;++i) {
            if(parent<0 && u[i].alive && u[i].type==U_REAVER && u[i].owner==ENEMY && sc64_controller_unit(i)==0)parent=i;
            if(target<0 && u[i].alive && u[i].owner==SIDE && u[i].type==U_ZERGLING && !invincible_units[i])target=i;
        }
        assert(parent>=0 && target>=0 && u[parent].raw_owner==0 && sc64_controller_transfer_unit(parent,7));
        /* A real shared packet creates a half-HP debt on a normal live actor. */
        unsigned hp_before;assert(sc64_pr_q8_actor_hp(target,&hp_before) && hp_before>128);
        hit_target_impl_ex(target,128,U_MARINE,ENEMY,SC64_STOCK_DMG_IGNORE_ARMOR,SC64_HIT_PACKET_Q8);
        assert(sc64_pr_hp_debt[target]==128);
        if(!strcmp(argv[1],"write-outside") || !strcmp(argv[1],"write-moving")) {
            int sx=-1,sy=-1;
            for(int y=64;y<WORLD_H-64 && sx<0;y+=16)for(int x=64;x<WORLD_W-112;x+=16) {
                int clear=1;
                for(int k=0;k<=48;k+=4)if(terrain_blocked((x+k)/16,y/16))clear=0;
                for(int j=0;j<MAX_BUILDINGS && clear;++j)if(b[j].alive)
                    for(int k=0;k<=48;k+=4)if(dist2(x+k,y,b[j].x,b[j].y)<40*40){clear=0;break;}
                for(int j=0;j<MAX_UNITS && clear;++j)if(j!=parent && j!=target && u[j].alive)
                    for(int k=0;k<=48;k+=4)if(dist2(x+k,y,u[j].x,u[j].y)<36*36){clear=0;break;}
                if(clear && direct_path_clear(x,y,x+48,y)){sx=x;sy=y;break;}
            }
            assert(sx>=0);u[parent].x=sx;u[parent].y=sy;u[parent].cd=0;
            u[target].x=sx+48;u[target].y=sy;
        }
        race_zl_ai_minerals[1]=15;assert(race_scarab_ai_restock_stage(parent));
        race_scarab_human_parent_step(parent);
        Sc64ScarabParent p;Sc64ScarabCounts counts;
        assert(race_scarab_parent_read(parent,0,&p,&counts) && p.active_slot>=0);
        assert(sc64_scarab_stage_payer[parent]==13 && race_zl_ai_minerals[1]==0);
        if(strcmp(argv[1],"write-training") && strcmp(argv[1],"write-two")) {
            for(int i=0;i<110;++i) {
                assert(race_scarab_parent_read(parent,0,&p,&counts));
                if(counts.inside)break;
                race_scarab_human_parent_step(parent);
            }
            assert(race_scarab_parent_read(parent,0,&p,&counts) && counts.inside==1);
        }
        if(!strcmp(argv[1],"write-outside") || !strcmp(argv[1],"write-moving")) {
            tick_combat(&u[parent],target);
            assert(race_scarab_parent_read(parent,0,&p,&counts) && counts.outside==1);
            if(!strcmp(argv[1],"write-moving")) {
                int child=-1;
                for(int i=0;i<MAX_UNITS;++i){int owner;unsigned phase;
                    if(race_scarab_child_lookup(i,&owner,&phase) && owner==parent && phase==SC64_SCARAB_CHILD_OUTSIDE){child=i;break;}}
                assert(child>=0);int x=u[child].x,y=u[child].y;
                for(int i=0;i<30 && u[child].alive && u[child].x==x && u[child].y==y;++i) {
                    ++frame;if(sc64_battlecruiser_source_tick_due())race_scarab_control_dispatch(child);
                }
                fprintf(stderr,"moving child=%d alive=%d xy=%d,%d start=%d,%d order=%d target=%d carry=%d timer=%d impacts=%u\n",child,u[child].alive,u[child].x,u[child].y,x,y,u[child].order,u[child].target,u[child].carry,u[child].work_timer,race_impact_count);
                Unit wire;Sc64ZpPackedUnitSave row;
                assert(sc64_scarab_flight_to_wire(&u[child],&wire,1,0));
                fprintf(stderr,"moving waypoint x=%u y=%u narrow-encode=%d\n",wire.path_timer,wire.path_retry,sc64_zp_unit_save_encode(&row,&wire,(unsigned)race_cols*16,(unsigned)race_rows*16));
                assert((wire.path_timer>24 || wire.path_retry>12) && !sc64_zp_unit_save_encode(&row,&wire,(unsigned)race_cols*16,(unsigned)race_rows*16));
                assert(u[child].alive && (u[child].x!=x || u[child].y!=y) && !race_impact_count);
            }
        } else assert(!strcmp(argv[1],"write-training") || !strcmp(argv[1],"write-inside") || !strcmp(argv[1],"write-two"));
        memset(device.bytes,0xff,sizeof device.bytes);
        int saved=race_save_game();
#ifdef SC64_CF_TEST_DIAGNOSTICS
        fprintf(stderr,"save=%d stage=%d pending=%d\n",saved,sc64_cf_test_stage,race_scarab_probe_bullets_pending());
        if(!saved && sc64_cf_test_stage==1) {
            Sc64StageSaveAuth a;const Sc64StageSaveAuth *guard_auth=&a;
            Sc64StageTagPair pair={SC64_COMPOUND_FLASH_TAG,SAVE_VERSION,SC64_STAGE_SAVE_SCARAB};
            assert(sc64_stage_save_authenticate(&a,&race_data,pair.outer,race_data.version,&pair,1));
            int graph=race_scarab_save_candidate_valid(&race_data,(unsigned)race_cols*16,(unsigned)race_rows*16 SC64_GCTX_ARG);
            int zl=race_zl_save_candidate_world_valid(&race_data,race_cols,race_rows SC64_GCTX_ARG);
            int cargo=race_save_worker_cargo_prepare(&race_data,(unsigned)race_cols*16,(unsigned)race_rows*16,0 SC64_GCTX_ARG);
            fprintf(stderr,"graph=%d cargo=%d zl=%d\n",graph,cargo,zl);
        }
#endif
        assert(saved);
        if(!strcmp(argv[1],"write-two")) {int old=minerals;side_file(argv[2],".old",&old,sizeof old,1);minerals+=37;assert(race_save_game());}
        copy_file(argv[2],1);
        if(!strcmp(argv[1],"write-moving")) {
            FlightSnapshot original=flight_snapshot(parent);side_file(argv[2],".before",&original,sizeof original,1);
            FlightOutcome outcome=flight_finish(&original);side_file(argv[2],".after",&outcome,sizeof outcome,1);
        }
    } else if(!strncmp(argv[1],"game-cut-",9)) {
        const char *phase=argv[1]+9;game_init();copy_file(argv[2],0);assert(race_load_game());
        static u8 winner[GBA_FJ_SLOT];memcpy(winner,device.bytes+gba_fj_base(3),sizeof winner);
        live_capture(&before_live);erases=programs=0;fault_phase=phase;
        assert(!race_save_game() && fault_triggered);fault_phase=0;live_unchanged();
        assert(!memcmp(winner,device.bytes+gba_fj_base(3),sizeof winner));
        char path[1024];assert(snprintf(path,sizeof path,"%s.%s.cut",argv[2],phase)>0);copy_file(path,1);
    } else if(!strcmp(argv[1],"game-retry")) {
        game_init();copy_file(argv[2],0);assert(race_load_game());live_capture(&before_live);
        assert(race_save_game());live_unchanged();
        char path[1024];assert(snprintf(path,sizeof path,"%s.recovered",argv[2])>0);copy_file(path,1);
    } else if(!strcmp(argv[1],"game-check")) {
        game_init();copy_file(argv[2],0);assert(race_load_game() && race_selected_mission==SC64_Z10_GLOBAL_ID);
        int found=0;for(int i=0;i<MAX_UNITS;++i)if(u[i].alive && u[i].type==U_REAVER &&
            u[i].raw_owner==0 && sc64_controller_unit(i)==7 && sc64_scarab_stage_payer[i]==13)++found;
        assert(found==1 && race_zl_ai_minerals[1]==0);
    } else if(!strcmp(argv[1],"stale-marker")) {
        launch_z10_from_normal_menu();memset(device.bytes,0xff,sizeof device.bytes);
        /* Physically reachable partial owner marker; data and commit blank. */
        device.bytes[gba_fj_base(2)+SC64_CFR_MARKER_OFFSET]=sc64_cfr_marker[0];
        live_capture(&before_live);erases=programs=0;inject_reselect=1;
        assert(!race_save_game() && injected && !erases && !programs &&
               !memcmp(external_after,device.bytes,sizeof external_after));live_unchanged();
    } else if(!strcmp(argv[1],"cut-write")) {
        launch_z10_from_normal_menu();memset(device.bytes,0xff,sizeof device.bytes);
        live_capture(&before_live);fail_prefix=1;assert(!race_save_game());fail_prefix=0;live_unchanged();
        assert(gba_fj_u32(device.bytes+gba_fj_metadata(2))==SC64_COMPOUND_FLASH_TAG &&
               gba_fj_u32(device.bytes+gba_fj_base(2))==0xffffffffu);
        copy_file(argv[2],1);
    } else if(!strcmp(argv[1],"cut-read")) {
        launch_z10_from_normal_menu();copy_file(argv[2],0);static u8 prior[131072];
        memcpy(prior,device.bytes,sizeof prior);live_capture(&before_live);erases=programs=0;
        assert(!race_load_game());live_unchanged();
        assert(race_save_game() && (erases || programs));live_unchanged();
        char recovered[1024];assert(snprintf(recovered,sizeof recovered,"%s.recovered",argv[2])>0);
        copy_file(recovered,1);
    } else if(!strcmp(argv[1],"verify-recovered")) {
        game_init();copy_file(argv[2],0);assert(race_load_game() && race_selected_mission==SC64_Z10_GLOBAL_ID);
    } else if(!strcmp(argv[1],"stale-shot")) {
        int caster=-1,dead=-1;launch_z10_from_normal_menu();memset(device.bytes,0xff,sizeof device.bytes);
        for(int i=0;i<MAX_UNITS;++i){if(caster<0 && u[i].alive)caster=i;if(dead<0 && !u[i].alive)dead=i;}
        assert(caster>=0 && dead>=0);u[caster].order=ORDER_ATTACK_TARGET;u[caster].target=dead;
        memset(sc64_bc_shots,0,sizeof sc64_bc_shots);sc64_bc_shots[0].state=SC64_BC_SHOT_CHARGING;
        sc64_bc_shots[0].caster=(u16)caster;sc64_bc_shots[0].target=(u16)dead;
        live_capture(&before_live);erases=programs=0;assert(!race_save_game());
        assert(!erases && !programs);live_unchanged();
    } else if(!strcmp(argv[1],"retained-v1")) {
        launch_z10_from_normal_menu();copy_file(argv[2],0);GbaFlashJournalRecord r;
        assert(gba_fj_probe(&io,2,&r)==GBA_FJ_OK && sc64_cf_u16(device.bytes+gba_fj_metadata(2)+20)==1);
        static u8 prior[131072];memcpy(prior,device.bytes,sizeof prior);live_capture(&before_live);erases=programs=0;
        assert(!race_load_game());live_unchanged();
        assert(!race_save_game() && !erases && !programs && !memcmp(prior,device.bytes,sizeof prior));live_unchanged();
    } else if(!strcmp(argv[1],"only-bad") || !strcmp(argv[1],"only-bad-flight") || !strcmp(argv[1],"fallback-bad")) {
        launch_z10_from_normal_menu();copy_file(argv[2],0);
        GbaFlashJournalRecord a,b,*r;int sa=gba_fj_probe(&io,2,&a),sb=gba_fj_probe(&io,3,&b);
        assert(sa==GBA_FJ_OK);r=(sb==GBA_FJ_OK && gba_fj_newer(b.sequence,a.sequence))?&b:&a;
        u32 at=gba_fj_metadata(r->index)+16,base=gba_fj_u32(device.bytes+at+16);
        u32 controls=at+SC64_CF_HEADER+base+MAX_UNITS+MAX_BUILDINGS;unsigned j;
        for(j=0;j<MAX_UNITS+MAX_BUILDINGS && device.bytes[controls+j]==255;++j){}
        assert(j<MAX_UNITS+MAX_BUILDINGS);
        if(!strcmp(argv[1],"only-bad-flight")) {
            for(j=0;j<MAX_UNITS && device.bytes[controls+j]!=255;++j){}
            assert(j<MAX_UNITS);u32 flight=controls+(MAX_UNITS+MAX_BUILDINGS)+MAX_UNITS;
            device.bytes[flight+j*4]=1; /* nonzero waypoint in an empty slot */
        } else device.bytes[controls+j]=254;
        gba_fj_put32(device.bytes+gba_fj_metadata(r->index)+8,gba_fj_hash(2166136261u,device.bytes+at,r->length));
        static u8 prior[131072];memcpy(prior,device.bytes,sizeof prior);live_capture(&before_live);erases=programs=0;
        if(strcmp(argv[1],"fallback-bad")) {assert(!race_load_game());live_unchanged();}
        else {int old;side_file(argv[2],".old",&old,sizeof old,0);assert(race_load_game() && minerals==old);}
        assert(!memcmp(prior,device.bytes,sizeof prior) && !erases && !programs);
        live_capture(&before_live);
        assert(!race_save_game() && !memcmp(prior,device.bytes,sizeof prior) && !erases && !programs);live_unchanged();
    } else if(!strcmp(argv[1],"foreign-v1") || !strcmp(argv[1],"foreign-v2") || !strcmp(argv[1],"foreign-header")) {
        launch_z10_from_normal_menu();copy_file(argv[2],0);
        GbaFlashJournalRecord r;assert(gba_fj_probe(&io,2,&r)==GBA_FJ_OK);
        u32 at=gba_fj_metadata(r.index)+16;
        if(!strcmp(argv[1],"foreign-v1"))device.bytes[at+4]=1;
        else if(!strcmp(argv[1],"foreign-v2"))device.bytes[at+4]=2;
        else device.bytes[at+12]|=0x80;
        assert(gba_fj_probe(&io,2,&r)==GBA_FJ_NONE); /* deliberately bad checksum */
        static u8 prior[131072],ctrl_u[MAX_UNITS],ctrl_b[MAX_BUILDINGS],debts[MAX_UNITS+MAX_BUILDINGS];
        static Unit units[MAX_UNITS];static Building buildings[MAX_BUILDINGS];
        static Sc64BcYamatoShot shots[SC64_BC_SHOT_LIMIT];
        memcpy(prior,device.bytes,sizeof prior);memcpy(units,u,sizeof units);memcpy(buildings,b,sizeof buildings);
        memcpy(ctrl_u,sc64_current_unit,sizeof ctrl_u);memcpy(ctrl_b,sc64_current_building,sizeof ctrl_b);
        memcpy(debts,sc64_pr_hp_debt,sizeof debts);memcpy(shots,sc64_bc_shots,sizeof shots);
        int old_minerals=minerals,old_gas=gas,old_frame=frame;erases=programs=0;
        assert(!race_save_game() && !erases && !programs && !memcmp(prior,device.bytes,sizeof prior));
        assert(!memcmp(units,u,sizeof units) && !memcmp(buildings,b,sizeof buildings) &&
               !memcmp(ctrl_u,sc64_current_unit,sizeof ctrl_u) && !memcmp(ctrl_b,sc64_current_building,sizeof ctrl_b) &&
               !memcmp(debts,sc64_pr_hp_debt,sizeof debts) && !memcmp(shots,sc64_bc_shots,sizeof shots) &&
               old_minerals==minerals && old_gas==gas && old_frame==frame);
    } else if(!strncmp(argv[1],"read-",5)) {
        game_init();copy_file(argv[2],0);assert(race_load_game());
        int found=0;
        for(int i=0;i<MAX_UNITS;++i)if(u[i].alive && u[i].type==U_REAVER &&
            u[i].raw_owner==0 && sc64_controller_unit(i)==7) {
            Sc64ScarabParent p;Sc64ScarabCounts counts;
            int ok=race_scarab_parent_read(i,0,&p,&counts);
            fprintf(stderr,"loaded slot=%d ok=%d active=%d inside=%u outside=%u payer=%u town1=%u\n",i,ok,ok?p.active_slot:-9,ok?counts.inside:0,ok?counts.outside:0,sc64_scarab_stage_payer[i],race_zl_ai_minerals[1]);
            assert(ok && sc64_scarab_stage_payer[i]==13 && race_zl_ai_minerals[1]==0);
            if(!strcmp(argv[1],"read-training"))assert(p.active_slot>=0);
            else if(!strcmp(argv[1],"read-inside"))assert(counts.inside==1);
            else if(!strcmp(argv[1],"read-outside") || !strcmp(argv[1],"read-moving"))assert(counts.outside==1);
            else assert(0);
            assert(!sc64_controller_transfer_unit(i,2));
            int fractional=0;for(int j=0;j<MAX_UNITS;++j)if(u[j].alive && sc64_pr_hp_debt[j]==128)++fractional;
            assert(fractional>=1);
            if(!strcmp(argv[1],"read-moving")) {
                FlightSnapshot expected,actual=flight_snapshot(i);side_file(argv[2],".before",&expected,sizeof expected,0);
                assert(!memcmp(&actual,&expected,sizeof actual));
                FlightOutcome expected_outcome,actual_outcome=flight_finish(&actual);
                side_file(argv[2],".after",&expected_outcome,sizeof expected_outcome,0);
                assert(!memcmp(&actual_outcome,&expected_outcome,sizeof actual_outcome));
            }
            found++;
        }
        assert(found==1);
    } else assert(0);
    puts("Z10 compound Flash paid state: PASS");return 0;
}
