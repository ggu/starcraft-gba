.PHONY: test-field-menu-repeat
build/test-field-menu-repeat: tests/field_menu_repeat_run.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $< common/gba.c -o $@
test-field-menu-repeat: build/test-field-menu-repeat
	./build/test-field-menu-repeat
test: test-field-menu-repeat

# Optional: prepared P10 profile and its verified private inputs are required.
# SC64_MENU_P10_EXTRA_FLAGS can select a separately verified overlay.
SC64_MENU_P10_EXTRA_FLAGS ?=
build/test-field-menu-repeat-p10: tests/field_menu_card_p10.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(SC64_MENU_P10_EXTRA_FLAGS) $(PROTOSS_P10_FLAGS) $< common/gba.c $(sort $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES)) -o $@
.PHONY: test-field-menu-repeat-p10
test-field-menu-repeat-p10: build/test-field-menu-repeat-p10
	./build/test-field-menu-repeat-p10
