# SPDX-License-Identifier: GPL-3.0-or-later
# Included from the top-level Makefile after the P10 rules; not a standalone
# makefile. The paid Scarab service (starcraft/sc64_paid_scarab.h): Reavers
# hold Scarabs bought one at a time, and an attack launches one. It is part of
# the Protoss P10 product vector only; every other product leaves it off and
# builds byte-identically.
PROTOSS_SCARAB_FLAGS := -DSC64_PAID_SCARAB
P10_SCARAB_FACT_DIR := build/generated/p10-scarab-fact
P10_SCARAB_FACT_HEADER := $(P10_SCARAB_FACT_DIR)/sc64_fact_tables_generated.h
$(P10_SCARAB_FACT_HEADER): $(P8_AVAILABILITY_HEADER) tools/sc64_fact_tables_gen.c $(wildcard starcraft/*.h) $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile mk/protoss-p10/Makefile mk/paid-scarab/Makefile | build
	mkdir -p $(P10_SCARAB_FACT_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(PROTOSS_SCARAB_FLAGS) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT tools/sc64_fact_tables_gen.c -o build/sc64-p10-scarab-fact-gen
	./build/sc64-p10-scarab-fact-gen > $@.tmp
	mv $@.tmp $@

# HOST checks for the product (sanitized). Needs the prepared P10 inputs.
# The pre-service save is written by a checkout of the unchanged default
# branch: make test-sc64-paid-scarab PSC_BASE_TREE=<path to that checkout>.
PSC_DIR := build/paid-scarab
PSC_SRC := tests/sc64_paid_scarab_product.c
PSC_DEPS := $(PSC_SRC) $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) mk/paid-scarab/Makefile
$(PSC_DIR)/product: $(PSC_DEPS) | build
	@mkdir -p $(PSC_DIR)
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) $(PSC_SRC) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-paid-scarab test-sc64-paid-scarab-old
test-sc64-paid-scarab: $(PSC_DIR)/product
	for t in economy combat splash loss validate save-process; do ./$(PSC_DIR)/product $$t || exit 1; done
	rm -f $(PSC_DIR)/scarab.sav $(PSC_DIR)/scarab.sav.state
	./$(PSC_DIR)/product save-write $(PSC_DIR)/scarab.sav
	./$(PSC_DIR)/product save-read $(PSC_DIR)/scarab.sav
	$(MAKE) test-sc64-paid-scarab-old
test-sc64-paid-scarab-old: $(PSC_DIR)/product
	@if [ -z "$(PSC_BASE_TREE)" ]; then echo "test-sc64-paid-scarab-old: set PSC_BASE_TREE to an unchanged default-branch checkout; skipped"; exit 0; fi
	cp $(PSC_SRC) $(PSC_BASE_TREE)/tests/
	cd $(PSC_BASE_TREE) && $(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_BASE_FLAGS) $(PROTOSS_RESEARCH_FLAGS) $(PSC_SRC) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o ../psc-old-writer
	rm -f $(PSC_DIR)/old.sav $(PSC_DIR)/old.sav.state
	$(PSC_BASE_TREE)/../psc-old-writer old-write $(CURDIR)/$(PSC_DIR)/old.sav
	./$(PSC_DIR)/product old-read $(PSC_DIR)/old.sav
