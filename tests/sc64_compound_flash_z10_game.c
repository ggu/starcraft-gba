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
static int cf_read(void *c,uint32_t p,void *v,uint32_t n){return gba_flash1m_read(c,p,v,n);}
static int cf_program(void *c,uint32_t p,const void *v,uint32_t n){return gba_flash1m_program(c,p,v,n);}
static int cf_erase(void *c,uint32_t p){return gba_flash1m_erase_sector(c,p);}
static GbaFlashJournalIo io={&flash,cf_read,cf_erase,cf_program};
const GbaFlashJournalIo *sc64_q8_flash_host_io(void){return &io;}
static void copy_file(const char *path,int writing)
{
    FILE *f=fopen(path,writing?"wb":"rb");assert(f);
    if(writing)assert(fwrite(device.bytes,1,sizeof device.bytes,f)==sizeof device.bytes);
    else assert(fread(device.bytes,1,sizeof device.bytes,f)==sizeof device.bytes && fgetc(f)==EOF);
    assert(!fclose(f));
}
int main(int argc,char **argv)
{
    assert(argc==3);gba_init();
    if(!strncmp(argv[1],"write-",6)) {
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
        if(strcmp(argv[1],"write-training")) {
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
                assert(u[child].alive && (u[child].x!=x || u[child].y!=y) && !race_impact_count);
            }
        } else assert(!strcmp(argv[1],"write-training") || !strcmp(argv[1],"write-inside"));
        memset(device.bytes,0xff,sizeof device.bytes);
        assert(race_save_game());copy_file(argv[2],1);
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
            assert(fractional>=1);found++;
        }
        assert(found==1);
    } else assert(0);
    puts("Z10 compound Flash paid state: PASS");return 0;
}
