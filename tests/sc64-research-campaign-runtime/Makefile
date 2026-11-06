# SPDX-License-Identifier: GPL-3.0-or-later
# Explicit HOST-only campaign stage; no active-job journal flag is permitted.
.PHONY: test-sc64-research-campaign-runtime-p10
test-sc64-research-campaign-runtime-p10:
	@mkdir -p build/research-campaign-runtime
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) -I$(SC64_PR_POLICY_DIR) $(PROTOSS_P10_FLAGS) -DSC64_SOURCE_SIGHT_ALL_RACES -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS tests/sc64_research_campaign_runtime_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-campaign-runtime/p10
	./build/research-campaign-runtime/p10

# Explicit HOST experiment. Exports and reloads real 32 KiB journal bytes.
.PHONY: test-sc64-research-campaign-journal-p10
test-sc64-research-campaign-journal-p10:
	@mkdir -p build/research-campaign-runtime
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) -I$(SC64_PR_POLICY_DIR) $(PROTOSS_P10_FLAGS) -DSC64_SOURCE_SIGHT_ALL_RACES -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL tests/sc64_research_campaign_journal_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-campaign-runtime/journal-p10
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) -DSC64_SOURCE_SIGHT_ALL_RACES tests/sc64_research_campaign_journal_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-campaign-runtime/journal-p10-old
	./build/research-campaign-runtime/journal-p10-old old-write build/research-campaign-runtime/old-p10.sav
	./build/research-campaign-runtime/journal-p10 old-load build/research-campaign-runtime/old-p10.sav
	./build/research-campaign-runtime/journal-p10 over-old build/research-campaign-runtime/old-p10.sav
	./build/research-campaign-runtime/journal-p10 fallback build/research-campaign-runtime/old-p10.sav
	./build/research-campaign-runtime/journal-p10 write build/research-campaign-runtime/journal-p10.sav
	./build/research-campaign-runtime/journal-p10 load build/research-campaign-runtime/journal-p10.sav
