/* Asset-free dyld inventory bounds check. */
#include "../../experiments/ares-headless/observer_images.h"
#include <cassert>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
int main(){
 char path[]="/tmp/sc64-images-XXXXXX";int fd=mkstemp(path);assert(fd>=0);
 assert(sc64_observer_images_write(fd));
 uint32_t header[3]={};assert(pread(fd,header,sizeof header,0)==sizeof header);
 assert(header[0]==0x5343494du && header[1]==1 && header[2] && header[2]<=128);
 assert(lseek(fd,0,SEEK_END)==(off_t)(12+header[2]*sizeof(Sc64ObserverImageRecord)));
 close(fd);unlink(path);puts("bounded dyld inventory: PASS");
}
