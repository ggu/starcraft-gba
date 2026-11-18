# Private opt-in cartridges with unique filenames and identities.
NATIVE_MELEE_START_FLAGS := -DSC64_NATIVE_MELEE_STARTS
NATIVE_ELIMINATION_FLAGS := -DSC64_NATIVE_SKIRMISH_ELIMINATION
NATIVE_MELEE_HOST_FLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon

define NATIVE_MELEE_START_PROFILE
build/starcraft-native-melee-$(1).elf: $$(COMMON) $$(COMMON_HEADERS) $$(STARCRAFT) $$(wildcard starcraft/*.h) $(3) $(4) $$(SC64_AUDIO_BANK_CONFIG) common/gba.ld Makefile mk/native-melee-starts/Makefile | build
	$$(CC) $$(CFLAGS) $$(NATIVE_MELEE_START_FLAGS) $(8) $(2) $$(COMMON) $$(STARCRAFT) $(4) $$(LDFLAGS) -Wl,-Map,build/starcraft-native-melee-$(1).map -lgcc -o $$@
	$$(SIZE) $$@
build/StarCraft-Melee-Starts-Probe-$(1)-GBA.gba: build/starcraft-native-melee-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ '$(5)' $(6)
build/test-native-melee-start-$(1): tests/sc64_skirmish_opening_menu.c tests/sc64_skirmish_campaign_run.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $(3) common/gba.c $$(COMMON_HEADERS) Makefile mk/native-melee-starts/Makefile | build
	$$(CLANG) $$(NATIVE_MELEE_HOST_FLAGS) $$(NATIVE_MELEE_START_FLAGS) $(8) $(filter-out -DGBA_NO_DEBUG_INPUTS,$(2)) $$< common/gba.c starcraft/sc64_bc_body_art.c $(4) $(7) -o $$@
endef

$(eval $(call NATIVE_MELEE_START_PROFILE,terran,$(TERRAN_ROM_FLAGS),$(TERRAN_FULL_INPUTS),,SC MELEE T,SMLT))
$(eval $(call NATIVE_MELEE_START_PROFILE,z10,$(ZERG_Z10_RELEASE_FLAGS),$(ZERG_Z10_CANONICAL_INPUTS),,SC MELEE Z,SMLZ))
$(eval $(call NATIVE_MELEE_START_PROFILE,p10,$(PROTOSS_P10_FLAGS),$(PROTOSS_P10_INPUTS),$(P7_INTERCEPTOR_FLIGHT_SOURCES),SC MELEE P,SMLP,$(P7_PROJECTILE_SOURCES)))
$(eval $(call NATIVE_MELEE_START_PROFILE,showcase,$(SHOWCASE_FLAGS),$(SHOWCASE_INPUTS),$(P7_INTERCEPTOR_FLIGHT_SOURCES),SC MELEE S,SMLS))

.PHONY: sc64-native-melee-start-probe test-sc64-native-melee-start-probe
sc64-native-melee-start-probe: $(foreach profile,terran z10 p10 showcase,build/StarCraft-Melee-Starts-Probe-$(profile)-GBA.gba)
test-sc64-native-melee-start-probe: $(addprefix build/test-native-melee-start-,terran z10 p10 showcase)
	@for test in $^; do "$$test" || exit $$?; done

# Independent elimination preview ROMs use their own cartridge identities.
$(eval $(call NATIVE_MELEE_START_PROFILE,elimination-terran,$(TERRAN_ROM_FLAGS),$(TERRAN_FULL_INPUTS),,SC ELIM T,SELT,,$(NATIVE_ELIMINATION_FLAGS)))
$(eval $(call NATIVE_MELEE_START_PROFILE,elimination-z10,$(ZERG_Z10_RELEASE_FLAGS),$(ZERG_Z10_CANONICAL_INPUTS),,SC ELIM Z,SELZ,,$(NATIVE_ELIMINATION_FLAGS)))
$(eval $(call NATIVE_MELEE_START_PROFILE,elimination-p10,$(PROTOSS_P10_FLAGS),$(PROTOSS_P10_INPUTS),$(P7_INTERCEPTOR_FLIGHT_SOURCES),SC ELIM P,SELP,$(P7_PROJECTILE_SOURCES),$(NATIVE_ELIMINATION_FLAGS)))
$(eval $(call NATIVE_MELEE_START_PROFILE,elimination-showcase,$(SHOWCASE_FLAGS),$(SHOWCASE_INPUTS),$(P7_INTERCEPTOR_FLIGHT_SOURCES),SC ELIM S,SELS,,$(NATIVE_ELIMINATION_FLAGS)))

.PHONY: sc64-native-skirmish-elimination-arm
sc64-native-skirmish-elimination-arm: $(foreach profile,terran z10 p10 showcase,build/StarCraft-Melee-Starts-Probe-elimination-$(profile)-GBA.gba)

.PHONY: test-sc64-native-skirmish-elimination
build/test-native-skirmish-elimination: tests/sc64_skirmish_elimination.c tests/sc64_skirmish_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/native-melee-starts/Makefile | build
	$(CLANG) $(NATIVE_MELEE_HOST_FLAGS) $(NATIVE_MELEE_START_FLAGS) $(NATIVE_ELIMINATION_FLAGS) $(filter-out -DGBA_NO_DEBUG_INPUTS,$(TERRAN_ROM_FLAGS)) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
test-sc64-native-skirmish-elimination: build/test-native-skirmish-elimination
	./$<
