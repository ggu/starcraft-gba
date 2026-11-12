# Private prepared-input gate. Default ROM targets never opt in.
SC64_NUKE_RUNTIME_FLAGS := -DSC64_SHARED_NUKE_PROBE -I$(SC64_NUKE_ASSET_DIR)
SC64_NUKE_RUNTIME_ASSETS := $(addprefix $(SC64_NUKE_ASSET_DIR)/,sc64_nuke_source_generated.h sc64_nuke_building_source_generated.h sc64_nuke_ghost_source_generated.h)
SC64_NUKE_FIXTURE_FLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon

define SC64_NUKE_FIXTURE
build/test-sc64-nuke-world-$(1): tests/sc64_nuke_world_game.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $(3) $$(SC64_NUKE_RUNTIME_ASSETS) common/gba.c $$(COMMON_HEADERS) mk/sc64-nuke/Makefile Makefile | build
	$$(CLANG) $$(SC64_NUKE_FIXTURE_FLAGS) $$(SC64_NUKE_RUNTIME_FLAGS) $(2) $$< common/gba.c $$(filter-out starcraft/game.c,$$(STARCRAFT)) $(4) -o $$@
endef
$(eval $(call SC64_NUKE_FIXTURE,terran,$(TERRAN_ROM_FLAGS),$(TERRAN_FULL_INPUTS)))
$(eval $(call SC64_NUKE_FIXTURE,z10,$(ZERG_Z10_CANONICAL_FLAGS),$(ZERG_Z10_CANONICAL_INPUTS)))
$(eval $(call SC64_NUKE_FIXTURE,showcase,$(SHOWCASE_FLAGS),$(SHOWCASE_INPUTS)))

SC64_NUKE_P10_FACT_DIR := build/generated/p10-nuke-probe
SC64_NUKE_P10_FACT_HEADER := $(SC64_NUKE_P10_FACT_DIR)/sc64_fact_tables_generated.h
$(SC64_NUKE_P10_FACT_HEADER): tools/sc64_fact_tables_gen.c $(wildcard starcraft/*.h) $(COMMON_HEADERS) $(P8_AVAILABILITY_HEADER) mk/sc64-nuke/Makefile | build
	mkdir -p $(SC64_NUKE_P10_FACT_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(filter-out -DSC64_FACT_TABLES,$(PROTOSS_P10_BASE_FLAGS)) -DSC64_SHARED_NUKE_PROBE tools/sc64_fact_tables_gen.c -o $(SC64_NUKE_P10_FACT_DIR)/generator
	$(SC64_NUKE_P10_FACT_DIR)/generator > $@.tmp
	mv $@.tmp $@
$(eval $(call SC64_NUKE_FIXTURE,p10,$(patsubst -I$(P10_FACT_V2_DIR),-I$(SC64_NUKE_P10_FACT_DIR),$(PROTOSS_P10_FLAGS)),$(filter-out $(P10_FACT_V2_HEADER),$(PROTOSS_P10_INPUTS)) $(SC64_NUKE_P10_FACT_HEADER),$(P7_INTERCEPTOR_FLIGHT_SOURCES)))

define SC64_NUKE_ARM
build/starcraft-nuke-$(1).elf: $$(COMMON) $$(COMMON_HEADERS) $$(STARCRAFT) $$(wildcard starcraft/*.h) $(3) $(4) $$(SC64_NUKE_RUNTIME_ASSETS) common/gba.ld mk/sc64-nuke/Makefile Makefile | build
	$$(CC) $$(CFLAGS) $$(SC64_NUKE_RUNTIME_FLAGS) $(2) $$(COMMON) $$(STARCRAFT) $(4) $$(LDFLAGS) -Wl,-Map,build/starcraft-nuke-$(1).map -lgcc -o $$@
	$$(SIZE) $$@
build/StarCraft-Nuke-Probe-$(1)-GBA.gba: build/starcraft-nuke-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ '$(5)' $(6)
endef
$(eval $(call SC64_NUKE_ARM,terran,$(TERRAN_ROM_FLAGS),$(TERRAN_FULL_INPUTS),,SC NUKE T,SCNT))
$(eval $(call SC64_NUKE_ARM,z10,$(ZERG_Z10_RELEASE_FLAGS),$(ZERG_Z10_CANONICAL_INPUTS),,SC NUKE Z,SCNZ))
$(eval $(call SC64_NUKE_ARM,p10,$(patsubst -I$(P10_FACT_V2_DIR),-I$(SC64_NUKE_P10_FACT_DIR),$(PROTOSS_P10_FLAGS)),$(filter-out $(P10_FACT_V2_HEADER),$(PROTOSS_P10_INPUTS)) $(SC64_NUKE_P10_FACT_HEADER),$(P7_INTERCEPTOR_FLIGHT_SOURCES),SC NUKE P,SCNP))
$(eval $(call SC64_NUKE_ARM,showcase,$(SHOWCASE_FLAGS),$(SHOWCASE_INPUTS),$(P7_INTERCEPTOR_FLIGHT_SOURCES),SC NUKE S,SCNS))

.PHONY: sc64-nuke-probe
sc64-nuke-probe: $(foreach profile,terran z10 p10 showcase,build/StarCraft-Nuke-Probe-$(profile)-GBA.gba)

.PHONY: test-sc64-nuke-world
test-sc64-nuke-world: $(addprefix build/test-sc64-nuke-world-,terran z10 p10 showcase)
	@for test in $^; do "$$test" || exit $$?; done

.PHONY: test-sc64-nuke-save-rows
build/test-sc64-nuke-save-rows: tests/sc64_nuke_save_rows.c starcraft/sc64_nuke_save_rows.h starcraft/sc64_nuke_rules.h starcraft/sc64_save_tag_namespace.h | build
	$(CLANG) $(SC64_NUKE_FIXTURE_FLAGS) $< -o $@
test-sc64-nuke-save-rows: build/test-sc64-nuke-save-rows
	./build/test-sc64-nuke-save-rows
