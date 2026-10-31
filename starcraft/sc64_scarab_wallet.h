/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_WALLET_H
#define SC64_SCARAB_WALLET_H

#include <stdint.h>

/* A transient binding, not new persistent storage. Campaign source wallets
 * are uint32_t; engine wallets are int. Never alias one through the other. */
typedef struct {
    int *engine;
    uint32_t *source;
} Sc64ScarabWallet;

static inline int sc64_scarab_wallet_value_valid(int value)
{ return value >= 0 && value <= 65535; }

static inline int sc64_scarab_wallet_read(Sc64ScarabWallet bank, int *value)
{
    if (!value || (!!bank.engine == !!bank.source)) return 0;
    if (bank.engine) {
        if (!sc64_scarab_wallet_value_valid(*bank.engine)) return 0;
        *value = *bank.engine;
    } else {
        if (*bank.source > 65535u) return 0;
        *value = (int)*bank.source;
    }
    return 1;
}

/* Only after a successful read and bounded transaction preflight. No yield,
 * ownership change or wallet callback may occur between preflight and commit. */
static inline void sc64_scarab_wallet_commit(Sc64ScarabWallet bank, int value)
{
    if (bank.engine) *bank.engine = value;
    else *bank.source = (uint32_t)value;
}

#endif
