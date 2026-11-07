/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../../experiments/ares-headless/hybrid_snapshot_contract.h"
#include <cassert>
int main() {
    constexpr auto n=8u*1024u*1024u;
    assert(sc64_hybrid_snapshot_allowed(true,true,true,false,false,false,n,0));
    assert(sc64_hybrid_snapshot_allowed(true,true,true,false,false,false,n,2));
    assert(!sc64_hybrid_snapshot_allowed(false,true,true,false,false,false,n,0));
    assert(!sc64_hybrid_snapshot_allowed(true,false,true,false,false,false,n,0));
    assert(!sc64_hybrid_snapshot_allowed(true,true,false,false,false,false,n,0));
    assert(!sc64_hybrid_snapshot_allowed(true,true,true,true,false,false,n,0));
    assert(!sc64_hybrid_snapshot_allowed(true,true,true,false,true,false,n,0));
    assert(!sc64_hybrid_snapshot_allowed(true,true,true,false,false,true,n,0));
    assert(!sc64_hybrid_snapshot_allowed(true,true,true,false,false,false,n-1,0));
    assert(!sc64_hybrid_snapshot_allowed(true,true,true,false,false,false,n,3));
}
