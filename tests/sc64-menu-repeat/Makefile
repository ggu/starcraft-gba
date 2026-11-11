.PHONY: test-field-menu-repeat
build/test-field-menu-repeat: tests/field_menu_repeat_run.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $< common/gba.c -o $@
test-field-menu-repeat: build/test-field-menu-repeat
	./build/test-field-menu-repeat
test: test-field-menu-repeat

# Optional: prepared profile and verified private inputs are required.
# SC64_MENU_P10_EXTRA_FLAGS can select a separately verified overlay.
SC64_MENU_P10_EXTRA_FLAGS ?=
SC64_MENU_HOST_SOURCES = common/gba.c $(sort $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES))
SC64_MENU_HOST_FLAGS = -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(SC64_MENU_P10_EXTRA_FLAGS)

build/field-menu-repeat-p10.o: tests/field_menu_card_p10.c $(wildcard starcraft/*.h) $(SOURCE_WEAPON_INPUTS) build/walk-pair-verified common/gba.h | build
	$(CLANG) $(SC64_MENU_HOST_FLAGS) $(PROTOSS_P10_BASE_FLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@
build/test-field-menu-repeat-p10: build/field-menu-repeat-p10.o $(SC64_MENU_HOST_SOURCES) | build
	$(CLANG) $(SC64_MENU_HOST_FLAGS) $(PROTOSS_P10_BASE_FLAGS) $^ -o $@
.PHONY: test-field-menu-repeat-p10
test-field-menu-repeat-p10: build/test-field-menu-repeat-p10
	./build/test-field-menu-repeat-p10

build/field-menu-repeat-showcase.o: tests/field_menu_card_p10.c $(wildcard starcraft/*.h) $(UNIT_FACING_INPUTS) $(COMMAND_ICON_HEADER) $(SOURCE_WEAPON_INPUTS) build/walk-pair-verified common/gba.h | build
	$(CLANG) $(SC64_MENU_HOST_FLAGS) $(SHOWCASE_FLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@
build/test-field-menu-repeat-showcase: build/field-menu-repeat-showcase.o $(SC64_MENU_HOST_SOURCES) $(SOURCE_WEAPON_INPUTS) build/walk-pair-verified | build
	$(CLANG) $(SC64_MENU_HOST_FLAGS) $(SHOWCASE_FLAGS) $(filter %.o %.c,$^) -o $@
.PHONY: test-field-menu-repeat-showcase
test-field-menu-repeat-showcase: build/test-field-menu-repeat-showcase
	./build/test-field-menu-repeat-showcase

SC64_MENU_DEPS := $(wildcard build/field-menu-repeat-p10.d build/field-menu-repeat-showcase.d)
-include $(SC64_MENU_DEPS)
