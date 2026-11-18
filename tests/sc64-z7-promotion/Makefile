# Shared production fixtures follow the current default Zerg product.
CANONICAL_ZERG_PROFILE_FLAGS = $(ZERG_Z7_CANONICAL_FLAGS)
CANONICAL_ZERG_PROFILE_INPUTS = $(ZERG_Z7_CANONICAL_INPUTS)
build/test-sc64-zerg-canonical-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(CANONICAL_ZERG_PROFILE_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(CANONICAL_ZERG_PROFILE_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@

$(eval $(call Z7_FULL_RENDER_FIXTURE,census-owners,sc64_z7_prune_game.c,-DSC64_Z7_PRUNE_STARTED -DSC64_Z7_CENSUS_OWNER_TABLE,tests/sc64-z7-promotion/Makefile))
Z7_CENSUS_PROBE_FLAGS = -DSC64_Z7_CENSUS_OWNER_TABLE -DSC64_Z7_VALIDATED_CENSUS -DSC64_Z7_FUSED_PROVENANCE_PROBE
$(eval $(call Z7_FULL_RENDER_FIXTURE,census-composed,sc64_z7_prune_game.c,-DSC64_Z7_PRUNE_STARTED $(Z7_CENSUS_PROBE_FLAGS),tests/sc64-z7-promotion/Makefile))
.PHONY: test-sc64-z7-census-owners test-sc64-z7-census-composed
test-sc64-z7-census-owners: build/test-sc64-z7-full-render-prune build/test-sc64-z7-full-render-census-owners
	./build/test-sc64-z7-full-render-prune
	./build/test-sc64-z7-full-render-census-owners
test-sc64-z7-census-composed: test-sc64-z7-census-owners build/test-sc64-z7-full-render-census-composed
	./build/test-sc64-z7-full-render-census-composed
