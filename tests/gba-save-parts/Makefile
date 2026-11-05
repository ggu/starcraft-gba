.PHONY: test-gba-save-parts-selected
build/test-gba-save-parts-selected: tests/gba_save_parts_selected.c common/gba.c common/gba.h Makefile tests/gba-save-parts/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_SHARED_HP_REGEN -Icommon $< -o $@
build/test-gba-save-selection-hp: tests/gba_save_selection.c common/gba.c common/gba.h Makefile tests/gba-save-parts/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -Wno-misleading-indentation -fsanitize=address,undefined -DGBA_HOST -DSC64_SHARED_HP_REGEN -Icommon $< -o $@
build/test-gba-save-parts-selected-scarab: tests/gba_save_parts_selected.c common/gba.c common/gba.h Makefile tests/gba-save-parts/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_SHARED_HP_REGEN -DSC64_SCARAB_RUNTIME -Icommon $< -o $@
build/test-gba-save-selection-hp-scarab: tests/gba_save_selection.c common/gba.c common/gba.h Makefile tests/gba-save-parts/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -Wno-misleading-indentation -fsanitize=address,undefined -DGBA_HOST -DSC64_SHARED_HP_REGEN -DSC64_SCARAB_RUNTIME -Icommon $< -o $@
test-gba-save-parts-selected: build/test-gba-save-parts-selected build/test-gba-save-selection-hp build/test-gba-save-parts-selected-scarab build/test-gba-save-selection-hp-scarab
	./build/test-gba-save-parts-selected
	./build/test-gba-save-selection-hp
	./build/test-gba-save-parts-selected-scarab
	./build/test-gba-save-selection-hp-scarab
test: test-gba-save-parts-selected

define GBA_PREPARED_SAVE_TEST
build/test-gba-save-prepared-$(1): tests/gba_save_prepared.c tests/gba_save_selection.c common/gba.c common/gba.h Makefile tests/gba-save-parts/Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -Wno-misleading-indentation -fsanitize=address,undefined -DGBA_HOST $(2) -Icommon $$< -o $$@
endef
$(eval $(call GBA_PREPARED_SAVE_TEST,scarab,-DSC64_SCARAB_RUNTIME))
$(eval $(call GBA_PREPARED_SAVE_TEST,worker,-DSC64_WORKER_CARGO_KIND))
$(eval $(call GBA_PREPARED_SAVE_TEST,hp-worker,-DSC64_SHARED_HP_REGEN -DSC64_WORKER_CARGO_KIND))
$(eval $(call GBA_PREPARED_SAVE_TEST,hp-worker-scarab,-DSC64_SHARED_HP_REGEN -DSC64_WORKER_CARGO_KIND -DSC64_SCARAB_RUNTIME))
.PHONY: test-gba-save-prepared
test-gba-save-prepared: build/test-gba-save-prepared-scarab build/test-gba-save-prepared-worker build/test-gba-save-prepared-hp-worker build/test-gba-save-prepared-hp-worker-scarab
	./build/test-gba-save-prepared-scarab
	./build/test-gba-save-prepared-worker
	./build/test-gba-save-prepared-hp-worker
	./build/test-gba-save-prepared-hp-worker-scarab
test: test-gba-save-prepared

