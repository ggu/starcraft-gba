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

.PHONY: all clean test verify probe sc64 backwater skirmish campaign races unified \
	verify-sc64 verify-backwater verify-skirmish verify-campaign verify-races verify-unified \
	test-starcraft test-sc64 test-backwater test-skirmish test-skirmish-match \
	test-campaign test-races test-races-controller test-race-routes test-race-route-sizes \
	test-races-menu test-unified test-collision test-terrain-cache

# The public default uses only original-art StarCraft sources.
all: verify
verify: build/StarCraft-GBA.gba
	$(PYTHON) tools/verify_roms.py

build:
	mkdir -p build

build/starcraft.elf: $(COMMON) $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h common/gba.ld Makefile | build
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

build/test-sc64-frontend: tests/sc64_frontend.c starcraft/sc64_frontend.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon tests/sc64_frontend.c -o $@

build/test-race-routes: tests/race_routes.c starcraft/sc64_routes.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I. tests/race_routes.c -o $@

build/test-race-routes-96: tests/race_routes.c starcraft/sc64_routes.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I. -DRACE_TEST_COLS=96 -DRACE_TEST_ROWS=96 tests/race_routes.c -o $@

build/test-race-routes-rect: tests/race_routes.c starcraft/sc64_routes.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I. -DRACE_TEST_COLS=96 -DRACE_TEST_ROWS=64 tests/race_routes.c -o $@

build/test-terrain-cache: tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/sc64_terrain_cache.c -o $@

build/test-collision: tests/sc64_collision.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -DSC64_SKIRMISH -Icommon tests/sc64_collision.c common/gba.c -o $@

test-starcraft: build/test-starcraft build/test-starcraft-selection build/test-race-rules build/test-skirmish-ai build/test-sc64-frontend build/test-race-routes build/test-race-routes-96 build/test-race-routes-rect build/test-terrain-cache build/test-collision
	./build/test-starcraft
	./build/test-starcraft-selection
	./build/test-race-rules
	./build/test-skirmish-ai
	./build/test-sc64-frontend
	./build/test-race-routes
	./build/test-race-routes-96
	./build/test-race-routes-rect
	./build/test-terrain-cache
	./build/test-collision
	$(PYTHON) -m unittest discover -s tests -p 'test_sc64*.py'

test: test-starcraft

probe: build/probe.gba
build/probe.elf: $(COMMON) tests/platform_probe.c common/gba.h common/gba.ld Makefile | build
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

build/starcraft64.elf: $(COMMON) $(STARCRAFT) $(wildcard starcraft/*.h) $(SC64_INPUTS) common/gba.h common/gba.ld Makefile | build
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
build/starcraft64-backwater.elf: $(COMMON) $(STARCRAFT) $(wildcard starcraft/*.h) $(BACKWATER_INPUTS) common/gba.h common/gba.ld Makefile | build
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
build/starcraft64-skirmish.elf: $(COMMON) $(STARCRAFT) $(wildcard starcraft/*.h) $(BACKWATER_INPUTS) common/gba.h common/gba.ld Makefile | build
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
build/starcraft64-campaign.elf: $(COMMON) $(STARCRAFT) $(wildcard starcraft/*.h) $(CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
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
build/starcraft64-races.elf: $(COMMON) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.ld Makefile | build
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

unified: verify-unified
build/starcraft64-unified.elf: $(COMMON) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.ld Makefile | build
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
build/test-unified-frontend: tests/sc64_frontend.c starcraft/sc64_frontend.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -DSC64_UNIFIED -Icommon tests/sc64_frontend.c -o $@
test-unified: build/test-unified build/test-unified-frontend build/test-unified-menu build/test-unified-engine build/test-unified-factions build/test-unified-campaign
	./build/test-unified
	./build/test-unified-frontend
	./build/test-unified-menu
	./build/test-unified-engine
	./build/test-unified-factions
	./build/test-unified-campaign

clean:
	rm -rf build
