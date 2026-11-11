# HOST-only staged job integration; private P10 inputs supplied by operator.
SC64_RESEARCH_COVER_DIR ?= .local/hardware-feedback/verified-cover
.PHONY: test-sc64-research-jobs-p10
test-sc64-research-jobs-p10:
	@mkdir -p build/research-jobs
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_BASE_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS tests/sc64_research_jobs_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-jobs/host-p10
	./build/research-jobs/host-p10

.PHONY: test-sc64-research-jobs-z9
test-sc64-research-jobs-z9:
	@mkdir -p build/research-jobs
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS tests/sc64_research_jobs_game.c -o build/research-jobs/host-z9
	./build/research-jobs/host-z9

# Host-only custom active-job journal: separate producer and consumer processes.
.PHONY: build-sc64-research-job-journal-p10
build-sc64-research-job-journal-p10:
	@mkdir -p build/research-jobs
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_BASE_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL tests/sc64_research_jobs_journal_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-jobs/journal-p10
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_BASE_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL tests/sc64_research_jobs_journal_game.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-jobs/old-p10

# Optional Z9 active-job journal producer/consumer; input banks are operator-supplied.
.PHONY: build-sc64-research-job-journal-z9
build-sc64-research-job-journal-z9:
	@mkdir -p build/research-jobs
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL tests/sc64_research_jobs_journal_game.c -o build/research-jobs/journal-z9
