/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free whole-frame oracle for the selected-actor vital bars. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../starcraft/game.c"

enum { POISON=91 };

static int oracle_width(int width)
{
    if (width<13) return 13;
    return width-(width-1)%3;
}

/* Whole cells: at least one when any value remains, rounded to the nearest
 * cell boundary otherwise. */
static int oracle_fill(int value,int maximum,int width)
{
    if (value<=0 || maximum<=0) return 0;
    if (value>=maximum) return width;
    int percent=(int)((int64_t)value*100/maximum);
    int r=percent*width/100;
    if (r<3) return 3;
    if (r%3==2) return r+1;
    return r-r%3;
}

/* Rows relative to the bar top: 'K' frame, 'L'/'D' a strip's light/dark row,
 * ' ' untouched. Strip ids: 0 shield, 1 HP, 2 energy. */
static int oracle_rows(int shield,int energy,char *kind,int *strip)
{
    static const char hp_only[]="KLDK", with_shield[]="KLDKLDK";
    const char *base=shield ? with_shield : hp_only;
    int n=0;
    for (const char *p=base; *p; ++p,++n) {
        kind[n]=*p;
        strip[n]=shield && n<3 ? 0 : 1;
    }
    if (energy) {
        static const char tail[]=" KLDK";
        for (const char *p=tail; *p; ++p,++n) { kind[n]=*p; strip[n]=2; }
    }
    return n;
}

static void check_vitals(int x,int top,int width,int hp,int maxhp,
                         int shield,int maxshield,int energy,int maxenergy,
                         int clip_top,int clip_bottom)
{
    char kind[16]; int strip[16];
    int w=oracle_width(width),left=x-w/2;
    int rows=oracle_rows(maxshield>0,maxenergy>0,kind,strip);
    int fills[3]={oracle_fill(shield,maxshield,w),oracle_fill(hp,maxhp,w),
                  oracle_fill(energy,maxenergy,w)};
    int percent=hp>0 && maxhp>0 ? (int)((int64_t)hp*100/maxhp) : 0;
    u8 light[3]={C_BLUE,percent>=66 ? C_GREEN : percent>=33 ? C_GOLD : C_RED,
                 C_PURPLE};
    u8 dark[3]={C_BLUE_DARK,
                percent>=66 ? C_GREEN_DARK : percent>=33 ? C_GOLD_DARK : C_RED_DARK,
                C_ZERG};
    gfx_begin(POISON);
    gfx_set_clip_y(clip_top,clip_bottom);
    draw_actor_vitals(x,top,width,hp,maxhp,shield,maxshield,energy,maxenergy);
    for (int y=0;y<160;++y) for (int px=0;px<240;++px) {
        int expected=POISON,row=y-top,dx=px-left;
        if (maxhp>0 && y>=12 && y<120 && y>=clip_top && y<clip_bottom &&
            row>=0 && row<rows && dx>=0 && dx<w && kind[row]!=' ') {
            int s=strip[row];
            if (kind[row]=='K' || dx%3==0) expected=C_BLACK;
            else if (dx<fills[s]) expected=kind[row]=='L' ? light[s] : dark[s];
            else expected=kind[row]=='L' ? C_STONE : C_UI;
        }
        if (gfx_test_pixel(px,y)!=expected) {
            fprintf(stderr,"x=%d top=%d width=%d hp=%d/%d sh=%d/%d en=%d/%d "
                    "clip=%d..%d pixel %d,%d got %d want %d\n",x,top,width,hp,
                    maxhp,shield,maxshield,energy,maxenergy,clip_top,
                    clip_bottom,px,y,gfx_test_pixel(px,y),expected);
            assert(0);
        }
    }
}

int main(void)
{
    static const int maxima[]={-1,0,1,20,60,150,350,1000,100000};
    static const int widths[]={0,13,19,40};
    static const int positions[][2]={{-5,6},{8,12},{230,112},{250,140},{20,46}};
    gba_init();
    unsigned cases=0;
    for (unsigned m=0;m<sizeof(maxima)/sizeof(*maxima);++m) {
        int maximum=maxima[m];
        int values[]={-1,0,1,maximum/3,maximum*2/3,maximum-1,maximum,maximum+1};
        for (unsigned w=0;w<sizeof(widths)/sizeof(*widths);++w)
        for (unsigned p=0;p<sizeof(positions)/sizeof(*positions);++p)
        for (unsigned v=0;v<sizeof(values)/sizeof(*values);++v)
        for (unsigned clip=0;clip<2;++clip) {
            int x=positions[p][0],top=positions[p][1];
            int ct=clip ? 50 : 0,cb=clip ? 51 : 160;
            check_vitals(x,top,widths[w],values[v],maximum,0,0,0,0,ct,cb);
            ++cases;
            if (maximum<=0 || (w!=1 && w!=3)) continue;
            /* Shields and energy vary against a fixed partial HP. */
            check_vitals(x,top,widths[w],maximum/2,maximum,values[v],maximum,
                         0,0,ct,cb);
            check_vitals(x,top,widths[w],maximum/2,maximum,0,0,values[v],
                         maximum,ct,cb);
            check_vitals(x,top,widths[w],maximum,maximum,values[v],maximum,
                         maximum-values[v],maximum,ct,cb);
            cases+=3;
        }
    }
    printf("Selected-actor vital bars match the whole-frame oracle: %u cases\n",cases);
    return 0;
}
