/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../common/gba.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    unsigned old=17, next=29, out=0;
    const unsigned legacy=0x1234, modern=0x5678;
    save_write(&old,sizeof(old),legacy); /* first legacy record in slot 0 */
    save_write_migrating(&next,sizeof(next),modern,sizeof(old),legacy);
    save_test_corrupt_latest(sizeof(next),modern);
    assert(save_read(&out,sizeof(out),legacy) && out==old);
    assert(!save_read(&out,sizeof(out),modern));
    next=31; save_write_migrating(&next,sizeof(next),modern,sizeof(old),legacy);
    assert(save_read(&out,sizeof(out),modern) && out==31);
    assert(save_read(&out,sizeof(out),legacy) && out==old);
    next=33; save_write_migrating(&next,sizeof(next),modern,sizeof(old),legacy);
    assert(save_read(&out,sizeof(out),modern) && out==33);
    /* A newer legacy record in either slot must survive first new-format
     * write as well. Distinct tags isolate this case from the prior journal. */
    const unsigned legacy2=0x1235, modern2=0x5679;
    old=41; save_write(&old,sizeof(old),legacy2);
    old=43; save_write(&old,sizeof(old),legacy2);
    next=47; save_write_migrating(&next,sizeof(next),modern2,sizeof(old),legacy2);
    save_test_corrupt_latest(sizeof(next),modern2);
    assert(save_read(&out,sizeof(out),legacy2) && out==43);
    struct { unsigned first, appended; } expanded={59,61};
    unsigned legacy3=0x1236, modern3=0x5680;
    old=53; save_write(&old,sizeof(old),legacy3);
    save_write_migrating(&expanded,sizeof(expanded),modern3,sizeof(old),legacy3);
    save_test_corrupt_latest(sizeof(expanded),modern3);
    assert(save_read(&out,sizeof(out),legacy3) && out==53);
    save_write_migrating(&expanded,sizeof(expanded),modern3,sizeof(old),legacy3);
    struct { unsigned first, appended; } decoded={0};
    assert(save_read(&decoded,sizeof(decoded),modern3) && decoded.first==59 && decoded.appended==61);
    /* Invalid predecessor lengths must leave the existing journal untouched. */
    save_write_migrating(&expanded,sizeof(expanded),modern3,0xffffffffu,legacy3);
    decoded.first=decoded.appended=0;
    assert(save_read(&decoded,sizeof(decoded),modern3) && decoded.first==59 && decoded.appended==61);
    assert(save_read(&out,sizeof(out),legacy3) && out==53);
    /* Variable-length legacy and current records share one sequence clock.
     * Corrupting each new write simulates interruption before commit. */
    const unsigned legacy4=0x1237, modern4=0x5681;
    unsigned words[3]={71,73,79}, got[3]={0}, len=0;
    save_write(words, sizeof(words), legacy4);
    save_write_migrating(words, sizeof(words)-sizeof(unsigned), modern4, 0, legacy4);
    save_test_corrupt_latest(0, modern4);
    assert(save_read_latest(got,sizeof(got),legacy4,&len) && len==sizeof(words) && got[2]==79);
    save_write_migrating(words, sizeof(words), modern4, 0, legacy4);
    words[0]=83;
    save_write_migrating(words, sizeof(unsigned), modern4, 0, legacy4);
    assert(save_read_latest(got,sizeof(got),modern4,&len) && len==sizeof(unsigned) && got[0]==83);
    save_test_corrupt_latest(0,modern4);
    assert(save_read_latest(got,sizeof(got),modern4,&len) && len==sizeof(words) && got[0]==71);
    /* A checksum-valid old private BC record must not displace a loadable
     * legacy record when the new feature marker rejects that old format. */
    const unsigned legacy5=0x1238, modern5=0x5682;
    struct { unsigned marker, value; } feature={0x42430000u,101};
    old=97; save_write(&old,sizeof(old),legacy5);
    save_write_migrating(&feature,sizeof(feature),modern5,0,legacy5);
    feature.marker=0x42450000u;feature.value=103;
    save_write_migrating_feature(&feature,sizeof(feature),modern5,legacy5,
                                 0,0xffff0000u,0x42450000u);
    save_test_corrupt_latest(0,modern5);
    assert(save_read(&out,sizeof(out),legacy5) && out==97);
    puts("migration journal preserves latest legacy predecessor in either slot and across payload lengths");
    return 0;
}
