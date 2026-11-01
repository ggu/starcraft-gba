# Opt-in range-worker qualification, using current Z7 canonical inputs.
.PHONY: test-sc64-range-work-flags test-sc64-range-work-engine test-sc64-range-work-engine-default test-sc64-range-work-save-resume test-sc64-range-copy-alignment
test-sc64-range-work-flags:
	python3 tests/test_sc64_range_work_flags.py
build/test-sc64-range-work-engine: tests/sc64_direct_firing_points_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z7_CANONICAL_INPUTS) common/gba.h common/gba.c Makefile tests/sc64-range-work/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z7_CANONICAL_FLAGS) -DSC64_RANGE_WORK -DSC64_RANGE_WORK_BUDGET_PER_TICK=8 $< common/gba.c -o $@
test-sc64-range-work-engine: build/test-sc64-range-work-engine
	./build/test-sc64-range-work-engine
build/test-sc64-range-work-engine-default: tests/sc64_direct_firing_points_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z7_CANONICAL_INPUTS) common/gba.h common/gba.c Makefile tests/sc64-range-work/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z7_CANONICAL_FLAGS) -DSC64_RANGE_WORK $< common/gba.c -o $@
test-sc64-range-work-engine-default: build/test-sc64-range-work-engine-default
	./build/test-sc64-range-work-engine-default
build/test-sc64-range-work-save-resume: tests/sc64_range_work_save_resume.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z7_CANONICAL_INPUTS) common/gba.h common/gba.c Makefile tests/sc64-range-work/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z7_CANONICAL_FLAGS) -DSC64_RANGE_WORK -DSC64_RANGE_WORK_BUDGET_PER_TICK=8 $< common/gba.c -o $@
test-sc64-range-work-save-resume: build/test-sc64-range-work-save-resume
	./build/test-sc64-range-work-save-resume
RANGE_COPY_TEST_FLAGS = -DSC64_RANGE_WORK -DSC64_DIRECT_FIRING_POINTS -DSC64_IMPORTED_MISSION -DSC64_RACES -DSC64_ROUTE_PACK3
build/test-sc64-range-copy-alignment: tests/sc64_range_copy_alignment.c $(wildcard starcraft/*.h) Makefile tests/sc64-range-work/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -fno-omit-frame-pointer $(RANGE_COPY_TEST_FLAGS) -I. $< -o $@
build/test-sc64-range-copy-alignment-stride: tests/sc64_range_copy_alignment.c $(wildcard starcraft/*.h) Makefile tests/sc64-range-work/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -fno-omit-frame-pointer $(RANGE_COPY_TEST_FLAGS) -DSC64_ROUTE_ACTIVE_STRIDE -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_LARGE_MAPS -DSC64_ROUTE_VISIT_GRID -DSC64_ROUTE_WORD_CLEAR -DSC64_ROUTE_L_FAST_PATH -DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_DECAY_FREQ -I. $< -o $@
test-sc64-range-copy-alignment: build/test-sc64-range-copy-alignment build/test-sc64-range-copy-alignment-stride
	./build/test-sc64-range-copy-alignment
	./build/test-sc64-range-copy-alignment-stride
