# SPDX-License-Identifier: GPL-3.0-or-later
# Included from the top-level Makefile after the P10 candidate rules; it is not
# a standalone makefile. Run these targets from the repository root.

# The rest of the carried Protoss missions under the canonical flags. P1, P2, P3
# and P5 run from test-sc64-protoss-carried-missions in the top-level Makefile.
build/test-sc64-protoss-carried-p4: tests/sc64_p4_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-carried/Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-protoss-carried-p6: tests/sc64_p6_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-carried/Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-protoss-carried-p4-p6
test-sc64-protoss-carried-p4-p6: build/test-sc64-protoss-carried-p4 build/test-sc64-protoss-carried-p6
	./build/test-sc64-protoss-carried-p4 --attempt-tag canonical-carried-p4
	./build/test-sc64-protoss-carried-p6 --trace .local/sc64/carried-p6-controller.json
test-sc64-protoss-carried-missions: test-sc64-protoss-carried-p4-p6
