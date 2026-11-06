/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_OBSERVER_IMAGES_H
#define SC64_OBSERVER_IMAGES_H
#include <mach-o/dyld.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
/* Native Darwin arm64: 12-byte header (magic, observed count,
 * version/status/written), followed by records of address, slide, 16-bit
 * path byte count and non-NUL path bytes. No unbounded or truncated paths.
 * An image first loaded inside root->run may be absent. */
enum { SC64_OBSERVER_IMAGES_BYTES=67596, SC64_OBSERVER_IMAGE_PATH=512,
       SC64_IMAGES_OK=0, SC64_IMAGES_COUNT=1, SC64_IMAGES_PATH=2,
       SC64_IMAGES_IO=3, SC64_IMAGES_INCOMPLETE=4, SC64_IMAGES_FULL=5 };
static int sc64_observer_images_status(int fd,uint32_t count,unsigned status,unsigned written)
{
    uint32_t header[3]={0x5343494du,count,(2u<<24)|(status<<16)|written};
    if(fd>=0 && pwrite(fd,header,sizeof header,0)==(ssize_t)sizeof header)return 1;
    dprintf(STDERR_FILENO,"image inventory status write failed: count=%u status=%u written=%u\n",count,status,written);
    return 0;
}
static int sc64_observer_images_write(int fd)
{
    uint32_t count=_dyld_image_count(); unsigned written=0,status=SC64_IMAGES_INCOMPLETE;
    off_t offset=12;
    if(!sc64_observer_images_status(fd,count,status,written))return 0;
    if(count>65535u)status=SC64_IMAGES_COUNT;
    else {
        for(uint32_t i=0;i<count;++i) {
            const char *path=_dyld_get_image_name(i);
            const struct mach_header *image=_dyld_get_image_header(i);
            if(!path || !image){status=SC64_IMAGES_PATH;break;}
            size_t n=strnlen(path,SC64_OBSERVER_IMAGE_PATH);
            if(n==SC64_OBSERVER_IMAGE_PATH){status=SC64_IMAGES_PATH;break;}
            if((uint64_t)offset+18u+n>SC64_OBSERVER_IMAGES_BYTES){status=SC64_IMAGES_FULL;break;}
            uint64_t addr=(uint64_t)(uintptr_t)image;
            int64_t slide=(int64_t)_dyld_get_image_vmaddr_slide(i);
            uint16_t size=(uint16_t)n;
            if(pwrite(fd,&addr,8,offset)!=8 || pwrite(fd,&slide,8,offset+8)!=8 ||
               pwrite(fd,&size,2,offset+16)!=2 || (n && pwrite(fd,path,n,offset+18)!=(ssize_t)n))
                {status=SC64_IMAGES_IO;break;}
            offset+=(off_t)(18+n);++written;
        }
        if(written==count)status=SC64_IMAGES_OK;
    }
    if(!sc64_observer_images_status(fd,count,status,written))return 0;
    if(status!=SC64_IMAGES_OK)dprintf(STDERR_FILENO,"image inventory rejected: count=%u status=%u written=%u\n",count,status,written);
    return status==SC64_IMAGES_OK;
}
#endif
