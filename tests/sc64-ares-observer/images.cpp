/* Asset-free dyld inventory with the observer's Cocoa and weak MoltenVK. */
#include "../../experiments/ares-headless/observer_images.h"
#include <cassert>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <cstring>
int main(){
 void *vk=dlopen("/Applications/ares.app/Contents/Frameworks/libMoltenVK.dylib",RTLD_NOW|RTLD_LOCAL);assert(vk);
 char path[]="/tmp/sc64-images-XXXXXX";int fd=mkstemp(path);assert(fd>=0);
 int ok=sc64_observer_images_write(fd);
 uint32_t h[3]={};assert(pread(fd,h,sizeof h,0)==sizeof h);
 unsigned status=(h[2]>>16)&255u,written=h[2]&65535u;
 assert(h[0]==0x5343494du && (h[2]>>24)==2 && h[1]);
 assert((ok && !status && written==h[1]) || (!ok && status));
 off_t total=lseek(fd,0,SEEK_END);assert(total<=(off_t)SC64_OBSERVER_IMAGES_BYTES);
 if(ok){off_t at=12;for(unsigned i=0;i<written;++i){
   uint64_t addr;int64_t slide;uint16_t n;char name[512]={};
   assert(pread(fd,&addr,8,at)==8 && pread(fd,&slide,8,at+8)==8 && pread(fd,&n,2,at+16)==2);
   assert(n<sizeof name && pread(fd,name,n,at+18)==n);
   assert(addr==(uint64_t)(uintptr_t)_dyld_get_image_header(i));
   assert(slide==(int64_t)_dyld_get_image_vmaddr_slide(i));
   assert(!strcmp(name,_dyld_get_image_name(i)));at+=18+n;
  }assert(at==total);}

 printf("dyld images=%u status=%u written=%u bytes=%lld\n",h[1],status,written,(long long)total);
 close(fd);unlink(path);dlclose(vk);return 0;
}
