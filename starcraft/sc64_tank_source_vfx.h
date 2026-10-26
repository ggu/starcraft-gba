/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Source Tank VFX adapter. */
#if !defined(SC64_TANK_SOURCE_VFX) || !defined(SC64_RACES) || !defined(SC64_HAS_SPRITES)
#error "source Tank VFX requires its opt-in imported race profile"
#endif
#ifndef SC64_TANK_SOURCE_VFX_H
#define SC64_TANK_SOURCE_VFX_H
static int
sc64_private_tank_direction(int dx,int dy)
{
    static const short v[32][2]={{0,-256},{50,-251},{98,-237},{142,-213},{181,-181},{213,-142},{237,-98},{251,-50},{256,0},{251,50},{237,98},{213,142},{181,181},{142,213},{98,237},{50,251},{0,256},{-50,251},{-98,237},{-142,213},{-181,181},{-213,142},{-237,98},{-251,50},{-256,0},{-251,-50},{-237,-98},{-213,-142},{-181,-181},{-142,-213},{-98,-237},{-50,-251}};
    int best=0; long bestdot=-0x7fffffffL;
    for(int d=0;d<32;++d) { long dot=(long)dx*v[d][0]+(long)dy*v[d][1]; if(dot>bestdot){bestdot=dot;best=d;} }
    return best;
}
#endif
