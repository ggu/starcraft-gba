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

