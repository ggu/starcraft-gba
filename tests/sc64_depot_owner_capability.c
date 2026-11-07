/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_depot_owner_capability.h"
#if defined(EXPECT_OWNER_LIST_ON) && !defined(SC64_DEPOT_OWNER_LIST_PROBE)
#error "nearest-depot builds must enable owner lists by default"
#endif
#if defined(EXPECT_OWNER_LIST_OFF) && defined(SC64_DEPOT_OWNER_LIST_PROBE)
#error "the explicit scalar override must disable the owner-list probe"
#endif
int main(void) { return 0; }
