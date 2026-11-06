/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../../experiments/ares-headless/input_mapping.h"
static_assert(sc64_button_pressed(64,16)); /* D-pad Right */
static_assert(!sc64_button_pressed(256,16));
static_assert(sc64_button_pressed(256,64)); /* C-Right */
static_assert(!sc64_button_pressed(64,64));
static_assert(sc64_button_pressed(512,128) && sc64_button_pressed(1024,256));
static_assert(sc64_button_pressed(32,8) && !sc64_button_pressed(32,64));
static_assert(!sc64_button_pressed(0,511));
int main() { return 0; }
