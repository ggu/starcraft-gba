/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ZP_UNIT_STREAM_H
#define SC64_ZP_UNIT_STREAM_H

/* Delta-coded unit rows for full-storage profiles whose unit table is too
 * large for fixed 35-byte rows in one SRAM record (more than 240 slots).
 *
 * The save keeps up to SC64_ZPS_TEMPLATES template units. Every non-zero
 * slot stores one row: the base (a template, or the all-zero Unit), the
 * position, a mask of the other 25 fields that differ from the base, and
 * those fields at the widths of the fixed-row codec (sc64_zp_unit_save.h);
 * only the combined target and build fields are one bit wider, so actor
 * references up to 1023 fit. A crowd of one unit type shares a template, so
 * a Zergling that only moved stores its position and path fields: about
 * 8 to 14 bytes instead of 35. An all-zero slot stores nothing.
 *
 * offset[slot] gives each row's data byte offset, so any slot decodes on
 * its own; rows start on byte boundaries. The format is canonical: a field
 * is present exactly when it differs from the base, unused bits and bytes
 * are zero, and templates themselves use the zero base. Decoding checks all
 * of it, then applies the same field ranges as the fixed-row codec.
 *
 * A world whose rows do not fit the record is not saved (SAVE FAILED); the
 * encoder never truncates. */

#ifndef SC64_RACES
#error "the unit stream requires the RACES Unit layout"
#endif

#define SC64_ZPS_TEMPLATES 15u
#define SC64_ZPS_ZERO_BASE 15u
#define SC64_ZPS_FIELDS 25u
#define SC64_ZPS_NO_ROW 0xffffu
#define SC64_ZPS_REF_BIAS 1024

#ifndef SC64_ZPS_DATA_BYTES
#error "define SC64_ZPS_DATA_BYTES before including sc64_zp_unit_stream.h"
#endif
_Static_assert(SC64_ZPS_DATA_BYTES < SC64_ZPS_NO_ROW, "row offsets fit u16");

typedef struct {
    u16 used;                              /* data bytes in use */
    u16 template_offset[SC64_ZPS_TEMPLATES];
    u8 template_count, reserved;
    u16 offset[MAX_UNITS];                 /* SC64_ZPS_NO_ROW: all-zero Unit */
    u8 data[SC64_ZPS_DATA_BYTES];
} Sc64ZpUnitStream;

/* Field order of the mask; x and y are always stored. */
static const u8 sc64_zps_width[SC64_ZPS_FIELDS] = {
    18, 17, 3, 6, 1, 1, 16, 4, 11, 11, 17, 11, 16, 8, 16, 11, 5, 4, 11, 11,
    17, 9, 9, 16, 11
};
static const s32 sc64_zps_bias[SC64_ZPS_FIELDS] = {
    131072, 0, 0, 0, 0, 0, 0, 0, 0, 0, 65536, SC64_ZPS_REF_BIAS, 0, 0, 0,
    SC64_ZPS_REF_BIAS, 0, 0, 0, 0, 0, 1, 1, 0, 0
};

static int sc64_zps_field(const Unit *q, unsigned f)
{
    switch (f) {
    case 0: return q->hp;          case 1: return q->maxhp;
    case 2: return q->owner;       case 3: return q->type;
    case 4: return q->alive;       case 5: return q->selected;
    case 6: return q->cd;          case 7: return q->order;
    case 8: return q->tx;          case 9: return q->ty;
    case 10: return q->carry;      case 11: return q->build;
    case 12: return q->work_timer; case 13: return q->armor;
    case 14: return q->weapon;     case 15: return q->target;
    case 16: return q->path_timer; case 17: return q->path_retry;
    case 18: return q->path_x;     case 19: return q->path_y;
    case 20: return q->shield;     case 21: return q->raw_type;
    case 22: return q->raw_owner;  case 23: return q->flags;
    default: return q->shield_remainder;
    }
}

static void sc64_zps_set_field(Unit *q, unsigned f, int v)
{
    switch (f) {
    case 0: q->hp = v; break;               case 1: q->maxhp = v; break;
    case 2: q->owner = (u8)v; break;        case 3: q->type = (u8)v; break;
    case 4: q->alive = (u8)v; break;        case 5: q->selected = (u8)v; break;
    case 6: q->cd = (u16)v; break;          case 7: q->order = (u8)v; break;
    case 8: q->tx = (s16)v; break;          case 9: q->ty = (s16)v; break;
    case 10: q->carry = v; break;           case 11: q->build = (s16)v; break;
    case 12: q->work_timer = (u16)v; break; case 13: q->armor = (u8)v; break;
    case 14: q->weapon = (u16)v; break;     case 15: q->target = (s16)v; break;
    case 16: q->path_timer = (u16)v; break; case 17: q->path_retry = (u16)v; break;
    case 18: q->path_x = (s16)v; break;     case 19: q->path_y = (s16)v; break;
    case 20: q->shield = v; break;          case 21: q->raw_type = (s16)v; break;
    case 22: q->raw_owner = (s16)v; break;  case 23: q->flags = (u16)v; break;
    default: q->shield_remainder = (u16)v; break;
    }
}

/* The fixed-row codec's admission ranges with ten-bit actor references. */
static int sc64_zps_unit_valid(const Unit *q, unsigned world_width, unsigned world_height)
{
    if (!q || !world_width || !world_height || world_width > 2048 ||
        world_height > 2048 || q->x < 0 || (unsigned)q->x >= world_width ||
        q->y < 0 || (unsigned)q->y >= world_height ||
        q->hp < -131072 || q->hp >= 131072 || (q->alive && q->hp < 0) ||
        q->maxhp < 0 || q->maxhp > 131071 || q->owner > 4 ||
        q->type >= 64 || SC_RU_COUNT > 64 || q->type >= SC_RU_COUNT ||
        q->alive > 1 || q->selected > 1 || q->order > 15 ||
        q->tx < 0 || q->tx > 2047 || q->ty < 0 || q->ty > 2047 ||
        q->carry < -65536 || q->carry >= 65536 ||
        q->build < -SC64_ZPS_REF_BIAS || q->build >= SC64_ZPS_REF_BIAS ||
        q->target < -1 || q->target >= SC64_ZPS_REF_BIAS ||
        q->path_timer > 24 || q->path_retry > 12 ||
        q->path_x < 0 || q->path_x > 2047 || q->path_y < 0 || q->path_y > 2047 ||
        q->shield < 0 || q->shield > 131071 || q->raw_type < -1 ||
        q->raw_type > 255 || q->raw_owner < -1 || q->raw_owner > 255 ||
        q->shield_remainder > 1279) return 0;
    return 1;
}

typedef struct { u8 *data; unsigned cap, bit; } Sc64ZpsWriter;
typedef struct { const u8 *data; unsigned end, bit; } Sc64ZpsReader;

static int sc64_zps_put(Sc64ZpsWriter *w, unsigned width, u32 value)
{
    if (width < 32 && value >> width) return 0;
    for (unsigned i = 0; i < width; ++i, ++w->bit) {
        if ((w->bit >> 3) >= w->cap) return 0;
        if ((value >> i) & 1u) w->data[w->bit >> 3] |= (u8)(1u << (w->bit & 7u));
    }
    return 1;
}

static int sc64_zps_get(Sc64ZpsReader *r, unsigned width, u32 *value)
{
    u32 v = 0;
    for (unsigned i = 0; i < width; ++i, ++r->bit) {
        if ((r->bit >> 3) >= r->end) return 0;
        v |= (u32)((r->data[r->bit >> 3] >> (r->bit & 7u)) & 1u) << i;
    }
    *value = v;
    return 1;
}

/* Append one row of q against base; returns its byte offset or -1. */
static int sc64_zps_put_row(Sc64ZpsWriter *w, unsigned base_id, const Unit *base, const Unit *q)
{
    unsigned start = (w->bit + 7u) & ~7u;
    u32 mask = 0;
    w->bit = start;
    for (unsigned f = 0; f < SC64_ZPS_FIELDS; ++f)
        if (sc64_zps_field(q, f) != sc64_zps_field(base, f)) mask |= 1u << f;
    if (!sc64_zps_put(w, 4, base_id) || !sc64_zps_put(w, 11, (u32)q->x) ||
        !sc64_zps_put(w, 11, (u32)q->y) || !sc64_zps_put(w, SC64_ZPS_FIELDS, mask))
        return -1;
    for (unsigned f = 0; f < SC64_ZPS_FIELDS; ++f)
        if ((mask >> f & 1u) &&
            !sc64_zps_put(w, sc64_zps_width[f], (u32)(sc64_zps_field(q, f) + sc64_zps_bias[f])))
            return -1;
    return (int)(start >> 3);
}

static int sc64_zps_get_row(const Sc64ZpUnitStream *s, unsigned offset, const Unit *base,
                            unsigned *base_id_out, Unit *out, unsigned *end_bit)
{
    static const Unit zero_unit = {0};
    Sc64ZpsReader r = { s->data, s->used, offset * 8u };
    u32 base_id, x, y, mask, v;
    if (offset >= s->used || s->used > SC64_ZPS_DATA_BYTES ||
        !sc64_zps_get(&r, 4, &base_id) ||
        !sc64_zps_get(&r, 11, &x) || !sc64_zps_get(&r, 11, &y) ||
        !sc64_zps_get(&r, SC64_ZPS_FIELDS, &mask)) return 0;
    if (base_id == SC64_ZPS_ZERO_BASE) base = &zero_unit;
    else if (!base || base_id >= s->template_count) return 0;
    *out = *base;
    out->x = (s16)x; out->y = (s16)y;
    for (unsigned f = 0; f < SC64_ZPS_FIELDS; ++f) {
        if (!(mask >> f & 1u)) continue;
        if (!sc64_zps_get(&r, sc64_zps_width[f], &v)) return 0;
        int value = (int)v - (int)sc64_zps_bias[f];
        if (value == sc64_zps_field(base, f)) return 0;   /* non-canonical */
        sc64_zps_set_field(out, f, value);
    }
    /* Trailing pad bits of the row are zero. */
    for (unsigned b = r.bit; b & 7u; ++b)
        if ((s->data[b >> 3] >> (b & 7u)) & 1u) return 0;
    *base_id_out = base_id;
    *end_bit = r.bit;
    return 1;
}

/* Row base of the row at offset: SC64_ZPS_ZERO_BASE or a template index. */
static unsigned sc64_zps_row_base(const Sc64ZpUnitStream *s, unsigned offset)
{
    return offset < s->used && s->used <= SC64_ZPS_DATA_BYTES ? s->data[offset] & 15u
                                                             : SC64_ZPS_TEMPLATES + 1u;
}

static int sc64_zps_template(const Sc64ZpUnitStream *s, unsigned t, Unit *out)
{
    unsigned base_id, end;
    if (s->template_count > SC64_ZPS_TEMPLATES || s->reserved || t >= s->template_count)
        return 0;
    /* Rows always store their own position, so templates hold x = y = 0. */
    return sc64_zps_get_row(s, s->template_offset[t], 0, &base_id, out, &end) &&
           base_id == SC64_ZPS_ZERO_BASE && !out->x && !out->y;
}

/* Decode one slot of a candidate stream without touching the live world.
 * Whole-stream structure is checked by sc64_zp_stream_canonical(). */
static int sc64_zp_stream_decode(const Sc64ZpUnitStream *s, unsigned slot,
                                 unsigned world_width, unsigned world_height, Unit *out)
{
    static const Unit zero_unit = {0};
    Unit base, decoded;
    unsigned base_id, end;
    if (!s || !out || slot >= MAX_UNITS || s->used > SC64_ZPS_DATA_BYTES) return 0;
    memset(&decoded, 0, sizeof(decoded));
    if (s->offset[slot] != SC64_ZPS_NO_ROW) {
        base_id = sc64_zps_row_base(s, s->offset[slot]);
        if (base_id < SC64_ZPS_TEMPLATES) {
            if (!sc64_zps_template(s, base_id, &base)) return 0;
        } else if (base_id != SC64_ZPS_ZERO_BASE) return 0;
        if (!sc64_zps_get_row(s, s->offset[slot], &base, &base_id, &decoded, &end) ||
            !memcmp(&decoded, &zero_unit, sizeof(decoded))) return 0;
    }
    if (!sc64_zps_unit_valid(&decoded, world_width, world_height)) return 0;
    *out = decoded;
    return 1;
}

/* Whole-stream structure: templates, then rows in slot order, cover the
 * used bytes exactly once; everything after them is zero. */
static int sc64_zp_stream_canonical(const Sc64ZpUnitStream *s)
{
    Unit row, base;
    unsigned next = 0, end, base_id;
    if (!s || s->template_count > SC64_ZPS_TEMPLATES || s->reserved ||
        s->used > SC64_ZPS_DATA_BYTES) return 0;
    for (unsigned t = 0; t < SC64_ZPS_TEMPLATES; ++t) {
        if (t >= s->template_count) {
            if (s->template_offset[t] != SC64_ZPS_NO_ROW) return 0;
            continue;
        }
        if (s->template_offset[t] != next ||
            !sc64_zps_get_row(s, next, 0, &base_id, &row, &end) ||
            base_id != SC64_ZPS_ZERO_BASE || row.x || row.y) return 0;
        next = (end + 7u) >> 3;
    }
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        if (s->offset[i] == SC64_ZPS_NO_ROW) continue;
        base_id = sc64_zps_row_base(s, next);
        if (s->offset[i] != next ||
            (base_id < SC64_ZPS_TEMPLATES && !sc64_zps_template(s, base_id, &base)) ||
            !sc64_zps_get_row(s, next, &base, &base_id, &row, &end)) return 0;
        next = (end + 7u) >> 3;
    }
    if (next != s->used) return 0;
    for (unsigned i = s->used; i < SC64_ZPS_DATA_BYTES; ++i) if (s->data[i]) return 0;
    return 1;
}

/* Encode a whole unit table. Templates are the first unit of each
 * (owner, type) pair, up to SC64_ZPS_TEMPLATES; other units use the
 * template of their pair, of their type, or the zero base. */
static int sc64_zp_stream_encode(Sc64ZpUnitStream *s, const Unit *units,
                                 unsigned world_width, unsigned world_height)
{
    static const Unit zero_unit = {0};
    Sc64ZpsWriter w;
    unsigned count = 0;
    if (!s || !units) return 0;
    memset(s, 0, sizeof(*s));
    w.data = s->data; w.cap = SC64_ZPS_DATA_BYTES; w.bit = 0;
    for (unsigned t = 0; t < SC64_ZPS_TEMPLATES; ++t) s->template_offset[t] = SC64_ZPS_NO_ROW;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &units[i];
        unsigned t;
        if (!memcmp(q, &zero_unit, sizeof(*q))) continue;
        if (!sc64_zps_unit_valid(q, world_width, world_height)) return 0;
        for (t = 0; t < count; ++t)
            if (units[s->offset[t]].owner == q->owner && units[s->offset[t]].type == q->type) break;
        if (t == count && count < SC64_ZPS_TEMPLATES) s->offset[count++] = (u16)i;
    }
    /* offset[] temporarily held template source slots; write templates. */
    unsigned source[SC64_ZPS_TEMPLATES];
    for (unsigned t = 0; t < count; ++t) source[t] = s->offset[t];
    for (unsigned i = 0; i < MAX_UNITS; ++i) s->offset[i] = SC64_ZPS_NO_ROW;
    Unit templates[SC64_ZPS_TEMPLATES];
    for (unsigned t = 0; t < count; ++t) {
        templates[t] = units[source[t]];
        templates[t].x = templates[t].y = 0;
        int at = sc64_zps_put_row(&w, SC64_ZPS_ZERO_BASE, &zero_unit, &templates[t]);
        if (at < 0) return 0;
        s->template_offset[t] = (u16)at;
    }
    s->template_count = (u8)count;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &units[i];
        unsigned t, base_id = SC64_ZPS_ZERO_BASE;
        if (!memcmp(q, &zero_unit, sizeof(*q))) continue;
        for (t = 0; t < count; ++t)
            if (units[source[t]].owner == q->owner && units[source[t]].type == q->type) break;
        if (t == count)
            for (t = 0; t < count; ++t) if (units[source[t]].type == q->type) break;
        if (t < count) base_id = t;
        int at = sc64_zps_put_row(&w, base_id,
                                  base_id == SC64_ZPS_ZERO_BASE ? &zero_unit : &templates[base_id], q);
        if (at < 0) return 0;
        s->offset[i] = (u16)at;
    }
    s->used = (u16)((w.bit + 7u) >> 3);
    return s->used <= SC64_ZPS_DATA_BYTES;
}

#endif
