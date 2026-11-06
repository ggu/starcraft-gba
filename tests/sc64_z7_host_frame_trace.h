/* SPDX-License-Identifier: GPL-3.0-or-later
 * HOST-only per-call fingerprints of selected game state and displayed pixels.
 * FNV-1a hashes are diagnostic comparisons, not byte-exact state proof. */
#if !defined(GBA_HOST) || !defined(Z7_CARGO_DIGEST) || Z7_ARM_MODE != 0
#error "Z7 host frame trace requires the full HOST cargo replay"
#endif
static FILE *host_frame_trace;
static uint64_t host_trace_hash(uint64_t h,const void *data,size_t n)
{
    const u8 *p=(const u8 *)data;
    while(n--)h=(h^*p++)*UINT64_C(1099511628211);
    return h;
}
#define Z7_TRACE_ADD(h, x) ((h)=host_trace_hash((h),&(x),sizeof(x)))
static void host_trace_frame(void)
{
    uint64_t actor=UINT64_C(14695981039346656037);
    uint64_t world=actor, display=actor;
    u32 fields[4]={(u32)frames_run,(u32)frame,(u32)screen,(u32)mode};
#ifdef Z7_HOST_DEBUG_SNAPSHOT
    if (frames_run == 816 || frames_run == 817) {
        char path[1024];
        const char *prefix = getenv("SC64_HOST_SNAPSHOT_PREFIX");
        FILE *snapshot;
        if (!prefix || snprintf(path,sizeof(path),"%s-%u.bin",prefix,(unsigned)frames_run) >= (int)sizeof(path) || !(snapshot=fopen(path,"wb"))) {
            fputs("cannot open HOST snapshot\n",stderr);exit(2);
        }
        if (fwrite(u,sizeof(u),1,snapshot)!=1 || fwrite(b,sizeof(b),1,snapshot)!=1 ||
            fwrite(m,sizeof(m),1,snapshot)!=1 || fwrite(geysers,sizeof(geysers),1,snapshot)!=1 ||
            fwrite(tracers,sizeof(tracers),1,snapshot)!=1 || fclose(snapshot)) {
            fputs("cannot write HOST snapshot\n",stderr);exit(2);
        }
        printf("SNAPSHOT call=%u unit_size=%zu units=%zu buildings=%zu minerals=%zu geysers=%zu tracers=%zu\n",
            (unsigned)frames_run,sizeof(Unit),sizeof(u),sizeof(b),sizeof(m),sizeof(geysers),sizeof(tracers));
    }
#endif
    Z7_TRACE_ADD(actor,u);Z7_TRACE_ADD(actor,b);Z7_TRACE_ADD(actor,m);
    Z7_TRACE_ADD(actor,geysers);Z7_TRACE_ADD(actor,tracers);
    Z7_TRACE_ADD(world,race_z7);Z7_TRACE_ADD(world,race_campaign);
    Z7_TRACE_ADD(world,visible);Z7_TRACE_ADD(world,explored);Z7_TRACE_ADD(world,seed);
    Z7_TRACE_ADD(world,minerals);Z7_TRACE_ADD(world,gas);Z7_TRACE_ADD(world,supply_used);
    Z7_TRACE_ADD(world,supply_cap);Z7_TRACE_ADD(world,won);Z7_TRACE_ADD(world,lost);
    Z7_TRACE_ADD(world,cx);Z7_TRACE_ADD(world,cy);Z7_TRACE_ADD(world,camx);Z7_TRACE_ADD(world,camy);
    Z7_TRACE_ADD(world,menu_open);Z7_TRACE_ADD(world,menu_cursor);Z7_TRACE_ADD(world,selected_building);
    display=host_trace_hash(display,(const void *)pixels,240u*160u);
    Z7_TRACE_ADD(display,host_palette);
    uint64_t hashes[3]={actor,world,display};
    if(fwrite(fields,sizeof(fields),1,host_frame_trace)!=1 ||
       fwrite(hashes,sizeof(hashes),1,host_frame_trace)!=1) {
        fputs("cannot write HOST frame trace\n",stderr);exit(2);
    }
}
#undef Z7_TRACE_ADD
