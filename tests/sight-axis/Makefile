# SPDX-License-Identifier: GPL-3.0-or-later
SIGHT_AXIS_TEST_FLAGS ?= -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined
build/test-sc64-sight-axis-integer: tests/sc64_sight_axis.c starcraft/sc64_sight_axis.h tests/sight-axis/Makefile | build
	$(CLANG) $(SIGHT_AXIS_TEST_FLAGS) $< -o $@
.PHONY: test-sc64-sight-axis-integer
test-sc64-sight-axis-integer: build/test-sc64-sight-axis-integer
	./build/test-sc64-sight-axis-integer
test: test-sc64-sight-axis-integer
