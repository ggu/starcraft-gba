/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_scarab_wallet.h"

int main(void)
{
    int human = 100, value = -7;
    uint32_t first = 200, second = 300;
    Sc64ScarabWallet h = {&human, 0, 65535}, a = {0, &first, 65535};
    Sc64ScarabWallet b = {0, &second, 65535};
    assert(sc64_scarab_wallet_read(h, &value) && value == 100);
    sc64_scarab_wallet_commit(h, value - 15);
    assert(human == 85 && first == 200 && second == 300);
    assert(sc64_scarab_wallet_read(a, &value) && value == 200);
    sc64_scarab_wallet_commit(a, value - 15);
    assert(human == 85 && first == 185 && second == 300);
    assert(sc64_scarab_wallet_read(b, &value) && value == 300);
    sc64_scarab_wallet_commit(b, value - 15);
    assert(human == 85 && first == 185 && second == 285);
    for (int n = 0; n <= 65535; ++n) {
        human = n; first = (uint32_t)n;
        assert(sc64_scarab_wallet_read(h, &value) && value == n);
        assert(sc64_scarab_wallet_read(a, &value) && value == n);
    }
    value = 77; human = -1;
    assert(!sc64_scarab_wallet_read(h, &value) && value == 77);
    human = 65536;
    assert(!sc64_scarab_wallet_read(h, &value) && value == 77);
    first = UINT32_MAX;
    assert(!sc64_scarab_wallet_read(a, &value) && value == 77);
    assert(!sc64_scarab_wallet_read((Sc64ScarabWallet){0}, &value));
    assert(!sc64_scarab_wallet_read((Sc64ScarabWallet){&human,&first,65535}, &value));
    first = 10000;
    assert(sc64_scarab_wallet_read((Sc64ScarabWallet){0,&first,10000}, &value));
    ++first;
    assert(!sc64_scarab_wallet_read((Sc64ScarabWallet){0,&first,10000}, &value));
    assert(!sc64_scarab_wallet_read(b, 0));
    assert(!sc64_scarab_wallet_value_valid(-1));
    assert(!sc64_scarab_wallet_value_valid(65536));
    puts("Scarab signed/source wallet isolation and bounds passed");
    return 0;
}
