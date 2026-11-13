# SPDX-License-Identifier: GPL-3.0-or-later

# Opt-in resource bank for the four normal mode-4 cartridges. Store the
# setting independently so flipping it does not re-emit other asset banks.
SC64_STATIC_MINERAL_WORDS ?= 0
ifneq ($(words $(SC64_STATIC_MINERAL_WORDS)),1)
$(error SC64_STATIC_MINERAL_WORDS must be 0 or 1)
endif
ifeq ($(filter 0 1,$(SC64_STATIC_MINERAL_WORDS)),)
$(error SC64_STATIC_MINERAL_WORDS must be 0 or 1)
endif
SC64_STATIC_MINERAL_WORDS_SOURCE ?= .local/sc64/backwater/generated/sc64_sprites_generated.h
SC64_STATIC_MINERAL_WORDS_HEADER := build/generated/sc64_static_mineral_words_generated.h
SC64_STATIC_MINERAL_WORDS_ELFS := build/starcraft-terran.elf build/starcraft-zerg-z10.elf \
	build/starcraft-protoss-p10.elf build/starcraft-showcase.elf
.PHONY: force-static-mineral-words-config sc64-static-mineral-words test-sc64-static-mineral-words test-sc64-static-mineral-words-prepared
force-static-mineral-words-config:
build/static-mineral-words-config: force-static-mineral-words-config | build
	@printf '%s\n' '$(SC64_STATIC_MINERAL_WORDS)' > $@.tmp
	@if cmp -s $@.tmp $@; then rm $@.tmp; else mv $@.tmp $@; fi
$(SC64_STATIC_MINERAL_WORDS_HEADER): $(SC64_STATIC_MINERAL_WORDS_SOURCE) tools/sc64_static_mineral_words.py tools/sc64_unit_facing.py | build
	mkdir -p $(dir $@)
	$(PYTHON) tools/sc64_static_mineral_words.py $< > $@.tmp
	mv $@.tmp $@
sc64-static-mineral-words: $(SC64_STATIC_MINERAL_WORDS_HEADER)
$(SC64_STATIC_MINERAL_WORDS_ELFS): build/static-mineral-words-config
ifeq ($(SC64_STATIC_MINERAL_WORDS),1)
$(SC64_STATIC_MINERAL_WORDS_ELFS): CFLAGS += -DSC64_STATIC_MINERAL_WORDS -Ibuild/generated
$(SC64_STATIC_MINERAL_WORDS_ELFS): $(SC64_STATIC_MINERAL_WORDS_HEADER)
endif
test-sc64-static-mineral-words:
	$(PYTHON) -m unittest discover -s tests -p 'test_sc64_static_mineral_words.py'
build/test-sc64-static-mineral-words-prepared: tests/sc64_static_mineral_words.c starcraft/sc64_static_mineral_draw.h common/gba.c $(COMMON_HEADERS) $(SC64_STATIC_MINERAL_WORDS_HEADER) | build
	$(CLANG) $(SPRITE_WORD_TEST_FLAGS) -DGBA_SCENE_CLIP -DGBA_SPRITE_WORDS -DSC64_STATIC_MINERAL_WORDS -Ibuild/generated -I$(dir $(SC64_STATIC_MINERAL_WORDS_SOURCE)) $< common/gba.c -o $@
test-sc64-static-mineral-words-prepared: build/test-sc64-static-mineral-words-prepared
	./build/test-sc64-static-mineral-words-prepared
test-starcraft: test-sc64-static-mineral-words
