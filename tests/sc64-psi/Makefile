# Shared Pylon power admission and production, using the prepared race vector.
build/test-sc64-psi-boundary: tests/sc64_psi_boundary_game.c tests/sc64_skirmish_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) tests/sc64_psi_boundary_game.c common/gba.c -o $@
.PHONY: test-sc64-psi-boundary
test-sc64-psi-boundary: build/test-sc64-psi-boundary
	./build/test-sc64-psi-boundary
test-races: test-sc64-psi-boundary
