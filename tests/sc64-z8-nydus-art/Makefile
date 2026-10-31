# Prepared-input check: actual Z8 source sprites and renderer binding.
build/test-sc64-z8-nydus-art: tests/sc64_z8_nydus_art.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z8_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z8_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z8-nydus-art
test-sc64-z8-nydus-art: build/test-sc64-z8-nydus-art
	./build/test-sc64-z8-nydus-art
test-zerg-z8: test-sc64-z8-nydus-art
