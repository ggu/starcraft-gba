/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_OBSERVER_IMAGES_H
#define SC64_OBSERVER_IMAGES_H
#include <mach-o/dyld.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
/* Fixed 12-byte header: magic, observed image count, version/status/written.
 * A library first loaded inside root->run may be absent. */
enum { SC64_OBSERVER_IMAGE_LIMIT=128, SC64_OBSERVER_IMAGE_PATH=512,
       SC64_IMAGES_OK=0, SC64_IMAGES_COUNT=1, SC64_IMAGES_PATH=2,
       SC64_IMAGES_IO=3, SC64_IMAGES_INCOMPLETE=4 };
struct Sc64ObserverImageRecord { uint64_t header; int64_t slide; char path[SC64_OBSERVER_IMAGE_PATH]; };
static int sc64_observer_images_status(int fd,uint32_t count,unsigned status,unsigned written)
{
    uint32_t header[3]={0x5343494du,count,(1u<<24)|(status<<16)|written};
    if(fd>=0 && pwrite(fd,header,sizeof header,0)==(ssize_t)sizeof header)return 1;
    /* A failed file write cannot guarantee a file record; stderr is a
     * separately bounded, best-effort fallback outside a signal handler. */
    dprintf(STDERR_FILENO,"image inventory status write failed: count=%u status=%u written=%u\n",count,status,written);
    return 0;
}
static int sc64_observer_images_write(int fd)
{
    uint32_t count=_dyld_image_count(); unsigned written=0,status=SC64_IMAGES_INCOMPLETE;
    if(!sc64_observer_images_status(fd,count,status,written))return 0;
    if(count>SC64_OBSERVER_IMAGE_LIMIT)status=SC64_IMAGES_COUNT;
    else {
        for(uint32_t i=0;i<count;++i) {
            Sc64ObserverImageRecord record={};
            const char *path=_dyld_get_image_name(i);
            const struct mach_header *image=_dyld_get_image_header(i);
            if(!path || !image){status=SC64_IMAGES_PATH;break;}
            size_t n=strnlen(path,SC64_OBSERVER_IMAGE_PATH);
            if(n==SC64_OBSERVER_IMAGE_PATH){status=SC64_IMAGES_PATH;break;}
            record.header=(uint64_t)(uintptr_t)image;
            record.slide=(int64_t)_dyld_get_image_vmaddr_slide(i);
            memcpy(record.path,path,n);
            if(pwrite(fd,&record,sizeof record,12+(off_t)i*sizeof record)!=(ssize_t)sizeof record){status=SC64_IMAGES_IO;break;}
            ++written;
        }
        if(written==count)status=SC64_IMAGES_OK;
    }
    if(!sc64_observer_images_status(fd,count,status,written))return 0;
    if(status!=SC64_IMAGES_OK) dprintf(STDERR_FILENO,"image inventory rejected: count=%u status=%u written=%u\n",count,status,written);
    return status==SC64_IMAGES_OK;
}
#endif
