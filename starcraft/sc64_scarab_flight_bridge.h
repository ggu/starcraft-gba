/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_FLIGHT_BRIDGE_H
#define SC64_SCARAB_FLIGHT_BRIDGE_H
/* Unregistered prerequisite. Call only for a fully authenticated current
 * Scarab namespace; never apply to historical generations. Unit is declared
 * by the including engine. Logical build is -1 and order is a 0..4 FSM. */
#define SC64_SCARAB_FLIGHT_RADIX 131072u
#define SC64_SCARAB_FLIGHT_KEYS 126353888u
static int sc64_scarab_flight_key(unsigned carry, unsigned timer, unsigned order,
                                   int build, unsigned *key)
{
    unsigned parent=carry&255u, phase=(carry>>8)&3u, frac=carry>>10, ordinal;
    if (!key || carry >= (1u<<28) || order>4 || build!=-1 || timer>1023 ||
        (timer&127u)>90 || ((timer>>7)&7u)>(timer&127u)) return 0;
    if (phase<2) {
        if (parent>=240 || frac || timer || order) return 0;
        *key=parent*2u+phase; return 1;
    }
    if (phase==2 && !(timer&127u)) return 0;
    if (parent<240) ordinal=parent;
    else if (parent==255) ordinal=240;
    else return 0;
    *key=480u+((phase-2u)*241u+ordinal)*262144u+frac;
    return 1;
}
static int sc64_scarab_flight_unkey(unsigned key, unsigned *carry)
{
    unsigned parent,phase,frac;
    if (!carry || key>=SC64_SCARAB_FLIGHT_KEYS) return 0;
    if (key<480u) { parent=key/2u;phase=key%2u;frac=0; }
    else { unsigned v=key-480u, group=v/262144u, ordinal=group%241u;
        phase=2u+group/241u;parent=ordinal==240u?255u:ordinal;frac=v%262144u; }
    *carry=parent|(phase<<8)|(frac<<10);return 1;
}
/* Transform only a verified raw-85 row. Other units pass through unchanged.
 * On failure neither output nor input changes, including in-place use. */
static int sc64_scarab_flight_to_wire(const Unit *in, Unit *out, int current, int legacy)
{
    Unit q; unsigned key, upper, code, a, b;
    if (!in || !out) return 0;
    if (in->type!=U_SCARAB && in->raw_type!=85) { *out=*in;return 1; }
    if (!current || !in->alive || in->type!=U_SCARAB || in->raw_type!=85 ||
        !sc64_scarab_flight_key((unsigned)in->carry,(unsigned)in->work_timer,
                                (unsigned)in->order,in->build,&key)) return 0;
    if ((((unsigned)in->carry >> 8)&3u)<2u && in->target!=-1) return 0;
    q=*in;
    if (legacy) { q.carry=(int)(key&65535u)-32768; q.build=(int)(key>>16); }
    else {
        q.carry=(int)(key%SC64_SCARAB_FLIGHT_RADIX)-65536;
        upper=key/SC64_SCARAB_FLIGHT_RADIX;code=upper%242u;upper/=242u;
        a=upper&1u;b=upper>>1; if(b>1u)return 0;
        q.build=(int)code-2;q.order=(int)((unsigned)q.order|(a<<3));
        q.work_timer=(int)((unsigned)q.work_timer|(b<<10));
    }
    *out=q;return 1;
}
static int sc64_scarab_flight_from_wire(const Unit *in, Unit *out, int current, int legacy)
{
    Unit q;unsigned key, carry, check;
    if (!in || !out) return 0;
    if (in->type!=U_SCARAB && in->raw_type!=85) { *out=*in;return 1; }
    if (!current || !in->alive || in->type!=U_SCARAB || in->raw_type!=85) return 0;
    q=*in;
    if (legacy) {
        if(q.carry < -32768 || q.carry>32767 || q.build<0 || q.build>2047) return 0;
        key=(unsigned)(q.carry+32768)|((unsigned)q.build<<16);
    } else {
        if(q.carry < -65536 || q.carry>65535 || q.build < -2 || q.build>239 ||
           q.order<0 || q.order>15 || q.work_timer<0 || q.work_timer>2047) return 0;
        key=(unsigned)(q.carry+65536)+SC64_SCARAB_FLIGHT_RADIX*
            ((unsigned)(q.build+2)+242u*((((unsigned)q.order>>3)&1u)+
                                         2u*((unsigned)q.work_timer>>10)));
        q.order&=7;q.work_timer&=1023;
    }
    q.build=-1;
    if (!sc64_scarab_flight_unkey(key,&carry)) return 0;
    q.carry=(int)carry;
    if (((carry>>8)&3u)<2u && q.target!=-1) return 0;
    if (!sc64_scarab_flight_key(carry,(unsigned)q.work_timer,(unsigned)q.order,q.build,&check) || check!=key) return 0;
    *out=q;return 1;
}
#endif
