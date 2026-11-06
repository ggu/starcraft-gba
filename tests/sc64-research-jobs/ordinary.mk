# Optional, prepared-input HOST controller; all gameplay actions are buttons.
.PHONY: build-sc64-research-ordinary-p10
build-sc64-research-ordinary-p10:
	@mkdir -p build/research-jobs
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL tests/sc64_research_ordinary_controller.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-jobs/ordinary-p10
