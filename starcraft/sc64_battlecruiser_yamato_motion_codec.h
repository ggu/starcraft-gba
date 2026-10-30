/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BATTLECRUISER_YAMATO_MOTION_CODEC_H
#define SC64_BATTLECRUISER_YAMATO_MOTION_CODEC_H
#include <stdint.h>
#ifdef GBA_HOST
#include <string.h>
#else
#include <stddef.h>
void *memcpy(void *, const void *, size_t);
#endif
#include "sc64_battlecruiser_rules.h"
#include "sc64_bc_yamato_guard_private.h"
_Static_assert(sizeof(int)==4 && sizeof(unsigned)==4, "shot codec requires 32-bit words");
_Static_assert(SC64_BC_YAMATO_FLINGY==158u && SC64_BC_YAMATO_TOP_SPEED==8533u &&
               SC64_BC_YAMATO_ACCELERATION==SC64_BC_YAMATO_TOP_SPEED &&
               SC64_BC_YAMATO_SCRIPT_HAS_SETSPEED==0u,
               "Yamato speed codec requires validated source facts");
/* Private representation experiment, not yet a source-motion implementation.
 * Source map coordinates are at most 4095 pixels.  Keeping the fixed launch
 * endpoint separately is required for weapon 30's bullet_type_fly. */
typedef struct {
    uint32_t x_q8,y_q8;
    int16_t destination_x,destination_y;
    uint16_t speed,turn_rate;
    uint8_t heading,velocity_direction,lifetime,turning;
} Sc64BcMotionFields;
static inline int sc64_bc_motion_pack(Sc64BcYamatoShot *shot,const Sc64BcMotionFields *m)
{
    uint32_t x,y,packed;
    if (!shot || !m || shot->state!=SC64_BC_SHOT_FLIGHT ||
        m->x_q8>=1u<<20 || m->y_q8>=1u<<20 ||
        m->destination_x< -32 || m->destination_x>4159 ||
        m->destination_y< -32 || m->destination_y>4159 ||
        (m->speed!=0 && m->speed!=SC64_BC_YAMATO_TOP_SPEED) || m->turn_rate>511 || m->turning>1) return 0;
    unsigned ex=(unsigned)(m->destination_x+32), ey=(unsigned)(m->destination_y+32);
    x=m->x_q8|((ex&4095u)<<20); y=m->y_q8|((ey&4095u)<<20);
    /* Flingy 158 starts at 0; its acceleration equals its top speed, and
     * its authenticated script never sets speed. The only reachable speeds
     * at a completed source tick are therefore 0 and 8533. */
    packed=(m->speed?1u:0u)|((uint32_t)m->velocity_direction<<1)|
           ((uint32_t)m->turn_rate<<9)|((uint32_t)m->turning<<18)|
           ((ex>>12)<<19)|((ey>>12)<<20);
    memcpy(&shot->x_q8,&x,sizeof(x)); memcpy(&shot->y_q8,&y,sizeof(y));
    shot->age=(unsigned short)(m->lifetime|((unsigned)m->heading<<8));
    shot->motion=packed;
    return 1;
}
static inline int sc64_bc_motion_unpack(const Sc64BcYamatoShot *shot,Sc64BcMotionFields *m)
{
    uint32_t x,y,p;
    if (!shot || !m || shot->state!=SC64_BC_SHOT_FLIGHT) return 0;
    memcpy(&x,&shot->x_q8,sizeof(x)); memcpy(&y,&shot->y_q8,sizeof(y)); p=shot->motion;
    m->x_q8=x&0xfffffu; m->y_q8=y&0xfffffu;
    m->destination_x=(int16_t)((int)((x>>20)|(((p>>19)&1u)<<12))-32);
    m->destination_y=(int16_t)((int)((y>>20)|(((p>>20)&1u)<<12))-32);
    m->speed=(uint16_t)((p&1u)?SC64_BC_YAMATO_TOP_SPEED:0); m->velocity_direction=(uint8_t)(p>>1);
    m->turn_rate=(uint16_t)((p>>9)&511u); m->turning=(uint8_t)((p>>18)&1u);
    m->lifetime=(uint8_t)shot->age; m->heading=(uint8_t)(shot->age>>8);
    return (p&0xffe00000u)==0;
}
#endif
