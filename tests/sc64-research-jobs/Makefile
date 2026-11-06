# HOST-only staged job integration; private P10 inputs supplied by operator.
SC64_RESEARCH_COVER_DIR ?= .local/hardware-feedback/verified-cover
.PHONY: test-sc64-research-jobs-p10
test-sc64-research-jobs-p10:
	@mkdir -p build/research-jobs
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS tests/sc64_research_jobs_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-jobs/host-p10
	./build/research-jobs/host-p10

.PHONY: test-sc64-research-jobs-z9
test-sc64-research-jobs-z9:
	@mkdir -p build/research-jobs
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS tests/sc64_research_jobs_game.c -o build/research-jobs/host-z9
	./build/research-jobs/host-z9
