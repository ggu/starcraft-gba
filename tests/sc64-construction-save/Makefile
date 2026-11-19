# SPDX-License-Identifier: GPL-3.0-or-later
SC64_CONSTRUCTION_SAVE_TESTS := build/test-sc64-construction-save \
	$(addprefix build/test-sc64-construction-save,-late -battlecruiser -nuke)
build/test-sc64-construction-save-late: SC64_CONSTRUCTION_SAVE_FLAGS = -DSC64_TERRAN_LATE_ACTORS
build/test-sc64-construction-save: SC64_CONSTRUCTION_SAVE_FLAGS = -DSC64_SKIRMISH_ROSTER
build/test-sc64-construction-save-battlecruiser: SC64_CONSTRUCTION_SAVE_FLAGS = -DSC64_BATTLECRUISER_EXPERIMENT
build/test-sc64-construction-save-nuke: SC64_CONSTRUCTION_SAVE_FLAGS = -DSC64_SHARED_NUKE_PROBE
$(SC64_CONSTRUCTION_SAVE_TESTS): tests/sc64_construction_save.c starcraft/sc64_construction_save.h \
	starcraft/race_rules.h starcraft/sc64_skirmish_menu.h starcraft/sc64_skirmish_catalog.h \
	Makefile tests/sc64-construction-save/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $(SC64_CONSTRUCTION_SAVE_FLAGS) $< -o $@

.PHONY: test-sc64-construction-save
test-sc64-construction-save: $(SC64_CONSTRUCTION_SAVE_TESTS)
	@set -e; for test in $^; do ./$$test; done
