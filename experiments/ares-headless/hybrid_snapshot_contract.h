/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_HYBRID_SNAPSHOT_CONTRACT_H
#define SC64_HYBRID_SNAPSHOT_CONTRACT_H
#include <cstddef>
/* A separate, not-yet-wired role. Never admit snapshots in the visual role. */
constexpr bool sc64_hybrid_snapshot_allowed(bool snapshot_role, bool transitioned,
                                            bool view_equal, bool cpu_jit,
                                            bool rsp_jit, bool homebrew,
                                            std::size_t ram_size, unsigned count) {
    return snapshot_role && transitioned && view_equal && !cpu_jit && !rsp_jit &&
           !homebrew && ram_size == 8u*1024u*1024u && count < 3;
}
#endif
