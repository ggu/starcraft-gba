# SPDX-License-Identifier: GPL-3.0-or-later
# Player command latency on the real engine, in the Terran product. Needs the
# top-level Makefile's flag and input variables, so it is included last.
build/test-sc64-command-delay-replace: tests/sc64_command_delay_replace_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/command-delay/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-command-delay-replace
test-sc64-command-delay-replace: build/test-sc64-command-delay-replace
	./build/test-sc64-command-delay-replace
test-sc64-terran-full-campaign: test-sc64-command-delay-replace
