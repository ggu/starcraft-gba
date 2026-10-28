/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exhaustive oracle for the optional ROM geometry kernel. The reference is
 * the same arithmetic fallback used by the runtime when table bounds miss. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

#ifndef SC64_CONTACT_MASK_TABLE
#error "Compile this fixture with SC64_CONTACT_MASK_TABLE"
#endif

static void
check_full_table(void)
{
    unsigned tuples = 0, pending_checks = 0;
    const unsigned masks[] = {0x00u, 0xffu, 0x55u, 0xaau, 0x01u, 0x80u};
    for (int radius = 6; radius <= 19; ++radius)
        for (int speed = 0; speed <= 14; ++speed)
            for (int oy = -32; oy <= 32; ++oy)
                for (int ox = -32; ox <= 32; ++ox) {
                    unsigned want = sc64_contact_mask_math_reference(
                        0, radius, speed, ox, oy);
                    unsigned got = sc64_contact_mask_table_lookup(
                        radius, speed, ox, oy);
                    assert(got == want);
                    ++tuples;
                    if (((unsigned)tuples & 1023u) == 0u) {
                        for (unsigned i = 0; i < sizeof(masks)/sizeof(masks[0]); ++i) {
                            unsigned seeded = masks[i];
                            assert(sc64_contact_mask_apply_geometry(
                                seeded, radius, speed, ox, oy) ==
                                sc64_contact_mask_math_reference(
                                    seeded, radius, speed, ox, oy));
                            ++pending_checks;
                        }
                    }
                }
    assert(tuples == 887250u);
    assert(pending_checks > 5000u);
}

static void
check_fallback_domain(void)
{
    const int radii[] = {0, 1, 5, 20, 30, 31};
    const int speeds[] = {-1, 15, 255, 1024};
    const int offsets[] = {-100, -33, 0, 33, 100};
    unsigned checks = 0;
    for (unsigned ri = 0; ri < sizeof(radii)/sizeof(radii[0]); ++ri)
        for (unsigned si = 0; si < sizeof(speeds)/sizeof(speeds[0]); ++si)
            for (unsigned yi = 0; yi < sizeof(offsets)/sizeof(offsets[0]); ++yi)
                for (unsigned xi = 0; xi < sizeof(offsets)/sizeof(offsets[0]); ++xi)
                    for (unsigned mask = 0; mask < 256u; ++mask) {
                        int radius = radii[ri], speed = speeds[si];
                        int ox = offsets[xi], oy = offsets[yi];
                        assert(!sc64_contact_mask_table_domain(radius,speed,ox,oy));
                        assert(sc64_contact_mask_apply_geometry(mask,radius,speed,ox,oy) ==
                               sc64_contact_mask_math_reference(mask,radius,speed,ox,oy));
                        ++checks;
                    }
    assert(checks == 6u * 4u * 5u * 5u * 256u);
}

int
main(void)
{
    check_full_table();
    check_fallback_domain();
    /* With zero stock speed every offset coincides; a deep overlap blocks
     * every direction, while a separated actor blocks none. */
    assert(sc64_contact_mask_apply_geometry(0,19,0,0,0) == 0xffu);
    assert(sc64_contact_mask_apply_geometry(0,19,0,20,0) == 0u);
    puts("contact mask math: exhaustive table and fallback domains PASS");
    return 0;
}
