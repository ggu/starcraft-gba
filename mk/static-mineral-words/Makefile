# SPDX-License-Identifier: GPL-3.0-or-later

# Resource bank for the four normal mode-4 cartridges. Keep the setting
# independently configurable for a byte-renderer control build.
SC64_STATIC_MINERAL_WORDS ?= 1
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
# Bind the generated bank to its actual input, even when a restored file is
# older than the output or was replaced within one filesystem clock tick.
SC64_STATIC_MINERAL_SOURCE_HASH := $(if $(filter 1,$(SC64_STATIC_MINERAL_WORDS)),$(shell test ! -f '$(SC64_STATIC_MINERAL_WORDS_SOURCE)' || $(PYTHON) -c 'import hashlib,sys; print(hashlib.sha256(open(sys.argv[1], "rb").read()).hexdigest())' '$(SC64_STATIC_MINERAL_WORDS_SOURCE)'))
SC64_STATIC_MINERAL_SIGNATURE := $(SC64_STATIC_MINERAL_WORDS)|$(if $(filter 1,$(SC64_STATIC_MINERAL_WORDS)),$(SC64_STATIC_MINERAL_WORDS_SOURCE)|$(SC64_STATIC_MINERAL_SOURCE_HASH))
SC64_STATIC_MINERAL_REBUILD := $(if $(shell test -f build/static-mineral-words-config && printf '%s\n' '$(SC64_STATIC_MINERAL_SIGNATURE)' | cmp -s - build/static-mineral-words-config && printf matched),,force-static-mineral-words-rebuild)
.PHONY: force-static-mineral-words-config force-static-mineral-words-rebuild sc64-static-mineral-words test-sc64-static-mineral-words test-sc64-static-mineral-words-prepared
force-static-mineral-words-config:
force-static-mineral-words-rebuild:
build/static-mineral-words-config: force-static-mineral-words-config | build
	@printf '%s\n' '$(SC64_STATIC_MINERAL_SIGNATURE)' > $@.tmp
	@if cmp -s $@.tmp $@; then rm $@.tmp; else mv $@.tmp $@; fi
$(SC64_STATIC_MINERAL_WORDS_HEADER): $(SC64_STATIC_MINERAL_WORDS_SOURCE) tools/sc64_static_mineral_words.py tools/sc64_unit_facing.py build/static-mineral-words-config $(SC64_STATIC_MINERAL_REBUILD) | build
	mkdir -p $(dir $@)
	$(PYTHON) tools/sc64_static_mineral_words.py $< > $@.tmp
	mv $@.tmp $@
sc64-static-mineral-words: $(SC64_STATIC_MINERAL_WORDS_HEADER)
$(SC64_STATIC_MINERAL_WORDS_ELFS): build/static-mineral-words-config $(SC64_STATIC_MINERAL_REBUILD)
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
