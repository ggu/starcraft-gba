/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int p7_rect_reference(unsigned,int,int,int,int*,int*,int*,int*);
int p7_rect_candidate(unsigned,int,int,int,int*,int*,int*,int*);
static unsigned long checks;
static void fail(const char *what,unsigned raw,int x,int y,int building)
{
    fprintf(stderr,"%s raw=%u x=%d y=%d building=%d\n",what,raw,x,y,building);
    exit(1);
}
static void one(unsigned raw,int x,int y,int building)
{
    int r[4]={701,702,703,704},c[4]={701,702,703,704};
    int rr=p7_rect_reference(raw,x,y,building,&r[0],&r[1],&r[2],&r[3]);
    int cr=p7_rect_candidate(raw,x,y,building,&c[0],&c[1],&c[2],&c[3]);
    ++checks;
    if(rr!=cr)fail("return differs",raw,x,y,building);
    for(int i=0;i<4;++i)if(r[i]!=c[i])fail("out value differs",raw,x,y,building);
}
static void null_and_alias(unsigned raw,int x,int y,int building)
{
    for(int missing=0;missing<4;++missing) {
        int r[4]={711,712,713,714},c[4]={711,712,713,714};
        int *rp[4]={&r[0],&r[1],&r[2],&r[3]};
        int *cp[4]={&c[0],&c[1],&c[2],&c[3]};
        rp[missing]=0;cp[missing]=0;
        int rr=p7_rect_reference(raw,x,y,building,rp[0],rp[1],rp[2],rp[3]);
        int cr=p7_rect_candidate(raw,x,y,building,cp[0],cp[1],cp[2],cp[3]);
        ++checks;
        if(rr!=cr)fail("null return differs",raw,x,y,building);
        for(int i=0;i<4;++i)if(r[i]!=c[i])fail("null output write differs",raw,x,y,building);
    }
    {
        int r=721,c=721;
        int rr=p7_rect_reference(raw,x,y,building,&r,&r,&r,&r);
        int cr=p7_rect_candidate(raw,x,y,building,&c,&c,&c,&c);
        ++checks;
        if(rr!=cr||r!=c)fail("all-aliased output order differs",raw,x,y,building);
    }
    {
        int r[2]={731,732},c[2]={731,732};
        int rr=p7_rect_reference(raw,x,y,building,&r[0],&r[0],&r[1],&r[1]);
        int cr=p7_rect_candidate(raw,x,y,building,&c[0],&c[0],&c[1],&c[1]);
        ++checks;
        if(rr!=cr||r[0]!=c[0]||r[1]!=c[1])fail("paired alias output order differs",raw,x,y,building);
    }
}
int main(void)
{
    const int limit=(INT_MAX-256)/2;
    const int coords[]={INT_MIN,-1,0,1,2,2047,2048,4095,limit-1,limit,limit+1,INT_MAX};
    const int building_modes[]={0,1,-1,2};
    for(unsigned raw=0;raw<=227u;++raw)
        for(unsigned bi=0;bi<sizeof(building_modes)/sizeof(building_modes[0]);++bi)
            for(unsigned xi=0;xi<sizeof(coords)/sizeof(coords[0]);++xi)
                for(unsigned yi=0;yi<sizeof(coords)/sizeof(coords[0]);++yi)
                    one(raw,coords[xi],coords[yi],building_modes[bi]);
    const unsigned invalid[]={228u,255u,0x80000000u,UINT_MAX,UINT_MAX-1u};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i)
        for(unsigned bi=0;bi<sizeof(building_modes)/sizeof(building_modes[0]);++bi)
            for(unsigned xi=0;xi<sizeof(coords)/sizeof(coords[0]);++xi)
                one(invalid[i],coords[xi],coords[sizeof(coords)/sizeof(coords[0])-1u-xi],building_modes[bi]);
    for(unsigned raw=0;raw<=227u;++raw)
        for(unsigned bi=0;bi<sizeof(building_modes)/sizeof(building_modes[0]);++bi)
            null_and_alias(raw,coords[0],coords[1],building_modes[bi]);
    printf("P7 raw rectangle table exact: all raws, domains, coordinate guards, null/aliased outputs; checks=%lu\n",checks);
    return 0;
}
