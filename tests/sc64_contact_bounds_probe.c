/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "../starcraft/sc64_contact_bounds_probe.h"
static const int dx[8]={1,1,0,-1,-1,-1,0,1};
static const int dy[8]={0,1,1,1,0,-1,-1,-1};
static unsigned scalar(int x,int y,int speed,int width,int height)
{
    unsigned result=0;
    for(int k=0;k<8;k++) {
        int sx=x+dx[k]*speed,sy=y+dy[k]*speed;
        if(sx<0||sy<0||sx>=width||sy>=height) result|=1u<<k;
    }
    return result;
}
int main(void)
{
    for(int width=1;width<=33;width++)for(int height=1;height<=33;height++)
    for(int y=0;y<height;y++)for(int x=0;x<width;x++)for(int speed=0;speed<=16;speed++) {
        unsigned mask=0xffffffffu;
        assert(sc64_contact_bounds_fast(x,y,speed,width,height,&mask));
        assert(mask==scalar(x,y,speed,width,height));
    }
    const int points[]={0,1,15,16,17,4095,8175,8190,8191};
    for(unsigned i=0;i<sizeof(points)/sizeof(points[0]);i++)
    for(unsigned j=0;j<sizeof(points)/sizeof(points[0]);j++)for(int speed=0;speed<=16;speed++) {
        unsigned mask=0;assert(sc64_contact_bounds_fast(points[i],points[j],speed,8192,8192,&mask));
        assert(mask==scalar(points[i],points[j],speed,8192,8192));
    }
    unsigned mask=0x1234u;
    assert(!sc64_contact_bounds_fast(0,0,0,1,1,NULL));
    assert(!sc64_contact_bounds_fast(-1,0,0,1,1,&mask)&&mask==0x1234u);
    assert(!sc64_contact_bounds_fast(0,-1,0,1,1,&mask));
    assert(!sc64_contact_bounds_fast(1,0,0,1,1,&mask));
    assert(!sc64_contact_bounds_fast(0,1,0,1,1,&mask));
    assert(!sc64_contact_bounds_fast(0,0,-1,1,1,&mask));
    assert(!sc64_contact_bounds_fast(0,0,17,1,1,&mask));
    assert(!sc64_contact_bounds_fast(0,0,0,0,1,&mask));
    assert(!sc64_contact_bounds_fast(0,0,0,1,0,&mask));
    assert(!sc64_contact_bounds_fast(0,0,0,INT_MAX,1,&mask));
    assert(!sc64_contact_bounds_fast(0,0,0,1,INT_MAX,&mask));
    puts("contact direction bounds and fallback oracle passed");
}
