/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../../experiments/ares-headless/hybrid_register_csv.h"
#include <cassert>
int main() {
 std::array<std::uint64_t,32> regs{};regs[28]=0x123456789abcdef0ull;regs[31]=~0ull;
 auto h=sc64_register_csv_header();auto r=sc64_register_csv_row(2,4322,0x80001234,0,regs);
 assert(h.find(",r28,r29,r30,r31\n")!=std::string::npos);
 assert(r.find("2,4322,0000000080001234,0000000000000000,")==0);
 assert(r.find(",123456789abcdef0,")!=std::string::npos);
 assert(r.find(",ffffffffffffffff\n")!=std::string::npos);
 assert(h.size()+3*r.size()<8192);
}
