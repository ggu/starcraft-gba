/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_STORE_HOOK_H
#define SC64_STORE_HOOK_H
#include <cstdint>
#include <cstddef>
/* Host-only sink supplied by the observer; these calls never access guest memory. */
extern "C" void sc64_store_instruction_begin(std::uint64_t pc,std::uint32_t opcode,std::uint64_t base);
extern "C" void sc64_store_instruction_end();
extern "C" void sc64_store_committed(std::uint64_t vaddr,std::uint32_t paddr,unsigned width,std::uint64_t data);
#endif
