/* Real current product bindings: baseline enum and codec shape only. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#if defined(SC64_ZP_FULL_STORAGE)
#if defined(SC64_P7_TYPED_COMPACT_ENABLED)
#define ROW Sc64P7TypedUnitSave
#define CODEC "p7-typed"
#elif defined(SC64_Z7_DERIVED_SAVE)
#define ROW Sc64DerivedUnitSave
#define CODEC "z7-derived"
#else
#define ROW Sc64ZpPackedUnitSave
#define CODEC "zp-packed"
#endif
#else
#define ROW Sc64PackedUnitSave
#define CODEC "compact"
#endif
int main(void) {
 assert(SC64_SHARED_EGG_SAVE_VERSION==(SAVE_VERSION ^ 0x40000000u));
#ifdef SC64_SHARED_TRANSPORT
 assert(SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION==(SC64_TRANSPORT_SAVE_VERSION ^ 0x40000000u));
#endif
 printf("legacy=%d count=%d egg=%d overlord=%d codec=%s row=%zu",SC_RU_LEGACY_COUNT,SC_RU_COUNT,SC64_SHARED_EGG_TYPE,U_OVERLORD,CODEC,sizeof(ROW));
#ifdef SC64_Z7_EGG_PRODUCTION
 printf(" legacy_egg=%d",U_Z7_EGG);
#endif
 puts("");return 0;
}
