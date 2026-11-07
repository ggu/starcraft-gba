# Optional asset-free oracle for Arbiter finder-square bounds.
.PHONY: test-sc64-aura-square
build/test-sc64-aura-square: tests/sc64_aura_square_probe.c starcraft/sc64_aura_square_probe.h tests/sc64-aura-square/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/sc64_aura_square_probe.c -o $@
test-sc64-aura-square: build/test-sc64-aura-square
	./build/test-sc64-aura-square
