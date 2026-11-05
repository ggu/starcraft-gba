# Real campaign product picker, with the ordinary production feature vectors.
SCOURGE_PICKER_HOST_FLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon
build/test-sc64-campaign-scourge-z9: tests/sc64_campaign_scourge_picker.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(SCOURGE_PICKER_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-campaign-scourge-z10: tests/sc64_campaign_scourge_picker.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(SCOURGE_PICKER_HOST_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-campaign-scourge
test-sc64-campaign-scourge: build/test-sc64-campaign-scourge-z9 build/test-sc64-campaign-scourge-z10
	./build/test-sc64-campaign-scourge-z9
	./build/test-sc64-campaign-scourge-z10
