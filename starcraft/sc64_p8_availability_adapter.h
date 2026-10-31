#ifndef SC64_P8_AVAILABILITY_ADAPTER_H
#define SC64_P8_AVAILABILITY_ADAPTER_H
#include "sc64_unit_availability.h"
#include "sc64_p8_availability_generated.h"
/* Human production belongs to source slot 7, including rescued actors.
 * Other owners retain their producer/worker source slot. */
static int race_p8_source_product_available(int owner,int raw_owner,int raw)
{
    unsigned available=0;
    int slot=owner==SIDE ? 7 : raw_owner;
    if (slot<0 || slot>=8 || raw<0 || raw>=228 ||
        (owner!=SIDE && race_p8_source_owner((unsigned)slot)!=owner)) return 0;
    return sc64_availability_bitmap_get(sc64_p8_availability_bits,
        SC64_P8_AVAILABILITY_BIT_BYTES,(unsigned)slot,(unsigned)raw,&available) && available;
}
static int race_p8_unit_product_available(int owner,int raw_owner,int type)
{ return !race_p8_active() || race_p8_source_product_available(owner,raw_owner,sc64_race_raw_unit(type)); }
static int race_p8_building_product_available(int owner,int raw_owner,int type)
{ return !race_p8_active() || race_p8_source_product_available(owner,raw_owner,sc64_race_raw_building(type)); }
static int race_p8_live_products_valid(void)
{
    if (!race_p8_active()) return 1;
    for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && SC64_PENDING_BUILD_ORDER(u[i].order) &&
       !race_p8_source_product_available(u[i].owner,u[i].raw_owner,sc64_race_raw_building(u[i].target))) return 0;
    for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive) {
        if(b[i].queue_count<0 || b[i].queue_count>5 || b[i].queue_head<0 || b[i].queue_head>=5) return 0;
        for(int j=0;j<b[i].queue_count;j++) {
            int type=b[i].queue[(b[i].queue_head+j)%5];
            if(type<0 || type>=SC_RU_COUNT || !race_p8_source_product_available(b[i].owner,b[i].raw_owner,sc64_race_raw_unit(type))) return 0;
        }
    }
    return 1;
}
#endif
