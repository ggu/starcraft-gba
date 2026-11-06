# SPDX-License-Identifier: GPL-3.0-or-later
build/test-sc64-research-human-binding: tests/sc64_research_human_binding.c starcraft/sc64_research_human_binding.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-research-human-binding
test-sc64-research-human-binding: build/test-sc64-research-human-binding
	./build/test-sc64-research-human-binding
test: test-sc64-research-human-binding

# Optional: requires the already prepared normal P10 production inputs.
build/test-sc64-research-human-binding-game: tests/sc64_research_human_binding_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P10_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-research-human-binding-game
test-sc64-research-human-binding-game: build/test-sc64-research-human-binding-game
	./build/test-sc64-research-human-binding-game
