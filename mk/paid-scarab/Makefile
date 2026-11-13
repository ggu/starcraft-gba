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

