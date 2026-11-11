# Explicit opt-in, actual product vectors; private input paths are supplied by
# the operator. Not a default test or release flag.
SC64_RESEARCH_COVER_DIR ?= .local/hardware-feedback/verified-cover
.PHONY: test-sc64-research-p10 test-sc64-research-z9
test-sc64-research-p10:
	@mkdir -p build/research-wire
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_BASE_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL tests/sc64_research_journal_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-wire/host-p10
	./build/research-wire/host-p10
test-sc64-research-z9:
	@mkdir -p build/research-wire
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_RESEARCH_TEST_Z9 tests/sc64_research_journal_game.c -o build/research-wire/host-z9
	./build/research-wire/host-z9
# Compile the same fixture in separate source trees. Supply COMPAT_FLAGS as
# -DSC64_RESEARCH_COMPAT_PRODUCER for the baseline and as
# -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL for the consumer.
.PHONY: research-compat-p10 research-compat-z9
research-compat-p10:
	@mkdir -p build/compat
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_BASE_FLAGS) $(COMPAT_FLAGS) tests/sc64_research_compat_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/compat/p10
research-compat-z9:
	@mkdir -p build/compat
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(ZERG_Z9_CANONICAL_FLAGS) $(COMPAT_FLAGS) -DSC64_RESEARCH_TEST_Z9 tests/sc64_research_compat_game.c -o build/compat/z9
.PHONY: test-sc64-save-tag-namespace
test-sc64-save-tag-namespace:
	@mkdir -p build/research-wire
	$(CLANG) -std=c11 -O2 -Wall -Wextra -Werror tests/sc64_save_tag_namespace.c -o build/research-wire/save-tag-namespace
	./build/research-wire/save-tag-namespace
.PHONY: test-sc64-research-layout-compile
test-sc64-research-layout-compile:
	@mkdir -p build/research-wire
	$(CLANG) -std=c11 -Wall -Wextra -Werror -fsyntax-only tests/sc64_research_layout_compile.c
	@if $(CLANG) -std=c11 -Wall -Wextra -Werror -fsyntax-only -DSC64_TEST_UNSUPPORTED_LAYOUT tests/sc64_research_layout_compile.c >build/research-wire/layout-negative.log 2>&1; then cat build/research-wire/layout-negative.log; exit 1; fi
	@grep -q 'unsupported research journal building layout' build/research-wire/layout-negative.log
