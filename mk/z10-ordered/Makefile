# Experimental source-ordered Z10 AI. Distinct generation and ROM identity;
# the existing Z10 candidate and every default race profile remain unchanged.
Z10_ORDERED_REPORT ?= .local/build/z10-ai/report.json
Z10_ORDERED_HEADER := build/generated/z10-ordered/sc64_z10_ordered_ai_generated.h
$(Z10_ORDERED_HEADER): $(Z10_ORDERED_REPORT) tools/sc64_z10_ordered_ai_pack.py | build
	$(PYTHON) tools/sc64_z10_ordered_ai_pack.py $< $@
Z10_ORDERED_FLAGS = $(ZERG_Z10_CANONICAL_FLAGS) -DSC64_Z10_ORDERED_AI_STAGE -Ibuild/generated/z10-ordered
Z10_ORDERED_INPUTS = $(ZERG_Z10_CANONICAL_INPUTS) $(Z10_ORDERED_HEADER)
build/starcraft-z10-ordered.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z10_ORDERED_INPUTS) common/gba.ld Makefile mk/z10-ordered/Makefile | build
	$(CC) $(CFLAGS) $(Z10_ORDERED_FLAGS) -DGBA_NO_DEBUG_INPUTS $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-z10-ordered.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Z10-Ordered-GBA.gba: build/starcraft-z10-ordered.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z10 ORDER' SCZB
.PHONY: z10-ordered
z10-ordered: build/StarCraft-Z10-Ordered-GBA.gba
	$(PYTHON) tools/verify_roms.py --z10-ordered
build/test-sc64-z10-ordered-ai: tests/sc64_z10_ordered_ai_game.c tests/sc64_z10_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z10_ORDERED_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/z10-ordered/Makefile | build
	$(CLANG) $(Z8_HOST_FLAGS) $(Z10_ORDERED_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z10-ordered-ai
test-sc64-z10-ordered-ai: build/test-sc64-z10-ordered-ai
	./build/test-sc64-z10-ordered-ai
build/test-sc64-z10-ordered-controller: tests/sc64_z10_campaign_run.c tests/sc64_zerg_late_controller.h $(STARCRAFT) $(wildcard starcraft/*.h) $(Z10_ORDERED_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/z10-ordered/Makefile | build
	$(CLANG) $(Z8_HOST_FLAGS) $(Z10_ORDERED_FLAGS) $< -o $@
.PHONY: test-sc64-z10-ordered-controller
test-sc64-z10-ordered-controller: build/test-sc64-z10-ordered-controller
	mkdir -p build/z10-ordered
	./build/test-sc64-z10-ordered-controller build/z10-ordered/controller.jsonl
