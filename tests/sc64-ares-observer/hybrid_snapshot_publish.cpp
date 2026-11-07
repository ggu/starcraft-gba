/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../../experiments/ares-headless/hybrid_snapshot_publish.h"
#include "../../experiments/ares-headless/observer_read.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
int main() {
    char dir[]="/tmp/sc64-snapshot-XXXXXX";assert(mkdtemp(dir));
    char path[256];assert(std::snprintf(path,sizeof(path),"%s/view",dir)>0);
    auto hit=[](std::uint32_t a){return a==2?-1:1;};
    auto cache=[](std::uint32_t){return std::uint8_t(7);};
    auto ram=[](std::uint32_t){return std::uint8_t(8);};
    assert(!sc64_publish_snapshot(path,4,[&](std::uint8_t *p,std::size_t n){return sc64_read_ram(0,n,4,p,hit,cache,ram);}));
    assert(access(path,F_OK)!=0);
    auto good=[](std::uint8_t* p,std::size_t n){std::memset(p,7,n);return true;};
    assert(sc64_publish_snapshot(path,4,good));
    assert(!sc64_publish_snapshot(path,4,[](std::uint8_t* p,std::size_t n){std::memset(p,9,n);return true;}));
    char b[4];int fd=open(path,O_RDONLY);assert(fd>=0 && read(fd,b,4)==4 && close(fd)==0);
    for(char c:b)assert(c==7);
    assert(unlink(path)==0 && rmdir(dir)==0);
}
