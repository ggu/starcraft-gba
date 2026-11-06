/* SPDX-License-Identifier: GPL-3.0-or-later
 * Opt-in bounded route-build classification. This state is diagnostic only. */
#if !defined(Z7_BUSY_ATTRIBUTION) || defined(GBA_HOST)
#error "route build classification is restricted to the ARM busy attribution"
#endif
#define SC64_ROUTE_CLASS_KEYS 128u
static u32 route_class_active, route_class_component_context;
static u32 route_class_movement, route_class_component, route_class_depot;
static u32 route_class_empty, route_class_evicted, route_class_resets, route_class_wraps;
static u32 route_class_distinct, route_class_repeated, route_class_untracked;
static u16 route_class_keys[SC64_ROUTE_CLASS_KEYS];

__attribute__((noinline,section(".text"))) static void route_class_begin(void)
{
    route_class_movement=route_class_component=route_class_depot=0;
    route_class_empty=route_class_evicted=route_class_resets=route_class_wraps=0;
    route_class_distinct=route_class_repeated=route_class_untracked=0;
    route_class_component_context=0;route_class_active=1;
}
__attribute__((noinline,section(".text"))) static void route_class_note(int slot,u16 key,int depot)
{
    if(!route_class_active)return;
    if(depot)++route_class_depot;
    else if(route_class_component_context)++route_class_component;
    else ++route_class_movement;
    if(sc64_route_valid[slot])++route_class_evicted;else ++route_class_empty;
    unsigned i=0;while(i<route_class_distinct && route_class_keys[i]!=key)++i;
    if(i<route_class_distinct)++route_class_repeated;
    else if(route_class_distinct<SC64_ROUTE_CLASS_KEYS)route_class_keys[route_class_distinct++]=key;
    else ++route_class_untracked;
}
__attribute__((noinline,section(".text"))) static void route_class_reset_note(void)
{
    if(route_class_active)++route_class_resets;
}
__attribute__((noinline,section(".text"))) static void route_class_wrap_note(void)
{
    if(route_class_active)++route_class_wraps;
}
