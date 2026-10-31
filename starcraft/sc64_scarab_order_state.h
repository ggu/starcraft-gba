/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_ORDER_STATE_H
#define SC64_SCARAB_ORDER_STATE_H
/* Pinned OpenBW order_ScarabAttack states; these are not Unit.order values.
 * Bridge 0 remains reserved for hidden actors. No existing staged tag accepts
 * outside rows, so these codes do not reinterpret an accepted save. */
static inline int sc64_scarab_native_to_bridge(unsigned native,unsigned *bridge)
{
    unsigned code;
    if (!bridge) return 0;
    switch(native) { case 0:code=1;break;case 2:code=2;break;
                     case 3:code=3;break;case 6:code=4;break;default:return 0; }
    *bridge=code;return 1;
}
static inline int sc64_scarab_bridge_to_native(unsigned bridge,unsigned *native)
{
    unsigned state;
    if (!native) return 0;
    switch(bridge) { case 1:state=0;break;case 2:state=2;break;
                     case 3:state=3;break;case 4:state=6;break;default:return 0; }
    *native=state;return 1;
}
#endif
