# SPDX-License-Identifier: GPL-3.0-or-later
# Optional staged HOST target; requires prepared P10 data, no normal ROM enable.
SC64_RESEARCH_COMBAT_COVER_DIR ?= .local/hardware-feedback/verified-cover
.PHONY: test-sc64-research-combat-routes
test-sc64-research-combat-routes:
	@mkdir -p build/research-combat
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COMBAT_COVER_DIR) $(PROTOSS_P10_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS tests/sc64_research_combat_routes.c common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o build/research-combat/routes-p10
	./build/research-combat/routes-p10
