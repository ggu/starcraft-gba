# SPDX-License-Identifier: GPL-3.0-or-later
CONTROLLER_FLASH_COVER_DIR ?= .local/hardware-feedback/verified-cover
CONTROLLER_FLASH_POLICY_DIR ?= .local/research-campaign-runtime/policy
CONTROLLER_FLASH_OUT ?= build/controller-flash-p1
# Explicit genuine prior 0x47 producer output; caller supplies its verified path.
CONTROLLER_FLASH_LEGACY ?= .local/controller-flash-p1/legacy-47.sav
.PHONY: test-controller-flash-p1
test-controller-flash-p1:
	@mkdir -p $(CONTROLLER_FLASH_OUT)
	$(CLANG) $(P8_HOST_FLAGS) -I$(CONTROLLER_FLASH_COVER_DIR) -I$(CONTROLLER_FLASH_POLICY_DIR) $(PROTOSS_P10_BASE_FLAGS) \
	  -DSC64_CAMPAIGN_CONTROLLER_HOST_STAGE -DSC64_CONTROLLER_FLASH_P1_HOST_STAGE \
	  -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS \
	  -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL \
	  -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT \
	  -DSC64_Q8_FLASH_HOST_STAGE tests/sc64_controller_flash_p1_game.c $(P7_PROJECTILE_SOURCES) \
	  $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c common/gba_flash1m.c -o $(CONTROLLER_FLASH_OUT)/test
	$(CONTROLLER_FLASH_OUT)/test write $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/test read $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/test fallback $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/test only-bad $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/test unknown-new $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/test legacy-foreign-hash $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/test legacy-foreign-length $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/test legacy-foreign-empty $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/test legacy-known-malformed $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/test legacy $(CONTROLLER_FLASH_LEGACY) $(CONTROLLER_FLASH_OUT)/p1-migrated.sav
	$(CONTROLLER_FLASH_OUT)/test legacy-read $(CONTROLLER_FLASH_OUT)/p1-migrated.sav
	$(CONTROLLER_FLASH_OUT)/test fallback-legacy $(CONTROLLER_FLASH_OUT)/p1-migrated.sav

# Optional additional genuine 0x43/0x44 producer files, if already prepared.
.PHONY: test-controller-flash-p1-predecessors
test-controller-flash-p1-predecessors:
	$(CONTROLLER_FLASH_OUT)/test legacy-empty $(CONTROLLER_FLASH_43) $(CONTROLLER_FLASH_OUT)/p1-from-43.sav
	$(CONTROLLER_FLASH_OUT)/test legacy-empty-read $(CONTROLLER_FLASH_OUT)/p1-from-43.sav
	$(CONTROLLER_FLASH_OUT)/test legacy-empty $(CONTROLLER_FLASH_44) $(CONTROLLER_FLASH_OUT)/p1-from-44.sav
	$(CONTROLLER_FLASH_OUT)/test legacy-empty-read $(CONTROLLER_FLASH_OUT)/p1-from-44.sav

.PHONY: test-controller-flash-p1-faults
test-controller-flash-p1-faults:
	$(CLANG) $(P8_HOST_FLAGS) -I$(CONTROLLER_FLASH_COVER_DIR) -I$(CONTROLLER_FLASH_POLICY_DIR) $(PROTOSS_P10_BASE_FLAGS) \
	  -DSC64_CAMPAIGN_CONTROLLER_HOST_STAGE -DSC64_CONTROLLER_FLASH_P1_HOST_STAGE \
	  -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS \
	  -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL \
	  -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT \
	  -DSC64_Q8_FLASH_HOST_STAGE tests/sc64_controller_flash_p1_faults.c $(P7_PROJECTILE_SOURCES) \
	  $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c common/gba_flash1m.c -o $(CONTROLLER_FLASH_OUT)/faults
	$(CONTROLLER_FLASH_OUT)/faults stale-selected $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/faults stale-epoch $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/faults save-read $(CONTROLLER_FLASH_OUT)/p1.sav
	$(CONTROLLER_FLASH_OUT)/faults load-read $(CONTROLLER_FLASH_OUT)/p1.sav
