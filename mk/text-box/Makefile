# SPDX-License-Identifier: GPL-3.0-or-later
# The N64 campaign's in-mission text boxes (docs/TEXT-BOXES.md). The canonical
# race products enable them; the private header comes from the verified ROM.
TEXT_BOX_HEADER := .local/sc64/text-box/generated/sc64_text_box_generated.h
TEXT_BOX_FLAGS := -DSC64_TEXT_BOXES -I.local/sc64/text-box/generated

.PHONY: sc64-text-boxes test-sc64-text-box
sc64-text-boxes $(TEXT_BOX_HEADER): tools/sc64_text_box_assets.py .local/sc64/backwater/generated/palette-remap.bin
	$(PYTHON) tools/sc64_text_box_assets.py

build/test-sc64-text-box: tests/sc64_text_box.c starcraft/sc64_text_box.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -Icommon -Istarcraft $< -o $@
test-sc64-text-box: build/test-sc64-text-box
	./build/test-sc64-text-box
test-starcraft: test-sc64-text-box
