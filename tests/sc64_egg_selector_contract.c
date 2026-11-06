/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_engine_capabilities.h"
#if defined(EXPECT_OFF)
#if defined(SC64_SHARED_EGG_CUSTOM_SERVICE) || defined(SC64_SHARED_EGG_LATE_SERVICE)
#error "default selector must not enable Egg service"
#endif
#elif defined(EXPECT_CUSTOM)
#if !defined(SC64_SHARED_EGG_CUSTOM_SERVICE) || defined(SC64_SHARED_EGG_LATE_SERVICE)
#error "selector must choose only the custom Egg service"
#endif
#elif defined(EXPECT_LATE)
#if !defined(SC64_SHARED_EGG_LATE_SERVICE) || defined(SC64_SHARED_EGG_CUSTOM_SERVICE)
#error "selector must choose only the late Egg service"
#endif
#else
#error "specify the expected selector mode"
#endif
int main(void) { return 0; }
