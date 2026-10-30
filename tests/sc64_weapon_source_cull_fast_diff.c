/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Differential against the pre-fast-path 64-bit cull and real source bank. */
#define main sc64_cull_bank_original_main
#include "sc64_weapon_source_vfx_cull_bank.c"
#undef main
static int wide_reference(Sc64WeaponSourceVfxKey key,const Sc64WeaponSourceAttackProfile *p,int x0,int y0,int x1,int y1,int cx,int cy)
{
    if (!p || !p->projectile_image || !sc64_weapon_source_vfx_motion_has_proven_bounds(p->weapon_bullet_type)) return 0;
    int64_t dx=(int64_t)x1-x0,dy=(int64_t)y1-y0;
    uint64_t ax=dx<0?-dx:dx,ay=dy<0?-dy:dy;
    if(ax>UINT16_MAX||ay>UINT16_MAX)return 0;
    uint16_t rx,ry; if(!sc64_weapon_source_vfx_bounds_unpack(sc64_weapon_source_vfx_cached_bounds(),&rx,&ry))return 0;
    int lx,ly;if(!sc64_weapon_source_vfx_launch_origin_checked(p,key.direction,x0,y0,&lx,&ly))return 0;
    int64_t lo=x0,hi=x0,top=y0,bottom=y0;
    if(lx<lo)lo=lx;if(lx>hi)hi=lx;if(x1<lo)lo=x1;if(x1>hi)hi=x1;
    if(ly<top)top=ly;if(ly>bottom)bottom=ly;if(y1<top)top=y1;if(y1>bottom)bottom=y1;
    return sc64_weapon_source_vfx_bounds_miss_clip(lo,hi,top,bottom,cx,cy,rx,ry);
}
static uint32_t rng=0x90abcdefu;
static uint32_t next(void){rng=rng*1664525u+1013904223u;return rng;}
int main(void)
{
    static const int edges[]={INT_MIN,INT_MIN+1,-65536,-32769,-32768,-32767,-1,0,1,107,108,239,240,32766,32767,32768,65535,65536,INT_MAX-1,INT_MAX};
    unsigned long cases=0;
    for(unsigned pi=0;pi<SC64_WEAPON_SOURCE_PROFILE_COUNT;pi++) {
        const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[pi];
        Sc64WeaponSourceAttackProfile copied=*p;
        for(unsigned d=0;d<34;d++) {
            Sc64WeaponSourceVfxKey key={p->actor_raw,1,p->weapon_id,(uint8_t)d,p->mode,p->variant};
            uint32_t packed;
            if (sc64_weapon_source_vfx_pack(key,&packed)) {
                Sc64WeaponSourceVfxKey decoded;
                assert(sc64_weapon_source_vfx_unpack(packed,&decoded));
                assert(sc64_weapon_source_trace_profile(packed)==sc64_weapon_source_profile_lookup(
                    decoded.actor_raw,decoded.weapon_id,decoded.target_air,decoded.variant));
            }
            for(unsigned n=0;n<1200;n++) {
                int a[6];for(int i=0;i<6;i++)a[i]=(n<200)?edges[next()%(sizeof(edges)/sizeof(edges[0]))]:(int)(next()%65536u)-32768;
                int old=wide_reference(key,&copied,a[0],a[1],a[2],a[3],a[4],a[5]);
                int actual=sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,&copied,a[0],a[1],a[2],a[3],a[4],a[5]);
                assert(old==actual);
                int canonical=sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,p,a[0],a[1],a[2],a[3],a[4],a[5]);
                int wide = d > 31;
                for (int i=0;i<6;i++) if (a[i] < -32768 || a[i] > 32767) wide=1;
                if (wide) assert(old==canonical);
                else assert(!old || canonical);
                ++cases;
            }
        }
    }
    /* Exercise extrema independently of the prepared bank's actual offsets. */
    Sc64WeaponSourceAttackProfile synthetic=sc64_weapon_source_profiles[0];
    synthetic.projectile_image=1; synthetic.weapon_bullet_type=SC64_SOURCE_BULLET_FLY;
    static const int offsets[]={-128,-127,-1,0,1,126,127};
    for(unsigned d=0;d<34;d++) for(unsigned f=0;f<7;f++) for(unsigned u=0;u<7;u++) {
        Sc64WeaponSourceVfxKey key={0,1,0,(uint8_t)d,0,0};
        synthetic.forward_offset=offsets[f];synthetic.upward_offset=offsets[u];
        for(unsigned n=0;n<500;n++) {
            int x=(int)(next()%1600u)-800,y=(int)(next()%1600u)-800;
            int cx=(int)(next()%600u)-300,cy=(int)(next()%600u)-300;
            int x1=(n&1)?x+240:x-240,y1=(n&2)?y+108:y-108;
            assert(wide_reference(key,&synthetic,x,y,x1,y1,cx,cy)==
              sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,&synthetic,x,y,x1,y1,cx,cy));++cases;
        }
    }
    uint16_t rx,ry; assert(sc64_weapon_source_vfx_bounds_unpack(sc64_weapon_source_vfx_cached_bounds(),&rx,&ry));
    synthetic.forward_offset=synthetic.upward_offset=0;
    for(int d=0;d<32;d++) for(int camera=-32768;camera<=32767;camera+=65535) {
        Sc64WeaponSourceVfxKey key={0,1,0,(uint8_t)d,0,0};
        int xs[]={camera-(int)rx-1,camera-(int)rx,camera+239+(int)rx,camera+240+(int)rx};
        int ys[]={camera-(int)ry-1,camera-(int)ry,camera+107+(int)ry,camera+108+(int)ry};
        for(int i=0;i<4;i++) for(int j=0;j<4;j++) {
            assert(wide_reference(key,&synthetic,xs[i],ys[j],xs[i],ys[j],camera,camera)==
                   sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,&synthetic,xs[i],ys[j],xs[i],ys[j],camera,camera));++cases;
        }
    }
    Sc64WeaponSourceVfxKey invalid={0,1,0,0,0,0};
    assert(!sc64_weapon_source_vfx_trace_provably_offscreen_resolved(invalid,0,0,0,0,0,0,0));
    synthetic.weapon_bullet_type=SC64_SOURCE_BULLET_FOLLOW;
    assert(!sc64_weapon_source_vfx_trace_provably_offscreen_resolved(invalid,&synthetic,0,0,0,0,0,0));
    printf("wide-oracle differential %lu cases\n",cases);
    return 0;
}
