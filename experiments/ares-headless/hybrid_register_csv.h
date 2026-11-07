/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_HYBRID_REGISTER_CSV_H
#define SC64_HYBRID_REGISTER_CSV_H
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
/* Guest registers sampled at the capture boundary, not store-writer PCs. */
inline std::string sc64_register_csv_header() {
    std::string result="snapshot,completed_step,pc,raw_cpu_clock";
    for(unsigned i=0;i<32;++i) result+=",r"+std::to_string(i);
    return result+"\n";
}
inline std::string sc64_register_csv_row(unsigned index,unsigned step,std::uint64_t pc,
                                         std::uint64_t clock,const std::array<std::uint64_t,32>& regs) {
    char head[96];int n=std::snprintf(head,sizeof(head),"%u,%u,%016llx,%016llx",index,step,
        static_cast<unsigned long long>(pc),static_cast<unsigned long long>(clock));
    if(n<0 || static_cast<std::size_t>(n)>=sizeof(head))return {};
    std::string row(head,static_cast<std::size_t>(n));
    for(auto v:regs) { char field[18]; n=std::snprintf(field,sizeof(field),",%016llx",static_cast<unsigned long long>(v));
      if(n!=17)return {};row.append(field,static_cast<std::size_t>(n)); }
    return row+"\n";
}
#endif
