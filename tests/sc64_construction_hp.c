/* Independent integer model tests for optional 30 Hz construction HP. */
#include <assert.h>
#include <stdint.h>
typedef uint32_t u32;
#define SC64_RACES
#define SC64_CONSTRUCTION_HP
#include "../starcraft/sc64_construction_hp.h"

static void check_progression(int maximum, int total)
{
    int start = sc64_construction_hp_start(maximum);
    int accumulated = start;
    assert(start == (maximum > 0 ? (maximum / 10 ? maximum / 10 : 1) : 0));
    assert(sc64_construction_hp_gain(maximum, 0, total) == 0);
    assert(sc64_construction_hp_gain(maximum, total + 1, total) ==
           sc64_construction_hp_gain(maximum, total, total));
    for (int p = 1; p <= total; ++p) {
        int gain = sc64_construction_hp_gain(maximum, p, total);
        int max_gain = (maximum - start + total - 1) / total;
        assert(gain >= 0 && gain <= max_gain);
        accumulated += gain;
        assert(accumulated <= maximum);
    }
    assert(accumulated == maximum);
}

int main(void)
{
    check_progression(1, 1);
    check_progression(9, 1);
    check_progression(10, 13);
    check_progression(300, 375);
    check_progression(400, 375);
    check_progression(1800, 1875);
    check_progression(100000, 1500);

    /* Growth is additive: damage remains a constant absolute HP deficit. */
    int before = sc64_construction_hp_start(400);
    before += sc64_construction_hp_gain(400, 1, 375);
    before -= 5;
    for (int p = 2; p <= 375; ++p)
        before += sc64_construction_hp_gain(400, p, 375);
    assert(before == 395);

    /* Protoss shield construction growth is separate from the passive 7/320
       per 30 Hz tick; a concurrent passive increment still remains additive. */
    int shields = sc64_construction_shield_start(300);
    assert(shields == 30);
    shields += sc64_construction_shield_gain(300, 1, 375);
    assert(shields == 30);
    shields += 1; /* existing converted passive-recharge tick */
    shields -= 20; /* damage after the first construction tick */
    for (int p = 2; p <= 375; ++p) {
        shields += sc64_construction_shield_gain(300, p, 375);
        if (shields > 300) shields = 300;
    }
    assert(shields == 281); /* 19 points remain missing after regen */

    /* Native morph completion applies newMax-oldMax and clamps the survivor. */
    assert(sc64_construction_morph_hp(400, 400, 300) == 300);
    assert(sc64_construction_morph_hp(350, 400, 300) == 250);
    assert(sc64_construction_morph_hp(50, 400, 300) == 1);
    assert(sc64_construction_morph_hp(350, 400, 400) == 350);
    assert(sc64_construction_morph_hp(1250, 1250, 1500) == 1500);
    assert(sc64_construction_morph_hp(500, 1500, 1800) == 800);
    assert(sc64_construction_morph_hp(100, 1800, 1000) == 1);
    return 0;
}
