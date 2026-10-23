#ifndef SC64_TERRAN_FLIGHT_H
#define SC64_TERRAN_FLIGHT_H
#ifdef SC64_TERRAN_LATE_RUNTIME
/* Source UNIT state bit 4 is in-transit. Goals use 11-bit half-scale world
 * coordinates in the mission save; no unit slots or heap are needed. */
#define SC64_FLIGHT_ACTIVE (1u << 22)
#define SC64_FLIGHT_LAND (1u << 23)
static int race_late_can_lift(int type)
{
    return type == B_COMMAND || type == B_BARRACKS || type == B_FACTORY ||
           type == B_STARPORT || type == B_ENG_BAY || type == B_SCIENCE_FACILITY;
}
static int race_late_airborne(int slot)
{
    return race_late_active() && slot >= 0 && slot < MAX_BUILDINGS &&
           b[slot].alive && (b[slot].flags & 4) && race_late_can_lift(b[slot].type);
}
static int race_late_landing_site(int slot, int x, int y)
{
    if (!race_late_airborne(slot) || x < 24 || y < 24 ||
        x >= WORLD_W - 24 || y >= WORLD_H - 24) return 0;
    /* Use the normal construction terrain/spacing boundary. The airborne
     * caller is excluded from ground collision by the ordinary grid helper. */
    if (!can_place(b[slot].type, x, y)) return 0;
    for (int yy = y - 8; yy <= y + 8; yy += 16)
        for (int xx = x - 8; xx <= x + 8; xx += 16)
            if (terrain_blocked(xx / 16, yy / 16)) return 0;
    /* Reserve other accepted landing destinations while they are in flight,
     * so two buildings cannot receive an overlapping landing order. */
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        u32 goal = race_late.flight_goals[i];
        if (i != slot && b[i].alive && (goal & SC64_FLIGHT_LAND) &&
            dist2(x, y, goal & 2047u, (goal >> 11) & 2047u) < 40 * 40)
            return 0;
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) &&
            !sc64_race_unit_flying(u[i].type) &&
            dist2(x, y, u[i].x, u[i].y) < 20 * 20) return 0;
    return 1;
}
static int race_late_flight_order(int slot, int x, int y, int land)
{
    if (!race_late_airborne(slot) || b[slot].owner != SIDE ||
        x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H ||
        (land && !race_late_landing_site(slot, x, y))) return 0;
    race_late.flight_goals[slot] = (u32)x | ((u32)y << 11) |
                                  SC64_FLIGHT_ACTIVE | (land ? SC64_FLIGHT_LAND : 0);
    return 1;
}
static int race_late_lift(int slot)
{
    if (!race_late_active() || slot < 0 || slot >= MAX_BUILDINGS ||
        !b[slot].alive || b[slot].owner != SIDE || b[slot].under_construction ||
        !race_late_can_lift(b[slot].type) || (b[slot].flags & 4) ||
        b[slot].queue_count) return 0;
    b[slot].flags |= 4;
    race_late.flight_goals[slot] = 0;
    /* Lift-off detaches add-ons; the tower remains on the ground. */
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_CONTROL_TOWER && b[i].level == slot + 1)
            b[i].level = 0;
    invalidate_path_grid(); target_lists_valid = 0;
    return 1;
}
static void race_late_flight_tick(void)
{
    if (!race_late_active()) return;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        u32 goal = race_late.flight_goals[i];
        if (!race_late_airborne(i)) { race_late.flight_goals[i] = 0; continue; }
        if (!(goal & SC64_FLIGHT_ACTIVE)) continue;
        int x = goal & 2047u, y = (goal >> 11) & 2047u;
        /* A lifted Terran building's 1 source-pixel/native-frame movement
         * becomes 2 GBA pixels per five 30 Hz ticks at half map scale. */
        if (frame % 5 < 2) {
            b[i].x += x > b[i].x ? 1 : x < b[i].x ? -1 : 0;
            b[i].y += y > b[i].y ? 1 : y < b[i].y ? -1 : 0;
        }
        if (b[i].x != x || b[i].y != y) continue;
        if (goal & SC64_FLIGHT_LAND) {
            if (!race_late_landing_site(i, x, y)) continue;
            b[i].flags &= ~4;
            invalidate_path_grid(); target_lists_valid = 0;
            for (int j = 0; j < MAX_BUILDINGS; ++j)
                if (b[j].alive && b[j].type == B_CONTROL_TOWER &&
                    b[j].owner == b[i].owner && b[j].x == x + 48 &&
                    b[j].y == y + 8) { b[j].level = i + 1; break; }
        }
        race_late.flight_goals[i] = 0;
    }
}
#endif
#endif
