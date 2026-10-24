.DEFAULT_GOAL := all
PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size
CLANG ?= clang
PYTHON ?= python3
CFLAGS := -mcpu=arm7tdmi -mthumb -mthumb-interwork -O2 -g -std=c11 -ffreestanding -fno-builtin -fno-strict-aliasing -fno-common -ffunction-sections -fdata-sections -Wall -Wextra -Icommon
LDFLAGS := -mcpu=arm7tdmi -mthumb -mthumb-interwork -nostdlib -Tcommon/gba.ld -Wl,--gc-sections
COMMON := common/start.s common/main.c common/gba.c common/memory.c
COMMON_HEADERS := $(wildcard common/*.h)
STARCRAFT := $(wildcard starcraft/*.c)

SC64_FLAGS := -DSC64_IMPORTED_MISSION -DSC64_HAS_TERRAIN_ART -DSC64_HAS_SPRITES -Istarcraft -I.local/sc64/generated
SC64_INPUTS := .local/sc64/generated/sc64_mission_generated.h .local/sc64/generated/sc64_assets_generated.h .local/sc64/generated/sc64_sprites_generated.h
BACKWATER_FLAGS := -DSC64_IMPORTED_MISSION -DSC64_HAS_TERRAIN_ART -DSC64_HAS_SPRITES -Istarcraft -I.local/sc64/backwater/generated
BACKWATER_INPUTS := .local/sc64/backwater/generated/sc64_mission_generated.h .local/sc64/backwater/generated/sc64_assets_generated.h .local/sc64/backwater/generated/sc64_sprites_generated.h
CAMPAIGN_FLAGS := $(BACKWATER_FLAGS) -I.local/sc64/campaign/generated
CAMPAIGN_INPUTS := $(BACKWATER_INPUTS) .local/sc64/campaign/generated/sc64_campaign_generated.h
RACES_FLAGS := $(BACKWATER_FLAGS) -DSC64_RACES -I.local/sc64/races/generated
RACES_INPUTS := $(BACKWATER_INPUTS) .local/sc64/races/generated/sc64_races_generated.h .local/sc64/races/generated/sc64_race_sprites_generated.h .local/sc64/races/generated/sc64_menu_generated.h
UNIFIED_FLAGS := $(RACES_FLAGS) -DSC64_UNIFIED
RACE_CAMPAIGN_FLAGS := $(UNIFIED_FLAGS) -DSC64_IMPORTED_SKIRMISH -I.local/sc64/skirmish/generated
RACE_CAMPAIGN_INPUTS := $(RACES_INPUTS) .local/sc64/skirmish/generated/sc64_skirmish_generated.h
T3_FLAGS := $(RACE_CAMPAIGN_FLAGS) -DSC64_T3_ENABLED -DSC64_CAMPAIGN_RACE=0 $(T3_EXTRA_FLAGS)
Z3_OPTIMIZATION_FLAGS ?= -DSC64_ROUTE_PACK3 -DSC64_VISIBILITY_SPANS -DSC64_Z3_WINDOWED_SCHEDULE -DSC64_NO_FRAME_CLEAR -DGBA_PRESENT_DEADLINE -DSC64_TARGET_OCCUPANCY -DSC64_UNARMED_QUERY_SKIP
Z3_FLAGS := $(RACE_CAMPAIGN_FLAGS) -DSC64_Z3_ENABLED -DSC64_CAMPAIGN_RACE=1 $(Z3_OPTIMIZATION_FLAGS) $(Z3_EXTRA_FLAGS)
Z4_FLAGS := $(filter-out -I.local/sc64/races/generated,$(Z3_FLAGS)) -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated -I.local/sc64/z4/generated -DSC64_Z4_DATA_ENABLED -DSC64_Z4_ENABLED -DSC64_Z4_RUNTIME -DSC64_Z4_CAMPAIGN_ENTRY -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_ZP_DEFENSE_TARGETS
Z4_INPUTS := $(RACE_CAMPAIGN_INPUTS) .local/sc64/zp-stage/generated/sc64_races_generated.h .local/sc64/zp-stage/generated/sc64_race_sprites_generated.h .local/sc64/z4/generated/sc64_z4_scenario_generated.h
LARGE_PROBE_FLAGS := $(RACE_CAMPAIGN_FLAGS) -DSC64_LARGE_MAPS -DSC64_CAMPAIGN_RACE=2
P3_OPTIMIZATION_FLAGS ?= -DSC64_ROUTE_PACK3 -DSC64_P3_DEFENSE_TARGETS -DSC64_P3_WINDOWED_SCHEDULE -DSC64_NO_FRAME_CLEAR -DSC64_TERRAIN_SCROLL -DSC64_STATIC_KEY_CULL -DSC64_VISIBILITY_SPANS -DGBA_PRESENT_DEADLINE -DSC64_TARGET_OCCUPANCY -DSC64_UNARMED_QUERY_SKIP
P3_FLAGS := $(LARGE_PROBE_FLAGS) -DSC64_P3_ENABLED -DSC64_P3_RUNTIME $(P3_OPTIMIZATION_FLAGS) $(P3_EXTRA_FLAGS)
P4_FLAGS := $(filter-out -I.local/sc64/races/generated,$(P3_FLAGS)) -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated -I.local/sc64/p4/generated -DSC64_P4_DATA_ENABLED -DSC64_P4_ENABLED -DSC64_P4_RUNTIME -DSC64_P4_CAMPAIGN_ENTRY -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_ZP_DEFENSE_TARGETS
P4_INPUTS := $(RACE_CAMPAIGN_INPUTS) .local/sc64/zp-stage/generated/sc64_races_generated.h .local/sc64/zp-stage/generated/sc64_race_sprites_generated.h .local/sc64/p4/generated/sc64_p4_scenario_generated.h
P4_STORM_FLAGS := $(P4_FLAGS) -DSC64_P4_STORM_ENABLED
ZP_FULL_STORAGE_FLAGS := $(P4_STORM_FLAGS) -DSC64_ZP_FULL_STORAGE
Z5_FLAGS := $(filter-out -DSC64_ZP_DEFENSE_TARGETS -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated,$(Z4_FLAGS)) -I.local/sc64/z5/generated -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated -I.local/sc64/zp-stage/generated/scenarios -DSC64_ZP_FULL_STORAGE -DSC64_STATIC_KEY_CULL -DSC64_Z5_DATA_ENABLED -DSC64_Z5_ENABLED -DSC64_Z5_RUNTIME -DSC64_Z5_CAMPAIGN_ENTRY -DSC64_Z5_ACTORS -DSC64_Z5_DOODADS
Z5_INPUTS := $(Z4_INPUTS) .local/sc64/z5/generated/sc64_race_sprites_generated.h .local/sc64/zp-stage/generated/scenarios/sc64_z5_scenario_generated.h .local/sc64/zp-stage/generated/scenarios/sc64_z5_doodads_generated.h
Z5_CLOAK_FLAGS := $(Z5_FLAGS) -DSC64_Z5_CLOAK_ENABLED
Z5_CLOAK_INPUTS := $(Z5_INPUTS) .local/sc64/zp-stage/generated/scenarios/sc64_z5_abilities_generated.h
P5_FLAGS := $(filter-out -DSC64_ZP_DEFENSE_TARGETS -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated,$(P4_STORM_FLAGS)) -I.local/sc64/p5/generated -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated -I.local/sc64/zp-stage/generated/scenarios -DSC64_ZP_FULL_STORAGE -DSC64_P5_DATA_ENABLED -DSC64_P5_ENABLED -DSC64_P5_RUNTIME -DSC64_P5_CAMPAIGN_ENTRY -DSC64_P5_ACTORS -DSC64_Z6_ACTORS
P5_INPUTS := $(P4_INPUTS) .local/sc64/p5/generated/sc64_race_sprites_generated.h .local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h
P5_SHUTTLE_FLAGS := $(P5_FLAGS) -DSC64_P5_SHUTTLE_ENABLED
T4_STORAGE_FLAGS := $(RACE_CAMPAIGN_FLAGS) -DSC64_LARGE_MAPS -DSC64_T4_ENABLED -DSC64_T4_RUNTIME -DSC64_ROUTE_PACK3 -DSC64_T4_WINDOWED_SCHEDULE -DSC64_CAMPAIGN_RACE=0
T4_OPTIMIZATION_FLAGS ?= -DSC64_T4_COUNT_CACHE -DSC64_NO_FRAME_CLEAR -DSC64_TERRAIN_SCROLL -DSC64_STATIC_KEY_CULL -DSC64_VISIBILITY_SPANS -DSC64_UNARMED_QUERY_SKIP
T4_SCENARIO_FLAGS := $(T4_STORAGE_FLAGS) -DSC64_T4_SCENARIO_RUNTIME
T4_RUNTIME_FLAGS := $(T4_SCENARIO_FLAGS) $(T4_OPTIMIZATION_FLAGS)
T4_REPLAY_FLAGS := $(T4_RUNTIME_FLAGS) -DSC64_T4_REPLAY_ENTRY
T4_COMBINED_FLAGS := $(T4_RUNTIME_FLAGS) -DSC64_T3_ENABLED -DSC64_T4_CAMPAIGN_ENTRY
T5_PROBE_FLAGS := $(T4_COMBINED_FLAGS) -DSC64_T5_ENABLED -DSC64_T5_RUNTIME
T5_CONTROLLER_REPLAY_FLAGS := $(T5_PROBE_FLAGS) -DSC64_T5_CAMPAIGN_ENTRY
T6_STORAGE_FLAGS := $(T5_CONTROLLER_REPLAY_FLAGS) -DSC64_T6_ENABLED -DSC64_T6_STORAGE
T6_RUNTIME_FLAGS := $(T6_STORAGE_FLAGS) -DSC64_T6_RUNTIME
LATE_TERRAN_STORAGE_FLAGS := $(T6_RUNTIME_FLAGS) -DSC64_LATE_TERRAN_STORAGE -DSC64_TERRAN_LATE_ACTORS
LATE_TERRAN_STAGE := .local/sc64/terran-late/import-stage
LATE_TERRAN_RUNTIME_FLAGS := $(filter-out -I.local/sc64/races/generated,$(LATE_TERRAN_STORAGE_FLAGS)) -I$(LATE_TERRAN_STAGE) -I.local/sc64/races/generated -DSC64_TERRAN_LATE_ENABLED -DSC64_TERRAN_LATE_RUNTIME
LATE_TERRAN_HEADERS := $(LATE_TERRAN_STAGE)/sc64_races_generated.h $(LATE_TERRAN_STAGE)/sc64_race_sprites_generated.h $(foreach mission,7 8 9 10,$(LATE_TERRAN_STAGE)/sc64_t$(mission)_scenario_generated.h $(LATE_TERRAN_STAGE)/sc64_t$(mission)_availability_generated.h)
TERRAN_FULL_FLAGS := $(LATE_TERRAN_RUNTIME_FLAGS) -DSC64_T6_CAMPAIGN_ENTRY -DSC64_TERRAN_FULL_CAMPAIGN_ENTRY
TERRAN_FULL_INPUTS = $(RACE_CAMPAIGN_INPUTS) $(LATE_TERRAN_HEADERS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER)

T4_SCENARIO_HEADER := .local/sc64/races/generated/sc64_t4_scenario_generated.h
T4_SCENARIO_RECORDS := .local/sc64/races/generated/t4/records
T5_SCENARIO_HEADER := .local/sc64/races/generated/sc64_t5_scenario_generated.h
T5_AVAILABILITY_HEADER := .local/sc64/races/generated/sc64_t5_availability_generated.h
T5_SCENARIO_RECORDS := .local/sc64/races/generated/t5/records
T6_SCENARIO_HEADER := .local/sc64/races/generated/sc64_t6_scenario_generated.h
T6_AVAILABILITY_HEADER := .local/sc64/races/generated/sc64_t6_availability_generated.h
T6_SCENARIO_RECORDS := .local/sc64/races/generated/t6/records

.PHONY: all clean test verify probe sc64 backwater skirmish campaign races unified \
	verify-sc64 verify-backwater verify-skirmish verify-campaign verify-races verify-unified \
	test-starcraft test-sc64 test-backwater test-skirmish test-skirmish-match \
	test-campaign test-races test-races-controller test-race-routes test-race-route-sizes \
	test-races-menu test-unified test-collision test-terrain-cache race-campaigns \
	terran zerg protoss verify-terran verify-zerg verify-protoss verify-race-campaigns \
	test-terran test-zerg test-protoss test-race-campaigns test-sc64-t4-import \
	test-sc64-t4-storage sc64-t4-storage-probe

# The public default uses only original-art StarCraft sources.
all: verify
verify: build/StarCraft-GBA.gba
	$(PYTHON) tools/verify_roms.py

build:
	mkdir -p build

build/starcraft.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft.map -lgcc -o $@
	$(SIZE) $@

build/StarCraft-GBA.gba: build/starcraft.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC GBA DEMO' SCGD

build/test-starcraft: tests/host.c common/gba.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon tests/host.c common/gba.c $(STARCRAFT) -o $@

build/test-starcraft-selection: tests/sc64_selection_run.c common/gba.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon tests/sc64_selection_run.c common/gba.c -o $@

build/test-race-rules: tests/race_rules.c starcraft/race_rules.h starcraft/race_skirmish.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined tests/race_rules.c -o $@

build/test-skirmish-ai: tests/skirmish_ai.c starcraft/skirmish_ai.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined tests/skirmish_ai.c -o $@

build/test-sc64-frontend: tests/sc64_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon tests/sc64_frontend.c -o $@

build/test-sc64-demo-schedule: tests/sc64_demo_schedule.c common/demo_schedule.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I. -Icommon $< -o $@

build/test-sc64-demo-schedule-runtime: tests/sc64_demo_schedule_runtime.c common/main.c common/demo_schedule.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $< -o $@

build/test-sc64-bitgrid: tests/sc64_bitgrid.c starcraft/sc64_bitgrid.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon tests/sc64_bitgrid.c -o $@

build/test-sc64-map-math: tests/sc64_map_math.c starcraft/sc64_map_math.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/sc64_map_math.c -o $@

build/test-sc64-t3-rules: tests/sc64_t3_rules.c starcraft/sc64_t3.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/sc64_t3_rules.c -o $@

build/test-sc64-z3-rules: tests/sc64_z3_rules.c starcraft/sc64_z3.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/sc64_z3_rules.c -o $@

build/test-sc64-p3-rules: tests/sc64_p3_rules.c starcraft/sc64_p3.h starcraft/race_rules.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@

build/test-sc64-p3-model: tests/sc64_p3_model.c starcraft/sc64_p3_model.h starcraft/sc64_p3.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I. $< -o $@

# Import/storage probes do not expose Higher Ground in the playable catalog.
.PHONY: test-sc64-p3-import test-sc64-p3-scourge test-sc64-large-map sc64-large-probe
build/test-sc64-p3-runtime: tests/sc64_p3_runtime.c starcraft/sc64_p3_adapter.h starcraft/sc64_p3_model.h starcraft/sc64_p3.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined $< -o $@
build/test-sc64-p3-front: P3_FRONT_FLAGS=-DSC64_P3_RUNTIME -DSC64_P3_ENABLED
build/test-sc64-p3-front-import: P3_FRONT_FLAGS=-DSC64_P3_ENABLED
build/test-sc64-p3-front-incomplete: P3_FRONT_FLAGS=-DSC64_P3_RUNTIME
P3_FRONT_TESTS := build/test-sc64-p3-front build/test-sc64-p3-front-import build/test-sc64-p3-front-incomplete
$(P3_FRONT_TESTS): tests/sc64_p3_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon -DSC64_UNIFIED -DSC64_CAMPAIGN_RACE=2 $(P3_FRONT_FLAGS) $< -o $@

.PHONY: protoss-p3 test-sc64-p3
# Compatibility filenames point at the canonical campaign ROM/ELF.
build/starcraft-protoss-p3.elf: build/starcraft-protoss.elf | build
	ln -sf starcraft-protoss.elf $@
build/StarCraft-Protoss-P3-GBA.gba: build/StarCraft-Protoss-GBA.gba | build
	ln -sf StarCraft-Protoss-GBA.gba $@
protoss-p3: verify-protoss build/starcraft-protoss-p3.elf build/StarCraft-Protoss-P3-GBA.gba
build/test-sc64-p3-run: tests/sc64_p3_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-save: tests/sc64_p3_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-controller: tests/sc64_p3_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-unarmed-targets: tests/sc64_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) -DSC64_UNARMED_QUERY_SKIP $< common/gba.c -o $@
.PHONY: test-sc64-p3-unarmed-targets
test-sc64-p3-unarmed-targets: build/test-sc64-p3-unarmed-targets
	./build/test-sc64-p3-unarmed-targets
build/test-sc64-p3-defenses: tests/sc64_p3_defense_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-frame-coverage: tests/sc64_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-static-key: tests/sc64_static_key.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_TERRAIN_SCROLL -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-schedule: tests/sc64_demo_schedule_runtime.c common/main.c common/demo_schedule.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -DSC64_RACES -DSC64_UNIFIED -DSC64_P3_ENABLED -DSC64_P3_RUNTIME -DSC64_CAMPAIGN_RACE=2 -DSC64_P3_WINDOWED_SCHEDULE -Icommon $< -o $@
test-sc64-p3: test-sc64-p3-import build/test-sc64-p3-runtime $(P3_FRONT_TESTS) build/test-sc64-p3-run build/test-sc64-p3-save build/test-sc64-p3-hud test-sc64-p3-scourge build/test-sc64-p3-defenses build/test-sc64-p3-frame-coverage build/test-sc64-p3-schedule build/test-sc64-p3-controller build/test-sc64-p3-static-key
	./build/test-sc64-p3-runtime
	@for test in $(P3_FRONT_TESTS); do ./$$test || exit $$?; done
	./build/test-sc64-p3-run
	./build/test-sc64-p3-save
	./build/test-sc64-p3-hud
	./build/test-sc64-p3-defenses
	./build/test-sc64-p3-frame-coverage
	./build/test-sc64-p3-static-key
	./build/test-sc64-p3-schedule
	./build/test-sc64-p3-controller --trace .local/sc64/p3-controller-asan-win.json
build/test-sc64-p3-import: tests/sc64_p3_import.c starcraft/race_rules.h starcraft/sc64_types.h .local/sc64/races/generated/sc64_races_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_P3_ENABLED -DSC64_CAMPAIGN_RACE=2 -Istarcraft -I.local/sc64/races/generated $< -o $@
test-sc64-p3-import: build/test-sc64-p3-import
	./build/test-sc64-p3-import
build/test-sc64-t4-import: tests/sc64_t4_import.c starcraft/sc64_types.h .local/sc64/races/generated/sc64_races_generated.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -DSC64_UNIFIED -DSC64_T4_ENABLED -DSC64_CAMPAIGN_RACE=0 -Istarcraft $< -o $@
test-sc64-t4-import: build/test-sc64-t4-import
	./build/test-sc64-t4-import
build/test-sc64-t4-storage: tests/sc64_large_map.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_STORAGE_FLAGS) $< common/gba.c -o $@
build/test-sc64-t4-schedule: tests/sc64_demo_schedule_runtime.c common/main.c common/demo_schedule.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_STORAGE_FLAGS) $< -o $@
test-sc64-t4-storage: build/test-sc64-t4-storage build/test-sc64-t4-schedule
	./build/test-sc64-t4-storage
	./build/test-sc64-t4-schedule
build/starcraft-t4-storage-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T4_STORAGE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-t4-storage-probe.map -lgcc -o $@
	$(SIZE) $@
sc64-t4-storage-probe: build/starcraft-t4-storage-probe.elf
# T5 remains import-only: these fixtures do not enable a campaign entry.
$(T5_SCENARIO_HEADER): tools/sc64_t4_scenario_pack.py starcraft/sc64_trigger_types.h .local/sc64/races/generated/t5/manifest.json $(wildcard $(T5_SCENARIO_RECORDS)/*.bin)
	$(PYTHON) tools/sc64_t4_scenario_pack.py --profile t5
build/test-sc64-t5-import: tests/sc64_t5_import.c starcraft/sc64_types.h .local/sc64/races/generated/sc64_races_generated.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_T5_ENABLED -DSC64_CAMPAIGN_RACE=0 -Istarcraft -I.local/sc64/races/generated $< -o $@
build/test-sc64-t3t4t5-import: tests/sc64_t5_import.c starcraft/sc64_types.h .local/sc64/races/generated/sc64_races_generated.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_T3_ENABLED -DSC64_T4_ENABLED -DSC64_T5_ENABLED -DSC64_CAMPAIGN_RACE=0 -Istarcraft -I.local/sc64/races/generated $< -o $@
build/test-sc64-t5-scenario: tests/sc64_t5_scenario.c starcraft/sc64_trigger_types.h starcraft/sc64_t4_triggers.h $(T5_SCENARIO_HEADER) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -DSC64_T4_RUNTIME -DSC64_T5_ENABLED -Istarcraft -I.local/sc64/races/generated $< -o $@
.PHONY: test-sc64-t5-import test-sc64-t5-scenario
test-sc64-t5-import: build/test-sc64-t5-import build/test-sc64-t3t4t5-import
	./build/test-sc64-t5-import
	./build/test-sc64-t3t4t5-import
test-sc64-t5-scenario: build/test-sc64-t5-scenario
	./build/test-sc64-t5-scenario
$(T4_SCENARIO_HEADER): tools/sc64_t4_scenario_pack.py starcraft/sc64_trigger_types.h .local/sc64/races/generated/t4/manifest.json $(wildcard $(T4_SCENARIO_RECORDS)/*.bin)
	$(PYTHON) tools/sc64_t4_scenario_pack.py
.PHONY: sc64-t4-scenario test-sc64-t4-triggers test-sc64-t4-scenario test-sc64-t4-queries test-sc64-t4-actions test-sc64-t4-count-cache
sc64-t4-scenario: $(T4_SCENARIO_HEADER)
build/test-sc64-t4-triggers: tests/sc64_t4_triggers.c starcraft/sc64_t4_triggers.h starcraft/sc64_trigger_types.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -DSC64_T4_RUNTIME -I. $< -o $@
test-sc64-t4-triggers: build/test-sc64-t4-triggers
	./build/test-sc64-t4-triggers
build/test-sc64-t4-queries: tests/sc64_t4_queries.c starcraft/sc64_t4_queries.h starcraft/sc64_t4_world.h starcraft/sc64_trigger_types.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -DSC64_T4_RUNTIME -I. $< -o $@
test-sc64-t4-queries: build/test-sc64-t4-queries
	./build/test-sc64-t4-queries
build/test-sc64-t4-actions: tests/sc64_t4_actions.c starcraft/sc64_t4_actions.h starcraft/sc64_trigger_types.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -DSC64_T4_RUNTIME -I. $< -o $@
test-sc64-t4-actions: build/test-sc64-t4-actions
	./build/test-sc64-t4-actions
build/test-sc64-t4-count-cache: tests/sc64_t4_count_cache.c starcraft/sc64_t4_count_cache.h starcraft/sc64_t4_triggers.h starcraft/sc64_trigger_types.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -DSC64_T4_RUNTIME -I. $< -o $@
test-sc64-t4-count-cache: build/test-sc64-t4-count-cache
	./build/test-sc64-t4-count-cache
test-starcraft: build/test-sc64-t4-triggers build/test-sc64-t4-queries build/test-sc64-t4-actions
build/test-sc64-t4-scenario: tests/sc64_t4_scenario.c starcraft/sc64_t4_triggers.h starcraft/sc64_t4_actions.h starcraft/sc64_t4_world.h starcraft/sc64_t4_runtime.h starcraft/sc64_trigger_types.h $(T4_SCENARIO_HEADER) common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -DSC64_T4_RUNTIME -Istarcraft -I.local/sc64/races/generated $< -o $@
test-sc64-t4-scenario: build/test-sc64-t4-scenario
	./build/test-sc64-t4-scenario
build/test-sc64-t4-effects: tests/sc64_t4_effects.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_STORAGE_FLAGS) -DSC64_T4_SCENARIO_RUNTIME $< common/gba.c -o $@
.PHONY: test-sc64-t4-effects
test-sc64-t4-effects: build/test-sc64-t4-effects
	./build/test-sc64-t4-effects
build/test-sc64-t4-creation: tests/sc64_t4_creation.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_STORAGE_FLAGS) $< common/gba.c -o $@
test-sc64-t4-creation: build/test-sc64-t4-creation
	./build/test-sc64-t4-creation
build/test-sc64-t4-scenario-save: tests/sc64_t4_scenario_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_RUNTIME_FLAGS) $< common/gba.c -o $@
test-sc64-t4-scenario-save: build/test-sc64-t4-scenario-save
	./build/test-sc64-t4-scenario-save
build/test-sc64-t4-runtime: tests/sc64_t4_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_RUNTIME_FLAGS) $< common/gba.c -o $@
test-sc64-t4-runtime: build/test-sc64-t4-runtime
	./build/test-sc64-t4-runtime
build/test-sc64-t4-traps: tests/sc64_t4_traps.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_RUNTIME_FLAGS) $< common/gba.c -o $@
test-sc64-t4-traps: build/test-sc64-t4-traps
	./build/test-sc64-t4-traps
build/test-sc64-t4-prop-art: tests/sc64_t4_prop_art.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_RUNTIME_FLAGS) $< common/gba.c -o $@
test-sc64-t4-prop-art: build/test-sc64-t4-prop-art
	./build/test-sc64-t4-prop-art
build/starcraft-t4-scenario-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T4_RUNTIME_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-t4-scenario-probe.map -lgcc -o $@
	$(SIZE) $@
.PHONY: test-sc64-t4-creation test-sc64-t4-scenario-save test-sc64-t4-runtime test-sc64-t4-traps test-sc64-t4-prop-art sc64-t4-scenario-probe
sc64-t4-scenario-probe: build/starcraft-t4-scenario-probe.elf
build/test-sc64-t4-ping: tests/sc64_t4_ping.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_RUNTIME_FLAGS) $< common/gba.c -o $@
test-sc64-t4-ping: build/test-sc64-t4-ping
	./build/test-sc64-t4-ping
build/test-sc64-t4-controller: tests/sc64_t4_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_REPLAY_FLAGS) $< common/gba.c -o $@
build/starcraft-t4-replay.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T4_REPLAY_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-t4-replay.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-T4-Internal-GBA.gba: build/starcraft-t4-replay.elf tools/fix_rom.py tools/verify_roms.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC64 T4 TEST' SCT4
	$(PYTHON) tools/verify_roms.py --t4-internal
.PHONY: test-sc64-t4-ping sc64-t4-replay test-sc64-t3t4-save test-sc64-t3t4-controller \
	terran-t4-combined terran-t4 test-sc64-t3t4-runtime test-sc64-t3t4-skirmish
sc64-t4-replay: build/StarCraft-T4-Internal-GBA.gba
build/test-sc64-t3t4-save: tests/sc64_t4_scenario_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_COMBINED_FLAGS) $< common/gba.c -o $@
test-sc64-t3t4-save: build/test-sc64-t3t4-save
	./build/test-sc64-t3t4-save
build/test-sc64-t3t4-controller: tests/sc64_t4_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_COMBINED_FLAGS) $< common/gba.c -o $@
test-sc64-t3t4-controller: build/test-sc64-t3t4-controller
	./build/test-sc64-t3t4-controller --limit 30000 --trace .local/sc64/t3t4-controller-smoke.json
build/starcraft-t4-combined.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T4_COMBINED_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-t4-combined.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Terran-T4-GBA.gba: build/starcraft-t4-combined.elf tools/fix_rom.py tools/verify_roms.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC TERRAN T4' SCT5
	$(PYTHON) tools/verify_roms.py --terran-t4
terran-t4-combined: build/StarCraft-Terran-T4-GBA.gba
terran-t4: terran-t4-combined
build/test-sc64-t3t4-skirmish: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_COMBINED_FLAGS) $< common/gba.c -o $@
test-sc64-t3t4-skirmish: build/test-sc64-t3t4-skirmish
	./build/test-sc64-t3t4-skirmish
build/test-sc64-t4-entry-on: tests/sc64_t4_frontend_entry.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon -DSC64_UNIFIED -DSC64_CAMPAIGN_RACE=0 -DSC64_T4_SCENARIO_RUNTIME -DSC64_T4_REPLAY_ENTRY $< -o $@
build/test-sc64-t4-entry-off: tests/sc64_t4_frontend_entry.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon -DSC64_UNIFIED -DSC64_CAMPAIGN_RACE=0 -DSC64_T4_SCENARIO_RUNTIME $< -o $@
.PHONY: test-sc64-t4-campaign-frontend
build/test-sc64-t4-campaign-entry-on: tests/sc64_t4_campaign_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon -DSC64_T3_ENABLED -DSC64_UNIFIED -DSC64_CAMPAIGN_RACE=0 -DSC64_T4_SCENARIO_RUNTIME -DSC64_T4_RUNTIME -DSC64_T4_CAMPAIGN_ENTRY -DSC64_IMPORTED_SKIRMISH $< -o $@
build/test-sc64-t4-campaign-entry-off: tests/sc64_t4_campaign_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon -DSC64_T3_ENABLED -DSC64_UNIFIED -DSC64_CAMPAIGN_RACE=0 -DSC64_T4_SCENARIO_RUNTIME -DSC64_T4_RUNTIME -DSC64_IMPORTED_SKIRMISH $< -o $@
test-sc64-t4-campaign-frontend: build/test-sc64-t4-campaign-entry-on build/test-sc64-t4-campaign-entry-off
	./build/test-sc64-t4-campaign-entry-on
	./build/test-sc64-t4-campaign-entry-off

# This flag is exercised only by asset-free frontend checks until the T5
# runtime has a complete controller route. Existing ROM profiles stay gated.
T5_FRONTEND_TEST_FLAGS := -DSC64_T3_ENABLED -DSC64_UNIFIED -DSC64_CAMPAIGN_RACE=0 -DSC64_T4_SCENARIO_RUNTIME -DSC64_T4_RUNTIME -DSC64_T4_CAMPAIGN_ENTRY -DSC64_T5_ENABLED -DSC64_T5_RUNTIME -DSC64_IMPORTED_SKIRMISH
build/test-sc64-t5-campaign-entry-on: tests/sc64_t5_campaign_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $(T5_FRONTEND_TEST_FLAGS) -DSC64_T5_CAMPAIGN_ENTRY $< -o $@
build/test-sc64-t5-campaign-entry-off: tests/sc64_t5_campaign_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $(T5_FRONTEND_TEST_FLAGS) $< -o $@
.PHONY: test-sc64-t5-campaign-frontend
test-sc64-t5-campaign-frontend: build/test-sc64-t5-campaign-entry-on build/test-sc64-t5-campaign-entry-off
	./build/test-sc64-t5-campaign-entry-on
	./build/test-sc64-t5-campaign-entry-off
test-starcraft: test-sc64-t5-campaign-frontend
.PHONY: test-sc64-t4-frontend-entry
test-sc64-t4-frontend-entry: build/test-sc64-t4-entry-on build/test-sc64-t4-entry-off
	./build/test-sc64-t4-entry-on
	./build/test-sc64-t4-entry-off
test-starcraft: test-sc64-t4-frontend-entry test-sc64-t4-campaign-frontend
T4_FRAME_TESTS := $(addprefix build/test-sc64-t4-frame-,base scroll noclear cull scroll-noclear scroll-cull noclear-cull all)
build/test-sc64-t4-frame-scroll: T4_FRAME_FLAGS := -DSC64_TERRAIN_SCROLL
build/test-sc64-t4-frame-noclear: T4_FRAME_FLAGS := -DSC64_NO_FRAME_CLEAR
build/test-sc64-t4-frame-cull: T4_FRAME_FLAGS := -DSC64_STATIC_KEY_CULL
build/test-sc64-t4-frame-scroll-noclear: T4_FRAME_FLAGS := -DSC64_TERRAIN_SCROLL -DSC64_NO_FRAME_CLEAR
build/test-sc64-t4-frame-scroll-cull: T4_FRAME_FLAGS := -DSC64_TERRAIN_SCROLL -DSC64_STATIC_KEY_CULL
build/test-sc64-t4-frame-noclear-cull: T4_FRAME_FLAGS := -DSC64_NO_FRAME_CLEAR -DSC64_STATIC_KEY_CULL
build/test-sc64-t4-frame-all: T4_FRAME_FLAGS := -DSC64_TERRAIN_SCROLL -DSC64_NO_FRAME_CLEAR -DSC64_STATIC_KEY_CULL
$(T4_FRAME_TESTS): build/test-sc64-t4-frame-%: tests/sc64_t4_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_SCENARIO_FLAGS) $(T4_FRAME_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-t4-frame-coverage
test-sc64-t4-frame-coverage: $(T4_FRAME_TESTS)
	@for fixture in $^; do ./$$fixture || exit $$?; done
build/test-sc64-resource-actions: tests/sc64_resource_actions.c starcraft/sc64_resource_actions.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-resource-actions
test-sc64-resource-actions: build/test-sc64-resource-actions
	./build/test-sc64-resource-actions
build/test-sc64-t5-actor-rules: tests/sc64_t5_actor_rules.c starcraft/sc64_t5_actor_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-t5-actor-engine: tests/sc64_t5_actor_engine.c starcraft/race_rules.h starcraft/sc64_t5_actor_adapter.h starcraft/sc64_t5_actor_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -DSC64_T4_RUNTIME -DSC64_T5_RUNTIME $< -o $@
.PHONY: test-sc64-t5-actors
test-sc64-t5-actors: build/test-sc64-t5-actor-rules build/test-sc64-t5-actor-engine
	./build/test-sc64-t5-actor-rules
	./build/test-sc64-t5-actor-engine
test-starcraft: test-sc64-t5-actors
build/test-sc64-t5-ai: tests/sc64_t5_ai.c starcraft/sc64_t5_ai.h starcraft/sc64_t4_triggers.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-t5-ai
test-sc64-t5-ai: build/test-sc64-t5-ai
	./build/test-sc64-t5-ai
test-starcraft: test-sc64-t5-ai
build/test-sc64-t5-world: tests/sc64_t5_world.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-t5-mines: tests/sc64_t5_mines.c starcraft/sc64_t5_mine_adapter.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-t5-world test-sc64-t5-mines
test-sc64-t5-world: build/test-sc64-t5-world
	./build/test-sc64-t5-world
test-sc64-t5-mines: build/test-sc64-t5-mines
	./build/test-sc64-t5-mines
test-starcraft: test-sc64-t5-mines
build/test-sc64-t5-engine: tests/sc64_t5_engine.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-t5-engine
test-sc64-t5-engine: build/test-sc64-t5-engine
	./build/test-sc64-t5-engine
build/sc64-t5-controller-probe: tests/sc64_t5_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_PROBE_FLAGS) -DSC64_T5_PROBE_ENTRY $< common/gba.c -o $@
.PHONY: sc64-t5-controller-probe
sc64-t5-controller-probe: build/sc64-t5-controller-probe
	./build/sc64-t5-controller-probe --limit 60000 --trace .local/sc64/t5/controller-probe.json
build/test-sc64-t5-save: tests/sc64_t5_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-t5-save
test-sc64-t5-save: build/test-sc64-t5-save
	./build/test-sc64-t5-save
build/test-sc64-transport-rules: tests/sc64_transport_rules.c starcraft/sc64_transport_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-transport-rules
test-sc64-transport-rules: build/test-sc64-transport-rules
	./build/test-sc64-transport-rules
test-starcraft: test-sc64-transport-rules
build/test-sc64-unit-availability: tests/sc64_unit_availability.c starcraft/sc64_unit_availability.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-unit-availability
test-sc64-unit-availability: build/test-sc64-unit-availability
	./build/test-sc64-unit-availability
test-starcraft: test-sc64-unit-availability
$(T5_AVAILABILITY_HEADER): tools/sc64_t5_availability.py .local/sc64/races/generated/t5/manifest.json .local/sc64/races/generated/t5/records/PUNI.bin
	$(PYTHON) tools/sc64_t5_availability.py --out $@
.PHONY: sc64-t5-availability
sc64-t5-availability: $(T5_AVAILABILITY_HEADER)
build/test-sc64-t5-availability-engine: tests/sc64_t5_availability_engine.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-t5-availability-engine
test-sc64-t5-availability-engine: build/test-sc64-t5-availability-engine
	./build/test-sc64-t5-availability-engine
build/test-sc64-t5-render: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-t5-render
test-sc64-t5-render: build/test-sc64-t5-render
	./build/test-sc64-t5-render
build/test-sc64-t5-world build/test-sc64-t5-engine build/sc64-t5-controller-probe build/test-sc64-t5-save build/test-sc64-t5-tech build/test-sc64-transport-engine build/test-sc64-transport-queries build/test-sc64-t5-combat build/starcraft-t5-engine-probe.elf build/starcraft-t5-controller-replay.elf: $(T5_AVAILABILITY_HEADER)
build/test-sc64-t5-tech: tests/sc64_t5_tech.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-t5-tech
test-sc64-t5-tech: build/test-sc64-t5-tech
	./build/test-sc64-t5-tech
build/test-sc64-transport-engine: tests/sc64_transport_engine.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-transport-engine
test-sc64-transport-engine: build/test-sc64-transport-engine
	./build/test-sc64-transport-engine
build/test-sc64-transport-queries: tests/sc64_transport_queries.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-transport-queries
test-sc64-transport-queries: build/test-sc64-transport-queries
	./build/test-sc64-transport-queries
build/test-sc64-t5-combat: tests/sc64_t5_combat.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-t5-combat
test-sc64-t5-combat: build/test-sc64-t5-combat
	./build/test-sc64-t5-combat
build/starcraft-t5-engine-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T5_PROBE_FLAGS) -DSC64_T5_PROBE_ENTRY $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-t5-engine-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-T5-Engine-Probe.gba: build/starcraft-t5-engine-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC T5 PROBE' SCT5
.PHONY: sc64-t5-engine-probe
sc64-t5-engine-probe: build/StarCraft-T5-Engine-Probe.gba
build/starcraft-t5-controller-replay.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T5_CONTROLLER_REPLAY_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-t5-controller-replay.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-T5-Controller-Replay.gba: build/starcraft-t5-controller-replay.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC T5 MENU' SCT5
.PHONY: sc64-t5-controller-replay
sc64-t5-controller-replay: build/StarCraft-T5-Controller-Replay.gba
build/starcraft-t5-combined.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T5_CONTROLLER_REPLAY_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-t5-combined.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Terran-T5-GBA.gba: build/starcraft-t5-combined.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC TERRAN T5' SCT5
.PHONY: terran-t5
terran-t5: build/StarCraft-Terran-T5-GBA.gba
define T5_COMBINED_TEST
build/test-sc64-t5-$(1): $(2) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_CONTROLLER_REPLAY_FLAGS) $$< common/gba.c -o $$@
endef
$(eval $(call T5_COMBINED_TEST,t3-run,tests/sc64_t3_run.c))
$(eval $(call T5_COMBINED_TEST,t3-save,tests/sc64_t3_save.c))
$(eval $(call T5_COMBINED_TEST,t3-resume,tests/sc64_t3_resume.c))
$(eval $(call T5_COMBINED_TEST,t4-run,tests/sc64_t4_campaign_run.c))
$(eval $(call T5_COMBINED_TEST,t4-save,tests/sc64_t4_scenario_save.c))
$(eval $(call T5_COMBINED_TEST,skirmish,tests/sc64_split_run.c))
.PHONY: test-sc64-t5-combined
test-sc64-t5-combined: $(addprefix build/test-sc64-t5-,t3-run t3-save t3-resume t4-run t4-save skirmish)
	mkdir -p .local/sc64/t5
	./build/test-sc64-t5-t3-run
	./build/test-sc64-t5-t3-save
	./build/test-sc64-t5-t3-resume
	./build/test-sc64-t5-t4-run --limit 30000 --trace .local/sc64/t5/combined-t4-controller.json
	./build/test-sc64-t5-t4-save
	./build/test-sc64-t5-skirmish
build/starcraft-t6-storage-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T6_STORAGE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-t6-storage-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-T6-Storage-Probe.gba: build/starcraft-t6-storage-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC T6 STORE' SCT6
.PHONY: sc64-t6-prepare sc64-t6-storage-probe
sc64-t6-prepare:
	$(PYTHON) tools/sc64_races_pack.py
	$(PYTHON) tools/sc64_t4_scenario_pack.py --profile t6
	$(PYTHON) tools/sc64_t5_availability.py --profile t6
	$(PYTHON) tools/sc64_race_sprites.py --include-t6
sc64-t6-storage-probe: build/StarCraft-T6-Storage-Probe.gba
build/starcraft-t6-engine-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T6_RUNTIME_FLAGS) -DSC64_T6_PROBE_ENTRY $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-t6-engine-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-T6-Engine-Probe.gba: build/starcraft-t6-engine-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC T6 WORLD' SCT6
.PHONY: sc64-t6-engine-probe
sc64-t6-engine-probe: build/StarCraft-T6-Engine-Probe.gba
build/starcraft-terran-late-storage.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(LATE_TERRAN_STORAGE_FLAGS) -DSC64_T6_PROBE_ENTRY $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-terran-late-storage.map -lgcc -o $@
build/StarCraft-Terran-Late-Storage.gba: build/starcraft-terran-late-storage.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC T LATE' SCT7
.PHONY: sc64-terran-late-storage
sc64-terran-late-storage: build/StarCraft-Terran-Late-Storage.gba
.PHONY: sc64-terran-late-prepare sc64-terran-late-engine
sc64-terran-late-prepare:
	$(PYTHON) tools/sc64_races_pack.py --out $(LATE_TERRAN_STAGE)/sc64_races_generated.h
	$(PYTHON) tools/sc64_race_sprites.py --include-t6 --include-late-terran --out $(LATE_TERRAN_STAGE)/sprites --header $(LATE_TERRAN_STAGE)/sc64_race_sprites_generated.h
build/starcraft-terran-late-engine.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(LATE_TERRAN_HEADERS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(LATE_TERRAN_RUNTIME_FLAGS) -DSC64_TERRAN_LATE_PROBE_MISSION=10 $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-terran-late-engine.map -lgcc -o $@
build/StarCraft-Terran-Late-Engine.gba: build/starcraft-terran-late-engine.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC T LATE' SCT7
sc64-terran-late-engine: build/StarCraft-Terran-Late-Engine.gba
build/test-sc64-terran-late-world: tests/sc64_terran_late_world.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(LATE_TERRAN_HEADERS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(LATE_TERRAN_RUNTIME_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-terran-late-world
test-sc64-terran-late-world: build/test-sc64-terran-late-world
	./build/test-sc64-terran-late-world
build/test-sc64-terran-late-save: tests/sc64_terran_late_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(LATE_TERRAN_HEADERS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(LATE_TERRAN_RUNTIME_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-terran-late-save
test-sc64-terran-late-save: build/test-sc64-terran-late-save
	./build/test-sc64-terran-late-save
define T6_RUNTIME_TEST
build/test-sc64-t6-$(1): tests/sc64_t6_$(1).c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T6_RUNTIME_FLAGS) $$< common/gba.c -o $$@
.PHONY: test-sc64-t6-$(1)
test-sc64-t6-$(1): build/test-sc64-t6-$(1)
	./build/test-sc64-t6-$(1)
endef
$(eval $(call T6_RUNTIME_TEST,engine))
$(eval $(call T6_RUNTIME_TEST,world))
$(eval $(call T6_RUNTIME_TEST,save))
$(eval $(call T6_RUNTIME_TEST,tech))
build/test-sc64-compact-unit-save: tests/sc64_compact_unit_save.c starcraft/sc64_compact_unit_save.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-compact-unit-save
test-sc64-compact-unit-save: build/test-sc64-compact-unit-save
	./build/test-sc64-compact-unit-save
test-starcraft: test-sc64-compact-unit-save
build/test-sc64-terran-late-runtime: tests/sc64_terran_late_runtime.c starcraft/sc64_terran_late_runtime.h $(wildcard starcraft/sc64_t4_*.h) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-terran-late-runtime test-sc64-terran-late-actors
test-sc64-terran-late-runtime: build/test-sc64-terran-late-runtime
	./build/test-sc64-terran-late-runtime
build/test-sc64-terran-late-actors: tests/sc64_terran_late_actors.c starcraft/race_rules.h starcraft/sc64_terran_late_actor_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_T4_RUNTIME -DSC64_T5_RUNTIME -DSC64_T6_ENABLED -DSC64_TERRAN_LATE_ACTORS $< -o $@
test-sc64-terran-late-actors: build/test-sc64-terran-late-actors
	./build/test-sc64-terran-late-actors
test-starcraft: test-sc64-terran-late-runtime test-sc64-terran-late-actors
build/test-sc64-t6-repair: tests/sc64_t6_repair.c starcraft/sc64_t6_repair_adapter.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-t6-repair
test-sc64-t6-repair: build/test-sc64-t6-repair
	./build/test-sc64-t6-repair
test-starcraft: test-sc64-t6-repair
build/test-sc64-terran-late-state: tests/sc64_terran_late_mission_state.c starcraft/sc64_terran_late_mission_state.h starcraft/sc64_terran_late_runtime.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-terran-late-state
test-sc64-terran-late-state: build/test-sc64-terran-late-state
	./build/test-sc64-terran-late-state
test-starcraft: test-sc64-terran-late-state
build/test-sc64-late-terran-targets: tests/sc64_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(LATE_TERRAN_STORAGE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-late-terran-targets
test-sc64-late-terran-targets: build/test-sc64-late-terran-targets
	./build/test-sc64-late-terran-targets
build/test-sc64-late-terran-storage: tests/sc64_late_terran_storage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(LATE_TERRAN_STORAGE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-late-terran-storage
test-sc64-late-terran-storage: build/test-sc64-late-terran-storage
	./build/test-sc64-late-terran-storage
build/test-sc64-t6-import: tests/sc64_t6_import.c $(RACES_INPUTS) $(wildcard starcraft/*.h) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $(T6_STORAGE_FLAGS) $< -o $@
build/test-sc64-t6-scenario: tests/sc64_t6_scenario.c $(T6_SCENARIO_HEADER) $(wildcard starcraft/*.h) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -Icommon -Istarcraft -I.local/sc64/races/generated -DSC64_T4_RUNTIME -DSC64_T6_ENABLED $< -o $@
.PHONY: sc64-t6-scenario test-sc64-t6-import test-sc64-t6-scenario
sc64-t6-scenario:
	$(PYTHON) tools/sc64_t4_scenario_pack.py --profile t6
test-sc64-t6-import: build/test-sc64-t6-import
	./build/test-sc64-t6-import
test-sc64-t6-scenario: build/test-sc64-t6-scenario
	./build/test-sc64-t6-scenario
$(T6_AVAILABILITY_HEADER): tools/sc64_t5_availability.py .local/sc64/races/generated/t6/manifest.json $(T6_SCENARIO_RECORDS)/PUNI.bin
	$(PYTHON) tools/sc64_t5_availability.py --profile t6 --out $@
build/test-sc64-t6-availability: tests/sc64_t6_availability.c starcraft/sc64_unit_availability.h $(T6_AVAILABILITY_HEADER) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Istarcraft -I.local/sc64/races/generated $< -o $@
.PHONY: sc64-t6-availability test-sc64-t6-availability
sc64-t6-availability: $(T6_AVAILABILITY_HEADER)
test-sc64-t6-availability: build/test-sc64-t6-availability
	./build/test-sc64-t6-availability $(T6_SCENARIO_RECORDS)/PUNI.bin
build/test-sc64-t6-runtime-synthetic: tests/sc64_t6_runtime_synthetic.c starcraft/sc64_t6_runtime.h starcraft/sc64_t4_triggers.h starcraft/sc64_trigger_types.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -DSC64_T4_RUNTIME -DSC64_T6_RUNTIME $< -o $@
build/test-sc64-t6-runtime: tests/sc64_t6_runtime.c starcraft/sc64_t6_runtime.h starcraft/sc64_t6_scenario_adapter.h $(T6_SCENARIO_HEADER) starcraft/sc64_t4_triggers.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -Istarcraft -I.local/sc64/races/generated -DSC64_T4_RUNTIME -DSC64_T6_RUNTIME -DSC64_T6_ENABLED $< -o $@
.PHONY: test-sc64-t6-runtime-synthetic test-sc64-t6-runtime
test-sc64-t6-runtime-synthetic: build/test-sc64-t6-runtime-synthetic
	./build/test-sc64-t6-runtime-synthetic
test-sc64-t6-runtime: build/test-sc64-t6-runtime
	./build/test-sc64-t6-runtime
test-starcraft: test-sc64-t6-runtime-synthetic
build/test-sc64-compact-building-save: tests/sc64_compact_building_save.c starcraft/sc64_compact_building_save.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-compact-building-save
test-sc64-compact-building-save: build/test-sc64-compact-building-save
	./build/test-sc64-compact-building-save
test-starcraft: test-sc64-compact-building-save
build/test-sc64-t6-storage: tests/sc64_t6_storage_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T6_STORAGE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-t6-storage
test-sc64-t6-storage: build/test-sc64-t6-storage
	./build/test-sc64-t6-storage
define T6_STORAGE_TEST
build/test-sc64-t6-$(1): $(2) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T6_STORAGE_FLAGS) $(3) $$< common/gba.c -o $$@
endef
$(eval $(call T6_STORAGE_TEST,t3-run,tests/sc64_t3_run.c))
$(eval $(call T6_STORAGE_TEST,t3-save,tests/sc64_t3_save.c))
$(eval $(call T6_STORAGE_TEST,t3-resume,tests/sc64_t3_resume.c))
$(eval $(call T6_STORAGE_TEST,t4-run,tests/sc64_t4_campaign_run.c))
$(eval $(call T6_STORAGE_TEST,t4-save,tests/sc64_t4_scenario_save.c))
$(eval $(call T6_STORAGE_TEST,t5-save,tests/sc64_t5_save.c))
$(eval $(call T6_STORAGE_TEST,skirmish,tests/sc64_split_run.c))
$(eval $(call T6_STORAGE_TEST,t5-run,tests/sc64_t5_campaign_run.c,-DSC64_T5_PROBE_ENTRY))
.PHONY: test-sc64-t6-combined
test-sc64-t6-combined: $(addprefix build/test-sc64-t6-,t3-run t3-save t3-resume t4-run t4-save t5-save skirmish t5-run)
	mkdir -p .local/sc64/t6
	./build/test-sc64-t6-t3-run
	./build/test-sc64-t6-t3-save
	./build/test-sc64-t6-t3-resume
	./build/test-sc64-t6-t4-run --limit 30000 --trace .local/sc64/t6/storage-t4-controller.json
	./build/test-sc64-t6-t4-save
	./build/test-sc64-t6-t5-save
	./build/test-sc64-t6-skirmish
	./build/test-sc64-t6-t5-run --limit 200000 --trace .local/sc64/t6/storage-t5-controller.json
build/starcraft-t5-profile.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T5_CONTROLLER_REPLAY_FLAGS) -DSC64_DIAGNOSTIC_UPDATE $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-t5-profile.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-T5-Profile.gba: build/starcraft-t5-profile.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC T5 PROF' SCT5
.PHONY: sc64-t5-profile
sc64-t5-profile: build/StarCraft-T5-Profile.gba
.PHONY: test-sc64-t5-actor-rules
test-sc64-t5-actor-rules: build/test-sc64-t5-actor-rules
	./build/test-sc64-t5-actor-rules
build/test-sc64-t6-actors-on: tests/sc64_t6_actors.c starcraft/race_rules.h starcraft/sc64_t6_actor_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_T4_RUNTIME -DSC64_T5_RUNTIME -DSC64_T6_ENABLED $< -o $@
build/test-sc64-t6-actors-off: tests/sc64_t6_actors.c starcraft/race_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_T4_RUNTIME -DSC64_T5_RUNTIME $< -o $@
.PHONY: test-sc64-t6-actors
test-sc64-t6-actors: build/test-sc64-t6-actors-on build/test-sc64-t6-actors-off
	./build/test-sc64-t6-actors-on
	./build/test-sc64-t6-actors-off
test-starcraft: test-sc64-t6-actors
build/test-sc64-t4-actor-rules: tests/sc64_t4_actor_rules.c starcraft/race_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_T4_RUNTIME -I. $< -o $@
build/test-sc64-t4-world: tests/sc64_t4_world.c starcraft/sc64_t4_world.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I. $< -o $@
build/test-sc64-t4-roster: tests/sc64_t4_roster.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_STORAGE_FLAGS) $< common/gba.c -o $@
build/test-sc64-t4-actor-engine: tests/sc64_t4_actor_engine.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_STORAGE_FLAGS) $< common/gba.c -o $@
build/test-sc64-t4-raynor-combat: tests/sc64_t4_raynor_combat.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_STORAGE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-t4-raynor-combat
test-sc64-t4-raynor-combat: build/test-sc64-t4-raynor-combat build/test-race-rules build/test-race-rules-z3
	./build/test-sc64-t4-raynor-combat
	./build/test-race-rules
	./build/test-race-rules-z3
build/test-sc64-t4-rescue: tests/sc64_t4_rescue.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_STORAGE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-t4-actors
test-sc64-t4-actors: build/test-sc64-t4-actor-rules build/test-sc64-t4-world build/test-sc64-t4-roster build/test-sc64-t4-actor-engine build/test-sc64-t4-rescue
	./build/test-sc64-t4-actor-rules
	./build/test-sc64-t4-world
	./build/test-sc64-t4-roster
	./build/test-sc64-t4-actor-engine
	./build/test-sc64-t4-rescue
build/test-sc64-p3-scourge: tests/sc64_p3_scourge.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(LARGE_PROBE_FLAGS) -DSC64_P3_ENABLED $< common/gba.c -o $@
test-sc64-p3-scourge: build/test-sc64-p3-scourge
	./build/test-sc64-p3-scourge
build/test-sc64-large-map: tests/sc64_large_map.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(LARGE_PROBE_FLAGS) $< common/gba.c -o $@
test-sc64-large-map: build/test-sc64-large-map build/test-race-routes-large
	./build/test-sc64-large-map
	./build/test-race-routes-large
build/starcraft-large-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(LARGE_PROBE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-large-probe.map -lgcc -o $@
	$(SIZE) $@
sc64-large-probe: build/starcraft-large-probe.elf

.PHONY: test-sc64-race-collision
build/test-sc64-race-collision: tests/sc64_race_collision.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) $< common/gba.c -o $@
test-sc64-race-collision: build/test-sc64-race-collision
	./build/test-sc64-race-collision

build/test-race-rules-z3: tests/race_rules.c starcraft/race_rules.h starcraft/race_skirmish.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_Z3_ENABLED tests/race_rules.c -o $@

.PHONY: test-sc64-z3-import
build/test-sc64-z3-import: tests/sc64_z3_import.c starcraft/race_rules.h starcraft/sc64_types.h .local/sc64/races/generated/sc64_races_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_Z3_ENABLED -DSC64_CAMPAIGN_RACE=1 -Istarcraft -I.local/sc64/races/generated tests/sc64_z3_import.c -o $@
test-sc64-z3-import: build/test-sc64-z3-import
	./build/test-sc64-z3-import

.PHONY: test-sc64-t3-import
build/test-sc64-t3-import: tests/sc64_t3_import.c starcraft/race_rules.h starcraft/sc64_types.h .local/sc64/races/generated/sc64_races_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_T3_ENABLED -DSC64_CAMPAIGN_RACE=0 -Istarcraft -I.local/sc64/races/generated tests/sc64_t3_import.c -o $@
test-sc64-t3-import: build/test-sc64-t3-import
	./build/test-sc64-t3-import

# Staged T3 runtime remains opt-in until controller and timing checks pass.
.PHONY: terran-t3 test-sc64-t3-runtime
build/starcraft-terran-t3.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T3_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-terran-t3.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Terran-T3-GBA.gba: build/starcraft-terran-t3.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC TERRAN T3' SCT3
terran-t3: build/StarCraft-Terran-T3-GBA.gba
	$(PYTHON) tools/verify_roms.py --terran-t3
build/test-sc64-t3-run: tests/sc64_t3_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(T3_FLAGS) $< common/gba.c -o $@
build/test-sc64-t3-save: tests/sc64_t3_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(T3_FLAGS) $< common/gba.c -o $@
build/test-sc64-t3-targets: tests/sc64_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(T3_FLAGS) $< common/gba.c -o $@
build/test-sc64-t3-resume: tests/sc64_t3_resume.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(T3_FLAGS) $< common/gba.c -o $@
build/test-sc64-t3-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(T3_FLAGS) $< common/gba.c -o $@
test-sc64-t3-runtime: test-sc64-t3-import build/test-sc64-t3-run build/test-sc64-t3-save build/test-sc64-t3-targets build/test-sc64-t3-resume build/test-sc64-t3-hud
	./build/test-sc64-t3-targets
	./build/test-sc64-t3-save
	./build/test-sc64-t3-resume
	./build/test-sc64-t3-hud
	./build/test-sc64-t3-run

# Combined T3/T4 host coverage: exercise the T3 campaign with the larger
# shared pool and packed 128x128 terrain enabled by the T4 profile.
.PHONY: test-sc64-t3t4-runtime
build/test-sc64-t3t4-run: tests/sc64_t3_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_COMBINED_FLAGS) $< common/gba.c -o $@
build/test-sc64-t3t4-t3-save: tests/sc64_t3_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_COMBINED_FLAGS) $< common/gba.c -o $@
build/test-sc64-t3t4-targets: tests/sc64_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_COMBINED_FLAGS) $< common/gba.c -o $@
build/test-sc64-t3t4-resume: tests/sc64_t3_resume.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_COMBINED_FLAGS) $< common/gba.c -o $@
build/test-sc64-t3t4-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_COMBINED_FLAGS) $< common/gba.c -o $@
test-sc64-t3t4-runtime: build/test-sc64-t3t4-targets build/test-sc64-t3t4-t3-save build/test-sc64-t3t4-resume build/test-sc64-t3t4-hud build/test-sc64-t3t4-run
	./build/test-sc64-t3t4-targets
	./build/test-sc64-t3t4-t3-save
	./build/test-sc64-t3t4-resume
	./build/test-sc64-t3t4-hud
	./build/test-sc64-t3t4-run

# New Dominion is the canonical Zerg campaign ROM; legacy target paths alias it.
.PHONY: zerg-z3 test-sc64-z3-runtime
# Compatibility filenames point at the canonical campaign ROM/ELF; keep the
# replay tools working without compiling or storing a second Z3 profile.
build/starcraft-zerg-z3.elf: build/starcraft-zerg.elf | build
	ln -sf starcraft-zerg.elf $@
build/StarCraft-Zerg-Z3-GBA.gba: build/StarCraft-Zerg-GBA.gba | build
	ln -sf StarCraft-Zerg-GBA.gba $@
zerg-z3: verify-zerg build/starcraft-zerg-z3.elf build/StarCraft-Zerg-Z3-GBA.gba
build/test-sc64-z3-run: tests/sc64_z3_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
build/test-sc64-z3-save: tests/sc64_z3_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
build/test-sc64-z3-controller: tests/sc64_z3_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
build/test-sc64-z3-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
build/test-sc64-z3-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
test-sc64-z3-runtime: test-sc64-z3-import build/test-sc64-z3-run build/test-sc64-z3-save build/test-sc64-z3-controller build/test-sc64-z3-hud build/test-sc64-z3-frame-coverage
	./build/test-sc64-z3-run
	./build/test-sc64-z3-save
	./build/test-sc64-z3-hud
	./build/test-sc64-z3-frame-coverage
	./build/test-sc64-z3-controller --policy hybrid --trace .local/sc64/z3-controller-asan-win.json

build/test-sc64-frontend-terran: RACE_FRONTEND_ID=0
build/test-sc64-frontend-zerg: RACE_FRONTEND_ID=1
build/test-sc64-frontend-protoss: RACE_FRONTEND_ID=2
RACE_FRONTEND_TESTS := build/test-sc64-frontend-terran build/test-sc64-frontend-zerg build/test-sc64-frontend-protoss
$(RACE_FRONTEND_TESTS): tests/sc64_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_IMPORTED_SKIRMISH -DSC64_CAMPAIGN_RACE=$(RACE_FRONTEND_ID) -Icommon tests/sc64_frontend.c -o $@

build/test-sc64-frontend-t3-terran: RACE_FRONTEND_ID=0
build/test-sc64-frontend-t3-zerg: RACE_FRONTEND_ID=1
build/test-sc64-frontend-t3-protoss: RACE_FRONTEND_ID=2
RACE_T3_FRONTEND_TESTS := build/test-sc64-frontend-t3-terran build/test-sc64-frontend-t3-zerg build/test-sc64-frontend-t3-protoss
$(RACE_T3_FRONTEND_TESTS): tests/sc64_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_T3_ENABLED -DSC64_IMPORTED_SKIRMISH -DSC64_CAMPAIGN_RACE=$(RACE_FRONTEND_ID) -Icommon tests/sc64_frontend.c -o $@
build/test-sc64-frontend-t3-unified: tests/sc64_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_T3_ENABLED -Icommon tests/sc64_frontend.c -o $@

build/test-sc64-frontend-z3-terran build/test-sc64-frontend-z3-t3-terran: RACE_FRONTEND_ID=0
build/test-sc64-frontend-z3-zerg build/test-sc64-frontend-z3-t3-zerg: RACE_FRONTEND_ID=1
build/test-sc64-frontend-z3-protoss build/test-sc64-frontend-z3-t3-protoss: RACE_FRONTEND_ID=2
RACE_Z3_FRONTEND_TESTS := build/test-sc64-frontend-z3-terran build/test-sc64-frontend-z3-zerg build/test-sc64-frontend-z3-protoss build/test-sc64-frontend-z3-t3-terran build/test-sc64-frontend-z3-t3-zerg build/test-sc64-frontend-z3-t3-protoss
RACE_Z3_UNIFIED_TESTS := build/test-sc64-frontend-z3-unified build/test-sc64-frontend-z3-t3-unified
build/test-sc64-frontend-z3-t3-terran build/test-sc64-frontend-z3-t3-zerg build/test-sc64-frontend-z3-t3-protoss build/test-sc64-frontend-z3-t3-unified: Z3_FRONT_EXTRA=-DSC64_T3_ENABLED
$(RACE_Z3_FRONTEND_TESTS): tests/sc64_z3_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_Z3_ENABLED -DSC64_IMPORTED_SKIRMISH -DSC64_CAMPAIGN_RACE=$(RACE_FRONTEND_ID) $(Z3_FRONT_EXTRA) -Icommon $< -o $@
$(RACE_Z3_UNIFIED_TESTS): tests/sc64_z3_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_UNIFIED -DSC64_Z3_ENABLED -DSC64_IMPORTED_SKIRMISH $(Z3_FRONT_EXTRA) -Icommon $< -o $@

build/test-race-routes: tests/race_routes.c starcraft/sc64_routes.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I. tests/race_routes.c -o $@

build/test-race-routes-96: tests/race_routes.c starcraft/sc64_routes.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I. -DRACE_TEST_COLS=96 -DRACE_TEST_ROWS=96 tests/race_routes.c -o $@

build/test-race-routes-t3: tests/race_routes.c starcraft/sc64_routes.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I. -DSC64_T3_ENABLED -DRACE_TEST_COLS=96 -DRACE_TEST_ROWS=96 $< -o $@

build/test-race-routes-rect: tests/race_routes.c starcraft/sc64_routes.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I. -DRACE_TEST_COLS=96 -DRACE_TEST_ROWS=64 tests/race_routes.c -o $@

build/test-race-routes-large: tests/race_routes.c starcraft/sc64_routes.h starcraft/sc64_bitgrid.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I. -Icommon -DSC64_LARGE_MAPS -DRACE_TEST_COLS=128 -DRACE_TEST_ROWS=96 $< -o $@

build/test-race-routes-pack3: tests/race_routes.c starcraft/sc64_routes.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I. -DSC64_ROUTE_PACK3 $< -o $@

build/test-race-routes-pack3-t3: tests/race_routes.c starcraft/sc64_routes.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I. -DSC64_ROUTE_PACK3 -DSC64_T3_ENABLED -DRACE_TEST_COLS=96 -DRACE_TEST_ROWS=96 $< -o $@

build/test-race-routes-pack3-large: tests/race_routes.c starcraft/sc64_routes.h starcraft/sc64_bitgrid.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I. -Icommon -DSC64_ROUTE_PACK3 -DSC64_LARGE_MAPS -DRACE_TEST_COLS=128 -DRACE_TEST_ROWS=96 $< -o $@

build/test-terrain-cache: tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/sc64_terrain_cache.c -o $@
build/test-terrain-scroll: tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_TERRAIN_CACHE_REAL_GFX -DSC64_TERRAIN_SCROLL -Icommon $< common/gba.c -o $@

build/test-collision: tests/sc64_collision.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -DSC64_SKIRMISH -Icommon tests/sc64_collision.c common/gba.c -o $@

test-starcraft: build/test-sc64-resource-actions build/test-sc64-t5-actor-rules build/test-sc64-t4-actor-rules build/test-sc64-t4-world build/test-sc64-t4-schedule build/test-sc64-t4-count-cache build/test-sc64-p3-schedule build/test-sc64-p3-runtime $(P3_FRONT_TESTS) build/test-sc64-demo-schedule build/test-sc64-demo-schedule-runtime build/test-starcraft build/test-starcraft-selection build/test-race-rules build/test-race-rules-z3 build/test-skirmish-ai build/test-sc64-frontend build/test-sc64-bitgrid build/test-sc64-map-math build/test-sc64-t3-rules build/test-sc64-z3-rules build/test-sc64-p3-rules build/test-sc64-p3-model $(RACE_FRONTEND_TESTS) $(RACE_T3_FRONTEND_TESTS) $(RACE_Z3_FRONTEND_TESTS) $(RACE_Z3_UNIFIED_TESTS) build/test-sc64-frontend-t3-unified build/test-race-routes build/test-race-routes-96 build/test-race-routes-t3 build/test-race-routes-rect build/test-race-routes-large build/test-race-routes-pack3 build/test-race-routes-pack3-t3 build/test-race-routes-pack3-large build/test-terrain-cache build/test-terrain-scroll build/test-collision
	./build/test-sc64-resource-actions
	./build/test-sc64-t5-actor-rules
	./build/test-sc64-t4-actor-rules
	./build/test-sc64-t4-world
	./build/test-sc64-t4-triggers
	./build/test-sc64-t4-queries
	./build/test-sc64-t4-actions
	./build/test-sc64-t4-count-cache
	./build/test-sc64-t4-schedule
	./build/test-sc64-p3-schedule
	./build/test-sc64-p3-runtime
	@for test in $(P3_FRONT_TESTS); do ./$$test || exit $$?; done
	./build/test-sc64-demo-schedule
	./build/test-sc64-demo-schedule-runtime
	./build/test-starcraft
	./build/test-starcraft-selection
	./build/test-race-rules
	./build/test-race-rules-z3
	./build/test-skirmish-ai
	./build/test-sc64-frontend
	./build/test-sc64-bitgrid
	./build/test-sc64-map-math
	./build/test-sc64-t3-rules
	./build/test-sc64-z3-rules
	./build/test-sc64-p3-rules
	./build/test-sc64-p3-model
	./build/test-sc64-frontend-terran
	./build/test-sc64-frontend-zerg
	./build/test-sc64-frontend-protoss
	./build/test-sc64-frontend-t3-terran
	./build/test-sc64-frontend-t3-zerg
	./build/test-sc64-frontend-t3-protoss
	./build/test-sc64-frontend-t3-unified
	@for check in $(RACE_Z3_FRONTEND_TESTS) $(RACE_Z3_UNIFIED_TESTS); do ./$$check || exit; done
	./build/test-race-routes
	./build/test-race-routes-96
	./build/test-race-routes-t3
	./build/test-race-routes-rect
	./build/test-race-routes-large
	./build/test-race-routes-pack3
	./build/test-race-routes-pack3-t3
	./build/test-race-routes-pack3-large
	./build/test-terrain-cache
	./build/test-terrain-scroll
	./build/test-collision
	$(PYTHON) -m unittest discover -s tests -p 'test_sc64*.py'

test: test-starcraft

probe: build/probe.gba
build/probe.elf: $(COMMON) $(COMMON_HEADERS) tests/platform_probe.c common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(COMMON) tests/platform_probe.c $(LDFLAGS) -Wl,-Map,build/probe.map -lgcc -o $@
build/probe.gba: build/probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'GBA PROBE' TSGD

# Optional profiles contain data derived from a user-supplied StarCraft 64 ROM.
# Prepare it locally with `./dev sc64 prepare --rom PATH --profile PROFILE`.
sc64: verify-sc64
.local/sc64/generated/sc64_mission_generated.h:
	@echo 'Private mission data is missing. Run ./dev sc64 prepare --rom PATH --profile wasteland first.'
	@false

build/starcraft64.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(SC64_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(SC64_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft64.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft64-Wasteland-GBA.gba: build/starcraft64.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC64 GBA' SC64
verify-sc64: build/StarCraft64-Wasteland-GBA.gba
	$(PYTHON) tools/verify_roms.py --sc64

.PHONY: test-sc64
build/test-starcraft64: tests/host.c common/gba.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SC64_INPUTS) common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(SC64_FLAGS) tests/host.c common/gba.c $(STARCRAFT) -o $@
build/test-sc64-mission: tests/sc64_mission_run.c common/gba.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SC64_INPUTS) common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(SC64_FLAGS) tests/sc64_mission_run.c common/gba.c -o $@
build/test-sc64-selection: tests/sc64_selection_run.c common/gba.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SC64_INPUTS) common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(SC64_FLAGS) tests/sc64_selection_run.c common/gba.c -o $@
test-sc64: build/test-starcraft64 build/test-sc64-mission build/test-sc64-selection
	./build/test-starcraft64
	./build/test-sc64-mission --trace build/sc64-completion-input.json
	./build/test-sc64-selection

.PHONY: test-backwater-mission test-backwater-render-cache
backwater: verify-backwater
build/starcraft64-backwater.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(BACKWATER_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(BACKWATER_FLAGS) -DSC64_BACKWATER $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft64-backwater.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft64-Backwater-GBA.gba: build/starcraft64-backwater.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC64 T2' SCB2
verify-backwater: build/StarCraft64-Backwater-GBA.gba
	$(PYTHON) tools/verify_roms.py --backwater
build/test-backwater: tests/backwater_adapter_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(BACKWATER_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(BACKWATER_FLAGS) -DSC64_BACKWATER tests/backwater_adapter_run.c common/gba.c -o $@
build/test-backwater-rules: tests/backwater_rules.c starcraft/sc64_backwater.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon tests/backwater_rules.c -o $@
build/test-backwater-mission: tests/backwater_mission_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(BACKWATER_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(BACKWATER_FLAGS) -DSC64_BACKWATER tests/backwater_mission_run.c common/gba.c -o $@
build/test-backwater-render-cache: tests/backwater_render_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(BACKWATER_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(BACKWATER_FLAGS) -DSC64_BACKWATER tests/backwater_render_cache.c common/gba.c -o $@
test-backwater: build/test-backwater build/test-backwater-rules build/test-backwater-render-cache build/test-backwater-mission
	./build/test-backwater
	./build/test-backwater-rules
	./build/test-backwater-render-cache
	./build/test-backwater-mission --trace build/backwater-completion-input.json

skirmish: verify-skirmish
build/starcraft64-skirmish.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(BACKWATER_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(BACKWATER_FLAGS) -DSC64_SKIRMISH $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft64-skirmish.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft64-Skirmish-GBA.gba: build/starcraft64-skirmish.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC64 SKIRM' SCK1
verify-skirmish: build/StarCraft64-Skirmish-GBA.gba
	$(PYTHON) tools/verify_roms.py --skirmish
build/test-skirmish: tests/skirmish_adapter_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(BACKWATER_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(BACKWATER_FLAGS) -DSC64_SKIRMISH tests/skirmish_adapter_run.c common/gba.c -o $@
build/test-skirmish-match: tests/skirmish_match_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(BACKWATER_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(BACKWATER_FLAGS) -DSC64_SKIRMISH tests/skirmish_match_run.c common/gba.c -o $@
test-skirmish-match: build/test-skirmish-match
	./build/test-skirmish-match
test-skirmish: build/test-skirmish build/test-skirmish-ai
	./build/test-skirmish
	./build/test-skirmish-ai

campaign: verify-campaign
build/starcraft64-campaign.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(CAMPAIGN_FLAGS) -DSC64_BACKWATER -DSC64_TERRAN_CAMPAIGN $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft64-campaign.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft64-Terran-Campaign-GBA.gba: build/starcraft64-campaign.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC64 CAMP' SCT1
verify-campaign: build/StarCraft64-Terran-Campaign-GBA.gba
	$(PYTHON) tools/verify_roms.py --campaign
build/test-campaign-backwater-mission: tests/backwater_mission_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(CAMPAIGN_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(CAMPAIGN_FLAGS) -DSC64_BACKWATER -DSC64_TERRAN_CAMPAIGN tests/backwater_mission_run.c common/gba.c -o $@
build/test-campaign-wasteland-mission: tests/sc64_mission_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(CAMPAIGN_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(CAMPAIGN_FLAGS) -DSC64_BACKWATER -DSC64_TERRAN_CAMPAIGN tests/sc64_mission_run.c common/gba.c -o $@
build/test-campaign: tests/campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(CAMPAIGN_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(CAMPAIGN_FLAGS) -DSC64_BACKWATER -DSC64_TERRAN_CAMPAIGN tests/campaign_run.c common/gba.c -o $@
test-campaign: build/test-campaign build/test-campaign-backwater-mission build/test-campaign-wasteland-mission
	./build/test-campaign
	./build/test-campaign-backwater-mission --trace build/campaign-backwater-completion-input.json
	./build/test-campaign-wasteland-mission --trace build/campaign-wasteland-completion-input.json

races: verify-races
build/starcraft64-races.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(RACES_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft64-races.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft64-Races-GBA.gba: build/starcraft64-races.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC64 RACES' SCR3
verify-races: build/StarCraft64-Races-GBA.gba
	$(PYTHON) tools/verify_roms.py --races
build/test-race-engine: tests/race_engine.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) tests/race_engine.c common/gba.c -o $@
build/test-race-campaign: tests/race_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) tests/race_campaign_run.c common/gba.c -o $@
build/test-race-z1-controls: tests/race_z1_controls.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) tests/race_z1_controls.c common/gba.c -o $@
build/test-race-factions: tests/race_factions_engine.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) tests/race_factions_engine.c common/gba.c -o $@
test-races: build/test-race-rules build/test-race-engine build/test-race-campaign build/test-race-factions build/test-race-routes-rect
	./build/test-race-rules
	./build/test-race-engine
	./build/test-race-campaign
	./build/test-race-factions
	./build/test-race-routes-rect
test-races-controller: build/test-race-z1-controls build/test-race-campaign
	./build/test-race-z1-controls --trace .local/sc64/races/generated/z1-controller.json
	./build/test-race-campaign --controller 1 --trace .local/sc64/races/generated/z2-controller.json
	./build/test-race-campaign --controller 2 --trace .local/sc64/races/generated/p1-controller.json
	./build/test-race-campaign --controller 3 --trace .local/sc64/races/generated/p2-controller.json
build/test-sc64-menu: tests/sc64_menu_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) tests/sc64_menu_run.c common/gba.c -o $@
test-races-menu: build/test-sc64-frontend build/test-sc64-menu
	./build/test-sc64-frontend
	./build/test-sc64-menu

# Split race-campaign ROMs use the shared race engine, with isolated save IDs.
race-campaigns: verify-terran verify-zerg verify-protoss
terran: verify-terran
zerg: verify-zerg
protoss: verify-protoss
build/starcraft-terran.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(TERRAN_FULL_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-terran.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Terran-GBA.gba: build/starcraft-terran.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC TERRAN 10' SCT8
verify-terran: build/StarCraft-Terran-GBA.gba
	$(PYTHON) tools/verify_roms.py --terran
build/test-sc64-terran-late-count-cache: tests/sc64_terran_late_count_cache.c starcraft/sc64_terran_late_count_cache.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined $< -o $@
build/test-sc64-present: tests/sc64_present.c common/gba_present.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-terran-defense-targets: tests/sc64_terran_defense_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_FULL_FLAGS) $< common/gba.c -o $@
build/test-sc64-terran-full-targets: tests/sc64_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_FULL_FLAGS) $< common/gba.c -o $@
build/test-sc64-terran-full-collision: tests/sc64_race_collision.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_FULL_FLAGS) $< common/gba.c -o $@
build/test-sc64-terran-full-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_FULL_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-terran-late-count-cache test-sc64-present test-sc64-terran-defense-targets test-sc64-terran-full-targets test-sc64-terran-full-spatial-hud
test-sc64-terran-late-count-cache: build/test-sc64-terran-late-count-cache
	./$<
test-sc64-terran-defense-targets: build/test-sc64-terran-defense-targets
	./$<
test-sc64-present: build/test-sc64-present
	./$<
test-starcraft: test-sc64-terran-late-count-cache test-sc64-present

build/test-sc64-z4-model: tests/sc64_z4_model.c starcraft/sc64_z4.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $< -o $@
build/test-sc64-p4-model: tests/sc64_p4_model.c starcraft/sc64_p4.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $< -o $@
.PHONY: test-sc64-zp-models
test-sc64-zp-models: build/test-sc64-z4-model build/test-sc64-p4-model
	./build/test-sc64-z4-model
	./build/test-sc64-p4-model
test-starcraft: test-sc64-zp-models

# Staged fourth chapter; canonical Zerg stays on its qualified third chapter.
build/starcraft-zerg-z4.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z4_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z4.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z4-GBA.gba: build/starcraft-zerg-z4.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z4' SCZ4
.PHONY: zerg-z4
zerg-z4: build/StarCraft-Zerg-Z4-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z4

build/starcraft-protoss-p4.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P4_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p4.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P4-GBA.gba: build/starcraft-protoss-p4.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC PROTOSS4' SCP4
.PHONY: protoss-p4
protoss-p4: build/StarCraft-Protoss-P4-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p4

build/starcraft-protoss-p4-storm.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P4_STORM_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p4-storm.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P4-Storm-GBA.gba: build/starcraft-protoss-p4-storm.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P4 STORM' SCP5
.PHONY: protoss-p4-storm
protoss-p4-storm: build/StarCraft-Protoss-P4-Storm-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p4-storm

build/starcraft-protoss-p5.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P5_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p5.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P5-GBA.gba: build/starcraft-protoss-p5.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC PROTOSS5' SCPA
.PHONY: protoss-p5
protoss-p5: build/StarCraft-Protoss-P5-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p5

build/starcraft-protoss-p5-shuttle.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P5_SHUTTLE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p5-shuttle.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P5-Shuttle-GBA.gba: build/starcraft-protoss-p5-shuttle.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P5 CARGO' SCPB
.PHONY: protoss-p5-shuttle
protoss-p5-shuttle: build/StarCraft-Protoss-P5-Shuttle-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p5-shuttle

build/test-sc64-p5-transport-on: tests/sc64_p5_transport.c starcraft/sc64_transport_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_P5_SHUTTLE_ENABLED $< -o $@
build/test-sc64-p5-transport-off: tests/sc64_p5_transport.c starcraft/sc64_transport_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-p5-transport
test-sc64-p5-transport: build/test-sc64-p5-transport-on build/test-sc64-p5-transport-off
	./build/test-sc64-p5-transport-on
	./build/test-sc64-p5-transport-off
test-starcraft: test-sc64-p5-transport

build/test-sc64-p5-shuttle-runtime: tests/sc64_p5_shuttle_runtime.c tests/sc64_p5_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P5_SHUTTLE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p5-shuttle-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P5_SHUTTLE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p5-shuttle-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P5_SHUTTLE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p5-shuttle-runtime
test-sc64-p5-shuttle-runtime: test-sc64-p5-transport build/test-sc64-p5-shuttle-runtime build/test-sc64-p5-shuttle-profile build/test-sc64-p5-shuttle-frame-coverage
	./build/test-sc64-p5-shuttle-runtime
	./build/test-sc64-p5-shuttle-profile
	./build/test-sc64-p5-shuttle-frame-coverage

build/test-sc64-p5-campaign-run: tests/sc64_p5_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P5_SHUTTLE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p5-campaign-run
test-sc64-p5-campaign-run: build/test-sc64-p5-campaign-run
	./build/test-sc64-p5-campaign-run

build/test-sc64-p5-runtime: tests/sc64_p5_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P5_FLAGS) $< common/gba.c -o $@
build/test-sc64-p5-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P5_FLAGS) $< common/gba.c -o $@
build/test-sc64-p5-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P5_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p5-runtime
test-sc64-p5-runtime: build/test-sc64-p5-runtime build/test-sc64-p5-profile build/test-sc64-p5-frame-coverage
	./build/test-sc64-p5-runtime
	./build/test-sc64-p5-profile
	./build/test-sc64-p5-frame-coverage

build/starcraft-zerg-z5.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z5_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z5.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z5-GBA.gba: build/starcraft-zerg-z5.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z5' SCZ5
.PHONY: zerg-z5
zerg-z5: build/StarCraft-Zerg-Z5-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z5

build/starcraft-zerg-z5-cloak.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_CLOAK_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z5_CLOAK_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z5-cloak.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z5-Cloak-GBA.gba: build/starcraft-zerg-z5-cloak.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z5 CLOAK' SCZC
.PHONY: zerg-z5-cloak
zerg-z5-cloak: build/StarCraft-Zerg-Z5-Cloak-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z5-cloak

build/test-sc64-z5-cloak-runtime: tests/sc64_z5_cloak_runtime.c tests/sc64_z5_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_CLOAK_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_CLOAK_FLAGS) $< common/gba.c -o $@
build/test-sc64-z5-cloak-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_CLOAK_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_CLOAK_FLAGS) $< common/gba.c -o $@
build/test-sc64-z5-cloak-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_CLOAK_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_CLOAK_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z5-cloak-runtime
test-sc64-z5-cloak-runtime: test-sc64-z5-ability-models build/test-sc64-z5-cloak-runtime build/test-sc64-z5-cloak-profile build/test-sc64-z5-cloak-frame-coverage
	./build/test-sc64-z5-cloak-runtime
	./build/test-sc64-z5-cloak-profile
	./build/test-sc64-z5-cloak-frame-coverage

build/test-sc64-z5-runtime: tests/sc64_z5_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_FLAGS) $< common/gba.c -o $@
build/test-sc64-z5-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_FLAGS) $< common/gba.c -o $@
build/test-sc64-z5-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z5-runtime
test-sc64-z5-runtime: build/test-sc64-z5-runtime build/test-sc64-z5-profile build/test-sc64-z5-frame-coverage
	./build/test-sc64-z5-runtime
	./build/test-sc64-z5-profile
	./build/test-sc64-z5-frame-coverage

# Capacity/save-layout experiment; it does not enable later campaign entries.
build/starcraft-zp-storage-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(ZP_FULL_STORAGE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zp-storage-probe.map -lgcc -o $@
	$(SIZE) $@
.PHONY: sc64-zp-full-storage-probe
sc64-zp-full-storage-probe: build/starcraft-zp-storage-probe.elf

build/test-sc64-zp-actors-on: tests/sc64_zp_actors.c starcraft/race_rules.h starcraft/sc64_zp_actor_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_ZP_CAMPAIGN_ACTORS $< -o $@
build/test-sc64-zp-actors-off: tests/sc64_zp_actors.c starcraft/race_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-zp-actors
test-sc64-zp-actors: build/test-sc64-zp-actors-on build/test-sc64-zp-actors-off
	./build/test-sc64-zp-actors-on
	./build/test-sc64-zp-actors-off
test-starcraft: test-sc64-zp-actors

build/test-sc64-z4-adapter: tests/sc64_z4_adapter.c starcraft/sc64_z4.h starcraft/sc64_z4_adapter.h .local/sc64/z4/generated/sc64_z4_scenario_generated.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -DGBA_HOST -DSC64_Z4_RUNTIME -Icommon -I.local/sc64/z4/generated $< -o $@
build/test-sc64-p4-adapter: tests/sc64_p4_adapter.c starcraft/sc64_p4.h starcraft/sc64_p4_adapter.h .local/sc64/p4/generated/sc64_p4_scenario_generated.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -I.local/sc64/p4/generated $< -o $@
build/test-sc64-z4-runtime: tests/sc64_z4_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z4_FLAGS) $< common/gba.c -o $@
build/test-sc64-p4-runtime: tests/sc64_p4_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P4_FLAGS) $< common/gba.c -o $@
build/test-sc64-z4-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z4_FLAGS) $< common/gba.c -o $@
build/test-sc64-p4-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P4_FLAGS) $< common/gba.c -o $@
build/test-sc64-z4-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z4_FLAGS) $< common/gba.c -o $@
build/test-sc64-p4-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P4_FLAGS) $< common/gba.c -o $@
build/test-sc64-zp-actor-engine: tests/sc64_zp_actor_integration.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z4_FLAGS) $< common/gba.c -o $@
build/test-sc64-zp-zerg-storage: tests/sc64_zp_storage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z4_FLAGS) $< common/gba.c -o $@
build/test-sc64-zp-protoss-storage: tests/sc64_zp_storage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P4_FLAGS) $< common/gba.c -o $@
build/test-sc64-zp-air-tech: tests/sc64_zp_air_tech.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z4_FLAGS) $< common/gba.c -o $@
build/test-sc64-z4-campaign-run: tests/sc64_z4_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z4_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z4-runtime test-sc64-p4-runtime test-sc64-zp-storage
test-sc64-z4-runtime: build/test-sc64-z4-adapter build/test-sc64-z4-runtime build/test-sc64-zp-actor-engine build/test-sc64-zp-air-tech build/test-sc64-z4-frame-coverage build/test-sc64-z4-profile
	./build/test-sc64-z4-adapter
	./build/test-sc64-zp-actor-engine
	./build/test-sc64-zp-air-tech
	./build/test-sc64-z4-runtime
	./build/test-sc64-z4-frame-coverage
	./build/test-sc64-z4-profile
test-sc64-p4-runtime: build/test-sc64-p4-adapter build/test-sc64-p4-runtime build/test-sc64-p4-frame-coverage build/test-sc64-p4-profile
	./build/test-sc64-p4-adapter
	./build/test-sc64-p4-runtime
	./build/test-sc64-p4-frame-coverage
	./build/test-sc64-p4-profile
test-sc64-zp-storage: build/test-sc64-zp-zerg-storage build/test-sc64-zp-protoss-storage
	./build/test-sc64-zp-zerg-storage
	./build/test-sc64-zp-protoss-storage

# Compare indexed target results with a full-array scan for every race build.
build/test-sc64-z3-occupancy-targets: tests/sc64_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-occupancy-targets: tests/sc64_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-z4-occupancy-targets: tests/sc64_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z4_FLAGS) $< common/gba.c -o $@
build/test-sc64-p4-occupancy-targets: tests/sc64_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P4_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-race-occupancy-targets
test-sc64-race-occupancy-targets: build/test-sc64-z3-occupancy-targets build/test-sc64-p3-occupancy-targets build/test-sc64-z4-occupancy-targets build/test-sc64-p4-occupancy-targets
	./build/test-sc64-z3-occupancy-targets
	./build/test-sc64-p3-occupancy-targets
	./build/test-sc64-z4-occupancy-targets
	./build/test-sc64-p4-occupancy-targets

build/test-sc64-psionic-storm: tests/sc64_psionic_storm.c starcraft/sc64_psionic_storm.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-psionic-storm
test-sc64-psionic-storm: build/test-sc64-psionic-storm
	./build/test-sc64-psionic-storm
test-starcraft: test-sc64-psionic-storm
build/test-sc64-p4-storm: tests/sc64_p4_storm.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P4_STORM_FLAGS) $< common/gba.c -o $@
build/test-sc64-p4-storm-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P4_STORM_FLAGS) $< common/gba.c -o $@
build/test-sc64-p4-storm-storage: tests/sc64_zp_storage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P4_STORM_FLAGS) $< common/gba.c -o $@
build/test-sc64-p4-storm-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P4_STORM_FLAGS) $< common/gba.c -o $@
build/test-sc64-p4-campaign-run: tests/sc64_p4_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P4_STORM_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p4-campaign
test-sc64-p4-campaign: test-sc64-p4-storm build/test-sc64-p4-campaign-run
	./build/test-sc64-p4-campaign-run --limit 180000 --attempt-tag make-qualified
.PHONY: test-sc64-p4-storm
test-sc64-p4-storm: test-sc64-psionic-storm build/test-sc64-p4-storm build/test-sc64-p4-storm-profile build/test-sc64-p4-storm-storage build/test-sc64-p4-storm-frame-coverage
	./build/test-sc64-p4-storm
	./build/test-sc64-p4-storm-profile
	./build/test-sc64-p4-storm-storage
	./build/test-sc64-p4-storm-frame-coverage
.PHONY: test-sc64-z4-campaign
test-sc64-z4-campaign: test-sc64-z4-runtime build/test-sc64-z4-campaign-run
	./build/test-sc64-z4-campaign-run --limit 150000 --trace .local/sc64/z4-controller-win-asan.json

# Chapter-five models are asset-free; adapters additionally require the
# authenticated numeric headers from sc64_zp_scenario_pack.py.
build/test-sc64-z5-rules: tests/sc64_z5.c starcraft/sc64_z5.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-p5-rules: tests/sc64_p5.c starcraft/sc64_p5.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-zp-chapter-five-models test-sc64-z5-adapter test-sc64-p5-adapter
test-sc64-zp-chapter-five-models: build/test-sc64-z5-rules build/test-sc64-p5-rules
	./build/test-sc64-z5-rules
	./build/test-sc64-p5-rules
test-starcraft: test-sc64-zp-chapter-five-models
build/test-sc64-z5-adapter: tests/sc64_z5_adapter.c starcraft/sc64_z5.h starcraft/sc64_z5_adapter.h .local/sc64/zp-stage/generated/scenarios/sc64_z5_scenario_generated.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
build/test-sc64-p5-adapter: tests/sc64_p5_adapter.c starcraft/sc64_p5.h starcraft/sc64_p5_adapter.h .local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
test-sc64-z5-adapter: build/test-sc64-z5-adapter
	./build/test-sc64-z5-adapter
test-sc64-p5-adapter: build/test-sc64-p5-adapter
	./build/test-sc64-p5-adapter

build/test-sc64-p5-source-model: tests/sc64_p5_source_model.c starcraft/sc64_p5_source_model.h .local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
build/test-sc64-p5-world: tests/sc64_p5_world.c starcraft/sc64_p5_world_adapter.h starcraft/sc64_p5_source_model.h starcraft/sc64_p5_adapter.h starcraft/sc64_p5.h starcraft/sc64_resource_actions.h .local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
.PHONY: test-sc64-p5-world
test-sc64-p5-world: build/test-sc64-p5-source-model build/test-sc64-p5-world
	./build/test-sc64-p5-source-model
	./build/test-sc64-p5-world

build/test-sc64-p5-actors-off: tests/sc64_p5_actor_rules.c starcraft/race_rules.h starcraft/sc64_zp_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-p5-actors-on: tests/sc64_p5_actor_rules.c starcraft/race_rules.h starcraft/sc64_zp_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_P5_ACTORS -DSC64_Z6_ACTORS $< -o $@
build/test-sc64-p5-actors-combined: tests/sc64_p5_actor_rules.c starcraft/race_rules.h starcraft/sc64_zp_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_P5_ACTORS -DSC64_Z6_ACTORS -DSC64_Z5_ACTORS -DSC64_T4_RUNTIME -DSC64_TERRAN_LATE_ACTORS $< -o $@
.PHONY: test-sc64-p5-actors
test-sc64-p5-actors: build/test-sc64-p5-actors-off build/test-sc64-p5-actors-on build/test-sc64-p5-actors-combined
	./build/test-sc64-p5-actors-off
	./build/test-sc64-p5-actors-on
	./build/test-sc64-p5-actors-combined
test-starcraft: test-sc64-p5-actors

build/test-sc64-z5-hp-regen: tests/sc64_z5_hp_regen.c starcraft/sc64_z5_hp_regen.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_Z5_RUNTIME $< -o $@
.PHONY: test-sc64-z5-hp-regen
test-sc64-z5-hp-regen: build/test-sc64-z5-hp-regen
	./build/test-sc64-z5-hp-regen
test-starcraft: test-sc64-z5-hp-regen

build/test-sc64-z5-cloak: tests/sc64_z5_cloak.c starcraft/sc64_z5_cloak.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_Z5_RUNTIME -DSC64_Z5_CLOAK_ENABLED $< -o $@
build/test-sc64-z5-detection: tests/sc64_z5_detection.c starcraft/sc64_z5_detection.h starcraft/sc64_z5_actor_rules.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-z5-ability-models
test-sc64-z5-ability-models: build/test-sc64-z5-cloak build/test-sc64-z5-detection
	./build/test-sc64-z5-cloak
	./build/test-sc64-z5-detection
test-starcraft: test-sc64-z5-ability-models

build/test-sc64-z6-model: tests/sc64_z6.c starcraft/sc64_z6.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-z6-adapter: tests/sc64_z6_adapter.c starcraft/sc64_z6.h starcraft/sc64_z6_adapter.h .local/sc64/zp-stage/generated/scenarios/sc64_z6_scenario_generated.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
.PHONY: test-sc64-z6-model test-sc64-z6-adapter
test-sc64-z6-model: build/test-sc64-z6-model
	./build/test-sc64-z6-model
test-sc64-z6-adapter: test-sc64-z6-model build/test-sc64-z6-adapter
	./build/test-sc64-z6-adapter
test-starcraft: test-sc64-z6-model

build/test-sc64-z5-actors-off: tests/sc64_z5_actor_rules.c starcraft/race_rules.h starcraft/sc64_zp_actor_rules.h starcraft/sc64_z5_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_ZP_CAMPAIGN_ACTORS $< -o $@
build/test-sc64-z5-actors-on: tests/sc64_z5_actor_rules.c starcraft/race_rules.h starcraft/sc64_zp_actor_rules.h starcraft/sc64_z5_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z5_ACTORS $< -o $@
build/test-sc64-z5-actors-t4: tests/sc64_z5_actor_rules.c starcraft/race_rules.h starcraft/sc64_zp_actor_rules.h starcraft/sc64_z5_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_T4_RUNTIME -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z5_ACTORS $< -o $@
.PHONY: test-sc64-z5-actors
test-sc64-z5-actors: build/test-sc64-z5-actors-off build/test-sc64-z5-actors-on build/test-sc64-z5-actors-t4
	./build/test-sc64-z5-actors-off
	./build/test-sc64-z5-actors-on
	./build/test-sc64-z5-actors-t4
test-starcraft: test-sc64-z5-actors

build/test-sc64-z6-actors-off: tests/sc64_z6_actor_rules.c starcraft/race_rules.h starcraft/sc64_z6_actor_rules.h starcraft/sc64_zp_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Istarcraft $< -o $@
build/test-sc64-z6-actors-on: tests/sc64_z6_actor_rules.c starcraft/race_rules.h starcraft/sc64_z6_actor_rules.h starcraft/sc64_zp_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Istarcraft -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z5_ACTORS -DSC64_Z6_ACTORS $< -o $@
build/test-sc64-z6-actors-t4: tests/sc64_z6_actor_rules.c starcraft/race_rules.h starcraft/sc64_z6_actor_rules.h starcraft/sc64_zp_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Istarcraft -DSC64_T4_RUNTIME -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z5_ACTORS -DSC64_Z6_ACTORS $< -o $@
.PHONY: test-sc64-z6-actors
test-sc64-z6-actors: build/test-sc64-z6-actors-off build/test-sc64-z6-actors-on build/test-sc64-z6-actors-t4
	./build/test-sc64-z6-actors-off
	./build/test-sc64-z6-actors-on
	./build/test-sc64-z6-actors-t4

build/test-sc64-compact-mineral-save: tests/sc64_compact_mineral_save.c starcraft/sc64_compact_mineral_save.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-zp-unit-save: tests/sc64_zp_unit_save.c starcraft/sc64_zp_unit_save.h starcraft/sc64_compact_unit_save.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-zp-building-types: tests/sc64_zp_building_types.c starcraft/sc64_zp_building_types.h starcraft/sc64_compact_building_save.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-zp-future-codecs
test-sc64-zp-future-codecs: build/test-sc64-compact-mineral-save build/test-sc64-zp-unit-save build/test-sc64-zp-building-types
	./build/test-sc64-compact-mineral-save
	./build/test-sc64-zp-unit-save
	./build/test-sc64-zp-building-types
build/test-sc64-zp-full-storage: tests/sc64_zp_full_storage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZP_FULL_STORAGE_FLAGS) $< common/gba.c -o $@
build/test-sc64-zp-full-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZP_FULL_STORAGE_FLAGS) $< common/gba.c -o $@
build/test-sc64-zp-full-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZP_FULL_STORAGE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-zp-full-storage
test-sc64-zp-full-storage: test-sc64-zp-future-codecs build/test-sc64-zp-full-storage build/test-sc64-zp-full-profile build/test-sc64-zp-full-frame-coverage
	./build/test-sc64-zp-full-storage
	./build/test-sc64-zp-full-profile
	./build/test-sc64-zp-full-frame-coverage
test-starcraft: test-sc64-z6-actors test-sc64-zp-future-codecs

build/test-sc64-z5-props-off: tests/sc64_z5_prop_rules.c starcraft/race_rules.h starcraft/sc64_z5_actor_rules.h starcraft/sc64_zp_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z5_ACTORS $< -o $@
build/test-sc64-z5-props-on: tests/sc64_z5_prop_rules.c starcraft/race_rules.h starcraft/sc64_z5_actor_rules.h starcraft/sc64_zp_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z5_ACTORS -DSC64_Z5_DOODADS $< -o $@
build/test-sc64-z5-props-t4: tests/sc64_z5_prop_rules.c starcraft/race_rules.h starcraft/sc64_z5_actor_rules.h starcraft/sc64_zp_actor_rules.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_T4_RUNTIME -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z5_ACTORS -DSC64_Z5_DOODADS $< -o $@
.PHONY: test-sc64-z5-props
test-sc64-z5-props: build/test-sc64-z5-props-off build/test-sc64-z5-props-on build/test-sc64-z5-props-t4
	./build/test-sc64-z5-props-off
	./build/test-sc64-z5-props-on
	./build/test-sc64-z5-props-t4
test-starcraft: test-sc64-z5-props

Z5_WORLD_FLAGS := $(filter-out -I.local/sc64/races/generated,$(Z3_FLAGS)) -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated -I.local/sc64/zp-stage/generated/scenarios -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_Z5_ACTORS -DSC64_Z5_DOODADS
build/test-sc64-z5-world: tests/sc64_z5_world.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) .local/sc64/zp-stage/generated/scenarios/sc64_z5_scenario_generated.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_WORLD_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z5-world
test-sc64-z5-world: build/test-sc64-z5-world
	./build/test-sc64-z5-world

Z6_WORLD_FLAGS := $(filter-out -DSC64_Z5_ACTORS -DSC64_Z5_DOODADS,$(Z5_WORLD_FLAGS)) -DSC64_ZP_FULL_STORAGE -DSC64_Z6_ACTORS
build/test-sc64-z6-world: tests/sc64_z6_world.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) .local/sc64/zp-stage/generated/scenarios/sc64_z6_scenario_generated.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_WORLD_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z6-world
test-sc64-z6-world: build/test-sc64-z6-world
	./build/test-sc64-z6-world

build/test-sc64-z4-defense-targets: tests/sc64_zp_defense_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z4_FLAGS) $< common/gba.c -o $@
build/test-sc64-p4-defense-targets: tests/sc64_zp_defense_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P4_STORM_FLAGS) $< common/gba.c -o $@
build/test-sc64-z5-defense-targets: tests/sc64_zp_defense_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-zp-defense-targets
test-sc64-zp-defense-targets: build/test-sc64-z4-defense-targets build/test-sc64-p4-defense-targets build/test-sc64-z5-defense-targets
	./build/test-sc64-z4-defense-targets
	./build/test-sc64-p4-defense-targets
	./build/test-sc64-z5-defense-targets
test-sc64-terran-full-targets: build/test-sc64-terran-full-targets
	./build/test-sc64-terran-full-targets
test-sc64-terran-full-spatial-hud: build/test-sc64-terran-full-collision build/test-sc64-terran-full-hud
	./build/test-sc64-terran-full-collision
	./build/test-sc64-terran-full-hud
test-sc64-terran-full-campaign: test-sc64-terran-defense-targets test-sc64-terran-full-targets test-sc64-terran-full-spatial-hud

build/test-terran: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_FULL_FLAGS) $< common/gba.c -o $@
build/test-sc64-terran-full-profile: tests/sc64_terran_full_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_FULL_FLAGS) $< common/gba.c -o $@
test-terran: build/test-terran build/test-sc64-terran-full-profile
	./build/test-terran
	./build/test-sc64-terran-full-profile

build/starcraft-zerg.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z3_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-GBA.gba: build/starcraft-zerg.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z3' SCZ3
verify-zerg: build/StarCraft-Zerg-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg
build/test-zerg: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) tests/sc64_split_run.c common/gba.c -o $@
build/test-sc64-zerg-promoted-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
test-zerg: build/test-zerg build/test-sc64-zerg-promoted-profile test-sc64-z3-runtime
	./build/test-zerg
	./build/test-sc64-zerg-promoted-profile

build/starcraft-protoss.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P3_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-GBA.gba: build/starcraft-protoss.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC PROTOSS3' SCP3
verify-protoss: build/StarCraft-Protoss-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss
build/test-protoss: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) tests/sc64_split_run.c common/gba.c -o $@
build/test-sc64-protoss-promoted-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
test-protoss: build/test-protoss build/test-sc64-protoss-promoted-profile test-sc64-p3
	./build/test-protoss
	./build/test-sc64-protoss-promoted-profile

test-race-campaigns: test-terran test-zerg test-protoss
verify-race-campaigns: verify-terran verify-zerg verify-protoss

unified: verify-unified
build/starcraft64-unified.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(UNIFIED_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft64-unified.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft64-GBA.gba: build/starcraft64-unified.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC64 UNIFIED' SCU1
verify-unified: build/StarCraft64-GBA.gba
	$(PYTHON) tools/verify_roms.py --unified
build/test-unified: tests/sc64_unified_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) tests/sc64_unified_run.c common/gba.c -o $@
build/test-unified-engine: tests/race_engine.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) tests/race_engine.c common/gba.c -o $@
build/test-unified-factions: tests/race_factions_engine.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) tests/race_factions_engine.c common/gba.c -o $@
build/test-unified-campaign: tests/race_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) tests/race_campaign_run.c common/gba.c -o $@
build/test-unified-menu: tests/sc64_menu_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) tests/sc64_menu_run.c common/gba.c -o $@
build/test-unified-frontend: tests/sc64_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -DSC64_UNIFIED -Icommon tests/sc64_frontend.c -o $@
build/test-unified-targets: tests/sc64_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) $< common/gba.c -o $@
build/test-sc64-zerg-townhalls: tests/sc64_zerg_townhalls.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) $< common/gba.c -o $@
build/test-unified-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) $< common/gba.c -o $@
test-unified: build/test-unified build/test-unified-frontend build/test-unified-menu build/test-unified-engine build/test-unified-factions build/test-unified-campaign build/test-unified-targets build/test-sc64-zerg-townhalls build/test-unified-hud
	./build/test-unified
	./build/test-unified-frontend
	./build/test-unified-menu
	./build/test-unified-engine
	./build/test-unified-factions
	./build/test-unified-campaign
	./build/test-unified-targets
	./build/test-sc64-zerg-townhalls
	./build/test-unified-hud

clean:
	rm -rf build

# Ordinary-controller probes do not mark a catalog mission qualified.
build/test-sc64-t6-campaign-run: tests/sc64_t6_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T6_RUNTIME_FLAGS) -DSC64_T6_PROBE_ENTRY $< common/gba.c -o $@
build/test-sc64-t8-campaign-run: tests/sc64_terran_assault_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(LATE_TERRAN_HEADERS) $(RACE_CAMPAIGN_INPUTS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(LATE_TERRAN_RUNTIME_FLAGS) -DSC64_TERRAN_LATE_PROBE_MISSION=11 $< common/gba.c -o $@
build/test-sc64-t10-campaign-run: tests/sc64_t10_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(LATE_TERRAN_HEADERS) $(RACE_CAMPAIGN_INPUTS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(LATE_TERRAN_RUNTIME_FLAGS) -DSC64_TERRAN_LATE_PROBE_MISSION=13 $< common/gba.c -o $@

# Explicit qualification builds expose the normal campaign menu, not probe entry.
build/starcraft-terran-t6-controller.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(T6_RUNTIME_FLAGS) -DSC64_T6_CAMPAIGN_ENTRY $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-terran-t6-controller.map -lgcc -o $@
build/StarCraft-Terran-T6-Controller.gba: build/starcraft-terran-t6-controller.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC TERRAN 6' SCT6
.PHONY: sc64-terran-t6-controller
sc64-terran-t6-controller: build/StarCraft-Terran-T6-Controller.gba
build/starcraft-terran-late-controller.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(LATE_TERRAN_HEADERS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(LATE_TERRAN_RUNTIME_FLAGS) -DSC64_T6_CAMPAIGN_ENTRY -DSC64_TERRAN_FULL_CAMPAIGN_ENTRY $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-terran-late-controller.map -lgcc -o $@
build/StarCraft-Terran-Late-Controller.gba: build/starcraft-terran-late-controller.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC TERRAN 10' SCT8
.PHONY: sc64-terran-late-controller
sc64-terran-late-controller: build/StarCraft-Terran-Late-Controller.gba

build/test-sc64-terran-late-repair: tests/sc64_terran_late_repair.c $(STARCRAFT) $(wildcard starcraft/*.h) $(LATE_TERRAN_HEADERS) $(RACE_CAMPAIGN_INPUTS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(LATE_TERRAN_RUNTIME_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-terran-late-repair
test-sc64-terran-late-repair: build/test-sc64-terran-late-repair
	./build/test-sc64-terran-late-repair

build/test-sc64-t7-campaign-run: tests/sc64_t7_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(LATE_TERRAN_HEADERS) $(RACE_CAMPAIGN_INPUTS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(LATE_TERRAN_RUNTIME_FLAGS) -DSC64_TERRAN_LATE_PROBE_MISSION=10 $< common/gba.c -o $@

build/test-sc64-t9-campaign-run: tests/sc64_t9_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(LATE_TERRAN_RUNTIME_FLAGS) -DSC64_TERRAN_LATE_PROBE_MISSION=12 $< common/gba.c -o $@

# Exercise the compact full-profile layout against the existing early mission
# controllers and persistence tests, not only the late-mission fixtures.
define TERRAN_FULL_TEST
build/test-sc64-full-$(1): $(2) $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_FULL_FLAGS) $(3) $$< common/gba.c -o $$@
endef
$(eval $(call TERRAN_FULL_TEST,t1-run,tests/sc64_mission_run.c))
$(eval $(call TERRAN_FULL_TEST,t2-run,tests/backwater_mission_run.c))
$(eval $(call TERRAN_FULL_TEST,t3-run,tests/sc64_t3_run.c))
$(eval $(call TERRAN_FULL_TEST,t3-save,tests/sc64_t3_save.c))
$(eval $(call TERRAN_FULL_TEST,t3-resume,tests/sc64_t3_resume.c))
$(eval $(call TERRAN_FULL_TEST,t4-run,tests/sc64_t4_campaign_run.c))
$(eval $(call TERRAN_FULL_TEST,t4-save,tests/sc64_t4_scenario_save.c))
$(eval $(call TERRAN_FULL_TEST,t5-save,tests/sc64_t5_save.c))
$(eval $(call TERRAN_FULL_TEST,t5-run,tests/sc64_t5_campaign_run.c,-DSC64_T5_PROBE_ENTRY))
$(eval $(call TERRAN_FULL_TEST,t6-run,tests/sc64_t6_campaign_run.c,-DSC64_T6_PROBE_ENTRY))
.PHONY: test-sc64-terran-full-regression
test-sc64-terran-full-regression: $(addprefix build/test-sc64-full-,t1-run t2-run t3-run t3-save t3-resume t4-run t4-save t5-save t5-run t6-run) test-terran
	mkdir -p .local/sc64/terran-late
	./build/test-sc64-full-t1-run --trace .local/sc64/terran-late/full-t1-controller.json
	./build/test-sc64-full-t2-run --trace .local/sc64/terran-late/full-t2-controller.json
	./build/test-sc64-full-t3-run
	./build/test-sc64-full-t3-save
	./build/test-sc64-full-t3-resume
	./build/test-sc64-full-t4-run --limit 30000 --trace .local/sc64/terran-late/full-t4-controller.json
	./build/test-sc64-full-t4-save
	./build/test-sc64-full-t5-save
	./build/test-sc64-full-t5-run --limit 200000 --trace .local/sc64/terran-late/full-t5-controller.json
	./build/test-sc64-full-t6-run --limit 30000 --trace .local/sc64/terran-late/full-t6-controller.json

.PHONY: test-sc64-terran-full-campaign
test-sc64-terran-full-campaign: test-sc64-terran-full-regression test-sc64-terran-late-world test-sc64-terran-late-save test-sc64-terran-late-repair $(addprefix build/test-sc64-t,7-campaign-run 8-campaign-run 9-campaign-run 10-campaign-run)
	./build/test-sc64-t7-campaign-run --limit 20000 --trace .local/sc64/terran-late/full-t7-controller.json
	./build/test-sc64-t8-campaign-run --limit 200000 --trace .local/sc64/terran-late/full-t8-controller.json
	./build/test-sc64-t9-campaign-run --limit 500000 --trace .local/sc64/terran-late/full-t9-controller.json
	./build/test-sc64-t10-campaign-run --limit 100000 --trace .local/sc64/terran-late/full-t10-controller.json
