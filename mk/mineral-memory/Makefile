# Remembered mineral presentation, legacy saves, and compact race records.
.PHONY: test-sc64-mineral-memory
MINERAL_MEMORY_HOST_FLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -Icommon
MINERAL_MEMORY_GAME_DEPS = tests/sc64_mineral_memory_game.c starcraft/game.c $(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile mk/mineral-memory/Makefile
build/test-sc64-mineral-memory-game: $(MINERAL_MEMORY_GAME_DEPS) | build
	$(CLANG) $(MINERAL_MEMORY_HOST_FLAGS) -Wno-unused-function -Wno-unused-variable $< common/gba.c -o $@
build/test-sc64-mineral-memory-fifo: $(MINERAL_MEMORY_GAME_DEPS) | build
	$(CLANG) $(MINERAL_MEMORY_HOST_FLAGS) -Wno-unused-function -Wno-unused-variable -DSC64_MINERAL_FIFO $< common/gba.c -o $@
build/test-sc64-mineral-memory-wire: tests/sc64_mineral_memory_wire.c starcraft/sc64_mineral_memory_wire.h common/gba.h Makefile mk/mineral-memory/Makefile | build
	$(CLANG) $(MINERAL_MEMORY_HOST_FLAGS) $< -o $@
build/test-sc64-mineral-memory-wire-small: tests/sc64_mineral_memory_wire.c starcraft/sc64_mineral_memory_wire.h common/gba.h Makefile mk/mineral-memory/Makefile | build
	$(CLANG) $(MINERAL_MEMORY_HOST_FLAGS) -DMAX_MINERALS=48 $< -o $@
build/test-sc64-mineral-memory-wire-32: tests/sc64_mineral_memory_wire.c starcraft/sc64_mineral_memory_wire.h common/gba.h Makefile mk/mineral-memory/Makefile | build
	$(CLANG) $(MINERAL_MEMORY_HOST_FLAGS) -DMAX_MINERALS=32 $< -o $@
build/test-sc64-mineral-memory-wire-80: tests/sc64_mineral_memory_wire.c starcraft/sc64_mineral_memory_wire.h common/gba.h Makefile mk/mineral-memory/Makefile | build
	$(CLANG) $(MINERAL_MEMORY_HOST_FLAGS) -DMAX_MINERALS=80 $< -o $@
test-sc64-mineral-memory: build/test-sc64-mineral-memory-game build/test-sc64-mineral-memory-fifo build/test-sc64-mineral-memory-wire build/test-sc64-mineral-memory-wire-small build/test-sc64-mineral-memory-wire-32 build/test-sc64-mineral-memory-wire-80
	./build/test-sc64-mineral-memory-game
	./build/test-sc64-mineral-memory-fifo
	./build/test-sc64-mineral-memory-wire
	./build/test-sc64-mineral-memory-wire-small
	./build/test-sc64-mineral-memory-wire-32
	./build/test-sc64-mineral-memory-wire-80
test: test-sc64-mineral-memory
