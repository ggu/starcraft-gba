/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_OBSERVER_FILE_BUDGET_H
#define SC64_OBSERVER_FILE_BUDGET_H
#include <sys/resource.h>
#include <cstdint>
constexpr rlim_t SC64_OBSERVER_SMALL_FILE = 2u*1024u*1024u;
constexpr rlim_t SC64_OBSERVER_RAM_FILE = 8u*1024u*1024u;
inline bool sc64_observer_file_limit(rlim_t size) {
    rlimit cap{};
    if ((size!=SC64_OBSERVER_SMALL_FILE && size!=SC64_OBSERVER_RAM_FILE) ||
        getrlimit(RLIMIT_FSIZE,&cap)!=0 || cap.rlim_max<SC64_OBSERVER_RAM_FILE) return false;
    cap.rlim_cur=size;return setrlimit(RLIMIT_FSIZE,&cap)==0;
}
#endif
