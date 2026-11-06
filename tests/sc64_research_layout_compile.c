/* SPDX-License-Identifier: GPL-3.0-or-later
 * The negative variant intentionally fails to compile. */
#include "../starcraft/sc64_protoss_research_padding.h"
_Static_assert(SC64_PR_PAD_TUPLE_VALID(96,28,1),"compact 96");
_Static_assert(SC64_PR_PAD_TUPLE_VALID(160,28,1),"compact 160");
_Static_assert(SC64_PR_PAD_TUPLE_VALID(160,25,3),"typed 160");
_Static_assert(!SC64_PR_PAD_TUPLE_VALID(80,28,1),"compact 80 must reject");
_Static_assert(!SC64_PR_PAD_TUPLE_VALID(64,25,3),"typed 64 must reject");
_Static_assert(!SC64_PR_PAD_TUPLE_VALID(160,25,1),"bad typed padding must reject");
#ifdef SC64_TEST_UNSUPPORTED_LAYOUT
_Static_assert(SC64_PR_PAD_TUPLE_VALID(80,28,1),"unsupported research journal building layout");
#endif
int main(void) { return 0; }
