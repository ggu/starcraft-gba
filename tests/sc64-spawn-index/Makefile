# Optional engine fixtures require the explicitly prepared private profile inputs.
# They are deliberately not dependencies of the clean default `test` target.
SPAWN_INDEX_HOST_FLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon
build/test-sc64-spawn-index-legacy-flat: tests/sc64_spawn_collision_index.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile tests/sc64-spawn-index/Makefile | build
	$(CLANG) $(SPAWN_INDEX_HOST_FLAGS) $(RACES_FLAGS) -DSC64_SHARED_TRANSPORT $< common/gba.c -o $@
build/test-sc64-spawn-index-legacy-memo: tests/sc64_spawn_collision_index.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile tests/sc64-spawn-index/Makefile | build
	$(CLANG) $(SPAWN_INDEX_HOST_FLAGS) $(RACES_FLAGS) -DSC64_SHARED_TRANSPORT -DSC64_CONTACT_CACHE -DSC64_CONTACT_MEMO_ORACLE $< common/gba.c -o $@
build/test-sc64-spawn-index-terran-flat: tests/sc64_spawn_collision_index.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(CANONICAL_DEATH_INPUTS) $(SOURCE_WEAPON_INPUTS) common/gba.h common/gba.c Makefile tests/sc64-spawn-index/Makefile | build
	$(CLANG) $(SPAWN_INDEX_HOST_FLAGS) $(filter-out -DSC64_CONTACT_CACHE -DSC64_CONTACT_MEMO_ORACLE,$(TERRAN_ROM_FLAGS)) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-spawn-index-terran-memo: tests/sc64_spawn_collision_index.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(CANONICAL_DEATH_INPUTS) $(SOURCE_WEAPON_INPUTS) common/gba.h common/gba.c Makefile tests/sc64-spawn-index/Makefile | build
	$(CLANG) $(SPAWN_INDEX_HOST_FLAGS) $(TERRAN_ROM_FLAGS) -DSC64_CONTACT_MEMO_ORACLE $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-spawn-index
test-sc64-spawn-index: build/test-sc64-spawn-index-legacy-flat build/test-sc64-spawn-index-legacy-memo build/test-sc64-spawn-index-terran-flat build/test-sc64-spawn-index-terran-memo
	./build/test-sc64-spawn-index-legacy-flat
	./build/test-sc64-spawn-index-legacy-memo
	./build/test-sc64-spawn-index-terran-flat
	./build/test-sc64-spawn-index-terran-memo

# Asset-free persistent index lifecycle and strict placement stress. The
# prepared real-engine contact and Nydus fixtures remain separate checks.
build/test-sc64-persistent-lifecycle: tests/sc64_contact_memo.c starcraft/sc64_persistent_collision.h starcraft/sc64_contact.h common/gba.h tests/sc64-spawn-index/Makefile | build
	$(CLANG) $(SPAWN_INDEX_HOST_FLAGS) -DSC64_PERSISTENT_COLLISION -DSC64_NYDUS_ENABLED -DSC64_CONTACT_LOCAL_MEMO_PROBE -DSC64_CONTACT_DEFER_STEERING_MEMO $< -o $@
.PHONY: test-sc64-persistent-lifecycle
test-sc64-persistent-lifecycle: build/test-sc64-persistent-lifecycle
	./build/test-sc64-persistent-lifecycle
test: test-sc64-persistent-lifecycle
