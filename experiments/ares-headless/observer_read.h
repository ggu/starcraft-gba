/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_OBSERVER_READ_H
#define SC64_OBSERVER_READ_H
#include <cstddef>
#include <cstdint>
/* Validate range before callbacks; on a cache conflict discard the private scratch buffer. */
constexpr bool sc64_observer_mask_allowed(int mask) { return mask>=0 && (mask & ~1023)==0; }
template<class CacheHit, class CacheRead, class RamRead>
bool sc64_read_ram(std::uint32_t address,std::size_t length,std::size_t ram_size,
                   std::uint8_t *out,CacheHit hit,CacheRead cached,RamRead ram) {
    if (!out || ram_size==0 || ram_size>8u*1024u*1024u ||
        address>=ram_size || length>ram_size-address) return false;
    for (std::size_t i=0;i<length;++i) {
        auto a=address+static_cast<std::uint32_t>(i);
        int state=hit(a);
        if (state<0 || state>1) return false;
        out[i]=state ? cached(a) : ram(a);
    }
    return true;
}
#endif
