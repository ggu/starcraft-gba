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
Z3_OPTIMIZATION_FLAGS ?= -DSC64_ROUTE_PACK3 -DSC64_VISIBILITY_SPANS -DSC64_Z3_WINDOWED_SCHEDULE
Z3_FLAGS := $(RACE_CAMPAIGN_FLAGS) -DSC64_Z3_ENABLED -DSC64_CAMPAIGN_RACE=1 $(Z3_OPTIMIZATION_FLAGS) $(Z3_EXTRA_FLAGS)
LARGE_PROBE_FLAGS := $(RACE_CAMPAIGN_FLAGS) -DSC64_LARGE_MAPS -DSC64_CAMPAIGN_RACE=2
P3_OPTIMIZATION_FLAGS ?= -DSC64_ROUTE_PACK3 -DSC64_P3_DEFENSE_TARGETS -DSC64_P3_WINDOWED_SCHEDULE -DSC64_NO_FRAME_CLEAR -DSC64_TERRAIN_SCROLL -DSC64_STATIC_KEY_CULL -DSC64_VISIBILITY_SPANS
P3_FLAGS := $(LARGE_PROBE_FLAGS) -DSC64_P3_ENABLED -DSC64_P3_RUNTIME $(P3_OPTIMIZATION_FLAGS) $(P3_EXTRA_FLAGS)
T4_STORAGE_FLAGS := $(RACE_CAMPAIGN_FLAGS) -DSC64_LARGE_MAPS -DSC64_T4_ENABLED -DSC64_T4_RUNTIME -DSC64_ROUTE_PACK3 -DSC64_T4_WINDOWED_SCHEDULE -DSC64_CAMPAIGN_RACE=0
T4_OPTIMIZATION_FLAGS ?= -DSC64_T4_COUNT_CACHE -DSC64_NO_FRAME_CLEAR -DSC64_TERRAIN_SCROLL -DSC64_STATIC_KEY_CULL -DSC64_VISIBILITY_SPANS
T4_SCENARIO_FLAGS := $(T4_STORAGE_FLAGS) -DSC64_T4_SCENARIO_RUNTIME
T4_RUNTIME_FLAGS := $(T4_SCENARIO_FLAGS) $(T4_OPTIMIZATION_FLAGS)
T4_REPLAY_FLAGS := $(T4_RUNTIME_FLAGS) -DSC64_T4_REPLAY_ENTRY
T4_COMBINED_FLAGS := $(T4_RUNTIME_FLAGS) -DSC64_T3_ENABLED -DSC64_T4_CAMPAIGN_ENTRY
T4_SCENARIO_HEADER := .local/sc64/races/generated/sc64_t4_scenario_generated.h
T4_SCENARIO_RECORDS := .local/sc64/races/generated/t4/records

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
build/starcraft-protoss-p3.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P3_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p3.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P3-GBA.gba: build/starcraft-protoss-p3.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC PROTOSS3' SCP3
protoss-p3: build/StarCraft-Protoss-P3-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p3
build/test-sc64-p3-run: tests/sc64_p3_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-save: tests/sc64_p3_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-controller: tests/sc64_p3_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
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

# Staged New Dominion runtime has its own ROM and save identity.
.PHONY: zerg-z3 test-sc64-z3-runtime
build/starcraft-zerg-z3.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z3_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z3.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z3-GBA.gba: build/starcraft-zerg-z3.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z3' SCZ3
zerg-z3: build/StarCraft-Zerg-Z3-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z3
build/test-sc64-z3-run: tests/sc64_z3_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
build/test-sc64-z3-save: tests/sc64_z3_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
build/test-sc64-z3-controller: tests/sc64_z3_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
build/test-sc64-z3-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
test-sc64-z3-runtime: test-sc64-z3-import build/test-sc64-z3-run build/test-sc64-z3-save build/test-sc64-z3-controller build/test-sc64-z3-hud
	./build/test-sc64-z3-run
	./build/test-sc64-z3-save
	./build/test-sc64-z3-hud
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

test-starcraft: build/test-sc64-t4-actor-rules build/test-sc64-t4-world build/test-sc64-t4-schedule build/test-sc64-t4-count-cache build/test-sc64-p3-schedule build/test-sc64-p3-runtime $(P3_FRONT_TESTS) build/test-sc64-demo-schedule build/test-sc64-demo-schedule-runtime build/test-starcraft build/test-starcraft-selection build/test-race-rules build/test-race-rules-z3 build/test-skirmish-ai build/test-sc64-frontend build/test-sc64-bitgrid build/test-sc64-map-math build/test-sc64-t3-rules build/test-sc64-z3-rules build/test-sc64-p3-rules build/test-sc64-p3-model $(RACE_FRONTEND_TESTS) $(RACE_T3_FRONTEND_TESTS) $(RACE_Z3_FRONTEND_TESTS) $(RACE_Z3_UNIFIED_TESTS) build/test-sc64-frontend-t3-unified build/test-race-routes build/test-race-routes-96 build/test-race-routes-t3 build/test-race-routes-rect build/test-race-routes-large build/test-race-routes-pack3 build/test-race-routes-pack3-t3 build/test-race-routes-pack3-large build/test-terrain-cache build/test-terrain-scroll build/test-collision
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
build/starcraft-terran.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(RACE_CAMPAIGN_FLAGS) -DSC64_CAMPAIGN_RACE=0 $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-terran.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Terran-GBA.gba: build/starcraft-terran.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC TERRAN' SCT2
verify-terran: build/StarCraft-Terran-GBA.gba
	$(PYTHON) tools/verify_roms.py --terran
build/test-terran: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACE_CAMPAIGN_FLAGS) -DSC64_CAMPAIGN_RACE=0 tests/sc64_split_run.c common/gba.c -o $@
test-terran: build/test-terran
	./build/test-terran

build/starcraft-zerg.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(RACE_CAMPAIGN_FLAGS) -DSC64_CAMPAIGN_RACE=1 $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-GBA.gba: build/starcraft-zerg.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG' SCZ2
verify-zerg: build/StarCraft-Zerg-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg
build/test-zerg: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACE_CAMPAIGN_FLAGS) -DSC64_CAMPAIGN_RACE=1 tests/sc64_split_run.c common/gba.c -o $@
test-zerg: build/test-zerg
	./build/test-zerg

build/starcraft-protoss.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(RACE_CAMPAIGN_FLAGS) -DSC64_CAMPAIGN_RACE=2 $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-GBA.gba: build/starcraft-protoss.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC PROTOSS' SCP2
verify-protoss: build/StarCraft-Protoss-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss
build/test-protoss: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACE_CAMPAIGN_FLAGS) -DSC64_CAMPAIGN_RACE=2 tests/sc64_split_run.c common/gba.c -o $@
test-protoss: build/test-protoss
	./build/test-protoss

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
