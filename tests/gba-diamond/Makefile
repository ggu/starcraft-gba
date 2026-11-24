# SPDX-License-Identifier: GPL-3.0-or-later
GBA_DIAMOND_FLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon

build/test-gba-diamond: tests/gba_diamond_spans.c common/gba.c common/gba.h tests/gba-diamond/Makefile | build
	$(CLANG) $(GBA_DIAMOND_FLAGS) $< -o $@

build/test-gba-diamond-clip: tests/gba_diamond_spans.c common/gba.c common/gba.h tests/gba-diamond/Makefile | build
	$(CLANG) $(GBA_DIAMOND_FLAGS) -DGBA_SCENE_CLIP $< -o $@

.PHONY: test-gba-diamond
test-gba-diamond: build/test-gba-diamond build/test-gba-diamond-clip
	./build/test-gba-diamond
	./build/test-gba-diamond-clip

test: test-gba-diamond
