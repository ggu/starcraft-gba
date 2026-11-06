# SPDX-License-Identifier: GPL-3.0-or-later
.PHONY: test-sc64-research-q8-live
Q8_EXTRA_FLAGS ?=
test-sc64-research-q8-live:
	@mkdir -p build/research-q8
	$(CLANG) $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(PROTOSS_P10_FLAGS) $(Q8_EXTRA_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT tests/sc64_research_q8_live_game.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o build/research-q8/live-p10
	./build/research-q8/live-p10
