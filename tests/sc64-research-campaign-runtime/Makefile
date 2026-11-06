# SPDX-License-Identifier: GPL-3.0-or-later
# Explicit HOST-only campaign stage; no active-job journal flag is permitted.
.PHONY: test-sc64-research-campaign-runtime-p10
test-sc64-research-campaign-runtime-p10:
	@mkdir -p build/research-campaign-runtime
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) -I$(SC64_PR_POLICY_DIR) $(PROTOSS_P10_FLAGS) -DSC64_SOURCE_SIGHT_ALL_RACES -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS tests/sc64_research_campaign_runtime_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-campaign-runtime/p10
	./build/research-campaign-runtime/p10

# Explicit HOST experiment. Exports and reloads real 32 KiB journal bytes.
SC64_CAMPAIGN_BASELINE_ROOT ?= .
.PHONY: test-sc64-research-campaign-journal-p10
test-sc64-research-campaign-journal-p10:
	@mkdir -p build/research-campaign-runtime
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) -I$(SC64_PR_POLICY_DIR) $(PROTOSS_P10_FLAGS) -DSC64_SOURCE_SIGHT_ALL_RACES -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL tests/sc64_research_campaign_journal_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-campaign-runtime/journal-p10
	$(CLANG) -I$(SC64_CAMPAIGN_BASELINE_ROOT)/common -I$(SC64_CAMPAIGN_BASELINE_ROOT)/starcraft $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) -DSC64_SOURCE_SIGHT_ALL_RACES -DSC64_CAMPAIGN_BASELINE_WRITER tests/sc64_research_campaign_journal_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-campaign-runtime/journal-p10-old
	./build/research-campaign-runtime/journal-p10-old old-write build/research-campaign-runtime/old-p10.sav
	./build/research-campaign-runtime/journal-p10 old-load build/research-campaign-runtime/old-p10.sav
	./build/research-campaign-runtime/journal-p10 over-old build/research-campaign-runtime/old-p10.sav
	./build/research-campaign-runtime/journal-p10 fallback build/research-campaign-runtime/old-p10.sav
	$(CLANG) -I$(SC64_CAMPAIGN_BASELINE_ROOT)/common -I$(SC64_CAMPAIGN_BASELINE_ROOT)/starcraft $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) -DSC64_SOURCE_SIGHT_ALL_RACES -DSC64_CAMPAIGN_BASELINE_WRITER -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL tests/sc64_research_campaign_journal_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-campaign-runtime/journal-p10-44
	./build/research-campaign-runtime/journal-p10-44 old-write build/research-campaign-runtime/old-44-p10.sav
	./build/research-campaign-runtime/journal-p10 old-load build/research-campaign-runtime/old-44-p10.sav
	./build/research-campaign-runtime/journal-p10 over-old build/research-campaign-runtime/old-44-p10.sav
	./build/research-campaign-runtime/journal-p10 fallback build/research-campaign-runtime/old-44-p10.sav
	./build/research-campaign-runtime/journal-p10 write build/research-campaign-runtime/journal-p10.sav
	./build/research-campaign-runtime/journal-p10 reject build/research-campaign-runtime/journal-p10.sav
	./build/research-campaign-runtime/journal-p10 load build/research-campaign-runtime/journal-p10.sav
	./build/research-campaign-runtime/journal-p10 custom-write build/research-campaign-runtime/custom-p10.sav
	./build/research-campaign-runtime/journal-p10 custom-load build/research-campaign-runtime/custom-p10.sav
	./build/research-campaign-runtime/journal-p10 core-write build/research-campaign-runtime/core-p10.sav
	./build/research-campaign-runtime/journal-p10 core-load build/research-campaign-runtime/core-p10.sav
	$(CLANG) -std=c11 -O1 -Wall -Wextra -Werror tests/sc64_save_tag_namespace.c -o build/research-campaign-runtime/tag-inventory
	./build/research-campaign-runtime/tag-inventory

.PHONY: test-sc64-research-campaign-journal-z9
test-sc64-research-campaign-journal-z9:
	@mkdir -p build/research-campaign-runtime
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) -I$(SC64_PR_POLICY_DIR) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL tests/sc64_research_campaign_journal_game.c -o build/research-campaign-runtime/journal-z9
	$(CLANG) -I$(SC64_CAMPAIGN_BASELINE_ROOT)/common -I$(SC64_CAMPAIGN_BASELINE_ROOT)/starcraft $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_CAMPAIGN_BASELINE_WRITER tests/sc64_research_campaign_journal_game.c -o build/research-campaign-runtime/journal-z9-old
	./build/research-campaign-runtime/journal-z9-old z-old-write build/research-campaign-runtime/old-z9.sav
	./build/research-campaign-runtime/journal-z9 z-load build/research-campaign-runtime/old-z9.sav
	./build/research-campaign-runtime/journal-z9 z-over-old build/research-campaign-runtime/old-z9.sav
	./build/research-campaign-runtime/journal-z9 z-load build/research-campaign-runtime/old-z9.sav
	./build/research-campaign-runtime/journal-z9 z-write build/research-campaign-runtime/new-z9.sav
	./build/research-campaign-runtime/journal-z9 z-load build/research-campaign-runtime/new-z9.sav

.PHONY: test-sc64-research-campaign-journal-terran
test-sc64-research-campaign-journal-terran:
	@mkdir -p build/research-campaign-runtime
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) -I$(SC64_PR_POLICY_DIR) $(TERRAN_ROM_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL tests/sc64_research_campaign_journal_game.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o build/research-campaign-runtime/journal-terran
	$(CLANG) -I$(SC64_CAMPAIGN_BASELINE_ROOT)/common -I$(SC64_CAMPAIGN_BASELINE_ROOT)/starcraft $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(TERRAN_ROM_FLAGS) -DSC64_CAMPAIGN_BASELINE_WRITER tests/sc64_research_campaign_journal_game.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o build/research-campaign-runtime/journal-terran-old
	./build/research-campaign-runtime/journal-terran-old t-old-write build/research-campaign-runtime/old-terran.sav
	./build/research-campaign-runtime/journal-terran t-load build/research-campaign-runtime/old-terran.sav
	./build/research-campaign-runtime/journal-terran t-over-old build/research-campaign-runtime/old-terran.sav
	./build/research-campaign-runtime/journal-terran t-load build/research-campaign-runtime/old-terran.sav
	./build/research-campaign-runtime/journal-terran t-write build/research-campaign-runtime/new-terran.sav
	./build/research-campaign-runtime/journal-terran t-load build/research-campaign-runtime/new-terran.sav
