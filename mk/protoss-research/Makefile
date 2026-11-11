# SPDX-License-Identifier: GPL-3.0-or-later
# Included from the top-level Makefile. Forge and Cybernetics Core research in
# the normal Protoss P10 product (custom games only): paid producer jobs under
# save tag 0x45, completed levels in the building rows' padding bits, no
# growth of the save record. Campaign saves keep the pre-research tag.
#
# PROTOSS_P10_FLAGS (mk/protoss-p10/Makefile) is the base vector plus these
# flags. The staged experiment fixtures under tests/ add their own research,
# Q8 and Flash stage flags and conflict with the product vector, so they use
# PROTOSS_P10_BASE_FLAGS.
PROTOSS_RESEARCH_FLAGS := -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL -DSC64_RESEARCH_SRAM_PRODUCT

# HOST checks for the product (sanitized). Needs the prepared P10 inputs.
# The pre-research save is written by a checkout of the unchanged default
# branch: make test-sc64-protoss-research PR_BASE_TREE=<path to that checkout>.
SC64_RESEARCH_COVER_DIR ?= .local/hardware-feedback/verified-cover
PR_PRODUCT_DIR := build/protoss-research
PR_PRODUCT_SRC := tests/sc64_protoss_research_product.c
PR_PRODUCT_DEPS := $(PR_PRODUCT_SRC) $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) mk/protoss-research/Makefile
$(PR_PRODUCT_DIR)/product: $(PR_PRODUCT_DEPS) | build
	@mkdir -p $(PR_PRODUCT_DIR)
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) $(PR_PRODUCT_SRC) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-protoss-research test-sc64-protoss-research-old
test-sc64-protoss-research: $(PR_PRODUCT_DIR)/product
	./$(PR_PRODUCT_DIR)/product lifecycle
	./$(PR_PRODUCT_DIR)/product combat
	rm -f $(PR_PRODUCT_DIR)/research.sav $(PR_PRODUCT_DIR)/research.sav.state
	./$(PR_PRODUCT_DIR)/product save-write $(PR_PRODUCT_DIR)/research.sav
	./$(PR_PRODUCT_DIR)/product save-read $(PR_PRODUCT_DIR)/research.sav
	./$(PR_PRODUCT_DIR)/product save-slots $(PR_PRODUCT_DIR)/research.sav
	$(MAKE) test-sc64-protoss-research-old
test-sc64-protoss-research-old: $(PR_PRODUCT_DIR)/product
	@if [ -z "$(PR_BASE_TREE)" ]; then echo "test-sc64-protoss-research-old: set PR_BASE_TREE to an unchanged default-branch checkout; skipped"; exit 0; fi
	cp $(PR_PRODUCT_SRC) $(PR_BASE_TREE)/tests/
	cd $(PR_BASE_TREE) && $(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_BASE_FLAGS) -DSC64_PRODUCT_TEST_BASE $(PR_PRODUCT_SRC) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o ../old-writer
	rm -f $(PR_PRODUCT_DIR)/old.sav $(PR_PRODUCT_DIR)/old.sav.digest
	$(PR_BASE_TREE)/../old-writer old-write $(CURDIR)/$(PR_PRODUCT_DIR)/old.sav
	./$(PR_PRODUCT_DIR)/product old-read $(PR_PRODUCT_DIR)/old.sav
