#include "../../experiments/ares-headless/visual_capture.h"
#include <cassert>
int main(){assert(sc64_visual_capture_allowed(0));assert(sc64_visual_capture_allowed(1));assert(!sc64_visual_capture_allowed(2));assert(!sc64_visual_capture_allowed(-1));assert(!sc64_visual_capture_allowed(3));assert(sc64_visual_mask_allowed(1023));assert(!sc64_visual_mask_allowed(1024));}
