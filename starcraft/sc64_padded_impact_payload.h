/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_PADDED_IMPACT_PAYLOAD_H
#define SC64_PADDED_IMPACT_PAYLOAD_H

/* IPB and one count byte, followed by the part of the ordinary IMP1 body
 * that exceeds the building padding. The transport trailer follows the tail.
 * The low bit of each final building byte still belongs to that building. */
static const u8 sc64_padded_impact_marker[] = {'I', 'P', 'B'};

static unsigned sc64_padded_impact_tail_bytes(unsigned body_bytes)
{
    unsigned excess = body_bytes > SC64_BUILDING_PAD_IMPACT_BYTES ?
                      body_bytes - SC64_BUILDING_PAD_IMPACT_BYTES : 0;
    return SC64_BUILDING_PAD_IMPACT_META_BYTES + excess;
}

static void sc64_padded_impact_clear(RaceSave *save)
{
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i)
        save->buildings[i].bytes[SC64_PACKED_BUILDING_SAVE_BYTES - 1] &= 1u;
}

static unsigned sc64_padded_impact_encode(RaceSave *save, u8 *tail,
                                         const Sc64ImpactEvent *rows, unsigned count)
{
    if (count <= RACE_IMPACT_RAW_SAVE_CAPACITY || count > RACE_IMPACT_SAVE_CAPACITY)
        return 0;
    for (unsigned i = 0; i < count; ++i)
        if (!sc64_impact_event_valid(&rows[i], MAX_UNITS + MAX_BUILDINGS, 5u, SC_RU_COUNT))
            return 0;
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i)
        if (save->buildings[i].bytes[SC64_PACKED_BUILDING_SAVE_BYTES - 1] & 0xfeu)
            return 0;

    u8 packet[sizeof(Sc64ImpactSaveHeader) + SC64_BUILDING_PAD_IMPACT_BODY_MAX + 1] = {0};
    u8 *body = packet + sizeof(Sc64ImpactSaveHeader);
    unsigned body_bytes = sc64_impact_encode(rows, count, packet) - sizeof(Sc64ImpactSaveHeader);
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        unsigned bit = i * SC64_BUILDING_PAD_IMPACT_BITS, byte = bit >> 3;
        unsigned pair = body[byte] | ((unsigned)body[byte + 1] << 8);
        save->buildings[i].bytes[SC64_PACKED_BUILDING_SAVE_BYTES - 1] |=
            (u8)(((pair >> (bit & 7u)) & 0x7fu) << 1);
    }

    memcpy(tail, sc64_padded_impact_marker, sizeof(sc64_padded_impact_marker));
    tail[sizeof(sc64_padded_impact_marker)] = (u8)count;
    if (body_bytes > SC64_BUILDING_PAD_IMPACT_BYTES)
        memcpy(tail + SC64_BUILDING_PAD_IMPACT_META_BYTES, body + SC64_BUILDING_PAD_IMPACT_BYTES,
               body_bytes - SC64_BUILDING_PAD_IMPACT_BYTES);
    return sc64_padded_impact_tail_bytes(body_bytes);
}

static int sc64_padded_impact_decode(RaceSave *save, const u8 *tail, unsigned tail_bytes,
                                   Sc64ImpactEvent *candidate)
{
    if (tail_bytes < SC64_BUILDING_PAD_IMPACT_META_BYTES ||
        memcmp(tail, sc64_padded_impact_marker, sizeof(sc64_padded_impact_marker))) return -1;
    unsigned count = tail[sizeof(sc64_padded_impact_marker)];
    unsigned body_bytes = count * sizeof(Sc64ImpactEvent);
    if (count <= RACE_IMPACT_RAW_SAVE_CAPACITY || count > RACE_IMPACT_SAVE_CAPACITY ||
        tail_bytes != sc64_padded_impact_tail_bytes(body_bytes)) return -1;

    u8 packet[sizeof(Sc64ImpactSaveHeader) + SC64_BUILDING_PAD_IMPACT_BODY_MAX + 1] = {0};
    u8 *body = packet + sizeof(Sc64ImpactSaveHeader);
    sc64_impact_encode(NULL, 0, packet);
    packet[offsetof(Sc64ImpactSaveHeader, count)] = (u8)count;
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        unsigned bit = i * SC64_BUILDING_PAD_IMPACT_BITS, byte = bit >> 3;
        unsigned last = save->buildings[i].bytes[SC64_PACKED_BUILDING_SAVE_BYTES - 1];
        unsigned pair = (last >> 1) << (bit & 7u);
        body[byte] |= (u8)pair;
        body[byte + 1] |= (u8)(pair >> 8);
    }
    for (unsigned i = body_bytes; i < SC64_BUILDING_PAD_IMPACT_BYTES; ++i)
        if (body[i]) return -1;
    if (body_bytes > SC64_BUILDING_PAD_IMPACT_BYTES)
        memcpy(body + SC64_BUILDING_PAD_IMPACT_BYTES, tail + SC64_BUILDING_PAD_IMPACT_META_BYTES,
               body_bytes - SC64_BUILDING_PAD_IMPACT_BYTES);

    int decoded = sc64_impact_decode(packet, sc64_impact_save_bytes(count),
                                     RACE_IMPACT_SAVE_CAPACITY, MAX_UNITS + MAX_BUILDINGS,
                                     5u, SC_RU_COUNT, candidate);
    if (decoded >= 0) sc64_padded_impact_clear(save);
    return decoded;
}

#endif
