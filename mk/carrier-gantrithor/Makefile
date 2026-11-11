# SPDX-License-Identifier: GPL-3.0-or-later
# Included from the top-level Makefile after the P10 candidate rules; it is not
# a standalone makefile. Run these targets from the repository root.

# The Gantrithor as a Carrier-class parent in the P10 ROM's flag vector: full
# hangar on rescue, launch, return, rebuild, death, owner change, actor cap,
# in-process and fresh-process save continuation. It joins test-sc64-protoss-p10.
build/test-sc64-p10-gantrithor: tests/sc64_p10_gantrithor.c $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile mk/protoss-p10/Makefile mk/carrier-gantrithor/Makefile | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P10_FLAGS) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-p10-gantrithor
test-sc64-p10-gantrithor: build/test-sc64-p10-gantrithor
	mkdir -p $(P10_HOST_DIR)/gantrithor
	./build/test-sc64-p10-gantrithor $(P10_HOST_DIR)/gantrithor
test-sc64-protoss-p10: test-sc64-p10-gantrithor
