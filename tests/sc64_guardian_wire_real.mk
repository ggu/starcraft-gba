include Makefile
.PHONY: guardian-real
GUARD_HOST = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -DSC64_GUARDIAN_COCOON_STAGE
GUARD_EXTRA = common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
guardian-real:
	$(CLANG) $(GUARD_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-terran
	/tmp/guardian-real-terran
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-zerg
	/tmp/guardian-real-zerg
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-protoss
	/tmp/guardian-real-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-showcase
	/tmp/guardian-real-showcase
.PHONY: guardian-real-remaining
guardian-real-remaining:
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-protoss
	/tmp/guardian-real-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-showcase
	/tmp/guardian-real-showcase
.PHONY: guardian-real-zps
guardian-real-zps:
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-zerg
	/tmp/guardian-real-zerg
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-protoss
	/tmp/guardian-real-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-showcase
	/tmp/guardian-real-showcase
.PHONY: guardian-fact-generator
guardian-fact-generator:
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT tools/sc64_fact_tables_gen.c -o /tmp/guardian-fact-gen-off
	/tmp/guardian-fact-gen-off > /tmp/guardian-facts-off.h
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT -DSC64_GUARDIAN_COCOON_STAGE tools/sc64_fact_tables_gen.c -o /tmp/guardian-fact-gen-on
	/tmp/guardian-fact-gen-on > /tmp/guardian-facts-on.h
	cmp /tmp/guardian-facts-off.h /tmp/guardian-facts-on.h
.PHONY: guardian-journal-real
GUARD_JOURNAL_EXTRA = tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
guardian-journal-real:
	$(CLANG) $(GUARD_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-journal-terran
	/tmp/guardian-journal-terran
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-journal-zerg
	/tmp/guardian-journal-zerg
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-journal-protoss
	/tmp/guardian-journal-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-journal-showcase
	/tmp/guardian-journal-showcase
