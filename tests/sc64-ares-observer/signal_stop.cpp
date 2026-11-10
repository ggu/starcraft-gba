#define SC64_OBSERVER_STOP_ON_SIGXFSZ 1
#include "../../experiments/ares-headless/observer_sigxfsz.h"
#include <fcntl.h>
#include <sys/resource.h>
int main(int argc,char **argv){
 if(argc!=3)return 2;int record=open(argv[1],O_CREAT|O_EXCL|O_WRONLY,0600);
 if(record<0||!sc64_observer_signal_install(record))return 3;
 struct rlimit r={2u*1024u*1024u,8u*1024u*1024u};if(setrlimit(RLIMIT_FSIZE,&r))return 4;
 int fd=open(argv[2],O_CREAT|O_EXCL|O_WRONLY,0600);if(fd<0)return 5;
 char buf[4096]={};for(int i=0;i<513;++i)if(write(fd,buf,sizeof buf)!=(ssize_t)sizeof buf)return 6;
 return 7;
}
