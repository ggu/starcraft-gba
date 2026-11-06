/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../../experiments/ares-headless/observer_file_budget.h"
#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
int main() {
    rlimit r{SC64_OBSERVER_SMALL_FILE,SC64_OBSERVER_RAM_FILE};
    assert(setrlimit(RLIMIT_FSIZE,&r)==0);
    assert(sc64_observer_file_limit(SC64_OBSERVER_RAM_FILE));
    assert(getrlimit(RLIMIT_FSIZE,&r)==0 && r.rlim_cur==SC64_OBSERVER_RAM_FILE);
    assert(sc64_observer_file_limit(SC64_OBSERVER_SMALL_FILE));
    assert(getrlimit(RLIMIT_FSIZE,&r)==0 && r.rlim_cur==SC64_OBSERVER_SMALL_FILE);
    assert(!sc64_observer_file_limit(SC64_OBSERVER_SMALL_FILE+1));
    std::signal(SIGXFSZ,SIG_IGN);
    char name[]="/tmp/sc64-fsize-XXXXXX";int fd=mkstemp(name);assert(fd>=0);unlink(name);
    assert(lseek(fd,SC64_OBSERVER_SMALL_FILE-1,SEEK_SET)==SC64_OBSERVER_SMALL_FILE-1);
    char x=0;assert(write(fd,&x,1)==1);errno=0;assert(write(fd,&x,1)==-1 && errno==EFBIG);
    close(fd);
}
