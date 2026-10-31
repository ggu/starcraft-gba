# Asset-free source-clock mapping check in the default suite.
build/test-sc64-z8-clock: tests/sc64_z8_clock.c starcraft/sc64_z8_clock.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-z8-clock
test-sc64-z8-clock: build/test-sc64-z8-clock
	./build/test-sc64-z8-clock
test: test-sc64-z8-clock

# Prepared Z8 inputs: ordinary-menu continuation through both save codecs.
build/test-sc64-z8-clock-game: tests/sc64_z8_clock_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z8_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z8_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-z8-clock-derived-game: tests/sc64_z8_clock_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z8_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z8_CANONICAL_FLAGS) -DSC64_Z7_DERIVED_SAVE $< common/gba.c -o $@
.PHONY: test-sc64-z8-clock-game
test-sc64-z8-clock-game: build/test-sc64-z8-clock-game build/test-sc64-z8-clock-derived-game
	./build/test-sc64-z8-clock-game
	./build/test-sc64-z8-clock-derived-game
