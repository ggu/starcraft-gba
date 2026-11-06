/* Asset-free dyld inventory with the observer's Cocoa and weak MoltenVK. */
#include "../../experiments/ares-headless/observer_images.h"
#include <cassert>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
int main(){
 void *vk=dlopen("/Applications/ares.app/Contents/Frameworks/libMoltenVK.dylib",RTLD_NOW|RTLD_LOCAL);assert(vk);
 char path[]="/tmp/sc64-images-XXXXXX";int fd=mkstemp(path);assert(fd>=0);
 int ok=sc64_observer_images_write(fd);
 uint32_t h[3]={};assert(pread(fd,h,sizeof h,0)==sizeof h);
 unsigned status=(h[2]>>16)&255u,written=h[2]&65535u;
 assert(h[0]==0x5343494du && (h[2]>>24)==2 && h[1]);
 assert((ok && !status && written==h[1]) || (!ok && status));
 assert(lseek(fd,0,SEEK_END)<=(off_t)(SC64_OBSERVER_IMAGES_BYTES));
 printf("dyld images=%u status=%u written=%u\n",h[1],status,written);
 close(fd);unlink(path);dlclose(vk);return 0;
}
