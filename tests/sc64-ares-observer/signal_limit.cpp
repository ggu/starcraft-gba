/* Asset-free real SIGXFSZ test; no emulator or game input. */
#include "../../experiments/ares-headless/observer_sigxfsz.h"
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
int main() {
 char dir[]="/tmp/sc64-sigxfsz-XXXXXX"; assert(mkdtemp(dir));
 char record[256],data[256]; snprintf(record,sizeof record,"%s/record",dir);snprintf(data,sizeof data,"%s/data",dir);
 int fd=open(record,O_CREAT|O_EXCL|O_RDWR,0600);assert(fd>=0);
 pid_t child=fork();assert(child>=0);
 if(!child){struct rlimit r={2u*1024u*1024u,8u*1024u*1024u};if(setrlimit(RLIMIT_FSIZE,&r)||!sc64_observer_signal_install(fd))_exit(1);
  int out=open(data,O_CREAT|O_EXCL|O_WRONLY,0600);if(out<0)_exit(2);
  char buf[4096]={0};for(int i=0;i<513;i++)if(write(out,buf,sizeof buf)!=(ssize_t)sizeof buf)_exit(3);_exit(4);}
 int status=0;assert(waitpid(child,&status,0)==child);assert(WIFEXITED(status)&&WEXITSTATUS(status)==128+SIGXFSZ);
 Sc64ObserverSignalRecord r={};assert(pread(fd,&r,sizeof r,0)==sizeof r);assert(r.magic==0x53433634u&&r.version==1&&r.signal_number==SIGXFSZ&&r.host_pc&&r.host_sp);
 assert(lseek(fd,0,SEEK_END)==sizeof r);close(fd);unlink(record);unlink(data);rmdir(dir);puts("SIGXFSZ bounded diagnostic: PASS");
}
