/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_research_human_binding.h"

int main(void)
{
    /* Source-role census of actual P10-product campaign initializers at
     * 1286edbf. These arrays are test witnesses, not runtime map admission. */
    const int8_t roles[10][8] = {
        {1,3,3,2,2,2,2,0}, {2,2,3,2,2,2,1,0},
        {1,2,2,2,2,2,4,0}, {1,3,3,4,2,2,2,0},
        {2,2,3,1,4,2,2,0}, {2,3,3,1,2,3,4,0},
        {1,3,3,2,1,2,2,0}, {1,2,3,1,2,2,2,0},
        {1,2,3,2,2,1,1,0}, {1,0,3,1,2,2,2,2}
    };
    Sc64ResearchHumanBinding out, before;
    for (unsigned i = 0; i < 10; ++i) {
        char mission[4]; unsigned human = i == 9 ? 1u : 7u;
        snprintf(mission, sizeof mission, "P%u", i + 1);
        assert(sc64_research_bind_protoss_human(0,mission,human,roles[i],0,&out));
        assert(out.current_controller_key == human && out.payer_key == human &&
               out.wallet_slot == 0 && out.chapter == i + 1);
        before = out;
        assert(!sc64_research_bind_protoss_human(1,mission,human,roles[i],0,&out));
        for (int owner = 1; owner <= 4; ++owner)
            assert(!sc64_research_bind_protoss_human(0,mission,human,roles[i],owner,&out));
        assert(!memcmp(&out,&before,sizeof out));
        for (unsigned slot = 0; slot < 8; ++slot) {
            int8_t changed[8]; memcpy(changed,roles[i],sizeof changed);
            changed[slot] = slot == human ? 1 : 0;
            assert(!sc64_research_bind_protoss_human(0,mission,human,changed,0,&out));
            changed[slot] = -1;
            assert(!sc64_research_bind_protoss_human(0,mission,human,changed,0,&out));
            assert(!memcmp(&out,&before,sizeof out));
        }
    }
    const char *bad[] = {"", "P", "P0", "P01", "P11", "P1x", "P100", "T1", "Z5"};
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; ++i)
        assert(!sc64_research_bind_protoss_human(0,bad[i],7,roles[0],0,&out));
    assert(!sc64_research_bind_protoss_human(0,"P1",1,roles[0],0,&out));
    assert(!sc64_research_bind_protoss_human(0,0,7,roles[0],0,&out));
    assert(!sc64_research_bind_protoss_human(0,"P1",7,0,0,&out));
    assert(!sc64_research_bind_protoss_human(0,"P1",7,roles[0],0,0));
    puts("Protoss human controller/payer context binding: PASS");
    return 0;
}
