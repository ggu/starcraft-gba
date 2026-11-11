# SPDX-License-Identifier: GPL-3.0-or-later
# Include after the top-level Makefile. All optional inputs must already exist.
# make -f Makefile -f tests/sc64-campaign-controller/Makefile test-campaign-controller
CONTROLLER_COVER_DIR ?= .local/hardware-feedback/verified-cover
CONTROLLER_OUT ?= build/sc64-campaign-controller
CONTROLLER_FLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_CAMPAIGN_CONTROLLER_HOST_STAGE -Icommon -I$(CONTROLLER_COVER_DIR)
.PHONY: test-campaign-controller test-campaign-controller-p1 test-campaign-controller-z10
test-campaign-controller: test-campaign-controller-p1 test-campaign-controller-z10
test-campaign-controller-p1:
	@mkdir -p $(CONTROLLER_OUT)
	$(CLANG) $(CONTROLLER_FLAGS) $(PROTOSS_P10_BASE_FLAGS) tests/sc64_campaign_controller_p1_game.c common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $(CONTROLLER_OUT)/p1
	$(CONTROLLER_OUT)/p1
test-campaign-controller-z10:
	@mkdir -p $(CONTROLLER_OUT)
	$(CLANG) $(CONTROLLER_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) tests/sc64_campaign_controller_z10_game.c common/gba.c -o $(CONTROLLER_OUT)/z10
	$(CONTROLLER_OUT)/z10
