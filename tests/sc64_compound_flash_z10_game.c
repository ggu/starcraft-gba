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
    if(!strcmp(argv[1],"write")) {
        int parent=-1;launch_z10_from_normal_menu();
        for(int i=0;i<MAX_UNITS;++i)if(u[i].alive && u[i].type==U_REAVER &&
            u[i].owner==ENEMY && sc64_controller_unit(i)==0){parent=i;break;}
        assert(parent>=0 && u[parent].raw_owner==0 && sc64_controller_transfer_unit(parent,7));
        race_zl_ai_minerals[1]=15;assert(race_scarab_ai_restock_stage(parent));
        race_scarab_human_parent_step(parent);
        Sc64ScarabParent p;Sc64ScarabCounts counts;
        assert(race_scarab_parent_read(parent,0,&p,&counts) && p.active_slot>=0);
        assert(sc64_scarab_stage_payer[parent]==13 && race_zl_ai_minerals[1]==0);
        memset(device.bytes,0xff,sizeof device.bytes);
        assert(race_save_game());copy_file(argv[2],1);
    } else if(!strcmp(argv[1],"read")) {
        game_init();copy_file(argv[2],0);assert(race_load_game());
        int found=0;
        for(int i=0;i<MAX_UNITS;++i)if(u[i].alive && u[i].type==U_REAVER &&
            u[i].raw_owner==0 && sc64_controller_unit(i)==7) {
            Sc64ScarabParent p;Sc64ScarabCounts counts;
            assert(race_scarab_parent_read(i,0,&p,&counts) && p.active_slot>=0 &&
                   sc64_scarab_stage_payer[i]==13 && race_zl_ai_minerals[1]==0);
            assert(!sc64_controller_transfer_unit(i,2));found++;
        }
        assert(found==1);
    } else assert(0);
    puts("Z10 compound Flash training: PASS");return 0;
}
