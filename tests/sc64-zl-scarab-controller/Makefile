# SPDX-License-Identifier: GPL-3.0-or-later
# Explicit HOST-only stage. Invoke after the top-level Makefile; prepared
# optional inputs must already exist. No normal ROM flags are changed.
ZL_SCARAB_COVER_DIR ?= .local/hardware-feedback/verified-cover
ZL_SCARAB_OUT ?= build/zl-scarab-controller
# The experimental Scarab namespace has no worker-cargo codec. This isolated
# fixture refuses all saves; do not apply this exclusion to product vectors.
ZL_SCARAB_HOST_FLAGS = $(filter-out -DSC64_WORKER_CARGO_KIND,$(ZERG_Z10_CANONICAL_FLAGS))
.PHONY: test-zl-scarab-controller
test-zl-scarab-controller:
	@mkdir -p $(ZL_SCARAB_OUT)
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -I$(ZL_SCARAB_COVER_DIR) \
	  -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_NAMESPACE_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE \
	  -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE \
	  -DSC64_SCARAB_HUMAN_CONTROL_PROBE -DSC64_SCARAB_AI_RESTOCK_HOST_STAGE \
	  -DSC64_CAMPAIGN_CONTROLLER_HOST_STAGE -DSC64_SCARAB_ZL_CONTROLLER_HOST_STAGE \
	  -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT $(ZL_SCARAB_HOST_FLAGS) tests/sc64_zl_scarab_controller_game.c common/gba.c \
	  $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(ZL_SCARAB_OUT)/test
	$(ZL_SCARAB_OUT)/test

.PHONY: test-zl-scarab-reservation
test-zl-scarab-reservation:
	@mkdir -p $(ZL_SCARAB_OUT)
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -I$(ZL_SCARAB_COVER_DIR) \
	  -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_NAMESPACE_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE \
	  -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE \
	  -DSC64_SCARAB_HUMAN_CONTROL_PROBE -DSC64_SCARAB_AI_RESTOCK_HOST_STAGE \
	  -DSC64_CAMPAIGN_CONTROLLER_HOST_STAGE -DSC64_SCARAB_ZL_CONTROLLER_HOST_STAGE \
	  -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT $(ZL_SCARAB_HOST_FLAGS) \
	  tests/sc64_zl_scarab_reservation_game.c common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) \
	  $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(ZL_SCARAB_OUT)/reservation
	$(ZL_SCARAB_OUT)/reservation

# Full normal Z10 cargo flags with the still-save-fenced detached context.
.PHONY: test-zl-scarab-cargo-context
test-zl-scarab-cargo-context:
	@mkdir -p $(ZL_SCARAB_OUT)
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -I$(ZL_SCARAB_COVER_DIR) \
	  -DSC64_SCARAB_RUNTIME -DSC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE \
	  -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE \
	  -DSC64_SCARAB_HUMAN_CONTROL_PROBE -DSC64_SCARAB_AI_RESTOCK_HOST_STAGE \
	  -DSC64_CAMPAIGN_CONTROLLER_HOST_STAGE -DSC64_SCARAB_ZL_CONTROLLER_HOST_STAGE \
	  -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT $(ZERG_Z10_CANONICAL_FLAGS) \
	  tests/sc64_zl_scarab_controller_game.c common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) \
	  $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(ZL_SCARAB_OUT)/cargo-context
	$(ZL_SCARAB_OUT)/cargo-context

.PHONY: test-zl-scarab-cargo-q8-context
test-zl-scarab-cargo-q8-context:
	@mkdir -p $(ZL_SCARAB_OUT)
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -I$(ZL_SCARAB_COVER_DIR) -I$(CONTROLLER_FLASH_POLICY_DIR) \
	  -DSC64_SCARAB_RUNTIME -DSC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE \
	  -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE \
	  -DSC64_SCARAB_HUMAN_CONTROL_PROBE -DSC64_SCARAB_AI_RESTOCK_HOST_STAGE \
	  -DSC64_CAMPAIGN_CONTROLLER_HOST_STAGE -DSC64_SCARAB_ZL_CONTROLLER_HOST_STAGE \
	  -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT \
	  -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS \
	  -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL \
	  -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT \
	  -DSC64_Q8_FLASH_HOST_STAGE $(ZERG_Z10_CANONICAL_FLAGS) tests/sc64_zl_scarab_controller_game.c \
	  common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(ZL_SCARAB_OUT)/cargo-q8-context
	$(ZL_SCARAB_OUT)/cargo-q8-context

.PHONY: test-zl-scarab-compound-flash
# Authored HOST model, fresh process reader; prepared assets only.
test-zl-scarab-compound-flash:
	@mkdir -p $(ZL_SCARAB_OUT)
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -I$(ZL_SCARAB_COVER_DIR) -I$(CONTROLLER_FLASH_POLICY_DIR) \
	  -DSC64_SCARAB_RUNTIME -DSC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE -DSC64_COMPOUND_FLASH_Z10_HOST_STAGE -DSC64_CF_TEST_DIAGNOSTICS -DSC64_SCARAB_FLIGHT_CONTROL_PROBE \
	  -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE \
	  -DSC64_SCARAB_HUMAN_CONTROL_PROBE -DSC64_SCARAB_AI_RESTOCK_HOST_STAGE \
	  -DSC64_CAMPAIGN_CONTROLLER_HOST_STAGE -DSC64_SCARAB_ZL_CONTROLLER_HOST_STAGE \
	  -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT \
	  -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS \
	  -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL \
	  -DSC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT \
	  -DSC64_Q8_FLASH_HOST_STAGE $(ZERG_Z10_CANONICAL_FLAGS) tests/sc64_compound_flash_z10_game.c \
	  $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) common/gba_flash1m.c -o $(ZL_SCARAB_OUT)/compound
	$(ZL_SCARAB_OUT)/compound primitive-cuts $(ZL_SCARAB_OUT)/unused
	$(ZL_SCARAB_OUT)/compound write-training $(ZL_SCARAB_OUT)/z10-training.flash
	$(ZL_SCARAB_OUT)/compound read-training $(ZL_SCARAB_OUT)/z10-training.flash
	$(ZL_SCARAB_OUT)/compound write-inside $(ZL_SCARAB_OUT)/z10-inside.flash
	$(ZL_SCARAB_OUT)/compound read-inside $(ZL_SCARAB_OUT)/z10-inside.flash
	$(ZL_SCARAB_OUT)/compound write-outside $(ZL_SCARAB_OUT)/z10-outside.flash
	$(ZL_SCARAB_OUT)/compound read-outside $(ZL_SCARAB_OUT)/z10-outside.flash
	$(ZL_SCARAB_OUT)/compound write-moving $(ZL_SCARAB_OUT)/z10-moving.flash
	$(ZL_SCARAB_OUT)/compound read-moving $(ZL_SCARAB_OUT)/z10-moving.flash
	$(ZL_SCARAB_OUT)/compound foreign-v1 $(ZL_SCARAB_OUT)/z10-training.flash
	$(ZL_SCARAB_OUT)/compound foreign-v2 $(ZL_SCARAB_OUT)/z10-training.flash
	$(ZL_SCARAB_OUT)/compound foreign-header $(ZL_SCARAB_OUT)/z10-training.flash
	$(ZL_SCARAB_OUT)/compound only-bad $(ZL_SCARAB_OUT)/z10-training.flash
	$(ZL_SCARAB_OUT)/compound only-bad-flight $(ZL_SCARAB_OUT)/z10-training.flash
	$(ZL_SCARAB_OUT)/compound only-bad-marker $(ZL_SCARAB_OUT)/z10-training.flash
	$(ZL_SCARAB_OUT)/compound write-two $(ZL_SCARAB_OUT)/z10-two.flash
	$(ZL_SCARAB_OUT)/compound fallback-bad $(ZL_SCARAB_OUT)/z10-two.flash
	@for phase in marker commit-erase data-erase meta header base tail tag verify; do \
	  $(ZL_SCARAB_OUT)/compound game-cut-$$phase $(ZL_SCARAB_OUT)/z10-two.flash && \
	  $(ZL_SCARAB_OUT)/compound game-retry $(ZL_SCARAB_OUT)/z10-two.flash.$$phase.cut && \
	  $(ZL_SCARAB_OUT)/compound game-check $(ZL_SCARAB_OUT)/z10-two.flash.$$phase.cut.recovered || exit $$?; \
	done
	$(ZL_SCARAB_OUT)/compound stale-shot $(ZL_SCARAB_OUT)/unused
	$(ZL_SCARAB_OUT)/compound stale-marker $(ZL_SCARAB_OUT)/unused
	$(ZL_SCARAB_OUT)/compound cut-write $(ZL_SCARAB_OUT)/z10-cut.flash
	$(ZL_SCARAB_OUT)/compound cut-read $(ZL_SCARAB_OUT)/z10-cut.flash
	$(ZL_SCARAB_OUT)/compound verify-recovered $(ZL_SCARAB_OUT)/z10-cut.flash.recovered

# An actual prior v1 HOST image must be supplied separately; never retag it.
.PHONY: test-zl-scarab-compound-v1-fence
test-zl-scarab-compound-v1-fence:
	$(ZL_SCARAB_OUT)/compound retained-v1 $(COMPOUND_V1_IMAGE)

.PHONY: test-zl-scarab-compound-v2-fences
test-zl-scarab-compound-v2-fences:
	$(ZL_SCARAB_OUT)/compound retained-v2 $(COMPOUND_V2_IMAGE)
	$(ZL_SCARAB_OUT)/compound retained-v2-cut $(COMPOUND_V2_CUT_IMAGE)
