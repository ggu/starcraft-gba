# SPDX-License-Identifier: GPL-3.0-or-later

# Compare the opt-in byte reader against the same fixture built with the bit reader.
build/sc64-p7-typed-codec-reference.o: tests/sc64_p7_typed_codec.c starcraft/sc64_p7_typed_unit_save.h starcraft/sc64_p7_typed_building_save.h starcraft/sc64_zp_unit_save.h starcraft/sc64_compact_building_save.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_TYPED_UNIT_READER_REFERENCE -c $< -o $@
build/test-sc64-p7-typed-chunk-reader: tests/sc64_p7_typed_codec.c build/sc64-p7-typed-codec-reference.o Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_TYPED_UNIT_CHUNK_READER $< build/sc64-p7-typed-codec-reference.o -o $@
.PHONY: test-sc64-p7-typed-chunk-reader
test-sc64-p7-typed-chunk-reader: build/test-sc64-p7-typed-chunk-reader
	./build/test-sc64-p7-typed-chunk-reader
test-starcraft: test-sc64-p7-typed-chunk-reader

# Optional asset-free GBA microbenchmark; both ROMs use synthetic typed rows.
define SC64_TYPED_ARM_BENCH
build/sc64-typed-unit-bench-$(1).elf: tests/sc64_p7_typed_codec.c starcraft/sc64_p7_typed_unit_save.h starcraft/sc64_p7_typed_building_save.h starcraft/sc64_zp_unit_save.h starcraft/sc64_compact_building_save.h common/start.s common/gba.c common/memory.c $(COMMON_HEADERS) common/gba.ld Makefile | build
	$$(CC) $$(CFLAGS) -DSC64_TYPED_UNIT_ARM_BENCH $(2) common/start.s common/gba.c common/memory.c $$< $$(LDFLAGS) -lgcc -o $$@
build/sc64-typed-unit-bench-$(1).gba: build/sc64-typed-unit-bench-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC TYPED' SCTY
endef
$(eval $(call SC64_TYPED_ARM_BENCH,reference,))
$(eval $(call SC64_TYPED_ARM_BENCH,chunk,-DSC64_TYPED_UNIT_CHUNK_READER))
.PHONY: sc64-typed-unit-arm-bench
sc64-typed-unit-arm-bench: build/sc64-typed-unit-bench-reference.gba build/sc64-typed-unit-bench-chunk.gba
