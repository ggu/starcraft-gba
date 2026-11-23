/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST fixture: a staged probe (the research/Egg probe, the active nuke probe) in a fresh process loads a
 * save written by the stock P10 product (tests/sc64_paid_scarab_product.c save-write) with a Scarab in
 * flight. Stock P10 numbers the Scarab SC_RU_LEGACY_COUNT (raw 85); here that id belongs to the Egg or the
 * nuke missile, and the Scarab is U_SCARAB behind them. The codecs translate the pair, and only that pair.
 * HOST images are never installed on a cartridge or used as ARM input.
 *   read FILE   load the stock save; a live Scarab must come back as U_SCARAB
 *   pairs       the translation rule, pair by pair */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
#include "../starcraft/game.c"

#ifndef SC64_PAID_SCARAB
#error "Use a paid-Scarab staged vector"
#endif
_Static_assert(U_SCARAB != SC_RU_LEGACY_COUNT, "a staged identity must precede U_SCARAB in this vector");

static int scarabs(int type)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == type && u[i].raw_type == 85) ++n;
    return n;
}

/* The P7 typed row codec, with the Scarab row of the loaded world re-typed as the stock build writes it and
 * as no build writes it. Fields are patched in the encoded row (LSB-first bit packing). */
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
#define ROW_ENC(r, q) sc64_p7_typed_unit_encode_for(r, q, WORLD_W, WORLD_H, 1)
#define ROW_DEC(o, r) sc64_p7_typed_unit_decode_for(o, r, WORLD_W, WORLD_H, 1)
#else
#define ROW_ENC(r, q) sc64_p7_typed_unit_encode(r, q, WORLD_W, WORLD_H)
#define ROW_DEC(o, r) sc64_p7_typed_unit_decode(o, r, WORLD_W, WORLD_H)
#endif
static void row_patch(Sc64P7TypedUnitSave *r, unsigned off, unsigned width, unsigned val)
{
    for (unsigned i = 0; i < width; ++i, ++off) {
        u8 m = (u8)(1u << (off & 7u));
        if ((val >> i) & 1u) r->bytes[off >> 3] |= m; else r->bytes[off >> 3] &= (u8)~m;
    }
}
static void codec_rows(void)
{
    enum { TYPE_BIT = 43, RAW_BIT = 186 };   /* x11 y11 hp18 owner3 | type6 ... shield9 | raw8 */
    int s = -1;
    Sc64P7TypedUnitSave row, bad;
    Unit out;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].type == U_SCARAB) { s = i; break; }
    assert(s >= 0 && u[s].raw_type == 85);
    assert(ROW_ENC(&row, &u[s]));
    memset(&out, 0, sizeof(out));
    assert(ROW_DEC(&out, &row) && out.type == U_SCARAB && out.raw_type == 85);
    /* The stock pair: id SC_RU_LEGACY_COUNT, raw 85 -> this build's Scarab. */
    bad = row; row_patch(&bad, TYPE_BIT, 6, SC_RU_LEGACY_COUNT);
    memset(&out, 0, sizeof(out));
    assert(ROW_DEC(&out, &bad) && out.type == U_SCARAB && out.raw_type == 85);
    /* The staged id with any other raw type is not a Scarab and is not translated. */
    for (unsigned raw = 0; raw < 255; ++raw) {
        if (raw == 85) continue;
        bad = row; row_patch(&bad, TYPE_BIT, 6, SC_RU_LEGACY_COUNT); row_patch(&bad, RAW_BIT, 8, raw + 1);
        memset(&out, 0, sizeof(out));
        if (ROW_DEC(&out, &bad)) assert(out.type == SC_RU_LEGACY_COUNT && out.raw_type == (int)raw);
    }
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
    bad = row; row_patch(&bad, TYPE_BIT, 6, SC_RU_LEGACY_COUNT); row_patch(&bad, RAW_BIT, 8, 84 + 1);
    assert(!ROW_DEC(&out, &bad));            /* the Egg id with a Scarab-like row but the wrong raw type */
#endif
    puts("codec rows PASS (stock pair translated; the staged id with another raw type is never a Scarab)");
}

static void read_stock(const char *path)
{
    FILE *f = fopen(path, "rb");
    assert(f);
    gba_init();
    game_init();
    assert(fread(host_save, 1, sizeof(host_save), f) == sizeof(host_save));
    assert(fgetc(f) == EOF && !fclose(f));
    assert(race_load_game());
    printf("stock P10 save loaded: %d Scarab(s) as U_SCARAB (id %d); %d left on the staged id %d\n",
           scarabs(U_SCARAB), (int)U_SCARAB, scarabs(SC_RU_LEGACY_COUNT), (int)SC_RU_LEGACY_COUNT);
    assert(scarabs(U_SCARAB) >= 1);
    assert(scarabs(SC_RU_LEGACY_COUNT) == 0);
    codec_rows();
    /* The loaded world runs on: the Scarab flies and lands as in the product. */
    for (int t = 0; t < 400; ++t) game_update(0, 0);
    puts("read PASS");
}

static void pairs(void)
{
    struct { int type, raw, scarab; } cases[] = {
        { SC_RU_LEGACY_COUNT, 85, 1 },       /* the stock Scarab */
        { SC_RU_LEGACY_COUNT, 84, 0 },       /* next raw type: stays the staged identity's row */
        { SC_RU_LEGACY_COUNT, 36, 0 },       /* the Egg's own raw type */
        { SC_RU_LEGACY_COUNT, -1, 0 },
        { SC_RU_LEGACY_COUNT - 1, 85, 0 },   /* a legacy type with the Scarab's raw type */
        { SC_RU_LEGACY_COUNT + 1, 85, 0 },   /* a different staged id */
        { U_SCARAB, 85, 0 },                 /* already this build's Scarab: untouched */
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        Unit q;
        memset(&q, 0, sizeof(q));
        q.type = cases[i].type; q.raw_type = cases[i].raw;
        SC64_PSC_TRANSLATE_STOCK(q);
        int translated = cases[i].type != q.type;
        assert(translated == cases[i].scarab);
        assert(!translated || q.type == U_SCARAB);
    }
    puts("pairs PASS (only type SC_RU_LEGACY_COUNT with raw 85 becomes U_SCARAB)");
}

/* An image with a Scarab whose raw type is not 85 (no stock build writes one): the id the staged build gives
 * the Egg or the missile keeps its own type and raw checks, so the load is refused. */
static void reject_mismatch(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { puts("reject: no mismatched image (the stock writer refuses the pair); nothing to load"); return; }
    gba_init();
    game_init();
    assert(fread(host_save, 1, sizeof(host_save), f) == sizeof(host_save));
    assert(fgetc(f) == EOF && !fclose(f));
    assert(!race_load_game());
    puts("reject PASS (type SC_RU_LEGACY_COUNT with a raw type other than 85 is refused)");
}

int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "reject")) { reject_mismatch(argv[2]); return 0; }
    if (argc == 3 && !strcmp(argv[1], "read")) read_stock(argv[2]);
    else if (argc == 2 && !strcmp(argv[1], "pairs")) pairs();
    else assert(!"usage: read FILE | pairs");
    return 0;
}
