# SPDX-License-Identifier: GPL-3.0-or-later
# Optional row-codec witness. Does not admit any combined journal.
SCARAB_CARGO_COVER_DIR ?= .local/hardware-feedback/verified-cover
SCARAB_CARGO_OUT ?= build/scarab-cargo-row
SCARAB_CARGO_FLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_SCARAB_RUNTIME -Icommon -I$(SCARAB_CARGO_COVER_DIR)
.PHONY: test-scarab-cargo-row-p10 test-scarab-cargo-row-z10
test-scarab-cargo-row-p10:
	@mkdir -p $(SCARAB_CARGO_OUT)/facts
	$(CLANG) $(SCARAB_CARGO_FLAGS) -Istarcraft $(filter-out -DSC64_FACT_TABLES,$(PROTOSS_P10_BASE_FLAGS)) tools/sc64_fact_tables_gen.c -o $(SCARAB_CARGO_OUT)/facts-generator
	$(SCARAB_CARGO_OUT)/facts-generator > $(SCARAB_CARGO_OUT)/facts/sc64_fact_tables_generated.h
	$(CLANG) $(SCARAB_CARGO_FLAGS) -I$(SCARAB_CARGO_OUT)/facts $(PROTOSS_P10_BASE_FLAGS) tests/sc64_scarab_cargo_row_composition.c common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $(SCARAB_CARGO_OUT)/p10
	$(SCARAB_CARGO_OUT)/p10
test-scarab-cargo-row-z10: | $(LATE_ZERG_SIGHT_INPUTS)
	@mkdir -p $(SCARAB_CARGO_OUT)
	$(CLANG) $(SCARAB_CARGO_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) tests/sc64_scarab_cargo_row_composition.c common/gba.c -o $(SCARAB_CARGO_OUT)/z10
	$(SCARAB_CARGO_OUT)/z10

.PHONY: test-scarab-cargo-context-z10
test-scarab-cargo-context-z10: | $(LATE_ZERG_SIGHT_INPUTS)
	@mkdir -p $(SCARAB_CARGO_OUT)
	$(CLANG) $(SCARAB_CARGO_FLAGS) -DSC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE $(ZERG_Z10_CANONICAL_FLAGS) tests/sc64_scarab_cargo_row_composition.c common/gba.c -o $(SCARAB_CARGO_OUT)/context-z10
	$(SCARAB_CARGO_OUT)/context-z10
