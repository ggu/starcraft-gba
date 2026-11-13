# Showcase HOST suite. Included by the top-level Makefile after SHOWCASE_FLAGS.
SHOWCASE_FLAGS += -DSC64_TYPED_UNIT_CHUNK_READER
# The indexed build drops packed word sprites, so it also drops the walk-word
# poses that depend on them (sc64_unit_facing.h rejects one without the other).
build/test-sc64-showcase-front: tests/sc64_frontend.c starcraft/sc64_frontend.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_SHOWCASE_ENABLED -Icommon $< -o $@
build/test-sc64-showcase-game: tests/sc64_showcase_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(TANK_SOURCE_HEADER) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-showcase-game-packed: tests/sc64_showcase_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(TANK_SOURCE_HEADER) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) -DSC64_UNIT_WORD_SPRITES -DGBA_SCENE_CLIP $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-showcase-game-indexed: tests/sc64_showcase_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(TANK_SOURCE_HEADER) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(filter-out -DSC64_UNIT_WORD_SPRITES $(CANONICAL_WALK_WORDS_FLAGS),$(SHOWCASE_FLAGS)) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
test-sc64-showcase-front: build/test-sc64-showcase-front
	./build/test-sc64-showcase-front
test-starcraft: test-sc64-showcase-front
test-sc64-showcase: test-sc64-showcase-front build/test-sc64-showcase-game build/test-sc64-showcase-game-packed build/test-sc64-showcase-game-indexed
	./build/test-sc64-showcase-game
	./build/test-sc64-showcase-game-packed
	./build/test-sc64-showcase-game-indexed
