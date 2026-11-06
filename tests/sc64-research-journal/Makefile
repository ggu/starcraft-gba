# Explicit opt-in, actual product vectors; private input paths are supplied by
# the operator. Not a default test or release flag.
SC64_RESEARCH_COVER_DIR ?= .local/hardware-feedback/verified-cover
.PHONY: test-sc64-research-p10 test-sc64-research-z9
test-sc64-research-p10:
	@mkdir -p build/research-wire
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL tests/sc64_research_journal_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-wire/host-p10
	./build/research-wire/host-p10
test-sc64-research-z9:
	@mkdir -p build/research-wire
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_RESEARCH_TEST_Z9 tests/sc64_research_journal_game.c -o build/research-wire/host-z9
	./build/research-wire/host-z9
