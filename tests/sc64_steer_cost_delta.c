/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../starcraft/sc64_steer_cost_delta.h"

static const int dx[8]={1,1,0,-1,-1,-1,0,1};
static const int dy[8]={0,1,1,1,0,-1,-1,-1};
static int scalar_cost(int x,int y,int tx,int ty,int speed,int i)
{
    int a=x+dx[i]*speed-tx, b=y+dy[i]*speed-ty;
    return a*a+b*b;
}
static int winner(const int *cost,unsigned valid)
{
    int best=-1,least=0x7fffffff;
    for(int i=0;i<8;i++)if((valid&(1u<<i)) && cost[i]<least) {best=i;least=cost[i];}
    return best;
}
int main(void)
{
    uint32_t state=0x713298abu;
    const int edges[]={0,1,2,15,16,17,4095,8190,8191};
    for(int n=0;n<12000;n++) {
        int v[5];
        for(int i=0;i<4;i++) {
            state=state*1664525u+1013904223u;
            v[i]=n<729 ? edges[(n/(i%2?9:1)+i)%9] : (int)(state%8192u);
        }
        v[4]=n%17; Sc64SteerCostDelta delta;
        assert(sc64_steer_cost_delta_prepare(&delta,v[0],v[1],v[2],v[3],v[4]));
        int old[8],score[8];
        for(int i=0;i<8;i++) {old[i]=scalar_cost(v[0],v[1],v[2],v[3],v[4],i);score[i]=sc64_steer_cost_delta(&delta,dx[i],dy[i]);}
        for(unsigned mask=0;mask<256;mask++)assert(winner(old,mask)==winner(score,mask));
    }
    Sc64SteerCostDelta d;
    assert(!sc64_steer_cost_delta_prepare(NULL,0,0,0,0,0));
    assert(!sc64_steer_cost_delta_prepare(&d,-1,0,0,0,0));
    assert(!sc64_steer_cost_delta_prepare(&d,0,8192,0,0,0));
    assert(!sc64_steer_cost_delta_prepare(&d,0,0,-1,0,0));
    assert(!sc64_steer_cost_delta_prepare(&d,0,0,0,8192,0));
    assert(!sc64_steer_cost_delta_prepare(&d,0,0,0,0,-1));
    assert(!sc64_steer_cost_delta_prepare(&d,0,0,0,0,17));
    puts("steering cost ranking and fallback oracle passed");
}
