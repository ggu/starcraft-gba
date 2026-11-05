/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_WAYPOINT_BRIDGE_H
#define SC64_SCARAB_WAYPOINT_BRIDGE_H
/* Experimental autonomous-flight-only candidate conversion. The caller must
 * authenticate the flight generation and run this before flight_to_wire (and
 * after flight_from_wire). The route service recomputes path_x/path_y before
 * its next steer tick. The previous waypoint in path_timer/path_retry is the
 * persistent input to that tick. Source waypoints made by the current route
 * service are twice integer GBA coordinates. Rows are accepted only when both
 * current and previous waypoints can be restored exactly. This helper is not
 * a selected journal or a proof of all live-world readers. */
static int sc64_scarab_waypoint_to_wire(const Unit *in, Unit *out,
                                        unsigned width, unsigned height,
                                        int authenticated)
{
    Unit q;
    unsigned phase;
    if (!in || !out || !width || !height || width>2048 || height>2048) return 0;
    if (in->type!=U_SCARAB && in->raw_type!=85) { *out=*in; return 1; }
    if (!authenticated || in->type!=U_SCARAB || in->raw_type!=85) return 0;
    phase=((unsigned)in->carry>>8)&3u;
    if (phase<2) { *out=*in; return 1; }
    if ((unsigned)in->weapon>3u || (in->path_timer&1) || (in->path_retry&1) ||
        (unsigned)in->path_timer/2u>=width ||
        (unsigned)in->path_retry/2u>=height ||
        (!(in->weapon&2) && (in->path_timer || in->path_retry)) ||
        ((in->weapon&2) && (in->path_x!=in->path_timer || in->path_y!=in->path_retry)) ||
        (!(in->weapon&2) && (in->path_x<0 || in->path_y<0 ||
          in->path_x>2047 || in->path_y>2047 ||
          (unsigned)in->path_x>=width*2u || (unsigned)in->path_y>=height*2u))) return 0;
    q=*in;
    if (in->weapon&2) { q.path_x=in->path_timer/2; q.path_y=in->path_retry/2; }
    q.path_timer=0; q.path_retry=0;
    *out=q; return 1;
}
static int sc64_scarab_waypoint_from_wire(const Unit *in, Unit *out,
                                          unsigned width, unsigned height,
                                          int authenticated)
{
    Unit q;
    unsigned phase;
    if (!in || !out || !width || !height || width>2048 || height>2048) return 0;
    if (in->type!=U_SCARAB && in->raw_type!=85) { *out=*in; return 1; }
    if (!authenticated || in->type!=U_SCARAB || in->raw_type!=85) return 0;
    phase=((unsigned)in->carry>>8)&3u;
    if (phase<2) { *out=*in; return 1; }
    if ((unsigned)in->weapon>3u || in->path_timer || in->path_retry ||
        in->path_x<0 || in->path_y<0 ||
        (unsigned)in->path_x >= ((in->weapon&2) ? width : width*2u) ||
        (unsigned)in->path_y >= ((in->weapon&2) ? height : height*2u) ||
        in->path_x>2047 || in->path_y>2047) return 0;
    q=*in;
    if (in->weapon&2) {
        q.path_timer=in->path_x*2; q.path_retry=in->path_y*2;
        q.path_x=q.path_timer; q.path_y=q.path_retry;
    }
    *out=q; return 1;
}
#endif
