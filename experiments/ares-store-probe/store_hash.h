/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_STORE_HASH_H
#define SC64_STORE_HASH_H
#include <nall/nall.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
inline bool sc64_store_hash_equal(const std::uint8_t* bytes,std::size_t size,const std::array<std::uint8_t,32>& expected) {
 if(!bytes)return false;
 const auto actual=nall::Hash::SHA256(std::span<const std::uint8_t>(bytes,size)).output();
 if(actual.size()!=expected.size())return false;
 for(std::size_t i=0;i<expected.size();++i)if(actual[i]!=expected[i])return false;
 return true;
}
inline bool sc64_store_baseline_hash_valid(const std::uint8_t* bytes,std::size_t size) {
 static constexpr std::array<std::uint8_t,32> expected={0x5d,0x8a,0xf3,0xad,0x41,0x37,0xde,0x6c,0xd1,0xc6,0xd5,0x69,0xc9,0x5d,0xad,0x6e,0x93,0x8d,0xd5,0x32,0x24,0x5a,0x42,0x75,0x3b,0xa4,0xd4,0xa4,0x69,0xd2,0x57,0xa5};
 return size==8u*1024u*1024u && sc64_store_hash_equal(bytes,size,expected);
}
#endif
