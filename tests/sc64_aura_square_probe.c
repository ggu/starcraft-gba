/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../starcraft/sc64_aura_square_probe.h"

/* Original per-candidate scalar bounds, independent of the hoisted helper. */
static int scalar(int x,int y,int width,int height,int l,int t,int r,int b)
{
    int cx=x*2,cy=y*2,left=cx-160,top=cy-160,right=cx+160,bottom=cy+160;
    if(left<0)left=0; if(top<0)top=0;
    if(right>width)right=width; if(bottom>height)bottom=height;
    return l<right && r-1>=left && t<bottom && b-1>=top;
}
int main(void)
{
    const int points[]={-1,0,1,79,80,81,127,128,511,1023,2047,4095};
    uint32_t rng=0x91e10da5u;
    for(unsigned i=0;i<sizeof(points)/sizeof(points[0]);i++)
    for(unsigned j=0;j<sizeof(points)/sizeof(points[0]);j++) {
        int x=points[i],y=points[j],width=4096,height=3072;
        Sc64AuraSquare square=sc64_aura_square_make(x,y,width,height);
        int edges[]={square.left-1,square.left,square.left+1,square.right-1,square.right,square.right+1};
        int yedges[]={square.top-1,square.top,square.top+1,square.bottom-1,square.bottom,square.bottom+1};
        for(unsigned a=0;a<6;a++)for(unsigned b=0;b<6;b++)
        for(int w=0;w<=33;w+=11)for(int h=0;h<=33;h+=11)
            assert(scalar(x,y,width,height,edges[a],yedges[b],edges[a]+w,yedges[b]+h)==
                   sc64_aura_square_contains(&square,edges[a],yedges[b],edges[a]+w,yedges[b]+h));
        for(int n=0;n<128;n++) {
            rng=rng*1664525u+1013904223u;int l=(int)(rng%4200)-50;
            rng=rng*1664525u+1013904223u;int t=(int)(rng%3200)-50;
            rng=rng*1664525u+1013904223u;int rr=l+(int)(rng%90);
            rng=rng*1664525u+1013904223u;int bb=t+(int)(rng%90);
            assert(scalar(x,y,width,height,l,t,rr,bb)==sc64_aura_square_contains(&square,l,t,rr,bb));
        }
    }
    puts("aura square bounds parity passed");
}
