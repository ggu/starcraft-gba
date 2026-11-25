/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A completed fixed-size direction field and small exact byte differences
 * against it. The private copies never alias live route slots. */
#if !defined(SC64_ROUTE_KEEP_FIELDS) || !defined(SC64_ROUTE_LINEAR_FLOOD_PROBE) || !defined(SC64_ROUTE_PACK3)
#error "Wide point reuse requires grid rebuild notifications and packed linear floods"
#endif
enum { SC64_MEMO_SIDE = 128, SC64_MEMO_BYTES = 6144,
       SC64_MEMO_GOALS = 8, SC64_MEMO_PACKET_BYTES = 384,
       SC64_MEMO_ROTATE_AFTER = 16, SC64_MEMO_SMALL_FILL = 384 };
typedef struct {
    u16 goal, reached, count;
    u8 bytes[SC64_MEMO_PACKET_BYTES];
} Sc64WideDifference;
_Static_assert(sizeof(bfs_queue) >= SC64_MEMO_PACKET_BYTES, "wide route packet scratch");
static struct {
    _Alignas(4) u8 field[SC64_MEMO_BYTES];
    Sc64WideDifference diff[SC64_MEMO_GOALS];
    u16 goal, reached;
    u8 used, next, misses;
} sc64_wide_memo;

static void sc64_wide_memo_reset(void) { sc64_wide_memo.reached = 0; }

static int __attribute__((noinline))
sc64_route_fill_wide_cached(u8 *field, int goal)
{
    if (MAP_COLS != SC64_MEMO_SIDE || MAP_ROWS != SC64_MEMO_SIDE)
        return sc64_route_fill_linear(field, goal);
    if (sc64_wide_memo.reached) {
        if (sc64_wide_memo.goal == goal) {
            memcpy(field, sc64_wide_memo.field, SC64_MEMO_BYTES);
            return sc64_wide_memo.reached;
        }
        for (unsigned i = 0; i < sc64_wide_memo.used; ++i) {
            const Sc64WideDifference *entry = &sc64_wide_memo.diff[i];
            if (entry->goal != goal) continue;
            memcpy(field, sc64_wide_memo.field, SC64_MEMO_BYTES);
            for (unsigned e = 0; e < entry->count;) {
                unsigned at = entry->bytes[e] | (unsigned)entry->bytes[e + 1] << 8;
                unsigned size = entry->bytes[e + 2];
                e += 3;
                for (unsigned j = 0; j < size; ++j) field[at + j] = entry->bytes[e + j];
                e += size;
            }
            return entry->reached;
        }
    }
    int reached = sc64_route_fill_linear(field, goal);
    if (reached <= SC64_MEMO_SMALL_FILL) return reached;
    /* A large flood supplies the anchor. Rotate when repeatedly queried
     * destinations are too different for a bounded packet. */
    if (!sc64_wide_memo.reached || sc64_wide_memo.misses >= SC64_MEMO_ROTATE_AFTER) {
        memcpy(sc64_wide_memo.field, field, SC64_MEMO_BYTES);
        sc64_wide_memo.goal = goal;
        sc64_wide_memo.reached = reached;
        sc64_wide_memo.used = sc64_wide_memo.next = sc64_wide_memo.misses = 0;
        return reached;
    }
    /* Write the pending entry in already-consumed flood workspace, so a
     * rejected admission preserves the existing cache and uses no stack. */
    u8 *packet = (u8 *)(void *)bfs_queue;
    unsigned size = 0, end = 0, last_header = 0;
    for (unsigned i = 0; i < SC64_MEMO_BYTES; ++i) {
        if (field[i] == sc64_wide_memo.field[i]) continue;
        if (!size || i - end > 2 || packet[last_header] + i - end + 1 > 255) {
            if (size + 4 > SC64_MEMO_PACKET_BYTES) goto reject;
            packet[size++] = (u8)i;
            packet[size++] = (u8)(i >> 8);
            last_header = size++;
            packet[last_header] = 0;
            end = i;
        }
        if (size + i - end + 1 > SC64_MEMO_PACKET_BYTES) goto reject;
        packet[last_header] += i - end + 1;
        while (end <= i) packet[size++] = field[end++];
    }
    Sc64WideDifference *entry = &sc64_wide_memo.diff[sc64_wide_memo.next];
    entry->goal = goal;
    entry->reached = reached;
    entry->count = size;
    memcpy(entry->bytes, packet, size);
    if (++sc64_wide_memo.next == SC64_MEMO_GOALS) sc64_wide_memo.next = 0;
    if (sc64_wide_memo.used < SC64_MEMO_GOALS) ++sc64_wide_memo.used;
    sc64_wide_memo.misses = 0;
    return reached;
reject:
    ++sc64_wide_memo.misses;
    return reached;
}
