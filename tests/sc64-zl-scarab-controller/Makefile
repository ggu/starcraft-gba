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
