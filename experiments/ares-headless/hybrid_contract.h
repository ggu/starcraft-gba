/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_HYBRID_CONTRACT_H
#define SC64_HYBRID_CONTRACT_H
#include <cstddef>
#include <cstdint>
#include <cstring>
/* An alias conflict or incomplete view is never equality. */
static inline bool sc64_hybrid_views_equal(bool a_ok,const std::uint8_t *a,
                                            bool b_ok,const std::uint8_t *b,std::size_t n)
{ return a_ok && b_ok && a && b && n && n<=8u*1024u*1024u && std::memcmp(a,b,n)==0; }
static inline bool sc64_hybrid_capture_allowed(int capture) { return capture>=0 && capture<=2; }
#endif
