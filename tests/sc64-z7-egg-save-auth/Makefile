# Normal production vectors and real HOST save journals; no staged features.
EGG_AUTH_HOST_FLAGS ?= -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon
build/test-sc64-z7-nonmission-egg: tests/sc64_z7_nonmission_egg_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z7_CANONICAL_INPUTS) | build
	$(CLANG) $(EGG_AUTH_HOST_FLAGS) $(ZERG_Z7_CANONICAL_FLAGS) $< tests/sc64_yamato_host_sram_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
build/test-sc64-z9-nonmission-egg: tests/sc64_z7_nonmission_egg_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) | build
	$(CLANG) $(EGG_AUTH_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) $< tests/sc64_yamato_host_sram_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
.PHONY: test-sc64-nonmission-egg
test-sc64-nonmission-egg: build/test-sc64-z7-nonmission-egg build/test-sc64-z9-nonmission-egg
	./build/test-sc64-z7-nonmission-egg legacy
	./build/test-sc64-z7-nonmission-egg reject
	./build/test-sc64-z7-nonmission-egg fallback
	./build/test-sc64-z9-nonmission-egg legacy
	./build/test-sc64-z9-nonmission-egg reject
	./build/test-sc64-z9-nonmission-egg fallback
	./build/test-sc64-z9-nonmission-egg z9
