/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_OBSERVER_IMAGES_H
#define SC64_OBSERVER_IMAGES_H
#include <mach-o/dyld.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
/* Fresh owned file; bounded before guest execution. Binary fields are native
 * Darwin arm64. A library first loaded inside root->run may be absent. */
enum { SC64_OBSERVER_IMAGE_LIMIT=128, SC64_OBSERVER_IMAGE_PATH=512 };
struct Sc64ObserverImageRecord { uint64_t header; int64_t slide; char path[SC64_OBSERVER_IMAGE_PATH]; };
static int sc64_observer_images_write(int fd)
{
    uint32_t header[3]={0x5343494du,1u,_dyld_image_count()};
    if(fd<0 || header[2]>SC64_OBSERVER_IMAGE_LIMIT ||
       write(fd,header,sizeof header)!=(ssize_t)sizeof header)return 0;
    for(uint32_t i=0;i<header[2];++i) {
        Sc64ObserverImageRecord record={};
        const char *path=_dyld_get_image_name(i);
        const struct mach_header *image=_dyld_get_image_header(i);
        if(!path || !image)return 0;
        size_t n=strnlen(path,SC64_OBSERVER_IMAGE_PATH);
        if(n==SC64_OBSERVER_IMAGE_PATH)return 0;
        record.header=(uint64_t)(uintptr_t)image;
        record.slide=(int64_t)_dyld_get_image_vmaddr_slide(i);
        memcpy(record.path,path,n);
        if(write(fd,&record,sizeof record)!=(ssize_t)sizeof record)return 0;
    }
    return 1;
}
#endif
