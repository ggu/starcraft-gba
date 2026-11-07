/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_HYBRID_SNAPSHOT_PUBLISH_H
#define SC64_HYBRID_SNAPSHOT_PUBLISH_H
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
/* A failed cache-aware view never opens the target. Existing targets are never replaced. */
template<class ReadView>
bool sc64_publish_snapshot(const char *path,std::size_t size,ReadView read) {
    if (!path || size==0 || size>8u*1024u*1024u) return false;
    std::vector<std::uint8_t> scratch(size);
    if (!read(scratch.data(),size)) return false;
    int fd=::open(path,O_WRONLY|O_CREAT|O_EXCL,0600);
    if(fd<0)return false;
    std::size_t written=0;
    while(written<size) {
        auto n=::write(fd,scratch.data()+written,size-written);
        if(n<=0) { ::close(fd); return false; }
        written+=static_cast<std::size_t>(n);
    }
    return ::close(fd)==0;
}
#endif
