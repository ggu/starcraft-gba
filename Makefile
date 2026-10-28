.DEFAULT_GOAL := all

.PHONY: test-gba-mapped-sprite
build/test-gba-mapped-sprite: tests/gba_mapped_sprite.c common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -Icommon tests/gba_mapped_sprite.c common/gba.c -o $@
build/test-gba-mapped-sprite-full-screen: tests/gba_mapped_sprite.c common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon tests/gba_mapped_sprite.c common/gba.c -o $@
test-gba-mapped-sprite: build/test-gba-mapped-sprite build/test-gba-mapped-sprite-full-screen
	./build/test-gba-mapped-sprite
	./build/test-gba-mapped-sprite-full-screen
test: test-gba-mapped-sprite

.PHONY: test-sc64-stock-motion test-sc64-stock-motion-runtime
build/test-sc64-stock-motion: tests/sc64_stock_motion.c starcraft/sc64_stock_motion.h starcraft/sc64_stock_profiles.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
test-sc64-stock-motion: build/test-sc64-stock-motion
	./build/test-sc64-stock-motion
test: test-sc64-stock-motion

# Presentation-only walk cycle shared by every profile.
.PHONY: test-sc64-unit-walk
build/test-sc64-unit-walk: tests/sc64_unit_walk.c starcraft/sc64_unit_walk.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Istarcraft $< -o $@
test-sc64-unit-walk: build/test-sc64-unit-walk
	./build/test-sc64-unit-walk
test: test-sc64-unit-walk

# Caller-owned incremental range search; no runtime integration or assets.
.PHONY: test-sc64-range-work
build/test-sc64-range-work: tests/sc64_range_work.c starcraft/sc64_range_work.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer $< -o $@
test-sc64-range-work: build/test-sc64-range-work
	./build/test-sc64-range-work
test: test-sc64-range-work

# Asset-free minimap terrain renderer oracle; exercise both implementations.
.PHONY: test-sc64-minimap-terrain
build/gba-minimap-test.o: common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -Wno-unused-variable -Icommon -c $< -o $@
build/test-sc64-minimap-terrain-scalar: tests/sc64_minimap_terrain.c starcraft/sc64_minimap_terrain.h build/gba-minimap-test.o common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -Icommon -Istarcraft $< build/gba-minimap-test.o -o $@
build/test-sc64-minimap-terrain-blit: tests/sc64_minimap_terrain.c starcraft/sc64_minimap_terrain.h build/gba-minimap-test.o common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -DSC64_MINIMAP_BLIT -Icommon -Istarcraft $< build/gba-minimap-test.o -o $@
build/gba-minimap-words-test.o: common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -DGBA_SPRITE_WORDS -Wno-unused-variable -Icommon -c $< -o $@
build/test-sc64-minimap-terrain-words: tests/sc64_minimap_terrain.c starcraft/sc64_minimap_terrain.h build/gba-minimap-words-test.o common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -DGBA_SPRITE_WORDS -DSC64_MINIMAP_BLIT -Icommon -Istarcraft $< build/gba-minimap-words-test.o -o $@
test-sc64-minimap-terrain: build/test-sc64-minimap-terrain-scalar build/test-sc64-minimap-terrain-blit build/test-sc64-minimap-terrain-words
	./build/test-sc64-minimap-terrain-scalar
	./build/test-sc64-minimap-terrain-blit
	./build/test-sc64-minimap-terrain-words
test: test-sc64-minimap-terrain

# Asset-free Interceptor order decisions; no gameplay integration is enabled.
.PHONY: test-sc64-interceptor-orders
build/test-sc64-interceptor-orders: tests/sc64_interceptor_orders.c starcraft/sc64_interceptor_orders.c starcraft/sc64_interceptor_orders.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -Istarcraft $< starcraft/sc64_interceptor_orders.c -o $@
test-sc64-interceptor-orders: build/test-sc64-interceptor-orders
	./build/test-sc64-interceptor-orders
test-starcraft: test-sc64-interceptor-orders

# Fixed-capacity P8 native request-heap storage; source-only synthetic vectors.
.PHONY: test-sc64-p8-request-queue
build/test-sc64-p8-request-queue: tests/sc64_p8_request_queue.c starcraft/sc64_p8_request_queue.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -Istarcraft $< -o $@
test-sc64-p8-request-queue: build/test-sc64-p8-request-queue
	./build/test-sc64-p8-request-queue
test-starcraft: test-sc64-p8-request-queue

# Source-only P8 request-admission predicate; player census rows are supplied
# by the caller and this does not activate any P8 runtime path.
.PHONY: test-sc64-p8-ai-admission
build/test-sc64-p8-ai-admission: tests/sc64_p8_ai_admission.c starcraft/sc64_p8_ai_admission.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -Istarcraft $< -o $@
test-sc64-p8-ai-admission: build/test-sc64-p8-ai-admission
	./build/test-sc64-p8-ai-admission
test-starcraft: test-sc64-p8-ai-admission

# Asset-free P8 compact trigger scheduler contract. This does not include a
# scenario descriptor or bind P8 actions to game-world effects.
.PHONY: test-sc64-p8-runtime test-sc64-p8-source-contract
build/test-sc64-p8-runtime: tests/sc64_p8_runtime.c starcraft/sc64_p8_runtime.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon -Istarcraft $< -o $@
test-sc64-p8-runtime: build/test-sc64-p8-runtime
	./build/test-sc64-p8-runtime
test-starcraft: test-sc64-p8-runtime

# Explicit optional check against already-prepared, ignored P8 source input.
# It must never invoke the scenario packer or create its generated header.
P8_SCENARIO_DIR ?= .local/sc64/zp-stage/generated/scenarios
build/test-sc64-p8-source-contract: tests/sc64_p8_source_contract.c starcraft/sc64_p8_source_adapter.h starcraft/sc64_p8_runtime.h $(P8_SCENARIO_DIR)/sc64_p8_scenario_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon -Istarcraft -I$(P8_SCENARIO_DIR) $< -o $@
test-sc64-p8-source-contract: build/test-sc64-p8-source-contract
	./build/test-sc64-p8-source-contract

# Asset-free P8 typed-world service bridge. The fixture models the private
# generated header's row shape without embedding source scenario data.
.PHONY: test-sc64-p8-world
build/p8-fixture/sc64_p8_scenario_generated.h: tests/fixtures/p8/sc64_p8_scenario_shape_fixture.h | build
	mkdir -p build/p8-fixture
	cp $< $@
build/test-sc64-p8-world: tests/sc64_p8_world_adapter.c build/p8-fixture/sc64_p8_scenario_generated.h starcraft/sc64_p8_world_adapter.h starcraft/sc64_p8_source_adapter.h starcraft/sc64_z8_world_adapter.h starcraft/sc64_z8_triggers.h starcraft/sc64_resource_actions.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -Icommon -Istarcraft -Ibuild/p8-fixture $< -o $@
test-sc64-p8-world: build/test-sc64-p8-world
	./build/test-sc64-p8-world
test-starcraft: test-sc64-p8-world

# Asset-free shared-owner wait/presentation and source-pause tail fixture.
.PHONY: test-sc64-p8-timed-service
build/test-sc64-p8-timed-service: tests/sc64_p8_timed_service.c build/p8-fixture/sc64_p8_scenario_generated.h starcraft/sc64_p8_timed_service.h starcraft/sc64_p8_world_adapter.h starcraft/sc64_p8_source_adapter.h starcraft/sc64_z8_world_adapter.h starcraft/sc64_z8_triggers.h starcraft/sc64_resource_actions.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -Icommon -Istarcraft -Ibuild/p8-fixture $< -o $@
test-sc64-p8-timed-service: build/test-sc64-p8-timed-service
	./build/test-sc64-p8-timed-service
test-starcraft: test-sc64-p8-timed-service

# Optional bridge check against an already-prepared authenticated P8 header.
.PHONY: test-sc64-p8-world-source-contract
build/test-sc64-p8-world-source-contract: tests/sc64_p8_world_source.c starcraft/sc64_p8_world_adapter.h starcraft/sc64_p8_source_adapter.h starcraft/sc64_z8_world_adapter.h starcraft/sc64_z8_triggers.h starcraft/sc64_resource_actions.h $(P8_SCENARIO_DIR)/sc64_p8_scenario_generated.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -Icommon -Istarcraft -I$(P8_SCENARIO_DIR) $< -o $@
test-sc64-p8-world-source-contract: build/test-sc64-p8-world-source-contract
	./build/test-sc64-p8-world-source-contract

# Optional live-pool P8 adapter check using already-prepared Z7 actor facts.
# Run only after the listed private inputs have been reviewed as read-only.
build/test-sc64-p8-game-pool-bridge: tests/sc64_p8_game_pool_bridge.c build/p8-fixture/sc64_p8_scenario_generated.h $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -Ibuild/p8-fixture $(Z8_GAME_POOL_BRIDGE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p8-game-pool-bridge
test-sc64-p8-game-pool-bridge: build/test-sc64-p8-game-pool-bridge
	./build/test-sc64-p8-game-pool-bridge

# Optional private-descriptor P8 create-service check over the real pools.
# Requires prepared P8/Z7 inputs; this target does not pack or generate them.
P8_GAME_POOL_FLAGS = $(filter-out -DSC64_CONSTRUCTION_SOURCE_ART,$(Z8_GAME_POOL_BRIDGE_FLAGS)) -DSC64_P8_GAME_POOL_BRIDGE
build/test-sc64-p8-live-create: tests/sc64_p8_live_create.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) $(P8_SCENARIO_DIR)/sc64_p8_scenario_generated.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers -fsanitize=address,undefined -DGBA_HOST -Icommon -Istarcraft -I$(P8_SCENARIO_DIR) $(P8_GAME_POOL_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p8-live-create
test-sc64-p8-live-create: build/test-sc64-p8-live-create
	./build/test-sc64-p8-live-create

# Prepared P8 map actor/capacity preflight; never regenerates private inputs or
# adds a P8 menu entry. Point P8_PRIVATE_ROOT at the checkout holding prepared
# ignored data when this source worktree has no private imports of its own.
P8_PRIVATE_ROOT ?= .
P8_PREFLIGHT_SCENARIO_DIR ?= $(P8_PRIVATE_ROOT)/.local/sc64/zp-stage/generated/scenarios
P8_PREFLIGHT_FLAGS = $(subst -I.local/sc64,-I$(P8_PRIVATE_ROOT)/.local/sc64,$(P7_RUNTIME_FLAGS)) -DSC64_P8_DATA_ENABLED
build/test-sc64-p8-campaign-preflight: tests/sc64_p8_campaign_preflight.c starcraft/sc64_p8_preflight.h starcraft/sc64_p8_source_adapter.h $(STARCRAFT) $(wildcard starcraft/*.h) $(P8_PRIVATE_ROOT)/.local/sc64/zp-stage/generated/sc64_races_generated.h $(P8_PREFLIGHT_SCENARIO_DIR)/sc64_p8_scenario_generated.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -Istarcraft $(P8_PREFLIGHT_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p8-campaign-preflight
test-sc64-p8-campaign-preflight: build/test-sc64-p8-campaign-preflight
	./build/test-sc64-p8-campaign-preflight

PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size
CLANG ?= clang
PYTHON ?= python3
CFLAGS := -mcpu=arm7tdmi -mthumb -mthumb-interwork -O2 -g -std=c11 -ffreestanding -fno-builtin -fno-strict-aliasing -fno-common -ffunction-sections -fdata-sections -Wall -Wextra -Icommon
LDFLAGS := -mcpu=arm7tdmi -mthumb -mthumb-interwork -nostdlib -Tcommon/gba.ld -Wl,--gc-sections
COMMON := common/start.s common/main.c common/gba.c common/memory.c common/audio_bank.S
COMMON_HEADERS := $(wildcard common/*.h)
# The order-decision foundation is linked only by its isolated host check until
# a gameplay adapter is implemented and qualified.
STARCRAFT := $(filter-out starcraft/sc64_interceptor_orders.c,$(wildcard starcraft/*.c))

# Optional private StarCraft 64 audio bank from tools/sc64_audio_pack.py. Every
# imported profile links it when present; SC64_AUDIO=0 builds them silent.
SC64_AUDIO ?= auto
SC64_AUDIO_DIR := .local/sc64/audio/generated
SC64_AUDIO_FLAGS :=
SC64_AUDIO_INPUTS :=
ifneq ($(SC64_AUDIO),0)
ifneq ($(wildcard $(SC64_AUDIO_DIR)/sc64_audio_generated.h),)
SC64_AUDIO_FLAGS := -DSC64_AUDIO_IMPORTED -I$(SC64_AUDIO_DIR) '-DSC64_AUDIO_BLOB="$(SC64_AUDIO_DIR)/sc64_audio_generated.bin"'
SC64_AUDIO_INPUTS := $(SC64_AUDIO_DIR)/sc64_audio_generated.h $(SC64_AUDIO_DIR)/sc64_audio_generated.bin
else ifeq ($(SC64_AUDIO),1)
$(error SC64_AUDIO=1 but $(SC64_AUDIO_DIR)/sc64_audio_generated.h is missing; run tools/sc64_audio_pack.py)
endif
endif
SC64_FLAGS := -DSC64_IMPORTED_MISSION -DSC64_HAS_TERRAIN_ART -DSC64_HAS_SPRITES -Istarcraft -I.local/sc64/generated $(SC64_AUDIO_FLAGS)
SC64_INPUTS := .local/sc64/generated/sc64_mission_generated.h .local/sc64/generated/sc64_assets_generated.h .local/sc64/generated/sc64_sprites_generated.h $(SC64_AUDIO_INPUTS)
BACKWATER_FLAGS := -DSC64_IMPORTED_MISSION -DSC64_HAS_TERRAIN_ART -DSC64_HAS_SPRITES -Istarcraft -I.local/sc64/backwater/generated $(SC64_AUDIO_FLAGS)
BACKWATER_INPUTS := .local/sc64/backwater/generated/sc64_mission_generated.h .local/sc64/backwater/generated/sc64_assets_generated.h .local/sc64/backwater/generated/sc64_sprites_generated.h $(SC64_AUDIO_INPUTS)
CAMPAIGN_FLAGS := $(BACKWATER_FLAGS) -I.local/sc64/campaign/generated
CAMPAIGN_INPUTS := $(BACKWATER_INPUTS) .local/sc64/campaign/generated/sc64_campaign_generated.h
# Prepared imported builds use source construction art by default. The
# source-only original-art build remains independent of private inputs.
SC64_CONSTRUCTION_ART ?= 1
CONSTRUCTION_ART_DIR := .local/sc64/construction/generated
CONSTRUCTION_ART_HEADER := $(CONSTRUCTION_ART_DIR)/sc64_construction_sprites_generated.h
RACE_CONSTRUCTION_FLAGS :=
RACE_CONSTRUCTION_INPUTS :=
ifneq ($(SC64_CONSTRUCTION_ART),0)
RACE_CONSTRUCTION_FLAGS := -DSC64_CONSTRUCTION_SOURCE_ART -I$(CONSTRUCTION_ART_DIR)
RACE_CONSTRUCTION_INPUTS := $(CONSTRUCTION_ART_HEADER)
endif
# Every race-engine ROM uses the default profile's controls and menus
# (starcraft/field_controls.h).
SC64_INTERFACE_DIR := .local/sc64/interface/generated
SC64_INTERFACE_INPUTS := $(addprefix $(SC64_INTERFACE_DIR)/,sc64_font_generated.h sc64_icons_generated.h sc64_chrome_generated.h)
RACES_FLAGS := $(BACKWATER_FLAGS) -DSC64_RACES -DSC64_FIELD_CONTROLS -I.local/sc64/races/generated -I.local/sc64/briefings/generated -I$(SC64_INTERFACE_DIR) $(RACE_CONSTRUCTION_FLAGS)
RACES_INPUTS := $(BACKWATER_INPUTS) .local/sc64/races/generated/sc64_races_generated.h .local/sc64/races/generated/sc64_race_sprites_generated.h .local/sc64/races/generated/sc64_menu_generated.h .local/sc64/briefings/generated/sc64_briefing_generated.h $(SC64_INTERFACE_INPUTS) $(RACE_CONSTRUCTION_INPUTS)

.PHONY: sc64-interface test-sc64-font test-sc64-icons
sc64-interface:
	python3 tools/sc64_font_assets.py
	python3 tools/sc64_briefing_assets.py
	python3 tools/sc64_icon_assets.py
	python3 tools/sc64_chrome_assets.py
$(SC64_INTERFACE_INPUTS):
	@echo 'Private interface data is missing. Run make sc64-interface with the prepared local ROM and reference checkout.'
	@false
build/test-sc64-font: tests/sc64_font.c starcraft/sc64_font.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $< -o $@
test-sc64-font: build/test-sc64-font
	./build/test-sc64-font
build/test-sc64-icons: tests/sc64_icons.c starcraft/sc64_icons.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $< -o $@
test-sc64-icons: build/test-sc64-icons
	./build/test-sc64-icons
test-starcraft: test-sc64-font test-sc64-icons

.PHONY: sc64-briefings test-sc64-briefing
sc64-briefings:
	python3 tools/sc64_briefing_assets.py
build/test-sc64-briefing: tests/sc64_briefing.c starcraft/sc64_briefing.h starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $< -o $@
test-sc64-briefing: build/test-sc64-briefing
	./build/test-sc64-briefing
test-starcraft: test-sc64-briefing

.local/sc64/briefings/generated/sc64_briefing_generated.h:
	@echo 'Private briefing data is missing. Run make sc64-briefings with the prepared local ROM and reference checkout.'
	@false
build/test-sc64-stock-motion-runtime: tests/sc64_stock_motion_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) $< common/gba.c -o $@
test-sc64-stock-motion-runtime: build/test-sc64-stock-motion-runtime
	./build/test-sc64-stock-motion-runtime
build/test-sc64-scv-contact: tests/sc64_scv_contact_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) -DSC64_DIRECT_ATTACK $< common/gba.c -o $@
.PHONY: test-sc64-scv-contact
test-sc64-scv-contact: build/test-sc64-scv-contact
	./build/test-sc64-scv-contact
build/test-sc64-scv-contact-zp: tests/sc64_scv_contact_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_DIRECT_ATTACK $< common/gba.c -o $@
.PHONY: test-sc64-scv-contact-zp
test-sc64-scv-contact-zp: build/test-sc64-scv-contact-zp
	./build/test-sc64-scv-contact-zp
UNIFIED_FLAGS := $(RACES_FLAGS) -DSC64_UNIFIED
RACE_CAMPAIGN_FLAGS := $(UNIFIED_FLAGS) -DSC64_IMPORTED_SKIRMISH -I.local/sc64/skirmish/generated
RACE_CAMPAIGN_INPUTS := $(RACES_INPUTS) .local/sc64/skirmish/generated/sc64_skirmish_generated.h
T3_FLAGS := $(RACE_CAMPAIGN_FLAGS) -DSC64_T3_ENABLED -DSC64_CAMPAIGN_RACE=0 $(T3_EXTRA_FLAGS)
Z3_OPTIMIZATION_FLAGS ?= -DSC64_ROUTE_PACK3 -DSC64_VISIBILITY_SPANS -DSC64_Z3_WINDOWED_SCHEDULE -DSC64_NO_FRAME_CLEAR -DGBA_PRESENT_DEADLINE -DSC64_TARGET_OCCUPANCY -DSC64_UNARMED_QUERY_SKIP
Z3_FLAGS := $(RACE_CAMPAIGN_FLAGS) -DSC64_Z3_ENABLED -DSC64_CAMPAIGN_RACE=1 $(Z3_OPTIMIZATION_FLAGS) $(Z3_EXTRA_FLAGS)
Z4_FLAGS := $(filter-out -I.local/sc64/races/generated,$(Z3_FLAGS)) -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated -I.local/sc64/z4/generated -DSC64_Z4_DATA_ENABLED -DSC64_Z4_ENABLED -DSC64_Z4_RUNTIME -DSC64_Z4_CAMPAIGN_ENTRY -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_ZP_DEFENSE_TARGETS
UNIT_FACING_HEADER := .local/sc64/facing/generated/sc64_unit_facing_generated.h
TANK_SOURCE_HEADER := .local/sc64/tank-vfx/generated/sc64_tank_source_vfx_generated.h
Z4_INPUTS := $(RACE_CAMPAIGN_INPUTS) .local/sc64/zp-stage/generated/sc64_races_generated.h .local/sc64/zp-stage/generated/sc64_race_sprites_generated.h .local/sc64/z4/generated/sc64_z4_scenario_generated.h $(UNIT_FACING_HEADER)
LARGE_PROBE_FLAGS := $(RACE_CAMPAIGN_FLAGS) -DSC64_LARGE_MAPS -DSC64_CAMPAIGN_RACE=2
P3_OPTIMIZATION_FLAGS ?= -DSC64_ROUTE_PACK3 -DSC64_P3_DEFENSE_TARGETS -DSC64_P3_WINDOWED_SCHEDULE -DSC64_NO_FRAME_CLEAR -DSC64_TERRAIN_SCROLL -DSC64_STATIC_KEY_CULL -DSC64_VISIBILITY_SPANS -DGBA_PRESENT_DEADLINE -DSC64_TARGET_OCCUPANCY -DSC64_UNARMED_QUERY_SKIP
P3_FLAGS := $(LARGE_PROBE_FLAGS) -DSC64_P3_ENABLED -DSC64_P3_RUNTIME $(P3_OPTIMIZATION_FLAGS) $(P3_EXTRA_FLAGS)
P4_FLAGS := $(filter-out -I.local/sc64/races/generated,$(P3_FLAGS)) -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated -I.local/sc64/p4/generated -DSC64_P4_DATA_ENABLED -DSC64_P4_ENABLED -DSC64_P4_RUNTIME -DSC64_P4_CAMPAIGN_ENTRY -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_ZP_DEFENSE_TARGETS
P4_SCENARIO_HEADER := .local/sc64/p4/generated/sc64_p4_scenario_generated.h
P4_INPUTS := $(RACE_CAMPAIGN_INPUTS) .local/sc64/zp-stage/generated/sc64_races_generated.h .local/sc64/zp-stage/generated/sc64_race_sprites_generated.h $(P4_SCENARIO_HEADER)
P4_STORM_FLAGS := $(P4_FLAGS) -DSC64_P4_STORM_ENABLED
ZP_FULL_STORAGE_FLAGS := $(P4_STORM_FLAGS) -DSC64_ZP_FULL_STORAGE
Z5_FLAGS := $(filter-out -DSC64_ZP_DEFENSE_TARGETS -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated,$(Z4_FLAGS)) -I.local/sc64/z5/generated -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated -I.local/sc64/zp-stage/generated/scenarios -DSC64_ZP_FULL_STORAGE -DSC64_STATIC_KEY_CULL -DSC64_Z5_DATA_ENABLED -DSC64_Z5_ENABLED -DSC64_Z5_RUNTIME -DSC64_Z5_CAMPAIGN_ENTRY -DSC64_Z5_ACTORS -DSC64_Z5_DOODADS
Z5_INPUTS := $(Z4_INPUTS) .local/sc64/z5/generated/sc64_race_sprites_generated.h .local/sc64/zp-stage/generated/scenarios/sc64_z5_scenario_generated.h .local/sc64/zp-stage/generated/scenarios/sc64_z5_doodads_generated.h
Z5_CLOAK_FLAGS := $(Z5_FLAGS) -DSC64_Z5_CLOAK_ENABLED
Z5_QUERY_FLAGS ?= -DSC64_Z5_PRUNE_STARTED -DSC64_Z5_COUNT_SNAPSHOT
Z5_CONSUME_FLAGS := $(Z5_CLOAK_FLAGS) -DSC64_Z5_CONSUME_ENABLED $(Z5_QUERY_FLAGS)
Z5_CLOAK_INPUTS := $(Z5_INPUTS) .local/sc64/zp-stage/generated/scenarios/sc64_z5_abilities_generated.h
P5_FLAGS := $(filter-out -DSC64_ZP_DEFENSE_TARGETS -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated,$(P4_STORM_FLAGS)) -I.local/sc64/p5/generated -I.local/sc64/zp-stage/generated -I.local/sc64/races/generated -I.local/sc64/zp-stage/generated/scenarios -DSC64_ZP_FULL_STORAGE -DSC64_P5_DATA_ENABLED -DSC64_P5_ENABLED -DSC64_P5_RUNTIME -DSC64_P5_CAMPAIGN_ENTRY -DSC64_P5_ACTORS -DSC64_Z6_ACTORS
P5_INPUTS := $(P4_INPUTS) .local/sc64/p5/generated/sc64_race_sprites_generated.h .local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h $(UNIT_FACING_HEADER)
P5_VALIDATED_QUERY_FLAGS := -DSC64_P5_TYPE_FIRST_COUNTS -DSC64_P5_PRUNE_STARTED -DSC64_P5_COUNT_SNAPSHOT
P5_QUERY_FLAGS ?= $(P5_VALIDATED_QUERY_FLAGS)
P5_SHUTTLE_RUNTIME_FLAGS := $(P5_FLAGS) -DSC64_P5_SHUTTLE_ENABLED -DSC64_ZP_DEFENSE_TARGETS
P5_SHUTTLE_FLAGS := $(P5_SHUTTLE_RUNTIME_FLAGS) $(P5_QUERY_FLAGS)
P6_VALIDATED_QUERY_FLAGS := -DSC64_P6_PRUNE_STARTED -DSC64_P6_PREVALIDATED_TICK -DSC64_P6_TRAP_CULL -DSC64_P6_COUNT_SNAPSHOT -DSC64_P6_BURROW_CULL -DSC64_P6_BURROW_GROUPED -DSC64_P6_RESCUE_GROUPED -DSC64_P6_STORM_POSTCHECK_LOCAL
P6_QUERY_FLAGS ?= $(P6_VALIDATED_QUERY_FLAGS)
P6_EXTRA_FLAGS ?=
Z6_VALIDATED_QUERY_FLAGS := -DSC64_Z6_COUNT_SNAPSHOT -DSC64_COLLISION_FACT_HOIST -DSC64_ZP_FACT_RANGE_CULL
Z6_QUERY_FLAGS ?= $(Z6_VALIDATED_QUERY_FLAGS)
Z6_EXTRA_FLAGS ?=
P6_CAMPAIGN_FLAGS := -DSC64_P6_DATA_ENABLED -DSC64_P6_RUNTIME -DSC64_P6_ACTORS -DSC64_Z5_ACTORS -DSC64_Z5_DOODADS -DSC64_P6_COMBAT_ENABLED -DSC64_P6_CAMPAIGN_ENTRY -DSC64_P6_STORM_ENABLED
P6_RUNTIME_FLAGS := -I.local/sc64/p6/generated $(P5_SHUTTLE_FLAGS) $(P6_CAMPAIGN_FLAGS)
P6_PROBE_FLAGS := $(P6_QUERY_FLAGS) $(P6_EXTRA_FLAGS) $(P6_RUNTIME_FLAGS)
# Differential fixtures select each query option explicitly, including off builds.
P6_QUERY_TEST_BASE_FLAGS = $(filter-out $(P6_VALIDATED_QUERY_FLAGS),$(P6_PROBE_FLAGS))
P6_PROBE_INPUTS := $(P5_INPUTS) .local/sc64/zp-stage/generated/scenarios/sc64_p6_abilities_generated.h .local/sc64/p6/generated/sc64_race_sprites_generated.h .local/sc64/p6/generated/sc64_p6_doodads_generated.h .local/sc64/zp-stage/generated/scenarios/sc64_p6_scenario_generated.h
Z6_PROBE_FLAGS := $(Z6_QUERY_FLAGS) $(Z6_EXTRA_FLAGS) -I.local/sc64/z6/generated $(Z5_CONSUME_FLAGS) -DSC64_Z6_DATA_ENABLED -DSC64_Z6_RUNTIME -DSC64_Z6_ACTORS -DSC64_Z6_CAMPAIGN_ENTRY -DSC64_Z6_CLOAK_ENABLED -DSC64_Z6_STORM_ENABLED -DSC64_Z6_TRANSPORT_ENABLED
# Keep explicit collision variants independent of enabled query defaults.
Z6_QUERY_TEST_BASE_FLAGS = $(filter-out $(Z6_VALIDATED_QUERY_FLAGS) -DSC64_COLLISION_CLASS_CACHE,$(Z6_PROBE_FLAGS))
Z6_PROBE_INPUTS := $(Z5_CLOAK_INPUTS) .local/sc64/zp-stage/generated/scenarios/sc64_z6_transport_generated.h .local/sc64/zp-stage/generated/scenarios/sc64_z6_abilities_generated.h .local/sc64/z6/generated/sc64_race_sprites_generated.h .local/sc64/zp-stage/generated/scenarios/sc64_z6_scenario_generated.h
Z7_PROBE_FLAGS := $(Z6_PROBE_FLAGS) -DSC64_Z7_DATA_ENABLED -DSC64_Z7_RUNTIME -DSC64_Z7_CAMPAIGN_ENTRY -DSC64_Z7_COLONY_DEFENSE_ENABLED
Z7_DERIVED_SAVE_EXTRA_FLAGS ?=
Z7_DERIVED_SAVE_FLAGS = $(Z7_SCOURGE_PROBE_FLAGS) -DSC64_NEAREST_RESOURCE_DEPOT -DSC64_Z7_DERIVED_SAVE $(Z7_DERIVED_SAVE_EXTRA_FLAGS)
Z7_PROBE_INPUTS := $(Z6_PROBE_INPUTS) .local/sc64/zp-stage/generated/scenarios/sc64_z7_scenario_generated.h
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
# Keep qualified canonical draw options separate from experimental campaign
# probes, which retain their own RAM and timing qualification.
CANONICAL_DRAW_FLAGS ?= -DGBA_SPRITE_WORDS -DGBA_SCENE_CLIP -DSC64_HUD_BUFFER_REUSE
CANONICAL_MINIMAP_FLAGS ?= -DSC64_MINIMAP_BLIT
# Render caches kept in spare VRAM and on the other mode-4 page instead of
# EWRAM. The tile-renderer probe needs that OBJ VRAM itself and stays out.
# The persistent minimap raster also consumes EWRAM. Keep it with the cache
# group that frees that memory, not with flags shared by the tighter Showcase
# and diagnostic products.
CANONICAL_CACHE_FLAGS ?= -DSC64_TERRAIN_CACHE_VRAM -DSC64_HUD_VRAM_BANDS -DSC64_TERRAIN_ROW_MASKS -DSC64_MINIMAP_RASTER
CANONICAL_STATIC_FLAGS ?= $(if $(filter -DGBA_SCENE_CLIP,$(CANONICAL_DRAW_FLAGS)),-DSC64_STATIC_DIRTY_ROWS)
TERRAN_UNIT_WORD_FLAGS ?= -DSC64_UNIT_WORD_SPRITES
# Per-slot cached collision facts and one-pass blocked steering for the race
# collision predicate. Empty this to compare a canonical ROM without it.
CONTACT_CACHE_FLAGS ?= -DSC64_CONTACT_CACHE
# The Z6 and P6 storage profiles have no IWRAM reserve left for the hot
# steering pass or the inlined stock step, so both stay in ROM there (the late
# Terran profile makes the same choice in sc64_contact.h). Probe builds that
# measure the IWRAM placement do not use these flags.
CONTACT_ROM_PLACEMENT_FLAGS ?= $(if $(filter -DSC64_CONTACT_CACHE,$(CONTACT_CACHE_FLAGS)),-DSC64_CONTACT_STEER_ROM -DSC64_CONTACT_STOCK_ROM)
TERRAN_CANONICAL_FLAGS ?= $(if $(filter -DSC64_ROUTE_PACK3,$(TERRAN_FULL_FLAGS)),-DSC64_ROUTE_FLOOD_SPLIT) $(TERRAN_UNIT_WORD_FLAGS) $(CONTACT_CACHE_FLAGS)
WEAPON_SOURCE_DIR := .local/sc64/weapon-vfx/generated
WEAPON_SOURCE_HEADER := $(WEAPON_SOURCE_DIR)/sc64_weapon_source_generated.h
ATTACK_POSE_DIR := .local/sc64/attack-poses/generated
ATTACK_POSE_HEADER := $(ATTACK_POSE_DIR)/sc64_unit_attack_source_generated.h
SOURCE_WEAPON_INPUTS := $(WEAPON_SOURCE_HEADER) $(ATTACK_POSE_HEADER)
SOURCE_WEAPON_FLAGS := -DSC64_WEAPON_SOURCE_VFX -I$(WEAPON_SOURCE_DIR) -DSC64_UNIT_ATTACK_POSES_ENABLED -I$(ATTACK_POSE_DIR)
# Cache only the immutable presentation duration for the four qualified source
# weapon products. Empty the override to compare or disable it on those builds.
CANONICAL_TRACE_DURATION_FLAGS ?= -DSC64_TRACE_DURATION_CACHE
CANONICAL_TRACE_DURATION_FOR_WEAPON_FLAGS = $(if $(filter -DSC64_WEAPON_SOURCE_VFX,$(CANONICAL_WEAPON_FLAGS)),$(CANONICAL_TRACE_DURATION_FLAGS))
# Limit culling to the four delivered profiles that have been qualified.
# Clearing this override or removing the source renderer from the canonical
# weapon flags removes culling from those products.
CANONICAL_SOURCE_VFX_CULL_FLAGS ?= -DSC64_WEAPON_SOURCE_VFX_OFFSCREEN_CULL
CANONICAL_SOURCE_VFX_CULL_FOR_WEAPON_FLAGS = $(if $(filter -DSC64_WEAPON_SOURCE_VFX,$(CANONICAL_WEAPON_FLAGS)),$(CANONICAL_SOURCE_VFX_CULL_FLAGS))
CANONICAL_WEAPON_FLAGS ?= -DSC64_TANK_SOURCE_VFX -I.local/sc64/tank-vfx/generated $(SOURCE_WEAPON_FLAGS)
# Direct firing points only replace a failed center route with a bounded set
# of clear corridors. Clearing this override opts out of the staged fix.
CANONICAL_DIRECT_FIRING_FLAGS ?= -DSC64_DIRECT_FIRING_POINTS
DEATH_ART_DIR := .local/sc64/death/generated
DEATH_ART_HEADER := $(DEATH_ART_DIR)/sc64_death_sprites_generated.h
CANONICAL_DEATH_FLAGS ?= -DSC64_DEATH_SOURCE_ART -I$(DEATH_ART_DIR)
CANONICAL_DEATH_INPUTS = $(if $(filter -DSC64_DEATH_SOURCE_ART,$(CANONICAL_DEATH_FLAGS)),$(DEATH_ART_HEADER))
TERRAN_ROM_FLAGS = $(TERRAN_FULL_FLAGS) $(CANONICAL_DRAW_FLAGS) $(CANONICAL_CACHE_FLAGS) -DGBA_WINDOW_BANDS $(CANONICAL_STATIC_FLAGS) $(TERRAN_CANONICAL_FLAGS) $(CANONICAL_MINIMAP_FLAGS) $(UNIT_FACING_FLAGS) $(CANONICAL_WEAPON_FLAGS) $(CANONICAL_SOURCE_VFX_CULL_FOR_WEAPON_FLAGS) $(CANONICAL_TRACE_DURATION_FOR_WEAPON_FLAGS) $(CANONICAL_DIRECT_FIRING_FLAGS) $(CANONICAL_DEATH_FLAGS)
ZERG_UNIT_WORD_FLAGS ?= -DSC64_UNIT_WORD_SPRITES
ZERG_CANONICAL_FLAGS ?= -DSC64_TERRAIN_SCROLL -DSC64_STATIC_KEY_CULL -DGBA_WINDOW_BANDS $(CANONICAL_STATIC_FLAGS) $(if $(filter -DSC64_ROUTE_PACK3,$(Z4_FLAGS)),-DSC64_ROUTE_FLOOD_SPLIT) -DSC64_COLLISION_FACT_HOIST -DSC64_ZP_FACT_RANGE_CULL $(ZERG_UNIT_WORD_FLAGS) $(CONTACT_CACHE_FLAGS) $(CONTACT_ROM_PLACEMENT_FLAGS)
# Explicit legacy Z1-Z4 product: SCZ4, save generation 20, unchanged flags.
ZERG_Z4_ROM_FLAGS = $(Z4_FLAGS) $(CANONICAL_DRAW_FLAGS) $(CANONICAL_CACHE_FLAGS) $(ZERG_CANONICAL_FLAGS) $(CANONICAL_MINIMAP_FLAGS) $(UNIT_FACING_FLAGS) $(CANONICAL_WEAPON_FLAGS) $(CANONICAL_DEATH_FLAGS)
# Canonical Z1-Z6: the Z6 storage/runtime/query vector, with a word-packed
# copy of its sprite bank in place of the probe bank, and canonical drawing.
# Like P5 Shuttle and Z7, it restores the defense-target lists, which keep
# the full-scan defense fallback out of IWRAM in the larger storage profiles.
ZERG_Z6_BANK := .local/sc64/zerg-z6/generated
ZERG_Z6_FLAGS = $(patsubst -I.local/sc64/z6/generated,-I$(ZERG_Z6_BANK),$(Z6_PROBE_FLAGS)) -DSC64_ZP_DEFENSE_TARGETS
ZERG_Z6_INPUTS = $(filter-out .local/sc64/z5/generated/sc64_race_sprites_generated.h .local/sc64/z6/generated/sc64_race_sprites_generated.h,$(Z6_PROBE_INPUTS)) $(ZERG_Z6_BANK)/sc64_race_sprites_generated.h
ZERG_ROM_FLAGS = $(ZERG_Z6_FLAGS) $(CANONICAL_DRAW_FLAGS) $(CANONICAL_CACHE_FLAGS) $(ZERG_CANONICAL_FLAGS) $(CANONICAL_MINIMAP_FLAGS) $(UNIT_FACING_FLAGS) $(CANONICAL_WEAPON_FLAGS) $(CANONICAL_SOURCE_VFX_CULL_FOR_WEAPON_FLAGS) $(CANONICAL_TRACE_DURATION_FOR_WEAPON_FLAGS) $(CANONICAL_DIRECT_FIRING_FLAGS) $(CANONICAL_DEATH_FLAGS)
ZERG_Z3_ROM_FLAGS = $(Z3_FLAGS) $(CANONICAL_DRAW_FLAGS)
PROTOSS_P3_CANONICAL_FLAGS ?= $(if $(filter -DSC64_ROUTE_PACK3,$(P3_FLAGS)),-DSC64_ROUTE_FLOOD_SPLIT)
PROTOSS_P3_ROM_FLAGS = $(P3_FLAGS) $(CANONICAL_DRAW_FLAGS) -DGBA_WINDOW_BANDS $(CANONICAL_STATIC_FLAGS) $(PROTOSS_P3_CANONICAL_FLAGS)
PROTOSS_UNIT_WORD_FLAGS ?= -DSC64_UNIT_WORD_SPRITES
PROTOSS_CANONICAL_FLAGS ?= $(PROTOSS_P3_CANONICAL_FLAGS) -DSC64_COLLISION_FACT_HOIST -DSC64_ZP_FACT_RANGE_CULL -DSC64_P4_COUNT_CENSUS_ENABLED $(PROTOSS_UNIT_WORD_FLAGS) $(CONTACT_CACHE_FLAGS) $(CONTACT_ROM_PLACEMENT_FLAGS)
P4_CREATE_PREFIX_FLAGS ?= -DSC64_P4_CREATE_PREFIX -DSC64_P4_VALIDATOR_HOT
PROTOSS_P4_STORM_ROM_FLAGS = $(P4_CREATE_PREFIX_FLAGS) $(P4_STORM_FLAGS) $(CANONICAL_DRAW_FLAGS) $(CANONICAL_CACHE_FLAGS) -DGBA_WINDOW_BANDS $(CANONICAL_STATIC_FLAGS) $(PROTOSS_CANONICAL_FLAGS) $(CANONICAL_MINIMAP_FLAGS)
PROTOSS_P5_CANONICAL_FLAGS = $(P4_CREATE_PREFIX_FLAGS) $(P5_SHUTTLE_FLAGS) $(CANONICAL_DRAW_FLAGS) $(CANONICAL_CACHE_FLAGS) -DGBA_WINDOW_BANDS $(CANONICAL_STATIC_FLAGS) $(PROTOSS_CANONICAL_FLAGS) $(CANONICAL_MINIMAP_FLAGS) $(UNIT_FACING_FLAGS) $(CANONICAL_WEAPON_FLAGS) $(CANONICAL_DEATH_FLAGS)
# Pin the carried P5 and P6 query choices independently of probe overrides.
PROTOSS_P6_SPRITE_HEADER := .local/sc64/protoss-p6/generated/sc64_race_sprites_generated.h
PROTOSS_P6_CANONICAL_FLAGS = $(P4_CREATE_PREFIX_FLAGS) $(P6_VALIDATED_QUERY_FLAGS) -I.local/sc64/protoss-p6/generated -I.local/sc64/p6/generated $(P5_SHUTTLE_RUNTIME_FLAGS) $(P5_VALIDATED_QUERY_FLAGS) $(P6_CAMPAIGN_FLAGS) $(CANONICAL_DRAW_FLAGS) $(CANONICAL_CACHE_FLAGS) -DGBA_WINDOW_BANDS $(CANONICAL_STATIC_FLAGS) $(PROTOSS_CANONICAL_FLAGS) $(CANONICAL_MINIMAP_FLAGS) $(UNIT_FACING_FLAGS) $(CANONICAL_WEAPON_FLAGS) $(CANONICAL_DEATH_FLAGS)
PROTOSS_P6_CANONICAL_INPUTS = $(filter-out .local/sc64/p6/generated/sc64_race_sprites_generated.h,$(P6_PROBE_INPUTS)) $(PROTOSS_P6_SPRITE_HEADER) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS)
PROTOSS_P6_ROM_FLAGS = $(PROTOSS_P6_CANONICAL_FLAGS)
# Canonical Protoss now carries the complete P1-P7 storage/runtime profile.
# Use a distinct all-chapter bank so the P7 probe bank and explicit P6 bank
# remain independent, and put it first so the renderer cannot pick a probe bank.
PROTOSS_P7_SPRITE_DIR := .local/sc64/protoss-p7/generated
PROTOSS_P7_SPRITE_HEADER := $(PROTOSS_P7_SPRITE_DIR)/sc64_race_sprites_generated.h
# P7 has room for the steering pass and inline stock step in IWRAM. Keep the
# P6 placement choices and historical P7 probes independent of this profile.
PROTOSS_P7_CANONICAL_FLAGS = -I$(PROTOSS_P7_SPRITE_DIR) $(P7_FACT_V2_FLAGS) $(CANONICAL_DRAW_FLAGS) $(CANONICAL_CACHE_FLAGS) -DGBA_WINDOW_BANDS $(CANONICAL_STATIC_FLAGS) $(filter-out -DSC64_CONTACT_STEER_ROM -DSC64_CONTACT_STOCK_ROM,$(PROTOSS_CANONICAL_FLAGS)) $(CANONICAL_MINIMAP_FLAGS) $(UNIT_FACING_FLAGS) $(CANONICAL_WEAPON_FLAGS) $(CANONICAL_SOURCE_VFX_CULL_FOR_WEAPON_FLAGS) $(CANONICAL_TRACE_DURATION_FOR_WEAPON_FLAGS) $(CANONICAL_DIRECT_FIRING_FLAGS) $(CANONICAL_DEATH_FLAGS) -DSC64_P7_ARBITER_AURA_ONCE
PROTOSS_P7_CANONICAL_INPUTS = $(filter-out .local/sc64/p6/generated/sc64_race_sprites_generated.h .local/sc64/p7/generated/sc64_race_sprites_generated.h,$(P7_ARBITER_INPUTS)) $(PROTOSS_P7_SPRITE_HEADER) $(P7_FACT_V2_HEADER) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS)
PROTOSS_ROM_FLAGS = $(PROTOSS_P7_CANONICAL_FLAGS)
PROTOSS_CANONICAL_INPUTS = $(PROTOSS_P7_CANONICAL_INPUTS)
TERRAN_FULL_INPUTS = $(RACE_CAMPAIGN_INPUTS) $(LATE_TERRAN_HEADERS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) $(UNIT_FACING_HEADER)

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
	terran zerg zerg-z3 protoss protoss-p6 verify-terran verify-zerg verify-zerg-z3 verify-protoss verify-protoss-p6 verify-race-campaigns \
	test-terran test-zerg test-zerg-z3 test-protoss test-protoss-p6 test-sc64-protoss-p7-canonical test-race-campaigns test-sc64-t4-import \
	test-sc64-t4-storage sc64-t4-storage-probe

# The public default uses only original-art StarCraft sources.
all: verify
verify: build/StarCraft-GBA.gba
	$(PYTHON) tools/verify_roms.py

build:
	mkdir -p build

.PHONY: test-gfx-sprite
build/test-gfx-sprite: tests/gfx_sprite.c common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fno-strict-aliasing -fsanitize=address,undefined -DGBA_HOST -Icommon $< common/gba.c -o $@
build/test-gfx-sprite-words: tests/gfx_sprite.c common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fno-strict-aliasing -fsanitize=address,undefined -DGBA_HOST -DGBA_SPRITE_WORDS -Icommon $< common/gba.c -o $@
test-gfx-sprite: build/test-gfx-sprite build/test-gfx-sprite-words
	./build/test-gfx-sprite
	./build/test-gfx-sprite-words
test: test-gfx-sprite

.PHONY: test-gfx-window
build/test-gfx-window: tests/gfx_window.c common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fno-strict-aliasing -fsanitize=address,undefined -DGBA_HOST -Icommon $< common/gba.c -o $@
build/test-gfx-window-bands: tests/gfx_window.c common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fno-strict-aliasing -fsanitize=address,undefined -DGBA_HOST -DGBA_WINDOW_BANDS -Icommon $< common/gba.c -o $@
test-gfx-window: build/test-gfx-window build/test-gfx-window-bands
	./build/test-gfx-window
	./build/test-gfx-window-bands
test: test-gfx-window

.PHONY: test-gfx-clip
build/test-gfx-clip: tests/gfx_clip.c common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fno-strict-aliasing -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -Icommon $< common/gba.c -o $@
build/test-gfx-clip-packed: tests/gfx_clip.c common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fno-strict-aliasing -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -DGBA_SPRITE_WORDS -DGBA_WINDOW_BANDS -Icommon $< common/gba.c -o $@
test-gfx-clip: build/test-gfx-clip build/test-gfx-clip-packed
	./build/test-gfx-clip
	./build/test-gfx-clip-packed
test: test-gfx-clip

.PHONY: test-sc64-static-dirty-rows
build/test-sc64-static-dirty-rows: tests/sc64_static_dirty_rows.c starcraft/sc64_static_dirty_rows.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
test-sc64-static-dirty-rows: build/test-sc64-static-dirty-rows
	./build/test-sc64-static-dirty-rows
test: test-sc64-static-dirty-rows

.PHONY: test-sc64-static-dirty-rows-game
STATIC_DIRTY_ROWS_TEST_FLAGS := -DGBA_SCENE_CLIP -DSC64_STATIC_DIRTY_ROWS
build/test-sc64-static-dirty-rows-t4: tests/sc64_t4_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fno-strict-aliasing -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_RUNTIME_FLAGS) $(STATIC_DIRTY_ROWS_TEST_FLAGS) $< common/gba.c -o $@
build/test-sc64-static-dirty-rows-photon: tests/sc64_photon_cannon_renderer.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fno-strict-aliasing -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) $(STATIC_DIRTY_ROWS_TEST_FLAGS) $< common/gba.c -o $@
test-sc64-static-dirty-rows-game: build/test-sc64-static-dirty-rows-t4 build/test-sc64-static-dirty-rows-photon
	./build/test-sc64-static-dirty-rows-t4
	./build/test-sc64-static-dirty-rows-photon
test-canonical-render: test-sc64-static-dirty-rows-game

build/starcraft.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft.map -lgcc -o $@
	$(SIZE) $@

build/StarCraft-GBA.gba: build/starcraft.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC GBA DEMO' SCGD

build/test-starcraft: tests/host.c common/gba.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon tests/host.c common/gba.c $(STARCRAFT) -o $@

# Optional hold-B command ring for the default profile (docs/COMMAND-RING.md).
COMMAND_RING_FLAGS := -DSC_COMMAND_RING
build/starcraft-ring.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(COMMAND_RING_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-ring.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-GBA-Ring.gba: build/starcraft-ring.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC GBA RING' SCGR
.PHONY: ring test-command-ring
ring: build/StarCraft-GBA-Ring.gba
build/test-command-ring: tests/command_ring_run.c common/gba.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST $(COMMAND_RING_FLAGS) -Icommon tests/command_ring_run.c common/gba.c -o $@
test-command-ring: build/test-command-ring
	./build/test-command-ring

build/test-field-ui: tests/field_ui_run.c common/gba.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon tests/field_ui_run.c common/gba.c -o $@
# The ring build also gets the default renderer; run the same fixture there.
build/test-field-ui-ring: tests/field_ui_run.c common/gba.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC_COMMAND_RING -Icommon tests/field_ui_run.c common/gba.c -o $@

build/test-starcraft-selection: tests/sc64_selection_run.c common/gba.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon tests/sc64_selection_run.c common/gba.c -o $@

build/test-race-rules: tests/race_rules.c starcraft/race_rules.h starcraft/race_skirmish.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined tests/race_rules.c -o $@

build/test-sc64-weapon-vfx: tests/sc64_weapon_vfx.c starcraft/sc64_weapon_vfx.h starcraft/sc64_weapon_vfx_render.h common/gba.h | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-weapon-vfx
test-sc64-weapon-vfx: build/test-sc64-weapon-vfx
	./build/test-sc64-weapon-vfx

# The default profile's units draw a walk cycle while they move.
build/test-sc64-unit-walk-game: tests/sc64_unit_walk_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $< common/gba.c -o $@
test-sc64-unit-walk-game: build/test-sc64-unit-walk-game
	./build/test-sc64-unit-walk-game
.PHONY: test-sc64-unit-walk-game
test: test-sc64-unit-walk-game

build/test-sc64-weapon-vfx-game: tests/sc64_weapon_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-weapon-vfx-game
test-sc64-weapon-vfx-game: build/test-sc64-weapon-vfx-game
	./build/test-sc64-weapon-vfx-game

build/test-sc64-shield-bars: tests/sc64_shield_bars_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-shield-bars
test-sc64-shield-bars: build/test-sc64-shield-bars
	./build/test-sc64-shield-bars

build/test-skirmish-ai: tests/skirmish_ai.c starcraft/skirmish_ai.h | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined tests/skirmish_ai.c -o $@

build/test-sc64-frontend: tests/sc64_frontend.c starcraft/sc64_frontend.h starcraft/campaign_catalog.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon tests/sc64_frontend.c -o $@

build/test-sc64-demo-schedule: tests/sc64_demo_schedule.c common/demo_schedule.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I. -Icommon $< -o $@

build/test-sc64-demo-schedule-runtime: tests/sc64_demo_schedule_runtime.c common/main.c common/demo_schedule.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $< -o $@

build/test-gba-no-debug-inputs: tests/gba_no_debug_inputs.c common/main.c common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -DGBA_NO_DEBUG_INPUTS -DSC64_RACES -DSC64_UNIFIED -DSC64_Z3_ENABLED -DSC64_CAMPAIGN_RACE=1 -DSC64_Z3_WINDOWED_SCHEDULE -DSC64_DENSE_SCHEDULE -Icommon $< -o $@

.PHONY: test-gba-no-debug-inputs
test-gba-no-debug-inputs: build/test-gba-no-debug-inputs build/test-sc64-demo-schedule-runtime
	./build/test-gba-no-debug-inputs
	./build/test-sc64-demo-schedule-runtime

build/test-sc64-dense-schedule: tests/sc64_demo_schedule_runtime.c common/main.c common/demo_schedule.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_DENSE_SCHEDULE -Icommon $< -o $@

.PHONY: test-sc64-dense-schedule
test-sc64-dense-schedule: build/test-sc64-dense-schedule
	./build/test-sc64-dense-schedule
test-starcraft: test-sc64-dense-schedule test-gba-no-debug-inputs

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
# Legacy P3 keeps its original real files and save identity. Canonical P4
# uses the distinct Storm filenames below; never point these aliases back
# through the canonical target and create a symlink cycle.
build/starcraft-protoss-p3.elf: build/starcraft-protoss.elf | build
	ln -sf starcraft-protoss.elf $@
build/StarCraft-Protoss-P3-GBA.gba: build/StarCraft-Protoss-GBA.gba | build
	ln -sf StarCraft-Protoss-GBA.gba $@
protoss-p3: verify-protoss-p3 build/starcraft-protoss-p3.elf build/StarCraft-Protoss-P3-GBA.gba
build/test-sc64-p3-run: tests/sc64_p3_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-save: tests/sc64_p3_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_P3_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-controller: tests/sc64_p3_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_P3_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-unarmed-targets: tests/sc64_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) -DSC64_UNARMED_QUERY_SKIP $< common/gba.c -o $@
.PHONY: test-sc64-p3-unarmed-targets
test-sc64-p3-unarmed-targets: build/test-sc64-p3-unarmed-targets
	./build/test-sc64-p3-unarmed-targets
build/test-sc64-p3-defenses: tests/sc64_p3_defense_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P3_FLAGS) $< common/gba.c -o $@
build/test-sc64-p3-frame-coverage: tests/sc64_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_P3_ROM_FLAGS) $< common/gba.c -o $@
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
	./build/test-sc64-p3-controller --trace .local/sc64/p3-controller-stock-asan-win.json
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
build/test-sc64-t5-mines: tests/sc64_t5_mines.c starcraft/sc64_t5_mine_adapter.h starcraft/sc64_weapon_vfx.h common/gba.h Makefile | build
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

# Asset-free Nydus Canal link/eligibility/exit rules; gameplay stays opt-in.
build/test-sc64-nydus-rules: tests/sc64_nydus_rules.c starcraft/sc64_nydus_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-nydus-rules
test-sc64-nydus-rules: build/test-sc64-nydus-rules
	./build/test-sc64-nydus-rules
test-starcraft: test-sc64-nydus-rules
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

build/test-sc64-construction-save: tests/sc64_construction_save.c starcraft/sc64_construction_save.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-construction-save
test-sc64-construction-save: build/test-sc64-construction-save
	./build/test-sc64-construction-save

test-starcraft: test-sc64-compact-building-save test-sc64-construction-save
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

# New Dominion is retained as an explicit legacy build. The original
# unversioned outputs remain its real files; versioned names are aliases.
.PHONY: zerg-z3 verify-zerg-z3 test-zerg-z3 test-sc64-z3-runtime
build/starcraft-zerg-z3.elf: build/starcraft-zerg.elf | build
	ln -sf starcraft-zerg.elf $@
build/StarCraft-Zerg-Z3-GBA.gba: build/StarCraft-Zerg-GBA.gba | build
	ln -sf StarCraft-Zerg-GBA.gba $@
verify-zerg-z3: build/StarCraft-Zerg-GBA.gba build/starcraft-zerg.elf
	$(PYTHON) tools/verify_roms.py --zerg-z3
zerg-z3: verify-zerg-z3 build/starcraft-zerg-z3.elf build/StarCraft-Zerg-Z3-GBA.gba
build/test-sc64-z3-run: tests/sc64_z3_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
build/test-sc64-z3-save: tests/sc64_z3_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
build/test-sc64-z3-controller: tests/sc64_z3_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z3_FLAGS) $< common/gba.c -o $@
build/test-sc64-z3-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z3_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-z3-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z3_ROM_FLAGS) $< common/gba.c -o $@
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

build/test-terrain-key-once: tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_IMPORTED_MISSION -DSC64_RENDER_KEY_ONCE $< -o $@
build/test-terrain-key-once-scroll: tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_TERRAIN_CACHE_REAL_GFX -DSC64_TERRAIN_SCROLL -DSC64_IMPORTED_MISSION -DSC64_RENDER_KEY_ONCE -Icommon $< common/gba.c -o $@
.PHONY: test-terrain-key-once
test-terrain-key-once: build/test-terrain-key-once build/test-terrain-key-once-scroll
	./build/test-terrain-key-once
	./build/test-terrain-key-once-scroll
test-starcraft: test-terrain-key-once

define RENDER_KEY_ONCE_PROFILE
build/test-sc64-render-key-once-$(1): tests/sc64_z3_frame_coverage.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $(3) common/gba.c common/gba.h Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(2) -DSC64_RENDER_KEY_ONCE $$< common/gba.c -o $$@
endef
$(eval $(call RENDER_KEY_ONCE_PROFILE,p5,$(P5_SHUTTLE_FLAGS),$(P5_INPUTS)))
$(eval $(call RENDER_KEY_ONCE_PROFILE,p6,$(P6_PROBE_FLAGS),$(P6_PROBE_INPUTS)))
$(eval $(call RENDER_KEY_ONCE_PROFILE,z6,$(Z6_PROBE_FLAGS),$(Z6_PROBE_INPUTS)))
.PHONY: test-sc64-render-key-once
test-sc64-render-key-once: build/test-sc64-render-key-once-p5 build/test-sc64-render-key-once-p6 build/test-sc64-render-key-once-z6
	./build/test-sc64-render-key-once-p5
	./build/test-sc64-render-key-once-p6
	./build/test-sc64-render-key-once-z6

build/test-collision: tests/sc64_collision.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -DSC64_SKIRMISH -Icommon tests/sc64_collision.c common/gba.c -o $@

test-starcraft: build/test-sc64-resource-actions build/test-sc64-t5-actor-rules build/test-sc64-t4-actor-rules build/test-sc64-t4-world build/test-sc64-t4-schedule build/test-sc64-t4-count-cache build/test-sc64-p3-schedule build/test-sc64-p3-runtime $(P3_FRONT_TESTS) build/test-sc64-demo-schedule build/test-sc64-demo-schedule-runtime build/test-starcraft build/test-starcraft-selection build/test-field-ui build/test-field-ui-ring build/test-command-ring build/test-race-rules build/test-race-rules-z3 build/test-skirmish-ai build/test-sc64-frontend build/test-sc64-bitgrid build/test-sc64-map-math build/test-sc64-t3-rules build/test-sc64-z3-rules build/test-sc64-p3-rules build/test-sc64-p3-model test-sc64-weapon-vfx $(RACE_FRONTEND_TESTS) $(RACE_T3_FRONTEND_TESTS) $(RACE_Z3_FRONTEND_TESTS) $(RACE_Z3_UNIFIED_TESTS) build/test-sc64-frontend-t3-unified build/test-race-routes build/test-race-routes-96 build/test-race-routes-t3 build/test-race-routes-rect build/test-race-routes-large build/test-race-routes-pack3 build/test-race-routes-pack3-t3 build/test-race-routes-pack3-large build/test-terrain-cache build/test-terrain-scroll build/test-collision
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
	./build/test-field-ui
	./build/test-field-ui-ring
	./build/test-command-ring
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

build/test-sc64-no-depot-harvest: tests/sc64_no_depot_harvest.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $< common/gba.c -o $@
build/test-sc64-no-depot-harvest-z7: tests/sc64_no_depot_harvest.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_SCOURGE_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-no-depot-harvest test-sc64-no-depot-harvest-z7
test-sc64-no-depot-harvest: build/test-sc64-no-depot-harvest
	./build/test-sc64-no-depot-harvest
test-sc64-no-depot-harvest-z7: build/test-sc64-no-depot-harvest-z7
	./build/test-sc64-no-depot-harvest-z7
test-starcraft: test-sc64-no-depot-harvest

build/test-sc64-active-hall-depots-z6: tests/sc64_active_hall_depot.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-active-hall-depots-z7: tests/sc64_active_hall_depot.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_SCOURGE_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-active-hall-depots
test-sc64-active-hall-depots: build/test-sc64-active-hall-depots-z6 build/test-sc64-active-hall-depots-z7
	./build/test-sc64-active-hall-depots-z6
	./build/test-sc64-active-hall-depots-z7

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
	$(CC) $(CFLAGS) $(TERRAN_ROM_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-terran.map -lgcc -o $@
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
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-terran-full-frame: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-protoss-canonical-frame: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-protoss-canonical-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-zerg-canonical-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z6_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_ROM_FLAGS) $< common/gba.c -o $@
.PHONY: test-canonical-render
test-canonical-render: build/test-sc64-terran-full-frame build/test-sc64-terran-full-hud build/test-sc64-z4-frame-coverage build/test-sc64-zerg-canonical-hud build/test-sc64-protoss-canonical-frame build/test-sc64-protoss-canonical-hud
	./build/test-sc64-terran-full-frame
	./build/test-sc64-terran-full-hud
	./build/test-sc64-z4-frame-coverage
	./build/test-sc64-zerg-canonical-hud
	./build/test-sc64-protoss-canonical-frame
	./build/test-sc64-protoss-canonical-hud

build/test-sc64-geyser-art: tests/sc64_geyser_art.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) $(TANK_SOURCE_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-geyser-art
test-sc64-geyser-art: build/test-sc64-geyser-art
	./build/test-sc64-geyser-art
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

# Explicit legacy four-entry Zerg ROM; the paired Z3 target above also remains.
build/starcraft-zerg-z4.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(ZERG_Z4_ROM_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z4.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z4-GBA.gba: build/starcraft-zerg-z4.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z4' SCZ4
.PHONY: zerg-z4 verify-zerg-z4
verify-zerg-z4: build/StarCraft-Zerg-Z4-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z4
zerg-z4: verify-zerg-z4

# Canonical six-entry Zerg ROM: SCZ6, save generation 32.
build/starcraft-zerg-z6.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z6_INPUTS) $(TANK_SOURCE_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(ZERG_ROM_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z6.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z6-GBA.gba: build/starcraft-zerg-z6.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z6' SCZ6

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
	$(CC) $(CFLAGS) $(PROTOSS_P4_STORM_ROM_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p4-storm.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P4-Storm-GBA.gba: build/starcraft-protoss-p4-storm.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P4 STORM' SCP5
.PHONY: protoss-p4-storm
protoss-p4-storm: build/StarCraft-Protoss-P4-Storm-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p4-storm
build/test-protoss-p4-storm: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P4_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_P4_STORM_ROM_FLAGS) $< common/gba.c -o $@
.PHONY: test-protoss-p4-storm
test-protoss-p4-storm: build/test-protoss-p4-storm test-sc64-p4-runtime test-sc64-p4-campaign test-sc64-p4-occupancy-targets
	./$<

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
	$(CC) $(CFLAGS) $(PROTOSS_P5_CANONICAL_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p5-shuttle.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P5-Shuttle-GBA.gba: build/starcraft-protoss-p5-shuttle.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P5 CARGO' SCPB
.PHONY: protoss-p5-shuttle
protoss-p5-shuttle: build/StarCraft-Protoss-P5-Shuttle-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p5-shuttle

build/starcraft-protoss-p6.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P6_CANONICAL_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(PROTOSS_P6_ROM_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p6.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P6-GBA.gba: build/starcraft-protoss-p6.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC PROTOSS6' SCP6
.PHONY: protoss-p6
protoss-p6: verify-protoss-p6

build/starcraft-protoss-p7.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P7_CANONICAL_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(PROTOSS_P7_CANONICAL_FLAGS) $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-GBA.gba: build/starcraft-protoss-p7.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC PROTOSS7' SCP7
.PHONY: protoss-p7
protoss-p7: verify-protoss

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

# Canonical P5 regression uses the same strict feature set as the promoted ROM.
build/test-sc64-p5-canonical-runtime: tests/sc64_p5_shuttle_runtime.c tests/sc64_p5_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_P5_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-p5-canonical-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_P5_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-p5-canonical-frame: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_P5_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-p5-canonical-campaign-run: tests/sc64_p5_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_P5_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p5-canonical
test-sc64-p5-canonical: test-sc64-p5-transport build/test-sc64-p5-canonical-runtime build/test-sc64-p5-canonical-profile build/test-sc64-p5-canonical-frame build/test-sc64-p5-canonical-campaign-run
	./build/test-sc64-p5-canonical-runtime
	./build/test-sc64-p5-canonical-profile
	./build/test-sc64-p5-canonical-frame
	./build/test-sc64-p5-canonical-campaign-run --limit 180000 --attempt-tag canonical-p5

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

build/starcraft-zerg-z5-consume.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_CLOAK_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z5_CONSUME_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z5-consume.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z5-Consume-GBA.gba: build/starcraft-zerg-z5-consume.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z5 FEED' SCZD
.PHONY: zerg-z5-consume
zerg-z5-consume: build/StarCraft-Zerg-Z5-Consume-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z5-consume

build/test-sc64-z5-consume-runtime: tests/sc64_z5_consume_runtime.c tests/sc64_z5_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_CLOAK_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_CONSUME_FLAGS) $< common/gba.c -o $@
build/test-sc64-z5-consume-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_CLOAK_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_CONSUME_FLAGS) $< common/gba.c -o $@
build/test-sc64-z5-consume-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_CLOAK_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_CONSUME_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z5-consume-runtime
test-sc64-z5-consume-runtime: test-sc64-z5-ability-models build/test-sc64-z5-consume-runtime build/test-sc64-z5-consume-profile build/test-sc64-z5-consume-frame-coverage
	./build/test-sc64-z5-consume-runtime
	./build/test-sc64-z5-consume-profile
	./build/test-sc64-z5-consume-frame-coverage

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
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z4_ROM_FLAGS) $< common/gba.c -o $@
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
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z4_ROM_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z4-runtime test-sc64-p4-runtime test-sc64-p4-create-prefix test-sc64-zp-storage


$(P4_SCENARIO_HEADER): tools/sc64_p4_scenario_pack.py tools/sc64_p4_pack.py \
		.local/sc64/p4/source.chk .local/sc64/p4/manifest.json \
		$(wildcard .local/sc64/p4/records/*.bin) | build
	$(PYTHON) tools/sc64_p4_scenario_pack.py --output $@ \
		--report .local/sc64/p4/generated/scenario-audit.json

build/test-sc64-p4-create-prefix: tests/sc64_p4_create_prefix.c $(STARCRAFT) \
		$(wildcard starcraft/*.h) $(P4_INPUTS) \
		common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror \
		-Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined \
		-DGBA_HOST -Icommon $(PROTOSS_P4_STORM_ROM_FLAGS) $< common/gba.c -o $@

build/test-sc64-p4-runtime-fast: tests/sc64_p4_runtime.c $(STARCRAFT) \
		$(wildcard starcraft/*.h) $(P4_INPUTS) \
		common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST \
		-Icommon $(PROTOSS_P4_STORM_ROM_FLAGS) $< common/gba.c -o $@

test-sc64-p4-create-prefix: build/test-sc64-p4-create-prefix build/test-sc64-p4-runtime-fast
	./build/test-sc64-p4-create-prefix
	./build/test-sc64-p4-runtime-fast

test-sc64-z4-runtime: build/test-sc64-z4-adapter build/test-sc64-z4-runtime build/test-sc64-zp-actor-engine build/test-sc64-zp-air-tech build/test-sc64-z4-frame-coverage build/test-sc64-z4-profile
	./build/test-sc64-z4-adapter
	./build/test-sc64-zp-actor-engine
	./build/test-sc64-zp-air-tech
	./build/test-sc64-z4-runtime
	./build/test-sc64-z4-frame-coverage
	./build/test-sc64-z4-profile
test-sc64-p4-runtime: build/test-sc64-p4-adapter build/test-sc64-p4-runtime build/test-sc64-p4-frame-coverage build/test-sc64-p4-profile test-sc64-p4-create-prefix
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
.PHONY: test-sc64-p4-occupancy-targets
test-sc64-p4-occupancy-targets: build/test-sc64-p4-occupancy-targets
	./build/test-sc64-p4-occupancy-targets
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

build/test-sc64-p5-count-queries: tests/sc64_p5_count_queries.c tests/sc64_p5_world.c starcraft/sc64_p5_world_adapter.h starcraft/sc64_p5_source_model.h starcraft/sc64_p5_adapter.h starcraft/sc64_p5.h .local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
build/test-sc64-p5-count-snapshot: tests/sc64_p5_count_snapshot.c tests/sc64_p5_world.c starcraft/sc64_p5_count_snapshot.h starcraft/sc64_p5_world_adapter.h starcraft/sc64_p5_adapter.h .local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
build/test-sc64-p5-trigger-prune: tests/sc64_p5_trigger_prune.c starcraft/sc64_p5_adapter.h starcraft/sc64_p5.h .local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
.PHONY: test-sc64-p5-count-queries
test-sc64-p5-count-queries: build/test-sc64-p5-count-queries build/test-sc64-p5-count-snapshot build/test-sc64-p5-trigger-prune
	./build/test-sc64-p5-count-queries
	./build/test-sc64-p5-count-snapshot
	./build/test-sc64-p5-trigger-prune

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
build/test-sc64-z5-consume: tests/sc64_z5_consume.c starcraft/sc64_z5_consume.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-z5-ensnare: tests/sc64_z5_ensnare.c starcraft/sc64_z5_ensnare.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@

build/test-sc64-z5-detection: tests/sc64_z5_detection.c starcraft/sc64_z5_detection.h starcraft/sc64_z5_actor_rules.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-z5-ability-models
test-sc64-z5-ability-models: build/test-sc64-z5-cloak build/test-sc64-z5-detection build/test-sc64-z5-consume build/test-sc64-z5-ensnare
	./build/test-sc64-z5-cloak
	./build/test-sc64-z5-detection
	./build/test-sc64-z5-consume
	./build/test-sc64-z5-ensnare
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
build/test-sc64-p5-defense-targets: tests/sc64_zp_defense_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(P5_SHUTTLE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p5-defense-targets
test-sc64-p5-defense-targets: build/test-sc64-p5-defense-targets
	./build/test-sc64-p5-defense-targets

.PHONY: test-sc64-zp-defense-targets
test-sc64-zp-defense-targets: build/test-sc64-z4-defense-targets build/test-sc64-p4-defense-targets build/test-sc64-z5-defense-targets build/test-sc64-p5-defense-targets
	./build/test-sc64-z4-defense-targets
	./build/test-sc64-p4-defense-targets
	./build/test-sc64-z5-defense-targets
	./build/test-sc64-p5-defense-targets
test-sc64-terran-full-targets: build/test-sc64-terran-full-targets
	./build/test-sc64-terran-full-targets
test-sc64-terran-full-spatial-hud: build/test-sc64-terran-full-collision build/test-sc64-terran-full-hud
	./build/test-sc64-terran-full-collision
	./build/test-sc64-terran-full-hud
test-sc64-terran-full-campaign: test-sc64-terran-defense-targets test-sc64-terran-full-targets test-sc64-terran-full-spatial-hud

build/test-terran: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-terran-full-profile: tests/sc64_terran_full_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) $< common/gba.c -o $@
test-terran: build/test-terran build/test-sc64-terran-full-profile
	./build/test-terran
	./build/test-sc64-terran-full-profile

build/starcraft-zerg.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(ZERG_Z3_ROM_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-GBA.gba: build/starcraft-zerg.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z3' SCZ3
verify-zerg: build/StarCraft-Zerg-Z6-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg
build/test-zerg: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z6_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_ROM_FLAGS) tests/sc64_split_run.c common/gba.c -o $@
build/test-sc64-zerg-promoted-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z6_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_ROM_FLAGS) $< common/gba.c -o $@
test-zerg: build/test-zerg build/test-sc64-zerg-promoted-profile
	./build/test-zerg
	./build/test-sc64-zerg-promoted-profile
.PHONY: test-zerg-z6
test-zerg-z6: test-zerg
test-zerg-z3: test-sc64-z3-runtime
# Explicit legacy four-entry Zerg product.
build/test-zerg-z4: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z4_ROM_FLAGS) tests/sc64_split_run.c common/gba.c -o $@
build/test-sc64-zerg-z4-promoted-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z4_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z4_ROM_FLAGS) $< common/gba.c -o $@
.PHONY: test-zerg-z4
test-zerg-z4: build/test-zerg-z4 build/test-sc64-zerg-z4-promoted-profile test-sc64-z4-campaign
	./build/test-zerg-z4
	./build/test-sc64-zerg-z4-promoted-profile

build/starcraft-protoss.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(PROTOSS_P3_ROM_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-GBA.gba: build/starcraft-protoss.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC PROTOSS3' SCP3
verify-protoss: build/StarCraft-Protoss-P7-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss
.PHONY: verify-protoss-p6
verify-protoss-p6: build/StarCraft-Protoss-P6-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p6
verify-protoss-p3: build/StarCraft-Protoss-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p3
build/test-protoss: tests/sc64_split_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) tests/sc64_split_run.c common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-protoss-promoted-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-protoss-canonical-storm: tests/sc64_p4_storm.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-protoss-canonical-storage: tests/sc64_zp_storage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-protoss-canonical-controller: tests/sc64_p4_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-p4-census: tests/sc64_p4_census.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(filter-out -DSC64_P4_COUNT_CENSUS_ENABLED,$(PROTOSS_ROM_FLAGS)) -DSC64_P4_COUNT_CENSUS_ENABLED $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-p4-census
test-sc64-p4-census: build/test-sc64-p4-census
	./build/test-sc64-p4-census


test-protoss: build/test-protoss build/test-sc64-protoss-promoted-profile build/test-sc64-protoss-canonical-storm build/test-sc64-protoss-canonical-storage build/test-sc64-protoss-canonical-controller test-sc64-p4-census test-sc64-p4-create-prefix test-sc64-p5-canonical
	./build/test-protoss
	./build/test-sc64-protoss-promoted-profile
	./build/test-sc64-protoss-canonical-storm
	./build/test-sc64-protoss-canonical-storage
	./build/test-sc64-protoss-canonical-controller --limit 180000 --attempt-tag canonical-qualified

# P6 gameplay and codec checks must exercise the explicit P6 feature vector,
# independently of the tunable internal probe target.
build/test-sc64-protoss-canonical-p6: tests/sc64_p6_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P6_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_P6_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-protoss-canonical-p6-save: tests/sc64_p6_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P6_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_P6_ROM_FLAGS) $< common/gba.c -o $@
PROTOSS_P6_CONTROLLER_TRACE ?= .local/sc64/p6-canonical-controller-win.json
.PHONY: test-sc64-p6-canonical
test-sc64-p6-canonical: build/test-sc64-protoss-canonical-p6 build/test-sc64-protoss-canonical-p6-save
	./build/test-sc64-protoss-canonical-p6-save
	mkdir -p "$(dir $(PROTOSS_P6_CONTROLLER_TRACE))"
	./build/test-sc64-protoss-canonical-p6 --trace "$(PROTOSS_P6_CONTROLLER_TRACE)"
.PHONY: test-protoss-p6
test-protoss-p6: test-sc64-p6-canonical

PROTOSS_P7_CONTROLLER_TRACE ?= .local/sc64/p7-canonical-controller-win.jsonl
build/test-sc64-protoss-canonical-p7-controller: $(P7_CAMPAIGN_CONTROLLER) $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P7_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(PROTOSS_P7_CANONICAL_FLAGS) -DP7_CONTROLLER_PROFILE=\"PROTOSS_P7_CANONICAL_FLAGS\" $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-protoss-canonical-p7-save: tests/sc64_p7_interceptor_flight_game.c $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P7_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(PROTOSS_P7_CANONICAL_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
# The canonical P7 profile shares the OBJ tile VRAM between the projectile
# arena and the VRAM terrain cache; the host backs both with one array.
build/test-sc64-p7-obj-vram: tests/sc64_p7_obj_vram.c $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P7_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(PROTOSS_P7_CANONICAL_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-p7-obj-vram
test-sc64-p7-obj-vram: build/test-sc64-p7-obj-vram
	./build/test-sc64-p7-obj-vram
.PHONY: test-sc64-protoss-p7-canonical
test-sc64-protoss-p7-canonical: build/test-sc64-protoss-canonical-p7-controller build/test-sc64-protoss-canonical-p7-save build/test-sc64-p7-obj-vram tools/sc64_p7_campaign_trace.py
	./build/test-sc64-p7-obj-vram
	./build/test-sc64-protoss-canonical-p7-save
	./build/test-sc64-protoss-canonical-p7-controller $(PROTOSS_P7_CONTROLLER_TRACE)
	$(PYTHON) tools/sc64_p7_campaign_trace.py --require-win $(PROTOSS_P7_CONTROLLER_TRACE)

test-protoss: test-sc64-p6-canonical test-sc64-protoss-p7-canonical test-sc64-p7-fact-v2-normal-input

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
test-unified: build/test-unified build/test-unified-frontend build/test-unified-menu build/test-unified-engine build/test-unified-factions build/test-unified-campaign build/test-unified-targets build/test-sc64-zerg-townhalls build/test-unified-hud build/test-sc64-weapon-vfx-game build/test-sc64-shield-bars
	./build/test-unified
	./build/test-unified-frontend
	./build/test-unified-menu
	./build/test-unified-engine
	./build/test-unified-factions
	./build/test-unified-campaign
	./build/test-unified-targets
	./build/test-sc64-zerg-townhalls
	./build/test-unified-hud
	./build/test-sc64-weapon-vfx-game
	./build/test-sc64-shield-bars

clean:
	rm -rf build

# Ordinary-controller probes do not mark a catalog mission qualified.
build/test-sc64-t6-campaign-run: tests/sc64_t6_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(T6_SCENARIO_HEADER) $(T6_AVAILABILITY_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T6_RUNTIME_FLAGS) -DSC64_T6_PROBE_ENTRY $< common/gba.c -o $@
build/test-sc64-t8-campaign-run: tests/sc64_terran_assault_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) -DSC64_TERRAN_LATE_PROBE_MISSION=11 $< common/gba.c -o $@
build/test-sc64-t10-campaign-run: tests/sc64_t10_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) -DSC64_TERRAN_LATE_PROBE_MISSION=13 $< common/gba.c -o $@

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

build/test-sc64-t7-campaign-run: tests/sc64_t7_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) -DSC64_TERRAN_LATE_PROBE_MISSION=10 $< common/gba.c -o $@

build/test-sc64-t9-campaign-run: tests/sc64_t9_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) -DSC64_TERRAN_LATE_PROBE_MISSION=12 $< common/gba.c -o $@

# Exercise the compact full-profile layout against the existing early mission
# controllers and persistence tests, not only the late-mission fixtures.
define TERRAN_FULL_TEST
build/test-sc64-full-$(1): $(2) $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) $(3) $$< common/gba.c -o $$@
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
	./build/test-sc64-t7-campaign-run --limit 60000 --trace .local/sc64/terran-late/full-t7-controller.json
	./build/test-sc64-t8-campaign-run --limit 200000 --trace .local/sc64/terran-late/full-t8-controller.json
	./build/test-sc64-t9-campaign-run --limit 500000 --trace .local/sc64/terran-late/full-t9-controller.json
	./build/test-sc64-t10-campaign-run --limit 100000 --trace .local/sc64/terran-late/full-t10-controller.json


build/test-sc64-p6-model: tests/sc64_p6.c starcraft/sc64_p6.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-p6-actors: tests/sc64_p6_actor_rules.c starcraft/sc64_p6_actor_rules.h starcraft/sc64_z5_actor_rules.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-p6-adapter: tests/sc64_p6_adapter.c starcraft/sc64_p6.h starcraft/sc64_p6_adapter.h .local/sc64/zp-stage/generated/scenarios/sc64_p6_scenario_generated.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
.PHONY: test-sc64-p6-model test-sc64-p6-actors test-sc64-p6-adapter
test-sc64-p6-model: build/test-sc64-p6-model
	./build/test-sc64-p6-model
test-sc64-p6-actors: build/test-sc64-p6-actors
	./build/test-sc64-p6-actors
test-sc64-p6-adapter: test-sc64-p6-model build/test-sc64-p6-adapter
	./build/test-sc64-p6-adapter
test-starcraft: test-sc64-p6-model test-sc64-p6-actors

build/test-sc64-p6-bindings-off: tests/sc64_p6_actor_bindings.c starcraft/race_rules.h starcraft/sc64_zp_actor_rules.h starcraft/sc64_p6_actor_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-p6-bindings-on: tests/sc64_p6_actor_bindings.c starcraft/race_rules.h starcraft/sc64_zp_actor_rules.h starcraft/sc64_p6_actor_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_P6_ACTORS $< -o $@
build/test-sc64-p6-bindings-mixed: tests/sc64_p6_actor_bindings.c starcraft/race_rules.h starcraft/sc64_zp_actor_rules.h starcraft/sc64_p6_actor_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_T4_RUNTIME -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z5_ACTORS -DSC64_Z5_DOODADS -DSC64_Z6_ACTORS -DSC64_P5_ACTORS -DSC64_P6_ACTORS $< -o $@
.PHONY: test-sc64-p6-bindings
test-sc64-p6-bindings: build/test-sc64-p6-bindings-off build/test-sc64-p6-bindings-on build/test-sc64-p6-bindings-mixed
	./build/test-sc64-p6-bindings-off
	./build/test-sc64-p6-bindings-on
	./build/test-sc64-p6-bindings-mixed
test-starcraft: test-sc64-p6-bindings

build/test-sc64-p6-actor-source: tests/sc64_p6_actor_rules.c starcraft/sc64_p6_actor_rules.h starcraft/sc64_z5_actor_rules.h .local/sc64/zp-stage/generated/scenarios/sc64_p6_scenario_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_P6_SOURCE_AUDIT $< -o $@
.PHONY: test-sc64-p6-actor-source
test-sc64-p6-actor-source: build/test-sc64-p6-actor-source
	./build/test-sc64-p6-actor-source
test-sc64-p6-adapter: test-sc64-p6-actor-source

build/test-sc64-p6-world: tests/sc64_p6_world.c starcraft/sc64_p6_world_adapter.h starcraft/sc64_p6_adapter.h starcraft/sc64_p6_actor_rules.h starcraft/sc64_p6.h .local/sc64/zp-stage/generated/scenarios/sc64_p6_scenario_generated.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
.PHONY: test-sc64-p6-world
test-sc64-p6-world: test-sc64-p6-adapter build/test-sc64-p6-world
	./build/test-sc64-p6-world

# Internal P6 integration candidate, with a separate ROM identity and save
# generation. Canonical race outputs retain their qualified campaign menus.
build/starcraft-protoss-p6-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P6_PROBE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p6-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P6-Probe-GBA.gba: build/starcraft-protoss-p6-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P6 PROBE' SCPC
.PHONY: protoss-p6-probe
protoss-p6-probe: build/StarCraft-Protoss-P6-Probe-GBA.gba

build/test-sc64-p6-controller: tests/sc64_p6_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p6-controller
P6_CONTROLLER_TRACE ?= .local/sc64/p6-controller-win.json
test-sc64-p6-controller: build/test-sc64-p6-controller
	mkdir -p "$(dir $(P6_CONTROLLER_TRACE))"
	./build/test-sc64-p6-controller --trace "$(P6_CONTROLLER_TRACE)"

build/test-sc64-z5-campaign-run: tests/sc64_z5_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_CLOAK_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_CONSUME_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z5-campaign-run
test-sc64-z5-campaign-run: build/test-sc64-z5-campaign-run
	mkdir -p .local/sc64
	./build/test-sc64-z5-campaign-run --limit 60000 --trace .local/sc64/z5-controller-diagnostics.json --attempt .local/sc64/z5-controller-attempt.json --verified .local/sc64/z5-controller-win.json

build/test-sc64-p6-runtime: tests/sc64_p6_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p6-combat-runtime: tests/sc64_p6_combat_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p6-runtime
test-sc64-p6-runtime: build/test-sc64-p6-runtime build/test-sc64-p6-combat-runtime
	./build/test-sc64-p6-runtime
	./build/test-sc64-p6-combat-runtime

build/test-sc64-p6-game: tests/sc64_p6_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p6-game
test-sc64-p6-game: build/test-sc64-p6-game
	./build/test-sc64-p6-game

build/test-sc64-p6-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p6-save: tests/sc64_p6_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p6-combat-sizes: tests/sc64_p6_combat_sizes.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p6-combat: tests/sc64_p6_combat.c starcraft/sc64_p6_combat.h starcraft/sc64_p6_actor_rules.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-p6-profile test-sc64-p6-save test-sc64-p6-combat
test-sc64-p6-profile: build/test-sc64-p6-profile
	./build/test-sc64-p6-profile
test-sc64-p6-save: build/test-sc64-p6-save
	./build/test-sc64-p6-save
test-sc64-p6-combat: build/test-sc64-p6-combat build/test-sc64-p6-combat-sizes
	./build/test-sc64-p6-combat
	./build/test-sc64-p6-combat-sizes
.PHONY: test-sc64-p6-combat-model
test-sc64-p6-combat-model: build/test-sc64-p6-combat
	./build/test-sc64-p6-combat
test-starcraft: test-sc64-p6-combat-model

build/test-sc64-z5-count-snapshot: tests/sc64_z5_count_snapshot.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z5_CONSUME_FLAGS) -DSC64_Z5_PRUNE_STARTED $< common/gba.c -o $@
build/test-sc64-z5-trigger-prune: tests/sc64_z5_trigger_prune.c tests/sc64_z5_trigger_prune_reference.c starcraft/sc64_z5_adapter.h starcraft/sc64_z5.h .local/sc64/zp-stage/generated/scenarios/sc64_z5_scenario_generated.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_Z5_PRUNE_STARTED -I.local/sc64/zp-stage/generated/scenarios tests/sc64_z5_trigger_prune.c tests/sc64_z5_trigger_prune_reference.c -o $@
.PHONY: test-sc64-z5-count-queries
test-sc64-z5-count-queries: build/test-sc64-z5-count-snapshot build/test-sc64-z5-trigger-prune
	./build/test-sc64-z5-count-snapshot
	./build/test-sc64-z5-trigger-prune

build/starcraft-zerg-z6-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z6_PROBE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z6-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z6-Probe-GBA.gba: build/starcraft-zerg-z6-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z6 PROBE' SCZE
.PHONY: zerg-z6-probe
zerg-z6-probe: build/StarCraft-Zerg-Z6-Probe-GBA.gba

# Internal seventh mission: canonical profiles stay separately qualified.
build/starcraft-zerg-z7-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_PROBE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z7-Probe-GBA.gba: build/starcraft-zerg-z7-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z7 PROBE' SCZF
.PHONY: zerg-z7-probe
zerg-z7-probe: build/StarCraft-Zerg-Z7-Probe-GBA.gba

build/test-sc64-z7-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z7-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z7-game-save: tests/sc64_z7_game_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_PROBE_FLAGS) $< common/gba.c -o $@
Z8_GAME_POOL_BRIDGE_FLAGS = $(Z7_SCOURGE_PROBE_FLAGS) -DSC64_P6_ACTORS -DSC64_P7_ACTORS -DSC64_NYDUS_ENABLED -DSC64_Z8_GAME_POOL_BRIDGE
build/test-sc64-z8-game-bridge: tests/sc64_z8_game_bridge.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z8_GAME_POOL_BRIDGE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z7-derived-save: tests/sc64_z7_game_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_DERIVED_SAVE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z7-derived-egg-save: tests/sc64_z7_egg_save_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_DERIVED_SAVE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z7-derived-colonies: tests/sc64_z7_colonies_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_DERIVED_SAVE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z7-egg-save-gateoff: tests/sc64_z7_game_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_PROBE_FLAGS) -DSC64_Z7_EGG_PRODUCTION $< common/gba.c -o $@
build/test-sc64-derived-save: tests/sc64_derived_save.c starcraft/sc64_derived_save.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-derived-save test-sc64-z7-derived-save zerg-z7-derived-save-probe
test-sc64-derived-save: build/test-sc64-derived-save
	./build/test-sc64-derived-save
test-starcraft: test-sc64-derived-save
test-sc64-z7-derived-save: test-sc64-derived-save build/test-sc64-z7-derived-save build/test-sc64-z7-derived-egg-save build/test-sc64-z7-derived-colonies build/test-sc64-z7-game-save build/test-sc64-z7-egg-save-gateoff
	./build/test-sc64-z7-derived-save
	./build/test-sc64-z7-derived-egg-save
	./build/test-sc64-z7-derived-colonies
	./build/test-sc64-z7-game-save
	./build/test-sc64-z7-egg-save-gateoff
build/starcraft-zerg-z7-derived-save-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_DERIVED_SAVE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7-derived-save-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z7-Derived-Save-Probe-GBA.gba: build/starcraft-zerg-z7-derived-save-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z7 SAVE' SCZ7
zerg-z7-derived-save-probe: build/StarCraft-Zerg-Z7-Derived-Save-Probe-GBA.gba
.PHONY: test-sc64-z7-profile test-sc64-z7-frame-coverage test-sc64-z7-game-save test-sc64-z8-game-bridge
test-sc64-z7-profile: build/test-sc64-z7-profile
	./build/test-sc64-z7-profile
test-sc64-z7-frame-coverage: build/test-sc64-z7-frame-coverage
	./build/test-sc64-z7-frame-coverage
test-sc64-z7-game-save: build/test-sc64-z7-game-save
	./build/test-sc64-z7-game-save
test-sc64-z8-game-bridge: build/test-sc64-z8-game-bridge
	./build/test-sc64-z8-game-bridge

build/test-sc64-z7-gas-restore: tests/sc64_z7_gas_restore.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z7-gas-restore
test-sc64-z7-gas-restore: build/test-sc64-z7-gas-restore
	./build/test-sc64-z7-gas-restore
build/test-sc64-z7-colonies: tests/sc64_z7_colonies_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z7-colonies
test-sc64-z7-colonies: build/test-sc64-z7-colonies
	./build/test-sc64-z7-colonies

build/test-sc64-z6-runtime: tests/sc64_z6_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z6-save: tests/sc64_z6_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z6-hallucination: tests/sc64_z6_hallucination.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z6-game-save: tests/sc64_z6_game_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z6-runtime test-sc64-z6-save
test-sc64-z6-runtime: build/test-sc64-z6-runtime build/test-sc64-z6-hallucination
	./build/test-sc64-z6-runtime
	./build/test-sc64-z6-hallucination
test-sc64-z6-save: build/test-sc64-z6-save build/test-sc64-z6-game-save
	./build/test-sc64-z6-save
	./build/test-sc64-z6-game-save

# Full normal-input sixth-mission playthrough; imported private inputs required.
build/test-sc64-z6-controller: tests/sc64_z6_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z6-controller
test-sc64-z6-controller: build/test-sc64-z6-controller
	./build/test-sc64-z6-controller --trace build/z6-controller.json

build/test-sc64-z6-profile: tests/sc64_promoted_race_profile.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z6-profile
test-sc64-z6-profile: build/test-sc64-z6-profile
	./build/test-sc64-z6-profile

build/test-sc64-z6-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z6-frame-coverage
test-sc64-z6-frame-coverage: build/test-sc64-z6-frame-coverage
	./build/test-sc64-z6-frame-coverage

# Private numeric source rows required; the optimization stays default-off.
build/test-sc64-z6-count-snapshot: tests/sc64_z6_count_snapshot.c starcraft/sc64_z6_count_snapshot.h $(Z6_PROBE_INPUTS) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< -o $@
build/test-sc64-z6-count-world: tests/sc64_z6_count_world.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) -DSC64_Z6_COUNT_SNAPSHOT $< common/gba.c -o $@
build/test-sc64-z6-runtime-snapshot: tests/sc64_z6_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) -DSC64_Z6_COUNT_SNAPSHOT $< common/gba.c -o $@
.PHONY: test-sc64-z6-count-queries
test-sc64-z6-count-queries: build/test-sc64-z6-count-snapshot build/test-sc64-z6-count-world build/test-sc64-z6-runtime-snapshot
	./build/test-sc64-z6-count-snapshot
	./build/test-sc64-z6-count-world
	./build/test-sc64-z6-runtime-snapshot

build/test-sc64-z6-collision-facts-baseline: tests/sc64_collision_fact_hoist.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_QUERY_TEST_BASE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z6-collision-facts-hoist: tests/sc64_collision_fact_hoist.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_QUERY_TEST_BASE_FLAGS) -DSC64_COLLISION_FACT_HOIST $< common/gba.c -o $@
.PHONY: test-sc64-z6-collision-facts
test-sc64-z6-collision-facts: build/test-sc64-z6-collision-facts-baseline build/test-sc64-z6-collision-facts-hoist
	./build/test-sc64-z6-collision-facts-baseline
	./build/test-sc64-z6-collision-facts-hoist

build/test-sc64-z6-collision-classes: tests/sc64_collision_fact_hoist.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_QUERY_TEST_BASE_FLAGS) -DSC64_COLLISION_CLASS_CACHE $< common/gba.c -o $@
build/test-sc64-z6-collision-classes-hoist: tests/sc64_collision_fact_hoist.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_QUERY_TEST_BASE_FLAGS) -DSC64_COLLISION_FACT_HOIST -DSC64_COLLISION_CLASS_CACHE $< common/gba.c -o $@
.PHONY: test-sc64-z6-collision-classes
test-sc64-z6-collision-classes: test-sc64-z6-collision-facts build/test-sc64-z6-collision-classes build/test-sc64-z6-collision-classes-hoist
	./build/test-sc64-z6-collision-classes
	./build/test-sc64-z6-collision-classes-hoist

.PHONY: test-sc64-zp-fact-range
test-sc64-zp-fact-range:
	$(PYTHON) -m unittest discover -s tests -p 'test_sc64_zp_fact_range.py'

.PHONY: test-sc64-fact-tables
test-sc64-fact-tables:
	$(PYTHON) -m unittest discover -s tests -p 'test_sc64_fact_tables.py'

.PHONY: test-sc64-p7-actor-rules
test-sc64-p7-actor-rules:
	$(PYTHON) -m unittest discover -s tests -p 'test_sc64_p7_actor_rules.py'

build/test-sc64-p7-geometry: tests/sc64_p7_geometry.c starcraft/sc64_p7_geometry.h starcraft/race_rules.h $(wildcard starcraft/*actor_rules.h) common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_P7_ACTORS $< -o $@
.PHONY: test-sc64-p7-geometry
test-sc64-p7-geometry: build/test-sc64-p7-geometry
	./build/test-sc64-p7-geometry
test-starcraft: test-sc64-p7-geometry

build/test-sc64-p7-archon-rules: tests/sc64_p7_archon_rules.c starcraft/sc64_p7_archon_rules.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_RACES -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_P7_ACTORS -DSC64_P7_ARCHON_SPLASH $< -o $@
.PHONY: test-sc64-p7-archon-rules
test-sc64-p7-archon-rules: build/test-sc64-p7-archon-rules
	./build/test-sc64-p7-archon-rules
test-starcraft: test-sc64-p7-archon-rules

build/test-sc64-p7-observer-rules: tests/sc64_p7_observer_rules.c starcraft/sc64_p7_observer_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-p7-observer-rules
test-sc64-p7-observer-rules: build/test-sc64-p7-observer-rules
	./build/test-sc64-p7-observer-rules
test-starcraft: test-sc64-p7-observer-rules

# Optional route policies stay off in ROM defaults. Exercise each independently
# and together, with and without the borrowed visitation grid.
ROUTE_POLICY_TEST_FLAGS = -Icommon -DGBA_HOST -DSC64_RACES -DSC64_LARGE_MAPS -DSC64_ROUTE_PACK3
define route_policy_fixture
build/test-sc64-route-policy-$(1): tests/race_routes_partial.c starcraft/sc64_routes.h starcraft/sc64_bitgrid.h common/gba.h Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -I. $$(ROUTE_POLICY_TEST_FLAGS) $(2) $$< -o $$@
endef
$(eval $(call route_policy_fixture,baseline,))
$(eval $(call route_policy_fixture,visit,-DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,partial,-DSC64_ROUTE_PARTIAL_FLOOD))
$(eval $(call route_policy_fixture,frequency,-DSC64_ROUTE_DECAY_FREQ))
$(eval $(call route_policy_fixture,partial-frequency,-DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_DECAY_FREQ))
$(eval $(call route_policy_fixture,partial-visit,-DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,frequency-visit,-DSC64_ROUTE_DECAY_FREQ -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,partial-frequency-visit,-DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_DECAY_FREQ -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,split-baseline,-DSC64_ROUTE_FLOOD_SPLIT))
$(eval $(call route_policy_fixture,split-visit,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,split-partial,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_PARTIAL_FLOOD))
$(eval $(call route_policy_fixture,split-frequency,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_DECAY_FREQ))
$(eval $(call route_policy_fixture,split-partial-frequency,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_DECAY_FREQ))
$(eval $(call route_policy_fixture,split-partial-visit,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,split-frequency-visit,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_DECAY_FREQ -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,split-partial-frequency-visit,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_DECAY_FREQ -DSC64_ROUTE_VISIT_GRID))
ROUTE_POLICY_VARIANTS = baseline visit partial frequency partial-frequency partial-visit frequency-visit partial-frequency-visit
.PHONY: test-sc64-route-policies
test-sc64-route-policies: $(addprefix build/test-sc64-route-policy-,$(ROUTE_POLICY_VARIANTS))
	@$(foreach variant,$(ROUTE_POLICY_VARIANTS),./build/test-sc64-route-policy-$(variant) &&) true
test-starcraft: test-sc64-route-policies
.PHONY: test-sc64-route-flood-split
test-sc64-route-flood-split: $(addprefix build/test-sc64-route-policy-split-,$(ROUTE_POLICY_VARIANTS))
	@$(foreach variant,$(ROUTE_POLICY_VARIANTS),./build/test-sc64-route-policy-split-$(variant) &&) true
test-starcraft: test-sc64-route-flood-split

build/test-sc64-nearest-resource-depot-p5: tests/sc64_nearest_resource_depot.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P5_SHUTTLE_FLAGS) -DSC64_NEAREST_RESOURCE_DEPOT $< common/gba.c -o $@
build/test-sc64-nearest-resource-depot-z6: tests/sc64_nearest_resource_depot.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) -DSC64_NEAREST_RESOURCE_DEPOT $< common/gba.c -o $@
build/test-sc64-nearest-resource-depot-p6: tests/sc64_nearest_resource_depot.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) -DSC64_NEAREST_RESOURCE_DEPOT $< common/gba.c -o $@
.PHONY: test-sc64-nearest-resource-depot
test-sc64-nearest-resource-depot: build/test-sc64-nearest-resource-depot-p5 build/test-sc64-nearest-resource-depot-z6 build/test-sc64-nearest-resource-depot-p6
	./build/test-sc64-nearest-resource-depot-p5
	./build/test-sc64-nearest-resource-depot-z6
	./build/test-sc64-nearest-resource-depot-p6

build/test-sc64-production-exit: tests/sc64_production_exit.c starcraft/sc64_production_exit.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-production-exit
test-sc64-production-exit: build/test-sc64-production-exit
	./build/test-sc64-production-exit
test-starcraft: test-sc64-production-exit

build/test-sc64-p3-production-exit: tests/sc64_p3_production_exit.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P5_SHUTTLE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p3-production-exit
test-sc64-p3-production-exit: test-sc64-production-exit build/test-sc64-p3-production-exit
	./build/test-sc64-p3-production-exit

build/test-sc64-p3-expansion-controller: tests/sc64_p3_expansion_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P5_SHUTTLE_FLAGS) -DSC64_P3_PRODUCTION_EXIT -DSC64_NEAREST_RESOURCE_DEPOT $< common/gba.c -o $@
.PHONY: test-sc64-p3-expansion-controller
test-sc64-p3-expansion-controller: build/test-sc64-p3-expansion-controller
	./build/test-sc64-p3-expansion-controller --trace build/p3-expansion-controller.json

build/test-sc64-p7-model: tests/sc64_p7.c starcraft/sc64_p7.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-p7-adapter: tests/sc64_p7_adapter.c starcraft/sc64_p7_adapter.h starcraft/sc64_p7.h .local/sc64/zp-stage/generated/scenarios/sc64_p7_scenario_generated.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
.PHONY: test-sc64-p7-model test-sc64-p7-adapter
test-sc64-p7-model: build/test-sc64-p7-model
	./build/test-sc64-p7-model
test-sc64-p7-adapter: build/test-sc64-p7-adapter
	./build/test-sc64-p7-adapter
test-starcraft: test-sc64-p7-model

build/test-sc64-p6-frame-coverage: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p6-frame-coverage
test-sc64-p6-frame-coverage: build/test-sc64-p6-frame-coverage
	./build/test-sc64-p6-frame-coverage

# Z7 isolated runtime foundation: no menu/profile promotion until playthrough qualification.
build/test-sc64-z7-model: tests/sc64_z7.c starcraft/sc64_z7.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-z7-adapter: tests/sc64_z7_adapter.c starcraft/sc64_z7.h starcraft/sc64_z7_adapter.h .local/sc64/zp-stage/generated/scenarios/sc64_z7_scenario_generated.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
build/test-sc64-z7-runtime: tests/sc64_z7_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) .local/sc64/zp-stage/generated/scenarios/sc64_z7_scenario_generated.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) -DSC64_Z7_DATA_ENABLED -DSC64_Z7_RUNTIME $< common/gba.c -o $@
.PHONY: test-sc64-z7-model test-sc64-z7-adapter test-sc64-z7-runtime
test-sc64-z7-model: build/test-sc64-z7-model
	./build/test-sc64-z7-model
test-sc64-z7-adapter: build/test-sc64-z7-adapter
	./build/test-sc64-z7-adapter
test-sc64-z7-runtime: build/test-sc64-z7-runtime
	./build/test-sc64-z7-runtime
test-starcraft: test-sc64-z7-model

build/test-sc64-p6-storm: tests/sc64_p6_storm.c starcraft/sc64_p6_storm_adapter.h starcraft/sc64_psionic_storm.h .local/sc64/zp-stage/generated/scenarios/sc64_p6_abilities_generated.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
.PHONY: test-sc64-p6-storm
test-sc64-p6-storm: build/test-sc64-p6-storm
	./build/test-sc64-p6-storm

build/test-sc64-p6-storm-game: tests/sc64_campaign_spell_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z6-storm-game: tests/sc64_campaign_spell_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z6-cloak-game: tests/sc64_z6_cloak_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-campaign-spells
test-sc64-campaign-spells: build/test-sc64-p6-storm-game build/test-sc64-z6-storm-game build/test-sc64-z6-cloak-game
	./build/test-sc64-p6-storm-game
	./build/test-sc64-z6-storm-game
	./build/test-sc64-z6-cloak-game

build/test-sc64-p7-runtime: tests/sc64_p7_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) .local/sc64/zp-stage/generated/scenarios/sc64_p7_scenario_generated.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) -DSC64_P7_DATA_ENABLED -DSC64_P7_RUNTIME $< common/gba.c -o $@
build/test-sc64-p7-runtime-actors: tests/sc64_p7_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) .local/sc64/zp-stage/generated/scenarios/sc64_p7_scenario_generated.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) -DSC64_P7_DATA_ENABLED -DSC64_P7_RUNTIME -DSC64_P7_ACTORS $< common/gba.c -o $@
build/test-sc64-siege-splash-game: tests/sc64_siege_splash_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) -DSC64_P7_ACTORS $< common/gba.c -o $@
.PHONY: test-sc64-siege-splash-game
test-sc64-siege-splash-game: build/test-sc64-siege-splash-game
	./build/test-sc64-siege-splash-game
build/test-sc64-p7-archon-splash: tests/sc64_p7_archon_splash.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) -DSC64_P7_ACTORS -DSC64_P7_ARCHON_SPLASH $< common/gba.c -o $@
.PHONY: test-sc64-p7-archon-splash
test-sc64-p7-archon-splash: test-sc64-p7-archon-rules build/test-sc64-p7-archon-splash
	./build/test-sc64-p7-archon-splash
build/test-sc64-p7-observer-game: tests/sc64_p7_observer_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) .local/sc64/zp-stage/generated/scenarios/sc64_p7_scenario_generated.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) -DSC64_P7_DATA_ENABLED -DSC64_P7_RUNTIME -DSC64_P7_ACTORS -DSC64_P7_OBSERVER_ENABLED -DSC64_P7_ARCHON_SPLASH $< common/gba.c -o $@
.PHONY: test-sc64-p7-observer
test-sc64-p7-observer: test-sc64-p7-observer-rules build/test-sc64-p7-observer-game
	./build/test-sc64-p7-observer-game

# Internal Battery integration checks. These do not expose a playable P7 entry.
P7_BATTERY_DEFINES := -DSC64_P7_DATA_ENABLED -DSC64_P7_RUNTIME -DSC64_P7_ACTORS -DSC64_P7_BATTERY_ENABLED
P7_BATTERY_FLAGS := -I.local/sc64/p7/generated $(P6_PROBE_FLAGS) $(P7_BATTERY_DEFINES)
P7_BATTERY_P5_FLAGS := -I.local/sc64/p7/generated $(P5_SHUTTLE_FLAGS) $(P7_BATTERY_DEFINES)
P7_BATTERY_INPUTS := $(P6_PROBE_INPUTS) .local/sc64/p7/generated/sc64_race_sprites_generated.h .local/sc64/zp-stage/generated/scenarios/sc64_p7_scenario_generated.h .local/sc64/zp-stage/generated/scenarios/sc64_p7_battery_properties_generated.h
.local/sc64/zp-stage/generated/scenarios/sc64_p7_battery_properties_generated.h: tools/sc64_p7_battery_pack.py tools/sc64_zp_scenario_pack.py tools/sc64_races_pack.py .local/sc64/zp-stage/generated/source_maps/p7/source.chk .local/sc64/zp-stage/generated/source_maps/p7/manifest.json
	$(PYTHON) tools/sc64_p7_battery_pack.py
build/starcraft-protoss-p7-battery-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_BATTERY_FLAGS) -DSC64_P7_OBSERVER_ENABLED -DSC64_P7_ARCHON_SPLASH $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-battery-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Battery-Probe-GBA.gba: build/starcraft-protoss-p7-battery-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 PROBE' SCP7
.PHONY: protoss-p7-battery-probe
protoss-p7-battery-probe: build/StarCraft-Protoss-P7-Battery-Probe-GBA.gba

# Experimental Homeland menu/performance probes, not a qualified campaign win.
P7_RUNTIME_FLAGS := $(P7_BATTERY_FLAGS) -DSC64_P7_OBSERVER_ENABLED -DSC64_P7_ARCHON_SPLASH -DSC64_P7_CAMPAIGN_ENTRY
# Optional Arbiter cloak/cache checks; this remains off in every default profile.
P7_ARBITER_FLAGS := $(P7_RUNTIME_FLAGS) -DSC64_P7_ARBITER_CLOAK_CACHE_ENABLED
P7_ARBITER_PROBE_FLAGS := $(P7_ARBITER_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT
build/test-sc64-p7-runtime-reference: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_RUNTIME_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-runtime-prevalidated: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_RUNTIME_FLAGS) -DSC64_P7_PREVALIDATED_TICK $< common/gba.c -o $@
.PHONY: test-sc64-p7-prevalidated
test-sc64-p7-prevalidated: build/test-sc64-p7-runtime-reference build/test-sc64-p7-runtime-prevalidated
	./build/test-sc64-p7-runtime-reference > build/sc64-p7-runtime-reference.txt
	./build/test-sc64-p7-runtime-prevalidated > build/sc64-p7-runtime-prevalidated.txt
	cmp build/sc64-p7-runtime-reference.txt build/sc64-p7-runtime-prevalidated.txt

# Explicit opt-in P7 Arbiter host matrix. Existing profiles remain cache-off.
P7_ARBITER_INPUTS := $(P7_BATTERY_INPUTS)
P7_ARBITER_TEST_FLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon
build/test-sc64-p7-arbiter-reference: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_ARBITER_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-arbiter-prevalidated: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_ARBITER_FLAGS) -DSC64_P7_PREVALIDATED_TICK $< common/gba.c -o $@
build/test-sc64-p7-arbiter-count: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_ARBITER_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
build/test-sc64-p7-arbiter-cache-policy: tests/sc64_p7_arbiter_cache_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_ARBITER_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
build/test-sc64-p7-arbiter-battery: tests/sc64_p7_battery_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_ARBITER_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
build/test-sc64-p7-observer-no-cache: tests/sc64_p7_observer_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_RUNTIME_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
build/test-sc64-p7-battery-no-cache: tests/sc64_p7_battery_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_RUNTIME_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
build/test-sc64-p7-arbiter-initial-save: tests/sc64_p7_initial_save_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_ARBITER_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
.PHONY: test-sc64-p7-arbiter-host
test-sc64-p7-arbiter-host: build/test-sc64-p7-arbiter-initial-save build/test-sc64-p7-arbiter-reference build/test-sc64-p7-arbiter-prevalidated build/test-sc64-p7-arbiter-count build/test-sc64-p7-arbiter-cache-policy build/test-sc64-p7-arbiter-battery build/test-sc64-p7-observer-no-cache build/test-sc64-p7-battery-no-cache
	./build/test-sc64-p7-arbiter-reference > build/sc64-p7-arbiter-reference.txt
	./build/test-sc64-p7-arbiter-prevalidated > build/sc64-p7-arbiter-prevalidated.txt
	./build/test-sc64-p7-arbiter-count > build/sc64-p7-arbiter-count.txt
	cmp build/sc64-p7-arbiter-reference.txt build/sc64-p7-arbiter-prevalidated.txt
	cmp build/sc64-p7-arbiter-reference.txt build/sc64-p7-arbiter-count.txt
	./build/test-sc64-p7-arbiter-cache-policy
	./build/test-sc64-p7-arbiter-battery
	./build/test-sc64-p7-observer-no-cache
	./build/test-sc64-p7-battery-no-cache
	./build/test-sc64-p7-arbiter-initial-save

# Opt-in differential for the Arbiter's boolean aura union. This preserves the
# ordinary cache path as the reference and does not enable the gate in products.
P7_ARBITER_AURA_TEST := tests/sc64_p7_arbiter_aura.c
build/test-sc64-p7-arbiter-aura-reference: $(P7_ARBITER_AURA_TEST) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_ARBITER_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT -DSC64_P7_GEOMETRY_RECT_HOIST_ENABLED $< common/gba.c -o $@
build/test-sc64-p7-arbiter-aura-once: $(P7_ARBITER_AURA_TEST) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_ARBITER_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT -DSC64_P7_GEOMETRY_RECT_HOIST_ENABLED -DSC64_P7_ARBITER_AURA_ONCE $< common/gba.c -o $@
.PHONY: test-sc64-p7-arbiter-aura-once
test-sc64-p7-arbiter-aura-once: build/test-sc64-p7-arbiter-aura-reference build/test-sc64-p7-arbiter-aura-once
	./build/test-sc64-p7-arbiter-aura-reference
	./build/test-sc64-p7-arbiter-aura-once

# Optional profile link only; no gameplay or timing qualification is implied.
build/starcraft-protoss-p7-arbiter-cache-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_ARBITER_PROBE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-arbiter-cache-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Arbiter-Cache-Probe-GBA.gba: build/starcraft-protoss-p7-arbiter-cache-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 CLOAK' SC7A
.PHONY: protoss-p7-arbiter-cache-probe
protoss-p7-arbiter-cache-probe: build/StarCraft-Protoss-P7-Arbiter-Cache-Probe-GBA.gba
build/starcraft-protoss-p7-runtime-reference.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_RUNTIME_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-runtime-reference.map -lgcc -o $@
	$(SIZE) $@
build/starcraft-protoss-p7-runtime-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_RUNTIME_FLAGS) -DSC64_P7_PREVALIDATED_TICK $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-runtime-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Runtime-Reference-GBA.gba: build/starcraft-protoss-p7-runtime-reference.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 PROBE' SCP7
build/StarCraft-Protoss-P7-Runtime-Probe-GBA.gba: build/starcraft-protoss-p7-runtime-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 PROBE' SCP7
.PHONY: protoss-p7-runtime-reference protoss-p7-runtime-probe
protoss-p7-runtime-reference: build/StarCraft-Protoss-P7-Runtime-Reference-GBA.gba
protoss-p7-runtime-probe: build/StarCraft-Protoss-P7-Runtime-Probe-GBA.gba
build/test-sc64-p7-count-snapshot: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_RUNTIME_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
.PHONY: test-sc64-p7-count-snapshot protoss-p7-count-probe
test-sc64-p7-count-snapshot: build/test-sc64-p7-runtime-reference build/test-sc64-p7-runtime-prevalidated build/test-sc64-p7-count-snapshot
	./build/test-sc64-p7-runtime-reference > build/sc64-p7-runtime-reference.txt
	./build/test-sc64-p7-runtime-prevalidated > build/sc64-p7-runtime-prevalidated.txt
	./build/test-sc64-p7-count-snapshot > build/sc64-p7-count-snapshot.txt
	cmp build/sc64-p7-runtime-reference.txt build/sc64-p7-runtime-prevalidated.txt
	cmp build/sc64-p7-runtime-reference.txt build/sc64-p7-count-snapshot.txt
build/starcraft-protoss-p7-count-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_RUNTIME_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-count-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Count-Probe-GBA.gba: build/starcraft-protoss-p7-count-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 PROBE' SCP7
protoss-p7-count-probe: build/StarCraft-Protoss-P7-Count-Probe-GBA.gba
# Default-off P7 movement probe: the qualified Z6 collision-class cache and
# fact-range rejection plus the exact blocked-mover steering list.
P7_MOVE_BASE_FLAGS := $(P7_RUNTIME_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT
P7_MOVE_FLAGS := -DSC64_ZP_FACT_RANGE_CULL -DSC64_COLLISION_CLASS_CACHE -DSC64_COLLISION_STEER_LIST
build/test-sc64-p7-move-reference: tests/sc64_p7_steer_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_MOVE_BASE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-move-candidate: tests/sc64_p7_steer_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_MOVE_BASE_FLAGS) $(P7_MOVE_FLAGS) $< common/gba.c -o $@
# The menu, trigger and SRAM save/load fixture must also print the same trace.
build/test-sc64-p7-move-runtime: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_MOVE_BASE_FLAGS) $(P7_MOVE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p7-move protoss-p7-move-probe
test-sc64-p7-move: build/test-sc64-p7-move-reference build/test-sc64-p7-move-candidate build/test-sc64-p7-count-snapshot build/test-sc64-p7-move-runtime
	./build/test-sc64-p7-move-reference > build/sc64-p7-move-reference.txt
	./build/test-sc64-p7-move-candidate > build/sc64-p7-move-candidate.txt
	cmp build/sc64-p7-move-reference.txt build/sc64-p7-move-candidate.txt
	./build/test-sc64-p7-count-snapshot > build/sc64-p7-count-snapshot.txt
	./build/test-sc64-p7-move-runtime > build/sc64-p7-move-runtime.txt
	cmp build/sc64-p7-count-snapshot.txt build/sc64-p7-move-runtime.txt
	cat build/sc64-p7-move-candidate.txt
build/starcraft-protoss-p7-move-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_MOVE_BASE_FLAGS) $(P7_MOVE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-move-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Move-Probe-GBA.gba: build/starcraft-protoss-p7-move-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 PROBE' SCP7
protoss-p7-move-probe: build/StarCraft-Protoss-P7-Move-Probe-GBA.gba
# Default-off P7 contact-cache probe: the same base as the movement probe with
# the per-slot collision facts and one-pass steering in place of the class
# cache and steering list. The host trace must match the plain reference.
P7_CONTACT_FLAGS := -DSC64_ZP_FACT_RANGE_CULL -DSC64_CONTACT_CACHE
build/test-sc64-p7-contact-candidate: tests/sc64_p7_steer_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_MOVE_BASE_FLAGS) $(P7_CONTACT_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-contact-runtime: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_MOVE_BASE_FLAGS) $(P7_CONTACT_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p7-contact protoss-p7-contact-probe
test-sc64-p7-contact: build/test-sc64-p7-move-reference build/test-sc64-p7-contact-candidate build/test-sc64-p7-count-snapshot build/test-sc64-p7-contact-runtime
	./build/test-sc64-p7-move-reference > build/sc64-p7-move-reference.txt
	./build/test-sc64-p7-contact-candidate > build/sc64-p7-contact-candidate.txt
	cmp build/sc64-p7-move-reference.txt build/sc64-p7-contact-candidate.txt
	./build/test-sc64-p7-count-snapshot > build/sc64-p7-count-snapshot.txt
	./build/test-sc64-p7-contact-runtime > build/sc64-p7-contact-runtime.txt
	cmp build/sc64-p7-count-snapshot.txt build/sc64-p7-contact-runtime.txt
	cat build/sc64-p7-contact-candidate.txt
build/starcraft-protoss-p7-contact-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_MOVE_BASE_FLAGS) $(P7_CONTACT_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-contact-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Contact-Probe-GBA.gba: build/starcraft-protoss-p7-contact-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 PROBE' SCP7
protoss-p7-contact-probe: build/StarCraft-Protoss-P7-Contact-Probe-GBA.gba
# Default-off mode-0 renderer on the movement probe. The derived header puts
# terrain in flip-deduplicated 8x8 pools and leaves the shared input untouched.
TILE8_SOURCE := .local/sc64/zp-stage/generated/sc64_races_generated.h
TILE8_HEADER := .local/sc64/tile8/generated/sc64_races_generated.h
$(TILE8_HEADER): tools/sc64_tile8_pack.py $(TILE8_SOURCE)
	$(PYTHON) tools/sc64_tile8_pack.py --input $(TILE8_SOURCE) --output $@
# Mode 0 needs the VRAM gap the sampled-audio buffers use between the mode-4
# pages (see common/gba.c), so the tile probe builds silent, as SC64_AUDIO=0.
P7_TILE_FLAGS := -I.local/sc64/tile8/generated $(filter-out $(SC64_AUDIO_FLAGS),$(P7_MOVE_BASE_FLAGS) $(P7_MOVE_FLAGS)) -DSC64_TILE_RENDERER
build/test-sc64-p7-tile-reference: tests/sc64_p7_tile_render.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_MOVE_BASE_FLAGS) $(P7_MOVE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-tile-candidate: tests/sc64_p7_tile_render.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) $(TILE8_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_TILE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p7-tile protoss-p7-tile-probe
test-sc64-p7-tile: build/test-sc64-p7-tile-reference build/test-sc64-p7-tile-candidate
	./build/test-sc64-p7-tile-reference > build/sc64-p7-tile-reference.txt
	./build/test-sc64-p7-tile-candidate > build/sc64-p7-tile-candidate.txt
	cmp build/sc64-p7-tile-reference.txt build/sc64-p7-tile-candidate.txt
	tail -n 3 build/sc64-p7-tile-candidate.txt
build/starcraft-protoss-p7-tile-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) $(TILE8_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_TILE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-tile-probe.map -lgcc -o $@
	$(SIZE) $@
	$(PYTHON) tools/sc64_tile8_pack.py --check-elf $@
build/StarCraft-Protoss-P7-Tile-Probe-GBA.gba: build/starcraft-protoss-p7-tile-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 PROBE' SCP7
protoss-p7-tile-probe: build/StarCraft-Protoss-P7-Tile-Probe-GBA.gba
# Default-off P7 fact-table probe: the movement probe plus SC64_FACT_TABLES.
# A host generator evaluates the unchanged fact functions under the probe's
# exact flags and writes packed per-type ROM tables to an ignored build path.
P7_FACT_TABLE_DIR := build/generated/p7-fact-tables
P7_FACT_TABLE_HEADER := $(P7_FACT_TABLE_DIR)/sc64_fact_tables_generated.h
P7_FACT_FLAGS := -DSC64_FACT_TABLES -I$(P7_FACT_TABLE_DIR)
$(P7_FACT_TABLE_HEADER): tools/sc64_fact_tables_gen.c $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) Makefile | build
	mkdir -p $(P7_FACT_TABLE_DIR)
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -DGBA_HOST -Icommon -Istarcraft $(P7_MOVE_BASE_FLAGS) $(P7_MOVE_FLAGS) $< -o build/sc64-p7-fact-tables-gen
	./build/sc64-p7-fact-tables-gen > $@.tmp
	mv $@.tmp $@
build/test-sc64-p7-fact-check: tests/sc64_fact_tables_check.c $(P7_FACT_TABLE_HEADER) $(wildcard starcraft/*.h) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -DGBA_HOST -Icommon -Istarcraft $(P7_MOVE_BASE_FLAGS) $(P7_MOVE_FLAGS) $(P7_FACT_FLAGS) $< -o $@
build/test-sc64-p7-fact-candidate: tests/sc64_p7_steer_game.c $(P7_FACT_TABLE_HEADER) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_MOVE_BASE_FLAGS) $(P7_MOVE_FLAGS) $(P7_FACT_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-fact-runtime: tests/sc64_p7_prevalidated_game.c $(P7_FACT_TABLE_HEADER) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_MOVE_BASE_FLAGS) $(P7_MOVE_FLAGS) $(P7_FACT_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p7-fact-tables protoss-p7-fact-probe
test-sc64-p7-fact-tables: build/test-sc64-p7-fact-check build/test-sc64-p7-move-reference build/test-sc64-p7-fact-candidate build/test-sc64-p7-count-snapshot build/test-sc64-p7-fact-runtime
	./build/test-sc64-p7-fact-check
	./build/test-sc64-p7-move-reference > build/sc64-p7-move-reference.txt
	./build/test-sc64-p7-fact-candidate > build/sc64-p7-fact-candidate.txt
	cmp build/sc64-p7-move-reference.txt build/sc64-p7-fact-candidate.txt
	./build/test-sc64-p7-count-snapshot > build/sc64-p7-count-snapshot.txt
	./build/test-sc64-p7-fact-runtime > build/sc64-p7-fact-runtime.txt
	cmp build/sc64-p7-count-snapshot.txt build/sc64-p7-fact-runtime.txt
	cat build/sc64-p7-fact-candidate.txt
build/starcraft-protoss-p7-fact-probe.elf: $(P7_FACT_TABLE_HEADER) $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_MOVE_BASE_FLAGS) $(P7_MOVE_FLAGS) $(P7_FACT_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-fact-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Fact-Probe-GBA.gba: build/starcraft-protoss-p7-fact-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 PROBE' SCP7
protoss-p7-fact-probe: build/StarCraft-Protoss-P7-Fact-Probe-GBA.gba
build/test-sc64-p7-battery-rules: tests/sc64_p7_battery_rules.c starcraft/sc64_p7_battery_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-p7-battery-rules
test-sc64-p7-battery-rules: build/test-sc64-p7-battery-rules
	./build/test-sc64-p7-battery-rules
test-starcraft: test-sc64-p7-battery-rules
build/test-sc64-p7-power-geometry: tests/sc64_p7_power_geometry.c starcraft/sc64_p7_power_geometry.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-p7-power-geometry
test-sc64-p7-power-geometry: build/test-sc64-p7-power-geometry
	./build/test-sc64-p7-power-geometry
test-starcraft: test-sc64-p7-power-geometry
build/test-sc64-p7-battery-game: tests/sc64_p7_battery_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_BATTERY_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-battery-combined: tests/sc64_p7_battery_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_BATTERY_FLAGS) -DSC64_P7_OBSERVER_ENABLED -DSC64_P7_ARCHON_SPLASH $< common/gba.c -o $@
build/test-sc64-p5-battery-save: tests/sc64_p5_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_BATTERY_P5_FLAGS) $< common/gba.c -o $@
build/test-sc64-p5-battery-shuttle-save: tests/sc64_p5_shuttle_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_BATTERY_P5_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-battery-p5-union: tests/sc64_p7_union_tail_p5.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_BATTERY_P5_FLAGS) $< common/gba.c -o $@
build/test-sc64-p6-battery-save: tests/sc64_p6_save.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_BATTERY_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p7-battery
test-sc64-p7-battery: test-sc64-p7-battery-rules test-sc64-p7-power-geometry build/test-sc64-p7-battery-game build/test-sc64-p7-battery-combined build/test-sc64-p5-battery-save build/test-sc64-p5-battery-shuttle-save build/test-sc64-p7-battery-p5-union build/test-sc64-p6-battery-save
	./build/test-sc64-p7-battery-game
	./build/test-sc64-p7-battery-combined
	./build/test-sc64-p5-battery-save
	./build/test-sc64-p5-battery-shuttle-save
	./build/test-sc64-p7-battery-p5-union
	./build/test-sc64-p6-battery-save

build/test-sc64-p6-trigger-prune: tests/sc64_p6_trigger_prune.c tests/sc64_p6_trigger_prune_reference.c starcraft/sc64_p6_adapter.h starcraft/sc64_p6.h .local/sc64/zp-stage/generated/scenarios/sc64_p6_scenario_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios tests/sc64_p6_trigger_prune.c tests/sc64_p6_trigger_prune_reference.c -o $@
build/test-sc64-z6-trigger-prune: tests/sc64_z6_trigger_prune.c tests/sc64_z6_trigger_prune_reference.c starcraft/sc64_z6_adapter.h starcraft/sc64_z6.h .local/sc64/zp-stage/generated/scenarios/sc64_z6_scenario_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios tests/sc64_z6_trigger_prune.c tests/sc64_z6_trigger_prune_reference.c -o $@
build/test-sc64-p6-prevalidated: tests/sc64_p6_prevalidated.c tests/sc64_p6_prevalidated_reference.c starcraft/sc64_p6_adapter.h starcraft/sc64_p6.h .local/sc64/zp-stage/generated/scenarios/sc64_p6_scenario_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios tests/sc64_p6_prevalidated.c tests/sc64_p6_prevalidated_reference.c -o $@
.PHONY: test-sc64-p7-runtime test-sc64-six-query-options
test-sc64-p7-runtime: build/test-sc64-p7-runtime build/test-sc64-p7-runtime-actors
	./build/test-sc64-p7-runtime
	./build/test-sc64-p7-runtime-actors
test-sc64-six-query-options: build/test-sc64-p6-trigger-prune build/test-sc64-z6-trigger-prune build/test-sc64-p6-prevalidated
	./build/test-sc64-p6-trigger-prune
	./build/test-sc64-z6-trigger-prune
	./build/test-sc64-p6-prevalidated

# Private sixth-mission integration and opt-in performance fixtures.
build/test-sc64-z6-transport-tech: tests/sc64_z6_transport_tech.c starcraft/sc64_z6_transport_tech.h starcraft/sc64_z6_transport_adapter.h .local/sc64/zp-stage/generated/scenarios/sc64_z6_transport_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
build/test-sc64-z6-transport-save: tests/sc64_z6_transport_save.c starcraft/sc64_z6_transport_save.h starcraft/sc64_transport_rules.h .local/sc64/zp-stage/generated/scenarios/sc64_z6_transport_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -I.local/sc64/zp-stage/generated/scenarios $< -o $@
build/test-sc64-z6-transport-game: tests/sc64_z6_transport_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p6-count-snapshot: tests/sc64_p6_count_snapshot.c tests/sc64_p6_count_snapshot_reference.c starcraft/sc64_p6_count_snapshot.h starcraft/sc64_p6_world_adapter.h .local/sc64/zp-stage/generated/scenarios/sc64_p6_scenario_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -Icommon -I.local/sc64/zp-stage/generated/scenarios -DSC64_P6_COUNT_SNAPSHOT tests/sc64_p6_count_snapshot.c tests/sc64_p6_count_snapshot_reference.c -o $@
build/test-sc64-p6-trap-cull: tests/sc64_p6_trap_cull.c tests/sc64_p6_trap_cull_reference.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_QUERY_TEST_BASE_FLAGS) tests/sc64_p6_trap_cull.c tests/sc64_p6_trap_cull_reference.c common/gba.c -o $@
.PHONY: test-sc64-z6-transport test-sc64-p6-query-experiments
test-sc64-z6-transport: build/test-sc64-z6-transport-tech build/test-sc64-z6-transport-save build/test-sc64-z6-transport-game
	./build/test-sc64-z6-transport-tech
	./build/test-sc64-z6-transport-save
	./build/test-sc64-z6-transport-game
test-sc64-p6-query-experiments: test-sc64-six-query-options build/test-sc64-p6-count-snapshot build/test-sc64-p6-trap-cull
	./build/test-sc64-p6-count-snapshot
	./build/test-sc64-p6-trap-cull

build/test-sc64-p6-burrow-cull-on: tests/sc64_p6_burrow_cull.c tests/sc64_p6_burrow_cull_reference.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_QUERY_TEST_BASE_FLAGS) -DSC64_P6_BURROW_CULL tests/sc64_p6_burrow_cull.c tests/sc64_p6_burrow_cull_reference.c common/gba.c -o $@
build/test-sc64-p6-burrow-cull-off: tests/sc64_p6_burrow_cull.c tests/sc64_p6_burrow_cull_reference.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_QUERY_TEST_BASE_FLAGS) tests/sc64_p6_burrow_cull.c tests/sc64_p6_burrow_cull_reference.c common/gba.c -o $@
.PHONY: test-sc64-p6-burrow-cull
test-sc64-p6-burrow-cull: build/test-sc64-p6-burrow-cull-on build/test-sc64-p6-burrow-cull-off
test-sc64-p6-query-experiments: test-sc64-p6-burrow-cull

build/test-sc64-p6-storm-validation: tests/sc64_p6_storm_validation.c tests/sc64_p6_storm_validation_reference.c tests/sc64_p6_storm_validation_fixture.h starcraft/sc64_p6_storm_adapter.h starcraft/sc64_psionic_storm.h .local/sc64/zp-stage/generated/scenarios/sc64_p6_abilities_generated.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -I.local/sc64/zp-stage/generated/scenarios tests/sc64_p6_storm_validation.c tests/sc64_p6_storm_validation_reference.c common/gba.c -o $@
build/test-sc64-p6-storm-game-local: tests/sc64_campaign_spell_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_QUERY_TEST_BASE_FLAGS) -DSC64_P6_STORM_POSTCHECK_LOCAL $< common/gba.c -o $@
.PHONY: test-sc64-p6-storm-validation
test-sc64-p6-storm-validation: build/test-sc64-p6-storm-validation build/test-sc64-p6-storm-game build/test-sc64-p6-storm-game-local
	./build/test-sc64-p6-storm-validation
	./build/test-sc64-p6-storm-game
	./build/test-sc64-p6-storm-game-local
test-sc64-p6-query-experiments: test-sc64-p6-storm-validation

build/test-sc64-p6-rescue-grouped: tests/sc64_p6_rescue_grouped.c tests/sc64_p6_rescue_grouped_reference.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_QUERY_TEST_BASE_FLAGS) -DSC64_P6_RESCUE_GROUPED tests/sc64_p6_rescue_grouped.c tests/sc64_p6_rescue_grouped_reference.c common/gba.c -o $@
.PHONY: test-sc64-p6-rescue-grouped
test-sc64-p6-rescue-grouped: build/test-sc64-p6-rescue-grouped
	./build/test-sc64-p6-rescue-grouped
test-sc64-p6-query-experiments: test-sc64-p6-rescue-grouped

build/test-sc64-p6-burrow-grouped: tests/sc64_p6_burrow_cull.c tests/sc64_p6_burrow_cull_reference.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_QUERY_TEST_BASE_FLAGS) -DSC64_P6_BURROW_GROUPED tests/sc64_p6_burrow_cull.c tests/sc64_p6_burrow_cull_reference.c common/gba.c -o $@
build/test-sc64-p6-burrow-grouped-cull: tests/sc64_p6_burrow_cull.c tests/sc64_p6_burrow_cull_reference.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_QUERY_TEST_BASE_FLAGS) -DSC64_P6_BURROW_GROUPED -DSC64_P6_BURROW_CULL tests/sc64_p6_burrow_cull.c tests/sc64_p6_burrow_cull_reference.c common/gba.c -o $@
test-sc64-p6-burrow-cull: build/test-sc64-p6-burrow-grouped build/test-sc64-p6-burrow-grouped-cull
	./build/test-sc64-p6-burrow-cull-on
	./build/test-sc64-p6-burrow-cull-off
	./build/test-sc64-p6-burrow-grouped
	./build/test-sc64-p6-burrow-grouped-cull

build/test-sc64-probe-expansion: tests/sc64_probe_expansion.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) $< common/gba.c -o $@
build/test-sc64-probe-expansion-p6: tests/sc64_probe_expansion.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-probe-expansion-z6: tests/sc64_probe_expansion.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-probe-expansion
test-sc64-probe-expansion: build/test-sc64-probe-expansion build/test-sc64-probe-expansion-p6 build/test-sc64-probe-expansion-z6
	./build/test-sc64-probe-expansion
	./build/test-sc64-probe-expansion-p6
	./build/test-sc64-probe-expansion-z6

build/test-sc64-probe-cannon: tests/sc64_probe_cannon.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) $< common/gba.c -o $@
build/test-sc64-probe-cannon-p6: tests/sc64_probe_cannon.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P6_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-probe-cannon-z6: tests/sc64_probe_cannon.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z6_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z6_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-probe-cannon
test-sc64-probe-cannon: build/test-sc64-probe-cannon build/test-sc64-probe-cannon-p6 build/test-sc64-probe-cannon-z6
	./build/test-sc64-probe-cannon
	./build/test-sc64-probe-cannon-p6
	./build/test-sc64-probe-cannon-z6

build/test-sc64-weapon-vfx: tests/sc64_weapon_vfx.c starcraft/sc64_weapon_vfx.h starcraft/sc64_weapon_vfx_render.h common/gba.h | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-weapon-vfx-game: tests/sc64_weapon_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-weapon-vfx test-sc64-weapon-vfx-game
test-sc64-weapon-vfx: build/test-sc64-weapon-vfx
	./build/test-sc64-weapon-vfx
test-sc64-weapon-vfx-game: build/test-sc64-weapon-vfx-game
	./build/test-sc64-weapon-vfx-game
test-starcraft: test-sc64-weapon-vfx

build/test-sc64-shield-bars: tests/sc64_shield_bars_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) $< common/gba.c -o $@
build/test-sc64-shield-bars-protoss: tests/sc64_shield_bars_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_RECT_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-shield-bars
test-sc64-shield-bars: build/test-sc64-shield-bars build/test-sc64-shield-bars-protoss
	./build/test-sc64-shield-bars
	./build/test-sc64-shield-bars-protoss

# The same actual-attack/renderer fixture checks optional actor identities.
define WEAPON_VFX_PROFILE_FIXTURE
build/test-sc64-weapon-vfx-$(1): tests/sc64_weapon_vfx_game.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $(3) common/gba.c common/gba.h Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(2) $$< common/gba.c -o $$@
endef
$(eval $(call WEAPON_VFX_PROFILE_FIXTURE,terran,$(TERRAN_FULL_FLAGS),$(TERRAN_FULL_INPUTS)))
build/test-sc64-weapon-vfx-zerg: tests/sc64_weapon_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_DERIVED_SAVE_FLAGS) $< common/gba.c -o $@
# P7 flags are declared below; keep them recursively expanded here.
build/test-sc64-weapon-vfx-protoss: tests/sc64_weapon_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_RECT_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-weapon-vfx-profiles
test-sc64-weapon-vfx-profiles: test-sc64-weapon-vfx-game build/test-sc64-weapon-vfx-terran build/test-sc64-weapon-vfx-zerg build/test-sc64-weapon-vfx-protoss
	./build/test-sc64-weapon-vfx-terran
	./build/test-sc64-weapon-vfx-zerg
	./build/test-sc64-weapon-vfx-protoss

build/test-sc64-construction-vfx: tests/sc64_construction_vfx.c starcraft/sc64_construction_vfx.h common/gba.c common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $< common/gba.c -o $@
.PHONY: test-sc64-construction-vfx
test-sc64-construction-vfx: build/test-sc64-construction-vfx
	./build/test-sc64-construction-vfx
test-starcraft: test-sc64-construction-vfx

build/test-sc64-construction-animation: tests/sc64_construction_animation.c starcraft/sc64_construction_animation.h starcraft/sc64_construction_source.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-construction-animation
test-sc64-construction-animation: build/test-sc64-construction-animation
	./build/test-sc64-construction-animation
test-starcraft: test-sc64-construction-animation

# Optional source-art preview. It reuses already-prepared private race inputs;
# the default profile never extracts or links game art.
CONSTRUCTION_ART_FLAGS := $(UNIFIED_FLAGS) -DSC64_CONSTRUCTION_SOURCE_ART -I$(CONSTRUCTION_ART_DIR)
$(CONSTRUCTION_ART_HEADER): tools/sc64_construction_sprites.py tools/sc64_construction_programs.py tools/sc64_sprites.py common/gba.c common/gba.h starcraft/sc64_display.h .local/sc64/backwater/generated/palette.json
	$(PYTHON) tools/sc64_construction_sprites.py
build/test-sc64-construction-source: tests/sc64_construction_source_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) $(CONSTRUCTION_ART_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(CONSTRUCTION_ART_FLAGS) $< common/gba.c -o $@
# Includes the host backend in the fixture to inspect the displayed RGB palette.
WARP_COLOR_TEST_FLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon
WARP_COLOR_NOCLIP_FLAGS = $(filter-out -DGBA_SCENE_CLIP -DSC64_STATIC_DIRTY_ROWS,$(PROTOSS_ROM_FLAGS))
build/test-sc64-protoss-warp-color: tests/sc64_protoss_warp_color.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) $(CONSTRUCTION_ART_HEADER) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(WARP_COLOR_TEST_FLAGS) $(PROTOSS_ROM_FLAGS) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-protoss-warp-color-no-clip: tests/sc64_protoss_warp_color.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) $(CONSTRUCTION_ART_HEADER) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(WARP_COLOR_TEST_FLAGS) $(WARP_COLOR_NOCLIP_FLAGS) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-protoss-warp-color
test-sc64-protoss-warp-color: build/test-sc64-protoss-warp-color build/test-sc64-protoss-warp-color-no-clip
	./build/test-sc64-protoss-warp-color
	./build/test-sc64-protoss-warp-color-no-clip
build/starcraft-construction.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) $(CONSTRUCTION_ART_HEADER) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(CONSTRUCTION_ART_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-construction.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Construction-Preview-GBA.gba: build/starcraft-construction.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC BUILD ART' SCCA
.PHONY: sc64-construction-art test-sc64-construction-source construction-visuals
sc64-construction-art:
	$(PYTHON) tools/sc64_construction_sprites.py
test-sc64-construction-source: build/test-sc64-construction-source
	./build/test-sc64-construction-source
construction-visuals: build/StarCraft-Construction-Preview-GBA.gba
	$(PYTHON) -c 'from tools.verify_roms import verify; print(verify("StarCraft-Construction-Preview-GBA", "starcraft-construction", "SCCA", "SC BUILD ART"))'

# Transient death effects. The asset-free checks cover the authored renderer
# and the original-art game; each game fixture is also built with the feature
# compiled out and must print the same state/save digest over combat.
DEATH_VFX_TEST_FLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon
build/test-sc64-death-vfx: tests/sc64_death_vfx.c starcraft/sc64_death_vfx.h starcraft/sc64_death_vfx_render.h common/gba.c common/gba.h | build
	$(CLANG) $(DEATH_VFX_TEST_FLAGS) $< common/gba.c -o $@
build/test-sc64-death-vfx-default: tests/sc64_death_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(DEATH_VFX_TEST_FLAGS) $< -o $@
build/test-sc64-death-vfx-default-off: tests/sc64_death_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(DEATH_VFX_TEST_FLAGS) -DSC64_DEATH_VFX_DISABLED $< -o $@
# $(1)=name, $(2)=profile flags, $(3)=inputs
define DEATH_VFX_GAME_FIXTURE
build/test-sc64-death-vfx-$(1): tests/sc64_death_vfx_game.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $(3) common/gba.c common/gba.h Makefile | build
	$$(CLANG) $$(DEATH_VFX_TEST_FLAGS) $(2) $$< -o $$@
build/test-sc64-death-vfx-$(1)-off: tests/sc64_death_vfx_game.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $(3) common/gba.c common/gba.h Makefile | build
	$$(CLANG) $$(DEATH_VFX_TEST_FLAGS) $(2) -DSC64_DEATH_VFX_DISABLED $$< -o $$@
endef
$(eval $(call DEATH_VFX_GAME_FIXTURE,unified,$(UNIFIED_FLAGS),$(RACES_INPUTS)))
$(eval $(call DEATH_VFX_GAME_FIXTURE,terran,$(TERRAN_FULL_FLAGS),$(TERRAN_FULL_INPUTS)))
$(eval $(call DEATH_VFX_GAME_FIXTURE,zerg,$$(Z7_DERIVED_SAVE_FLAGS),$$(Z7_PROBE_INPUTS)))
$(eval $(call DEATH_VFX_GAME_FIXTURE,protoss,$$(P7_TYPED_RECT_FLAGS),$$(P7_ARBITER_INPUTS)))
# Run the fixture, then compare feature-on/off digests over the workload.
define DEATH_VFX_RUN
	./build/test-sc64-death-vfx-$(1)
	SC64_DEATH_VFX_HASH=1 ./build/test-sc64-death-vfx-$(1) > build/death-vfx-$(1).on
	SC64_DEATH_VFX_HASH=1 ./build/test-sc64-death-vfx-$(1)-off > build/death-vfx-$(1).off
	cmp build/death-vfx-$(1).on build/death-vfx-$(1).off
	@echo "PASS death VFX $(1) state/save digest unchanged: $$(cat build/death-vfx-$(1).on)"
endef
.PHONY: test-sc64-death-vfx test-sc64-death-vfx-default test-sc64-death-vfx-unified test-sc64-death-vfx-profiles
test-sc64-death-vfx: build/test-sc64-death-vfx
	./build/test-sc64-death-vfx
test-sc64-death-vfx-default: build/test-sc64-death-vfx-default build/test-sc64-death-vfx-default-off
	$(call DEATH_VFX_RUN,default)
test-sc64-death-vfx-unified: build/test-sc64-death-vfx-unified build/test-sc64-death-vfx-unified-off
	$(call DEATH_VFX_RUN,unified)
test-sc64-death-vfx-profiles: test-sc64-death-vfx-unified build/test-sc64-death-vfx-terran build/test-sc64-death-vfx-terran-off build/test-sc64-death-vfx-zerg build/test-sc64-death-vfx-zerg-off build/test-sc64-death-vfx-protoss build/test-sc64-death-vfx-protoss-off
	$(call DEATH_VFX_RUN,terran)
	$(call DEATH_VFX_RUN,zerg)
	$(call DEATH_VFX_RUN,protoss)

# Optional Z7 presentation-state placement experiment. Both binaries use the
# same full-render/canonical feature vector; only the candidate places the
# transient death-VFX pool in the unused halfword VRAM tail.
Z7_DEATH_VFX_VRAM_FLAGS = $(Z7_STEERING_FLAGS) -DSC64_REACHABLE_FORMATION -DSC64_Z7_PRUNE_STARTED -DSC64_ROUTE_DECAY_FREQ
build/test-sc64-death-vfx-z7-canonical: tests/sc64_death_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_FULL_RENDER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(DEATH_VFX_TEST_FLAGS) $(Z7_DEATH_VFX_VRAM_FLAGS) $< -o $@
build/test-sc64-death-vfx-z7-vram: tests/sc64_death_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_FULL_RENDER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(DEATH_VFX_TEST_FLAGS) $(Z7_DEATH_VFX_VRAM_FLAGS) -DSC64_DEATH_VFX_VRAM_STORAGE $< -o $@
.PHONY: test-sc64-death-vfx-z7-vram
test-sc64-death-vfx-z7-vram: build/test-sc64-death-vfx-z7-canonical build/test-sc64-death-vfx-z7-vram
	./build/test-sc64-death-vfx-z7-vram
	SC64_DEATH_VFX_HASH=1 SC64_DEATH_VFX_PIXEL_HASH=1 ./build/test-sc64-death-vfx-z7-canonical > build/death-vfx-z7-canonical.hash
	SC64_DEATH_VFX_HASH=1 SC64_DEATH_VFX_PIXEL_HASH=1 ./build/test-sc64-death-vfx-z7-vram > build/death-vfx-z7-vram.hash
	cmp build/death-vfx-z7-canonical.hash build/death-vfx-z7-vram.hash
	@echo "PASS Z7 death-VFX VRAM storage matches canonical RGB/state/save digests"

# Explicit optional ARM link pair; neither output is a canonical ROM target.
build/starcraft-zerg-z7-death-vfx-vram-baseline.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_FULL_RENDER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_DEATH_VFX_VRAM_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7-death-vfx-vram-baseline.map -lgcc -o $@
build/starcraft-zerg-z7-death-vfx-vram.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_FULL_RENDER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_DEATH_VFX_VRAM_FLAGS) -DSC64_DEATH_VFX_VRAM_STORAGE $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7-death-vfx-vram.map -lgcc -o $@

test-starcraft: test-sc64-death-vfx test-sc64-death-vfx-default
test-unified: test-sc64-death-vfx-unified

# Optional source death art preview, built like the construction preview.
DEATH_ART_FLAGS := $(UNIFIED_FLAGS) -DSC64_DEATH_SOURCE_ART -I$(DEATH_ART_DIR)
$(DEATH_ART_HEADER): tools/sc64_death_sprites.py tools/sc64_construction_sprites.py tools/sc64_sprites.py tools/sc64_unit_facing.py
	$(PYTHON) tools/sc64_death_sprites.py
build/test-sc64-death-source: tests/sc64_death_source_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) $(DEATH_ART_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(DEATH_VFX_TEST_FLAGS) $(DEATH_ART_FLAGS) $< -o $@
build/test-sc64-death-source-off: tests/sc64_death_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(DEATH_VFX_TEST_FLAGS) $(UNIFIED_FLAGS) -DSC64_DEATH_VFX_DISABLED $< -o $@
build/test-sc64-death-source-on: tests/sc64_death_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) $(DEATH_ART_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(DEATH_VFX_TEST_FLAGS) $(DEATH_ART_FLAGS) $< -o $@
build/starcraft-death.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) $(DEATH_ART_HEADER) common/gba.h common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(DEATH_ART_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-death.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Death-Preview-GBA.gba: build/starcraft-death.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC DEATH ART' SCDA
.PHONY: sc64-death-art test-sc64-death-source death-visuals
sc64-death-art:
	$(PYTHON) tools/sc64_death_sprites.py
test-sc64-death-source: build/test-sc64-death-source build/test-sc64-death-source-on build/test-sc64-death-source-off
	./build/test-sc64-death-source
	./build/test-sc64-death-source-on
	SC64_DEATH_VFX_HASH=1 ./build/test-sc64-death-source-on > build/death-vfx-source.on
	SC64_DEATH_VFX_HASH=1 ./build/test-sc64-death-source-off > build/death-vfx-source.off
	cmp build/death-vfx-source.on build/death-vfx-source.off
	@echo "PASS death VFX source-art state/save digest unchanged: $$(cat build/death-vfx-source.on)"
death-visuals: build/StarCraft-Death-Preview-GBA.gba
	$(PYTHON) -c 'from tools.verify_roms import verify; print(verify("StarCraft-Death-Preview-GBA", "starcraft-death", "SCDA", "SC DEATH ART"))'

build/test-sc64-construction-hp: tests/sc64_construction_hp.c starcraft/sc64_construction_hp.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-construction-hp-game: tests/sc64_construction_hp_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) -DSC64_CONSTRUCTION_HP $< common/gba.c -o $@
build/test-sc64-construction-hp-z7: tests/sc64_construction_hp_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_PROBE_FLAGS) -DSC64_CONSTRUCTION_HP $< common/gba.c -o $@
build/test-sc64-construction-hp-colonies: tests/sc64_z7_colonies_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_PROBE_FLAGS) -DSC64_CONSTRUCTION_HP $< common/gba.c -o $@
.PHONY: test-sc64-construction-hp test-sc64-construction-hp-game test-sc64-construction-hp-z7
test-sc64-construction-hp: build/test-sc64-construction-hp
	./build/test-sc64-construction-hp
test-sc64-construction-hp-game: build/test-sc64-construction-hp-game
	./build/test-sc64-construction-hp-game
test-sc64-construction-hp-z7: build/test-sc64-construction-hp-z7 build/test-sc64-construction-hp-colonies
	./build/test-sc64-construction-hp-z7
	./build/test-sc64-construction-hp-colonies
test-starcraft: test-sc64-construction-hp
test-unified: test-sc64-construction-hp-game

define DIRECT_ATTACK_PROFILE
build/test-sc64-direct-attack-$(1): tests/sc64_direct_attack_game.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $(3) common/gba.c common/gba.h Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(2) -DSC64_DIRECT_ATTACK $$< common/gba.c -o $$@
endef
$(eval $(call DIRECT_ATTACK_PROFILE,races,$(RACES_FLAGS),$(RACES_INPUTS)))
$(eval $(call DIRECT_ATTACK_PROFILE,unified,$(UNIFIED_FLAGS),$(RACES_INPUTS)))
$(eval $(call DIRECT_ATTACK_PROFILE,z7,$(Z7_PROBE_FLAGS) -DSC64_CONSTRUCTION_HP,$(Z7_PROBE_INPUTS)))
.PHONY: test-sc64-direct-attack
test-sc64-direct-attack: build/test-sc64-direct-attack-races build/test-sc64-direct-attack-unified build/test-sc64-direct-attack-z7
	./build/test-sc64-direct-attack-races
	./build/test-sc64-direct-attack-unified
	./build/test-sc64-direct-attack-z7

define ROUTE_WORD_CLEAR_FIXTURE
build/test-sc64-route-word-clear-$(1): tests/sc64_route_word_clear.c starcraft/sc64_routes.h | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_ROUTE_WORD_CLEAR -DROUTE_TEST_COLS=$(2) -DROUTE_TEST_ROWS=$(3) -I. $$< -o $$@
endef
$(eval $(call ROUTE_WORD_CLEAR_FIXTURE,128x128,128,128))
$(eval $(call ROUTE_WORD_CLEAR_FIXTURE,128x96,128,96))
$(eval $(call ROUTE_WORD_CLEAR_FIXTURE,10x10,10,10))
.PHONY: test-sc64-route-word-clear
test-sc64-route-word-clear: build/test-sc64-route-word-clear-128x128 build/test-sc64-route-word-clear-128x96 build/test-sc64-route-word-clear-10x10
	./build/test-sc64-route-word-clear-128x128
	./build/test-sc64-route-word-clear-128x96
	./build/test-sc64-route-word-clear-10x10
test-starcraft: test-sc64-route-word-clear

define ROUTE_L_FAST_FIXTURE
build/test-sc64-route-l-fast-$(1): tests/race_routes.c starcraft/sc64_routes.h starcraft/sc64_bitgrid.h Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_ROUTE_PACK3 -DSC64_LARGE_MAPS -DSC64_ROUTE_VISIT_GRID -DSC64_ROUTE_WORD_CLEAR -DSC64_ROUTE_L_FAST_PATH -DRACE_TEST_COLS=$(2) -DRACE_TEST_ROWS=$(3) $(4) -I. -Icommon $$< -o $$@
endef
$(eval $(call ROUTE_L_FAST_FIXTURE,64x64,64,64))
$(eval $(call ROUTE_L_FAST_FIXTURE,128x96,128,96))
$(eval $(call ROUTE_L_FAST_FIXTURE,128x128,128,128))
$(eval $(call ROUTE_L_FAST_FIXTURE,128x96-partial,128,96,-DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_DECAY_FREQ))
.PHONY: test-sc64-route-l-fast
test-sc64-route-l-fast: build/test-sc64-route-l-fast-64x64 build/test-sc64-route-l-fast-128x96 build/test-sc64-route-l-fast-128x128 build/test-sc64-route-l-fast-128x96-partial
	./build/test-sc64-route-l-fast-64x64
	./build/test-sc64-route-l-fast-128x96
	./build/test-sc64-route-l-fast-128x128
	./build/test-sc64-route-l-fast-128x96-partial
test-starcraft: test-sc64-route-l-fast

ROUTE_ACTIVE_STRIDE_TEST_FLAGS := -DSC64_ROUTE_ACTIVE_STRIDE -DSC64_ROUTE_PACK3 -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_LARGE_MAPS -DSC64_ROUTE_VISIT_GRID
build/test-sc64-route-active-stride: tests/sc64_route_active_stride.c starcraft/sc64_routes.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined $(ROUTE_ACTIVE_STRIDE_TEST_FLAGS) -DSC64_ROUTE_WORD_CLEAR -DSC64_ROUTE_L_FAST_PATH -DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_DECAY_FREQ $< -o $@
build/test-sc64-route-active-stride-fallback: tests/sc64_route_active_stride.c starcraft/sc64_routes.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined $(ROUTE_ACTIVE_STRIDE_TEST_FLAGS) $< -o $@
.PHONY: test-sc64-route-active-stride
test-sc64-route-active-stride: build/test-sc64-route-active-stride build/test-sc64-route-active-stride-fallback
	./build/test-sc64-route-active-stride
	./build/test-sc64-route-active-stride-fallback
test-starcraft: test-sc64-route-active-stride

Z7_EGG_PROBE_FLAGS := $(Z7_PROBE_FLAGS) -DSC64_CONSTRUCTION_HP -DSC64_DIRECT_ATTACK -DSC64_Z7_EGG_PRODUCTION
build/test-sc64-z7-egg-production: tests/sc64_z7_egg_production_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_EGG_PROBE_FLAGS) $< common/gba.c -o $@
build/test-sc64-z7-egg-save: tests/sc64_z7_egg_save_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_EGG_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z7-egg-production test-sc64-z7-egg-save
test-sc64-z7-egg-production: build/test-sc64-z7-egg-production
	./build/test-sc64-z7-egg-production
test-sc64-z7-egg-save: build/test-sc64-z7-egg-save
	./build/test-sc64-z7-egg-save

Z7_SOURCE_SIGHT_FLAGS := $(Z7_PROBE_FLAGS) -DSC64_CONSTRUCTION_HP -DSC64_DIRECT_ATTACK -DSC64_Z7_SOURCE_SIGHT
build/test-sc64-z7-source-sight: tests/sc64_z7_source_sight_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_SOURCE_SIGHT_FLAGS) $< common/gba.c -o $@
build/test-sc64-z7-source-sight-eggs: tests/sc64_z7_source_sight_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_SOURCE_SIGHT_FLAGS) -DSC64_Z7_EGG_PRODUCTION $< common/gba.c -o $@
.PHONY: test-sc64-z7-source-sight
test-sc64-z7-source-sight: build/test-sc64-z7-source-sight build/test-sc64-z7-source-sight-eggs
	./build/test-sc64-z7-source-sight
	./build/test-sc64-z7-source-sight-eggs

Z7_SCOURGE_PROBE_FLAGS := $(Z7_EGG_PROBE_FLAGS) -DSC64_Z7_SOURCE_SIGHT -DSC64_Z7_SCOURGE_COMBAT

# Optional Nydus Canal runtime, hosted by the Z7 Zerg actor/save profile. The
# fixture places linked raw-134 endpoints; canonical Z7 builds leave it off.
NYDUS_PROBE_FLAGS := $(Z7_SCOURGE_PROBE_FLAGS) -DSC64_NYDUS_ENABLED
build/test-sc64-nydus-game: tests/sc64_nydus_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(NYDUS_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-nydus-game
test-sc64-nydus-game: build/test-sc64-nydus-game
	./build/test-sc64-nydus-game
# ARM evidence for the same option on the Z7 controller profile. The -off ELF
# is the identical profile without the gate, for memory A/B; the probe ELF
# replaces common/main.c with an embedded-tape driver that logs via mGBA.
NYDUS_ARM_FLAGS = $(Z7_CONTROLLER_FLAGS) -DSC64_NYDUS_ENABLED
NYDUS_ARM_COMMON := common/start.s common/gba.c common/memory.c common/audio_bank.S
build/starcraft-nydus-z7-off.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_CONTROLLER_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-nydus-z7-off.map -lgcc -o $@
	$(SIZE) $@
build/starcraft-nydus-z7-on.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(NYDUS_ARM_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-nydus-z7-on.map -lgcc -o $@
	$(SIZE) $@
build/starcraft-nydus-arm-probe.elf: tests/sc64_nydus_arm_probe.c $(NYDUS_ARM_COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(NYDUS_ARM_FLAGS) -DSC64_NYDUS_PROBE_CYCLES $(NYDUS_ARM_COMMON) $< $(filter-out starcraft/game.c,$(STARCRAFT)) $(LDFLAGS) -Wl,-Map,build/starcraft-nydus-arm-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Nydus-Z7-%-GBA.gba: build/starcraft-nydus-z7-%.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC NYDUS $*' SCNY
build/StarCraft-Nydus-ARM-Probe-GBA.gba: build/starcraft-nydus-arm-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC NYDUS PRB' SCNP
.PHONY: nydus-arm-probe
nydus-arm-probe: build/StarCraft-Nydus-Z7-off-GBA.gba build/StarCraft-Nydus-Z7-on-GBA.gba build/StarCraft-Nydus-ARM-Probe-GBA.gba
build/test-sc64-z7-scourge: tests/sc64_z7_scourge_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_SCOURGE_PROBE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z7-scourge
test-sc64-z7-scourge: build/test-sc64-z7-scourge
	./build/test-sc64-z7-scourge

# Z7-only immutable-table prevalidation. Existing profiles leave this off.
Z7_PREVALIDATED_BASE_FLAGS := $(Z7_SCOURGE_PROBE_FLAGS) -DSC64_NEAREST_RESOURCE_DEPOT
Z7_PREVALIDATED_FLAGS := $(Z7_PREVALIDATED_BASE_FLAGS) -DSC64_Z7_PREVALIDATED_TICK
define Z7_PREVALIDATION_CHECK
build/test-sc64-z7-prevalidated-$(1)-$(2): tests/$(3) $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(4) $$< common/gba.c -o $$@
endef
$(eval $(call Z7_PREVALIDATION_CHECK,reference,runtime,sc64_z7_runtime.c,$(Z7_PREVALIDATED_BASE_FLAGS)))
$(eval $(call Z7_PREVALIDATION_CHECK,candidate,runtime,sc64_z7_runtime.c,$(Z7_PREVALIDATED_FLAGS)))
$(eval $(call Z7_PREVALIDATION_CHECK,reference,save,sc64_z7_game_save.c,$(Z7_PREVALIDATED_BASE_FLAGS)))
$(eval $(call Z7_PREVALIDATION_CHECK,candidate,save,sc64_z7_game_save.c,$(Z7_PREVALIDATED_FLAGS)))
.PHONY: test-sc64-z7-prevalidated zerg-z7-prevalidated-probe
test-sc64-z7-prevalidated: $(foreach mode,reference candidate,$(foreach check,runtime save,build/test-sc64-z7-prevalidated-$(mode)-$(check)))
	./build/test-sc64-z7-prevalidated-reference-runtime
	./build/test-sc64-z7-prevalidated-candidate-runtime
	./build/test-sc64-z7-prevalidated-reference-save
	./build/test-sc64-z7-prevalidated-candidate-save
build/starcraft-zerg-z7-prevalidated-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_PREVALIDATED_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7-prevalidated-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z7-Prevalidated-Probe-GBA.gba: build/starcraft-zerg-z7-prevalidated-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z7 FAST36' SCZV
zerg-z7-prevalidated-probe: build/StarCraft-Zerg-Z7-Prevalidated-Probe-GBA.gba

# Z7 same-update condition-count experiment, layered on authenticated metadata.
Z7_COUNT_SNAPSHOT_FLAGS := $(Z7_PREVALIDATED_FLAGS) -DSC64_Z7_COUNT_SNAPSHOT
$(eval $(call Z7_PREVALIDATION_CHECK,snapshot,runtime,sc64_z7_runtime.c,$(Z7_COUNT_SNAPSHOT_FLAGS)))
$(eval $(call Z7_PREVALIDATION_CHECK,snapshot,save,sc64_z7_game_save.c,$(Z7_COUNT_SNAPSHOT_FLAGS)))
build/test-sc64-z7-count-snapshot: tests/sc64_z7_count_snapshot.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_COUNT_SNAPSHOT_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z7-count-snapshot zerg-z7-count-snapshot-probe
test-sc64-z7-count-snapshot: test-sc64-z7-prevalidated build/test-sc64-z7-prevalidated-snapshot-runtime build/test-sc64-z7-prevalidated-snapshot-save build/test-sc64-z7-count-snapshot
	./build/test-sc64-z7-prevalidated-snapshot-runtime
	./build/test-sc64-z7-prevalidated-snapshot-save
	./build/test-sc64-z7-count-snapshot
build/starcraft-zerg-z7-count-snapshot-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_COUNT_SNAPSHOT_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7-count-snapshot-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z7-Count-Snapshot-Probe-GBA.gba: build/starcraft-zerg-z7-count-snapshot-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z7 CNT36' SCZW
zerg-z7-count-snapshot-probe: build/StarCraft-Zerg-Z7-Count-Snapshot-Probe-GBA.gba
# Ordinary-input qualification candidate: the Egg, sight and Scourge rules
# plus the existing nearest-depot worker policy; no campaign promotion.
Z7_CONTROLLER_REFERENCE_FLAGS := $(Z7_SCOURGE_PROBE_FLAGS) -DSC64_NEAREST_RESOURCE_DEPOT -DSC64_DENSE_SCHEDULE
# Qualified only for this experimental probe; standard race flags stay separate.
Z7_CONTROLLER_FLAGS := $(Z7_CONTROLLER_REFERENCE_FLAGS) -DSC64_Z7_PREVALIDATED_TICK -DSC64_Z7_COUNT_SNAPSHOT
build/test-sc64-z7-controller: tests/sc64_z7_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_CONTROLLER_FLAGS) -DSC64_REACHABLE_FORMATION $< common/gba.c -o $@
.PHONY: test-sc64-z7-controller zerg-z7-controller-probe
test-sc64-z7-controller: build/test-sc64-z7-controller
	mkdir -p .local/build/z7-controller
	./build/test-sc64-z7-controller --limit 300000 --trace .local/build/z7-controller/controller-trace.json
build/starcraft-zerg-z7-controller-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_CONTROLLER_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7-controller-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z7-Controller-Probe-GBA.gba: build/starcraft-zerg-z7-controller-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z7 RUN36' SCZQ
zerg-z7-controller-probe: build/StarCraft-Zerg-Z7-Controller-Probe-GBA.gba

# Host-only, per-update replay comparison for the default-off Z7 count cache.
# Private tapes are validated and streamed by tools/sc64_z7_trace_parity.py.
define Z7_TRACE_PARITY_BUILD
build/test-sc64-z7-trace-$(1): tests/sc64_z7_trace_parity.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(2) $$< common/gba.c -o $$@
endef
$(eval $(call Z7_TRACE_PARITY_BUILD,reference,$(Z7_CONTROLLER_REFERENCE_FLAGS)))
$(eval $(call Z7_TRACE_PARITY_BUILD,count-snapshot,$(Z7_CONTROLLER_FLAGS)))

# Default-off source-radius visibility experiment; independent of race ROMs.
Z7_SIGHT_SPAN_FLAGS := $(Z7_CONTROLLER_FLAGS) -DSC64_Z7_SIGHT_SPANS
$(eval $(call Z7_TRACE_PARITY_BUILD,sight-spans,$(Z7_SIGHT_SPAN_FLAGS)))
build/test-sc64-z7-sight-spans: tests/sc64_z7_source_sight_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_SIGHT_SPAN_FLAGS) $< common/gba.c -o $@
$(eval $(call Z7_PREVALIDATION_CHECK,sight-spans,runtime,sc64_z7_runtime.c,$(Z7_SIGHT_SPAN_FLAGS)))
$(eval $(call Z7_PREVALIDATION_CHECK,sight-spans,save,sc64_z7_game_save.c,$(Z7_SIGHT_SPAN_FLAGS)))
.PHONY: test-sc64-z7-sight-spans zerg-z7-sight-spans-probe
test-sc64-z7-sight-spans: build/test-sc64-z7-sight-spans build/test-sc64-z7-prevalidated-sight-spans-runtime build/test-sc64-z7-prevalidated-sight-spans-save
	./build/test-sc64-z7-sight-spans
	./build/test-sc64-z7-prevalidated-sight-spans-runtime
	./build/test-sc64-z7-prevalidated-sight-spans-save
build/starcraft-zerg-z7-sight-spans-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_SIGHT_SPAN_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7-sight-spans-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z7-Sight-Spans-Probe-GBA.gba: build/starcraft-zerg-z7-sight-spans-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z7 VIS36' SCZX
zerg-z7-sight-spans-probe: build/StarCraft-Zerg-Z7-Sight-Spans-Probe-GBA.gba

# Z7-only qualification of fresh defense owner/flight lists. Keep the canonical
# race flags unchanged; this probe has its own parity and timing qualification.
Z7_DEFENSE_TARGET_FLAGS := $(Z7_SIGHT_SPAN_FLAGS) -DSC64_ZP_DEFENSE_TARGETS
$(eval $(call Z7_TRACE_PARITY_BUILD,defense-targets,$(Z7_DEFENSE_TARGET_FLAGS)))
build/test-sc64-z7-defense-targets: tests/sc64_zp_defense_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(Z7_DEFENSE_TARGET_FLAGS) $< common/gba.c -o $@
$(eval $(call Z7_PREVALIDATION_CHECK,defense-targets,runtime,sc64_z7_runtime.c,$(Z7_DEFENSE_TARGET_FLAGS)))
$(eval $(call Z7_PREVALIDATION_CHECK,defense-targets,save,sc64_z7_game_save.c,$(Z7_DEFENSE_TARGET_FLAGS)))
.PHONY: test-sc64-z7-defense-targets zerg-z7-defense-targets-probe
test-sc64-z7-defense-targets: build/test-sc64-z7-defense-targets build/test-sc64-z7-prevalidated-defense-targets-runtime build/test-sc64-z7-prevalidated-defense-targets-save
	./build/test-sc64-z7-defense-targets
	./build/test-sc64-z7-prevalidated-defense-targets-runtime
	./build/test-sc64-z7-prevalidated-defense-targets-save
build/starcraft-zerg-z7-defense-targets-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_DEFENSE_TARGET_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7-defense-targets-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z7-Defense-Targets-Probe-GBA.gba: build/starcraft-zerg-z7-defense-targets-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z7 DEF36' SCZY
zerg-z7-defense-targets-probe: build/StarCraft-Zerg-Z7-Defense-Targets-Probe-GBA.gba

# Unpromoted Z7 qualification with the full canonical presentation vector.
# The standard `zerg` product remains the separately qualified Z1-Z6 ROM.
# Keep the historical full-render probe and its low-level fixtures on the
# collision-list path. The selected canonical game vector below uses CONTACT.
Z7_FULL_RENDER_FLAGS = $(patsubst -I.local/sc64/z6/generated,-I$(ZERG_Z6_BANK),$(Z7_DEFENSE_TARGET_FLAGS)) $(CANONICAL_DRAW_FLAGS) $(filter-out -DSC64_CONTACT_CACHE -DSC64_CONTACT_STEER_ROM -DSC64_CONTACT_STOCK_ROM,$(ZERG_CANONICAL_FLAGS)) $(CANONICAL_MINIMAP_FLAGS) $(UNIT_FACING_FLAGS) $(CANONICAL_WEAPON_FLAGS) $(CANONICAL_DEATH_FLAGS)
Z7_FULL_RENDER_INPUTS = $(filter-out .local/sc64/z5/generated/sc64_race_sprites_generated.h .local/sc64/z6/generated/sc64_race_sprites_generated.h,$(Z7_PROBE_INPUTS)) $(ZERG_Z6_BANK)/sc64_race_sprites_generated.h $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS)
build/starcraft-zerg-z7-full-render-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_FULL_RENDER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_FULL_RENDER_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7-full-render-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z7-Full-Render-Probe-GBA.gba: build/starcraft-zerg-z7-full-render-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z7 FULL36' SCZR
.PHONY: zerg-z7-full-render-probe test-sc64-z7-full-render
zerg-z7-full-render-probe: build/StarCraft-Zerg-Z7-Full-Render-Probe-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z7-full-render

define Z7_FULL_RENDER_FIXTURE
build/test-sc64-z7-full-render-$(1): tests/$(2) $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(Z7_FULL_RENDER_INPUTS) common/gba.c common/gba.h Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $$(Z7_FULL_RENDER_FLAGS) $(3) $$< common/gba.c -o $$@
endef
$(eval $(call Z7_FULL_RENDER_FIXTURE,profile,sc64_promoted_race_profile.c))
$(eval $(call Z7_FULL_RENDER_FIXTURE,save,sc64_z7_game_save.c))
$(eval $(call Z7_FULL_RENDER_FIXTURE,frame,sc64_z3_frame_coverage.c))
$(eval $(call Z7_FULL_RENDER_FIXTURE,scourge,sc64_z7_scourge_game.c))
test-sc64-z7-full-render: $(addprefix build/test-sc64-z7-full-render-,profile save frame scourge)
	./build/test-sc64-z7-full-render-profile
	./build/test-sc64-z7-full-render-save
	./build/test-sc64-z7-full-render-frame
	./build/test-sc64-z7-full-render-scourge

$(eval $(call Z7_FULL_RENDER_FIXTURE,formation-reference,sc64_reachable_formation.c))
$(eval $(call Z7_FULL_RENDER_FIXTURE,formation,sc64_reachable_formation.c,-DSC64_REACHABLE_FORMATION))
$(eval $(call Z7_FULL_RENDER_FIXTURE,formation-graph,sc64_formation_reachability.c,-DSC64_REACHABLE_FORMATION))
.PHONY: test-sc64-z7-reachable-formation
test-sc64-z7-reachable-formation: build/test-sc64-z7-full-render-formation-reference build/test-sc64-z7-full-render-formation build/test-sc64-z7-full-render-formation-graph
	./build/test-sc64-z7-full-render-formation-reference
	./build/test-sc64-z7-full-render-formation
	./build/test-sc64-z7-full-render-formation-graph

$(eval $(call Z7_FULL_RENDER_FIXTURE,prune-reference,sc64_z7_prune_game.c))
$(eval $(call Z7_FULL_RENDER_FIXTURE,prune,sc64_z7_prune_game.c,-DSC64_Z7_PRUNE_STARTED))
.PHONY: test-sc64-z7-prune-started
test-sc64-z7-prune-started: build/test-sc64-z7-full-render-prune-reference build/test-sc64-z7-full-render-prune
	./build/test-sc64-z7-full-render-prune-reference
	./build/test-sc64-z7-full-render-prune

# Separate full-render Z7 collision qualification; canonical products stay off.
Z7_STEERING_OPTIONS := -DSC64_COLLISION_CLASS_CACHE -DSC64_COLLISION_STEER_LIST -DSC64_COLLISION_PACKED_HIT
Z7_STEERING_FLAGS = $(Z7_FULL_RENDER_FLAGS) $(Z7_STEERING_OPTIONS)
$(eval $(call Z7_FULL_RENDER_FIXTURE,steering,sc64_z7_steer_game.c,$(Z7_STEERING_OPTIONS)))
.PHONY: test-sc64-z7-steering zerg-z7-steering-probe
test-sc64-z7-steering: build/test-sc64-z7-full-render-steering
	./build/test-sc64-z7-full-render-steering

# Carried Z5 controller under the complete experimental Z7 feature vector.
# Direct AttackUnit remains enabled; the controller must issue ground clicks
# when its policy requests Attack-Move. This does not promote the profile.
$(eval $(call Z7_FULL_RENDER_FIXTURE,carried-z5,sc64_z5_campaign_run.c,$(Z7_STEERING_OPTIONS) -DSC64_REACHABLE_FORMATION -DSC64_Z7_PRUNE_STARTED -DSC64_ROUTE_DECAY_FREQ))
$(eval $(call Z7_FULL_RENDER_FIXTURE,carried-z6,sc64_z6_campaign_run.c,$(Z7_STEERING_OPTIONS) -DSC64_REACHABLE_FORMATION -DSC64_Z7_PRUNE_STARTED -DSC64_ROUTE_DECAY_FREQ))
$(eval $(call Z7_FULL_RENDER_FIXTURE,z6-controller-policy,sc64_z6_controller_policy.c,$(Z7_STEERING_OPTIONS) -DSC64_REACHABLE_FORMATION -DSC64_Z7_PRUNE_STARTED -DSC64_ROUTE_DECAY_FREQ))
build/test-sc64-z7-full-render-z6-controller-policy: tests/sc64_z6_campaign_run.c
.PHONY: test-sc64-z7-carried-z6
test-sc64-z7-carried-z6: build/test-sc64-z7-full-render-carried-z6 build/test-sc64-z7-full-render-z6-controller-policy
	mkdir -p .local/build/z7-carried-z6
	./build/test-sc64-z7-full-render-z6-controller-policy
	./build/test-sc64-z7-full-render-carried-z6 --trace .local/build/z7-carried-z6/controller.json
	$(PYTHON) tools/sc64_zp_controller_replay.py --profile zerg-z6 --trace .local/build/z7-carried-z6/controller.json --validate-only
.PHONY: test-sc64-z7-carried-z5
test-sc64-z7-carried-z5: build/test-sc64-z7-full-render-carried-z5
	mkdir -p .local/build/z7-carried-z5 .local/sc64
	./build/test-sc64-z7-full-render-carried-z5 --limit 60000 --trace .local/build/z7-carried-z5/diagnostics.json --attempt .local/build/z7-carried-z5/attempt.json --verified .local/build/z7-carried-z5/controller.json

build/starcraft-zerg-z7-steering-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_FULL_RENDER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(Z7_STEERING_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7-steering-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z7-Steering-Probe-GBA.gba: build/starcraft-zerg-z7-steering-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z7 STEP36' SCZS
zerg-z7-steering-probe: build/StarCraft-Zerg-Z7-Steering-Probe-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z7-steering

# The canonical gameplay flags match the CONTACT standalone release. Keep
# physical-input-only main-loop selection local to that standalone target.
# Historical Z7 probes and their tests continue to use Z7_FULL_RENDER_FLAGS.
ZERG_Z7_CONTACT_MASK_TABLE_HEADER = build/generated/sc64_contact_mask_table_generated.h
ZERG_Z7_CANONICAL_FLAGS = $(filter-out $(Z7_STEERING_OPTIONS),$(Z7_STEERING_FLAGS)) -DSC64_CONTACT_CACHE -DSC64_CONTACT_STEER_ROM -DSC64_CONTACT_STOCK_ROM -DSC64_REACHABLE_FORMATION -DSC64_Z7_PRUNE_STARTED -DSC64_ROUTE_DECAY_FREQ -DSC64_CONTACT_MASK_TABLE -Ibuild/generated
ZERG_Z7_RELEASE_FLAGS = $(ZERG_Z7_CANONICAL_FLAGS) -DGBA_NO_DEBUG_INPUTS
ZERG_Z7_CANONICAL_INPUTS = $(Z7_FULL_RENDER_INPUTS) $(ZERG_Z7_CONTACT_MASK_TABLE_HEADER)
build/starcraft-zerg-z7.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z7_CANONICAL_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(ZERG_Z7_RELEASE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z7.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z7-GBA.gba: build/starcraft-zerg-z7.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z7' SCZ7
.PHONY: verify-zerg-z7 zerg-z7
verify-zerg-z7: build/StarCraft-Zerg-Z7-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z7
zerg-z7: verify-zerg-z7

# Preserve a named six-entry product alongside the existing `zerg` alias.
.PHONY: verify-zerg-z6 zerg-z6
verify-zerg-z6: build/StarCraft-Zerg-Z6-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z6
zerg-z6: verify-zerg-z6

define Z7_CANONICAL_FIXTURE
build/test-sc64-z7-canonical-$(1): tests/$(2) $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(ZERG_Z7_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $$(ZERG_Z7_CANONICAL_FLAGS) $(3) $$< common/gba.c -o $$@
endef
$(eval $(call Z7_CANONICAL_FIXTURE,profile,sc64_promoted_race_profile.c))
$(eval $(call Z7_CANONICAL_FIXTURE,save,sc64_z7_game_save.c))
$(eval $(call Z7_CANONICAL_FIXTURE,frame,sc64_z3_frame_coverage.c))
$(eval $(call Z7_CANONICAL_FIXTURE,scourge,sc64_z7_scourge_game.c))
$(eval $(call Z7_CANONICAL_FIXTURE,controller,sc64_z7_campaign_run.c))
$(eval $(call Z7_CANONICAL_FIXTURE,carried-z5,sc64_z5_campaign_run.c))
$(eval $(call Z7_CANONICAL_FIXTURE,carried-z6,sc64_z6_campaign_run.c))
$(eval $(call Z7_CANONICAL_FIXTURE,z6-controller-policy,sc64_z6_controller_policy.c))
build/test-sc64-z7-canonical-z6-controller-policy: tests/sc64_z6_campaign_run.c
ZERG_Z7_CONTROLLER_TRACE ?= .local/sc64/zerg-z7-canonical-controller.jsonl
.PHONY: test-sc64-z7-canonical-controller test-zerg-z7
test-sc64-z7-canonical-controller: build/test-sc64-z7-canonical-controller tools/sc64_zp_controller_replay.py
	mkdir -p "$(dir $(ZERG_Z7_CONTROLLER_TRACE))"
	./build/test-sc64-z7-canonical-controller --limit 240000 --trace "$(ZERG_Z7_CONTROLLER_TRACE)"
	$(PYTHON) tools/sc64_zp_controller_replay.py --profile zerg-z7 --trace "$(ZERG_Z7_CONTROLLER_TRACE)" --validate-only
.PHONY: test-sc64-z7-canonical-carried-z5 test-sc64-z7-canonical-carried-z6
test-sc64-z7-canonical-carried-z5: build/test-sc64-z7-canonical-carried-z5
	mkdir -p .local/build/z7-canonical-carried-z5 .local/sc64
	./build/test-sc64-z7-canonical-carried-z5 --limit 60000 --trace .local/build/z7-canonical-carried-z5/diagnostics.json --attempt .local/build/z7-canonical-carried-z5/attempt.json --verified .local/build/z7-canonical-carried-z5/controller.json
test-sc64-z7-canonical-carried-z6: build/test-sc64-z7-canonical-carried-z6 build/test-sc64-z7-canonical-z6-controller-policy tools/sc64_zp_controller_replay.py
	mkdir -p .local/build/z7-canonical-carried-z6
	./build/test-sc64-z7-canonical-z6-controller-policy
	./build/test-sc64-z7-canonical-carried-z6 --trace .local/build/z7-canonical-carried-z6/controller.json
	$(PYTHON) tools/sc64_zp_controller_replay.py --profile zerg-z6 --trace .local/build/z7-canonical-carried-z6/controller.json --validate-only
test-zerg-z7: test-sc64-contact-mask-table build/test-sc64-z7-canonical-profile build/test-sc64-z7-canonical-save build/test-sc64-z7-canonical-frame build/test-sc64-z7-canonical-scourge test-sc64-z7-canonical-controller test-sc64-z7-canonical-carried-z5 test-sc64-z7-canonical-carried-z6 test-sc64-z7-carried-missions test-sc64-z7-canonical-skirmish
	./build/test-sc64-z7-canonical-profile
	./build/test-sc64-z7-canonical-save
	./build/test-sc64-z7-canonical-frame
	./build/test-sc64-z7-canonical-scourge

build/test-sc64-skirmish-match-zerg-z7: tests/sc64_skirmish_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z7_CANONICAL_INPUTS) $(RACE_CAMPAIGN_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z7_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z7-canonical-skirmish
test-sc64-z7-canonical-skirmish: build/test-sc64-skirmish-match-zerg-z7
	$(PYTHON) tools/sc64_skirmish_controller_matrix.py --controller $< --output .local/sc64/skirmish/zerg-z7-matches

# Recheck every carried Z1-Z4 controller under the exact Z7 candidate vector.
ZERG_Z7_CARRIED_DEPS = $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z7_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile
build/test-sc64-z7-carried-z1: tests/race_z1_controls.c $(ZERG_Z7_CARRIED_DEPS) | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z7_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-z7-carried-z2: tests/race_campaign_run.c $(ZERG_Z7_CARRIED_DEPS) | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z7_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-z7-carried-z3: tests/sc64_z3_campaign_run.c $(ZERG_Z7_CARRIED_DEPS) | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z7_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-z7-carried-z4: tests/sc64_z4_campaign_run.c $(ZERG_Z7_CARRIED_DEPS) | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z7_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z7-carried-missions
test-sc64-z7-carried-missions: build/test-sc64-z7-carried-z1 build/test-sc64-z7-carried-z2 build/test-sc64-z7-carried-z3 build/test-sc64-z7-carried-z4
	mkdir -p .local/sc64/z7-carried
	./build/test-sc64-z7-carried-z1 --trace .local/sc64/z7-carried/z1.json
	./build/test-sc64-z7-carried-z2 --controller 1 --trace .local/sc64/z7-carried/z2.json
	./build/test-sc64-z7-carried-z3 --trace .local/sc64/z7-carried/z3.json
	./build/test-sc64-z7-carried-z4 --limit 150000 --trace .local/sc64/z7-carried/z4.json

# Full normal-input skirmishes, distinct from startup/save configuration checks.
define SKIRMISH_CONTROLLER_PROFILE
build/test-sc64-skirmish-match-$(1): tests/sc64_skirmish_campaign_run.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $(3) $(if $(filter protoss,$(1)),$(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES)) common/gba.c common/gba.h Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(2) $$< common/gba.c $(if $(filter protoss,$(1)),$(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES)) -o $$@
endef
$(eval $(call SKIRMISH_CONTROLLER_PROFILE,terran,$(TERRAN_FULL_FLAGS),$(TERRAN_FULL_INPUTS)))
$(eval $(call SKIRMISH_CONTROLLER_PROFILE,zerg,$(Z3_FLAGS),$(RACE_CAMPAIGN_INPUTS)))
$(eval $(call SKIRMISH_CONTROLLER_PROFILE,protoss,$(P3_FLAGS),$(RACE_CAMPAIGN_INPUTS)))

# Cargo indicators are render-only and share the source-sprite placement used
# by indexed and packed canonical profiles.
build/test-sc64-worker-cargo-indexed: tests/sc64_worker_cargo_render.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) $(CONSTRUCTION_ART_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(RACES_FLAGS) $< common/gba.c -o $@
build/test-sc64-worker-cargo-packed: tests/sc64_worker_cargo_render.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) $(CONSTRUCTION_ART_HEADER) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-worker-cargo
test-sc64-worker-cargo: build/test-sc64-worker-cargo-indexed build/test-sc64-worker-cargo-packed
	./build/test-sc64-worker-cargo-indexed
	./build/test-sc64-worker-cargo-packed

# Actual visible geyser pixels select a centered gas structure; includes a
# Protoss field-menu construction/harvest round trip on Blood Bath.
build/test-sc64-protoss-gas-placement: tests/sc64_protoss_gas_placement.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(CONSTRUCTION_ART_HEADER) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-protoss-gas-placement
test-sc64-protoss-gas-placement: build/test-sc64-protoss-gas-placement
	./build/test-sc64-protoss-gas-placement

# Optional normal-controller reproduction of Protoss Pylon construction on
# Blood Bath, under the same full profile as the shipped Showcase ROM.
build/test-sc64-protoss-skirmish-pylons: tests/sc64_protoss_skirmish_pylons.c tests/sc64_skirmish_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(CONSTRUCTION_ART_HEADER) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-protoss-skirmish-pylons
test-sc64-protoss-skirmish-pylons: build/test-sc64-protoss-skirmish-pylons
	./build/test-sc64-protoss-skirmish-pylons

.PHONY: test-sc64-skirmish-matches
test-sc64-skirmish-matches: $(addprefix build/test-sc64-skirmish-match-,terran zerg protoss)
	$(PYTHON) tools/sc64_skirmish_controller_matrix.py --controller build/test-sc64-skirmish-match-terran --output .local/sc64/skirmish/terran-matches
	$(PYTHON) tools/sc64_skirmish_controller_matrix.py --controller build/test-sc64-skirmish-match-zerg --output .local/sc64/skirmish/zerg-matches
	$(PYTHON) tools/sc64_skirmish_controller_matrix.py --controller build/test-sc64-skirmish-match-protoss --output .local/sc64/skirmish/protoss-matches

# Optional generation-40 P7 typed save codec fixture.
P7_TYPED_COMPACT_FLAGS := $(P7_ARBITER_FLAGS) -DSC64_P7_TYPED_COMPACT_ENABLED
build/test-sc64-p7-typed-compact: tests/sc64_p7_typed_compact_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(P7_TYPED_COMPACT_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p7-typed-compact
test-sc64-p7-typed-compact: build/test-sc64-p7-typed-compact
	./build/test-sc64-p7-typed-compact
build/test-sc64-p7-typed-runtime-reference: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_COMPACT_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-typed-runtime-prevalidated: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_COMPACT_FLAGS) -DSC64_P7_PREVALIDATED_TICK $< common/gba.c -o $@
build/test-sc64-p7-typed-runtime-count: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_COMPACT_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
build/test-sc64-p7-typed-battery: tests/sc64_p7_battery_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_COMPACT_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
build/test-sc64-p7-typed-cache-policy: tests/sc64_p7_arbiter_cache_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_COMPACT_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
build/test-sc64-p7-typed-initial-save: tests/sc64_p7_initial_save_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_COMPACT_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
.PHONY: test-sc64-p7-typed-compact-matrix
test-sc64-p7-typed-compact-matrix: build/test-sc64-p7-typed-runtime-reference build/test-sc64-p7-typed-runtime-prevalidated build/test-sc64-p7-typed-runtime-count build/test-sc64-p7-typed-battery build/test-sc64-p7-typed-cache-policy build/test-sc64-p7-typed-initial-save
	./build/test-sc64-p7-typed-runtime-reference > build/sc64-p7-typed-reference.txt
	./build/test-sc64-p7-typed-runtime-prevalidated > build/sc64-p7-typed-prevalidated.txt
	./build/test-sc64-p7-typed-runtime-count > build/sc64-p7-typed-count.txt
	cmp build/sc64-p7-typed-reference.txt build/sc64-p7-typed-prevalidated.txt
	cmp build/sc64-p7-typed-reference.txt build/sc64-p7-typed-count.txt
	./build/test-sc64-p7-typed-battery
	./build/test-sc64-p7-typed-cache-policy
	./build/test-sc64-p7-typed-initial-save

# Opt-in composition of generation-40 typed rows with the P7 steering-list probe.
# Existing profiles retain their save generations and feature flags.
P7_TYPED_MOVE_FLAGS := $(P7_TYPED_COMPACT_FLAGS) $(P7_MOVE_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT
build/test-sc64-p7-typed-move-runtime: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_MOVE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-typed-move-steer-reference: tests/sc64_p7_steer_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_COMPACT_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
build/test-sc64-p7-typed-move-steer: tests/sc64_p7_steer_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_MOVE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-typed-move-battery: tests/sc64_p7_battery_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_MOVE_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-typed-move-initial-save: tests/sc64_p7_initial_save_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_MOVE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p7-typed-move protoss-p7-typed-move-probe
test-sc64-p7-typed-move: build/test-sc64-p7-typed-runtime-count build/test-sc64-p7-typed-move-runtime build/test-sc64-p7-typed-move-steer-reference build/test-sc64-p7-typed-move-steer build/test-sc64-p7-typed-move-battery build/test-sc64-p7-typed-move-initial-save
	./build/test-sc64-p7-typed-runtime-count > build/sc64-p7-typed-move-count.txt
	./build/test-sc64-p7-typed-move-runtime > build/sc64-p7-typed-move-runtime.txt
	cmp build/sc64-p7-typed-move-count.txt build/sc64-p7-typed-move-runtime.txt
	./build/test-sc64-p7-typed-move-steer-reference > build/sc64-p7-typed-move-steer-reference.txt
	./build/test-sc64-p7-typed-move-steer > build/sc64-p7-typed-move-steer.txt
	cmp build/sc64-p7-typed-move-steer-reference.txt build/sc64-p7-typed-move-steer.txt
	./build/test-sc64-p7-typed-move-battery
	./build/test-sc64-p7-typed-move-initial-save
build/starcraft-protoss-p7-typed-move-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_TYPED_MOVE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-typed-move-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Typed-Move-Probe-GBA.gba: build/starcraft-protoss-p7-typed-move-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 MOVE40' SC7M
protoss-p7-typed-move-probe: build/StarCraft-Protoss-P7-Typed-Move-Probe-GBA.gba

# Ordinary-input Homeland controller for the existing generation-40 probe.
# This remains separate from canonical campaign promotion/ARM qualification.
build/test-sc64-p7-controller: tests/sc64_p7_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_MOVE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p7-controller
test-sc64-p7-controller: build/test-sc64-p7-controller
	mkdir -p .local/build/p7-controller
	./build/test-sc64-p7-controller .local/build/p7-controller/controller-trace.jsonl

# Separate seven-mission qualification profile. Existing probes keep their flags.
P7_CAMPAIGN_COMPOSITION_FLAGS := $(P7_TYPED_MOVE_FLAGS) -DSC64_P3_PRODUCTION_EXIT -DSC64_NEAREST_RESOURCE_DEPOT
define P7_CAMPAIGN_CONTROLLER
build/test-sc64-p7-campaign-$(1): tests/$(2).c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(P7_BATTERY_INPUTS) common/gba.c common/gba.h Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(P7_CAMPAIGN_COMPOSITION_FLAGS) $$< common/gba.c -o $$@
endef
$(eval $(call P7_CAMPAIGN_CONTROLLER,early,race_campaign_run))
$(eval $(call P7_CAMPAIGN_CONTROLLER,p3,sc64_p3_expansion_run))
$(eval $(call P7_CAMPAIGN_CONTROLLER,p4,sc64_p4_campaign_run))
$(eval $(call P7_CAMPAIGN_CONTROLLER,p5,sc64_p5_campaign_run))
$(eval $(call P7_CAMPAIGN_CONTROLLER,p6,sc64_p6_campaign_run))
$(eval $(call P7_CAMPAIGN_CONTROLLER,p7,sc64_p7_campaign_run))
$(eval $(call P7_CAMPAIGN_CONTROLLER,skirmish,sc64_skirmish_campaign_run))
$(eval $(call P7_CAMPAIGN_CONTROLLER,save,sc64_p7_typed_compact_game))
.PHONY: test-sc64-p7-campaign-composition protoss-p7-campaign-probe
test-sc64-p7-campaign-composition: $(addprefix build/test-sc64-p7-campaign-,early p3 p4 p5 p6 p7 skirmish save)
	mkdir -p .local/build/p7-campaign
	./build/test-sc64-p7-campaign-save
	./build/test-sc64-p7-campaign-early --controller 2 --trace .local/build/p7-campaign/p1.json
	./build/test-sc64-p7-campaign-early --controller 3 --trace .local/build/p7-campaign/p2.json
	./build/test-sc64-p7-campaign-p3 --trace .local/build/p7-campaign/p3.json
	./build/test-sc64-p7-campaign-p4 --attempt-tag p7-campaign
	./build/test-sc64-p7-campaign-p5 --attempt-tag p7-campaign
	./build/test-sc64-p7-campaign-p6 --trace .local/build/p7-campaign/p6.json
	./build/test-sc64-p7-campaign-p7 .local/build/p7-campaign/p7.jsonl
	$(PYTHON) tools/sc64_skirmish_controller_matrix.py --controller build/test-sc64-p7-campaign-skirmish --output .local/build/p7-campaign/skirmish

build/starcraft-protoss-p7-campaign-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_CAMPAIGN_COMPOSITION_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-campaign-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Campaign-Probe-GBA.gba: build/starcraft-protoss-p7-campaign-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 CAMP40' SC7C
protoss-p7-campaign-probe: build/StarCraft-Protoss-P7-Campaign-Probe-GBA.gba

# Opt-in exact source-rectangle reuse in P7 aura and detection.
P7_TYPED_RECT_FLAGS := $(P7_TYPED_MOVE_FLAGS) -DSC64_P7_GEOMETRY_RECT_HOIST_ENABLED
build/test-sc64-p7-rect-runtime: tests/sc64_p7_prevalidated_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_RECT_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-rect-steer: tests/sc64_p7_steer_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_RECT_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-rect-policy: tests/sc64_p7_arbiter_cache_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_RECT_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-rect-battery: tests/sc64_p7_battery_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_RECT_FLAGS) $< common/gba.c -o $@
build/test-sc64-p7-rect-initial-save: tests/sc64_p7_initial_save_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_RECT_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-p7-rect protoss-p7-rect-probe
test-sc64-p7-rect: build/test-sc64-p7-typed-move-runtime build/test-sc64-p7-typed-move-steer build/test-sc64-p7-rect-runtime build/test-sc64-p7-rect-steer build/test-sc64-p7-rect-policy build/test-sc64-p7-rect-battery build/test-sc64-p7-rect-initial-save
	./build/test-sc64-p7-typed-move-runtime > build/sc64-p7-rect-reference.txt
	./build/test-sc64-p7-rect-runtime > build/sc64-p7-rect-runtime.txt
	cmp build/sc64-p7-rect-reference.txt build/sc64-p7-rect-runtime.txt
	./build/test-sc64-p7-typed-move-steer > build/sc64-p7-rect-steer-reference.txt
	./build/test-sc64-p7-rect-steer > build/sc64-p7-rect-steer.txt
	cmp build/sc64-p7-rect-steer-reference.txt build/sc64-p7-rect-steer.txt
	./build/test-sc64-p7-rect-policy
	./build/test-sc64-p7-rect-battery
	./build/test-sc64-p7-rect-initial-save
build/starcraft-protoss-p7-rect-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_TYPED_RECT_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-rect-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Rect-Probe-GBA.gba: build/starcraft-protoss-p7-rect-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 RECT40' SC7R
protoss-p7-rect-probe: build/StarCraft-Protoss-P7-Rect-Probe-GBA.gba

# Asset-free packing coverage is part of the standard host suite.
build/test-sc64-p7-typed-codec: tests/sc64_p7_typed_codec.c starcraft/sc64_p7_typed_unit_save.h starcraft/sc64_p7_typed_building_save.h starcraft/sc64_zp_unit_save.h starcraft/sc64_compact_building_save.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-p7-typed-codec
test-sc64-p7-typed-codec: build/test-sc64-p7-typed-codec
	./build/test-sc64-p7-typed-codec
test-starcraft: test-sc64-p7-typed-codec

# Asset-free Carrier storage and codec coverage.
build/test-sc64-interceptor-pool-codec: tests/sc64_interceptor_pool_codec.c starcraft/sc64_interceptor_pool_codec.c starcraft/sc64_interceptor_pool_codec.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -Istarcraft tests/sc64_interceptor_pool_codec.c starcraft/sc64_interceptor_pool_codec.c -o $@
.PHONY: test-sc64-interceptor-pool-codec
test-sc64-interceptor-pool-codec: build/test-sc64-interceptor-pool-codec
	./build/test-sc64-interceptor-pool-codec
test-starcraft: test-sc64-interceptor-pool-codec

# Bounded source-coordinate projectile motion and serialization continuation.
build/test-sc64-interceptor-motion: tests/sc64_interceptor_motion.c starcraft/sc64_interceptor_motion.c starcraft/sc64_interceptor_motion.h starcraft/sc64_interceptor_motion_kernel.h starcraft/sc64_interceptor_direction_tables.h starcraft/sc64_interceptor_pool_codec.c starcraft/sc64_interceptor_pool_codec.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Istarcraft $< starcraft/sc64_interceptor_motion.c starcraft/sc64_interceptor_pool_codec.c -o $@
.PHONY: test-sc64-interceptor-motion
test-sc64-interceptor-motion: build/test-sc64-interceptor-motion
	./build/test-sc64-interceptor-motion
test-starcraft: test-sc64-interceptor-motion

# Generation 41 reserves the Mode 4 OBJ arena and saves all 100 projectile
# slots. Child training/flight/weapon-event integration remains separate.
P7_PROJECTILE_FLAGS := $(P7_TYPED_RECT_FLAGS) -DSC64_P7_PROJECTILES_ENABLED
P7_PROJECTILE_SOURCES := starcraft/sc64_interceptor_motion.c starcraft/sc64_interceptor_pool_codec.c
build/test-sc64-p7-projectile: tests/sc64_p7_projectile_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_PROJECTILE_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) -o $@
.PHONY: test-sc64-p7-projectile protoss-p7-projectile-probe
test-sc64-p7-projectile: build/test-sc64-p7-projectile
	./build/test-sc64-p7-projectile
build/starcraft-protoss-p7-projectile-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_PROJECTILE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-projectile-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Projectile-Probe-GBA.gba: build/starcraft-protoss-p7-projectile-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 SHOT41' SC7J
protoss-p7-projectile-probe: build/StarCraft-Protoss-P7-Projectile-Probe-GBA.gba

P7_CARRIER_TRAINING_FLAGS := $(P7_PROJECTILE_FLAGS) -DSC64_P7_CARRIER_TRAINING_ENABLED
build/test-sc64-p7-carrier-training: tests/sc64_p7_carrier_training_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_CARRIER_TRAINING_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) -o $@
.PHONY: test-sc64-p7-carrier-training protoss-p7-carrier-training-probe
test-sc64-p7-carrier-training: build/test-sc64-p7-carrier-training
	./build/test-sc64-p7-carrier-training
build/starcraft-protoss-p7-carrier-training-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_CARRIER_TRAINING_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-carrier-training-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Carrier-Training-Probe-GBA.gba: build/starcraft-protoss-p7-carrier-training-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 HANG42' SC7K
protoss-p7-carrier-training-probe: build/StarCraft-Protoss-P7-Carrier-Training-Probe-GBA.gba

# Generation 43: launched Interceptor flight, attack passes and return.
# The order model is outside $(STARCRAFT) until this profile links it.
P7_INTERCEPTOR_FLIGHT_FLAGS := $(P7_CARRIER_TRAINING_FLAGS) -DSC64_P7_INTERCEPTOR_FLIGHT_ENABLED
P7_INTERCEPTOR_FLIGHT_SOURCES := starcraft/sc64_interceptor_orders.c
build/test-sc64-p7-interceptor-flight: tests/sc64_p7_interceptor_flight_game.c $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_INTERCEPTOR_FLIGHT_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-p7-interceptor-flight protoss-p7-interceptor-flight-probe
test-sc64-p7-interceptor-flight: build/test-sc64-p7-interceptor-flight
	./build/test-sc64-p7-interceptor-flight
build/starcraft-protoss-p7-interceptor-flight-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_INTERCEPTOR_FLIGHT_FLAGS) $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-interceptor-flight-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Interceptor-Flight-Probe-GBA.gba: build/starcraft-protoss-p7-interceptor-flight-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 FLY43' SC7L
protoss-p7-interceptor-flight-probe: build/StarCraft-Protoss-P7-Interceptor-Flight-Probe-GBA.gba

# Dedicated filming build. Hidden menu and transient scenes stay off in all
# canonical profiles; the P7 bundle supplies Carrier/Ultralisk render actors.
# Presentation-only 32-direction unit facing (including Siege Tank turrets),
# from per-direction frames imported into a separate private header.
$(UNIT_FACING_HEADER): tools/sc64_unit_facing.py tools/sc64_race_sprites.py tools/sc64_sprites.py
	$(PYTHON) tools/sc64_unit_facing.py
$(TANK_SOURCE_HEADER): tools/sc64_tank_source_vfx.py tools/sc64_construction_sprites.py tools/sc64_sprites.py .local/sc64/backwater/generated/palette-remap.bin .local/sc64/backwater/generated/palette.json
	$(PYTHON) tools/sc64_tank_source_vfx.py --out $(dir $@)

# Retain the earlier Tank-only renderer and source-LUT oracle with/without
# scene clipping. The new canonical pipeline has separate source-bank fixtures.
TANK_SOURCE_LEGACY_TEST_FLAGS = $(filter-out -DSC64_WEAPON_SOURCE_VFX -DSC64_UNIT_ATTACK_POSES_ENABLED,$(TERRAN_ROM_FLAGS))
TANK_SOURCE_TEST_NOCLIP_FLAGS = $(filter-out -DGBA_SCENE_CLIP -DSC64_STATIC_DIRTY_ROWS,$(TANK_SOURCE_LEGACY_TEST_FLAGS))
build/test-sc64-tank-source-vfx-game: tests/sc64_tank_source_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(TANK_SOURCE_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TANK_SOURCE_LEGACY_TEST_FLAGS) $< common/gba.c -o $@
build/test-sc64-tank-source-vfx-game-no-clip: tests/sc64_tank_source_vfx_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(TANK_SOURCE_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TANK_SOURCE_TEST_NOCLIP_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-tank-source-vfx-game
test-sc64-tank-source-vfx-game: build/test-sc64-tank-source-vfx-game build/test-sc64-tank-source-vfx-game-no-clip
	./build/test-sc64-tank-source-vfx-game
	./build/test-sc64-tank-source-vfx-game-no-clip
build/starcraft-terran.elf build/starcraft-zerg-z4.elf build/starcraft-protoss-p5-shuttle.elf build/starcraft-protoss-p6.elf build/starcraft-showcase.elf: $(TANK_SOURCE_HEADER)
build/test-terran build/test-sc64-terran-full-profile build/test-zerg build/test-sc64-zerg-promoted-profile build/test-protoss build/test-sc64-protoss-promoted-profile build/test-sc64-protoss-canonical-storm build/test-sc64-protoss-canonical-storage build/test-sc64-protoss-canonical-controller build/test-sc64-p4-census: $(TANK_SOURCE_HEADER)
UNIT_FACING_FLAGS := -I.local/sc64/facing/generated -DSC64_UNIT_FACING
# StarCraft command-card buttons for the race order list, imported from the
# verified ROM into a private header (see tools/sc64_command_icons.py).
COMMAND_ICON_HEADER := .local/sc64/command-icons/generated/sc64_command_icons_generated.h
COMMAND_ICON_FLAGS := -I.local/sc64/command-icons/generated -DSC64_COMMAND_ICONS
# The HUD icons take the nearest runtime palette colours: the UI palette in
# common/gba.c and the imported entries beside the sprite remap.
$(COMMAND_ICON_HEADER): tools/sc64_command_icons.py tools/sc64_sprites.py common/gba.c .local/sc64/backwater/generated/palette-remap.bin .local/sc64/backwater/generated/palette.json
	$(PYTHON) tools/sc64_command_icons.py
SHOWCASE_UNIT_WORD_FLAGS ?= -DSC64_UNIT_WORD_SPRITES
SHOWCASE_SIEGE_FLAGS ?= -DSC64_SIEGE_SPLASH
# Clip world art before it reaches either HUD band. Keep the infrequent
# procedural diamond fallback in ROM so clipping retains the IWRAM reserve.
SHOWCASE_FLAGS := $(P7_TYPED_RECT_FLAGS) -DSC64_SHOWCASE_ENABLED -DGBA_SCENE_CLIP -DGBA_DIAMOND_ROM $(CANONICAL_MINIMAP_FLAGS) $(UNIT_FACING_FLAGS) $(CANONICAL_WEAPON_FLAGS) $(CANONICAL_SOURCE_VFX_CULL_FOR_WEAPON_FLAGS) $(CANONICAL_TRACE_DURATION_FOR_WEAPON_FLAGS) $(CANONICAL_DIRECT_FIRING_FLAGS) $(SHOWCASE_UNIT_WORD_FLAGS) $(SHOWCASE_SIEGE_FLAGS) $(CANONICAL_DEATH_FLAGS) -DGBA_MAPPED_BLIT_IWRAM -DSC64_FIELD_CONTROLS $(COMMAND_ICON_FLAGS)
build/starcraft-showcase.elf build/test-sc64-showcase-game build/test-sc64-showcase-game-packed build/test-sc64-showcase-game-indexed build/test-sc64-protoss-gas-placement build/test-sc64-protoss-skirmish-pylons build/test-sc64-native-weapon-game build/test-sc64-showcase-death-source: $(COMMAND_ICON_HEADER)
build/starcraft-showcase.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(SHOWCASE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-showcase.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Showcase-GBA.gba: build/starcraft-showcase.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC SHOWCASE' SCSH
.PHONY: showcase test-sc64-showcase test-sc64-showcase-front
showcase: build/StarCraft-Showcase-GBA.gba
build/test-sc64-showcase-front: tests/sc64_frontend.c starcraft/sc64_frontend.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_SHOWCASE_ENABLED -Icommon $< -o $@
build/test-sc64-showcase-game: tests/sc64_showcase_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) $< common/gba.c -o $@
build/test-sc64-showcase-game-packed: tests/sc64_showcase_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) -DSC64_UNIT_WORD_SPRITES -DGBA_SCENE_CLIP $< common/gba.c -o $@
build/test-sc64-showcase-game-indexed: tests/sc64_showcase_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(filter-out -DSC64_UNIT_WORD_SPRITES,$(SHOWCASE_FLAGS)) $< common/gba.c -o $@
test-sc64-showcase-front: build/test-sc64-showcase-front
	./build/test-sc64-showcase-front
test-starcraft: test-sc64-showcase-front
test-sc64-showcase: test-sc64-showcase-front build/test-sc64-showcase-game build/test-sc64-showcase-game-packed build/test-sc64-showcase-game-indexed
	./build/test-sc64-showcase-game
	./build/test-sc64-showcase-game-packed
	./build/test-sc64-showcase-game-indexed

build/test-sc64-t5-facing: tests/sc64_t5_facing.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) $(UNIT_FACING_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T5_PROBE_FLAGS) $(UNIT_FACING_FLAGS) -DSC64_UNIT_WORD_SPRITES $< common/gba.c -o $@
.PHONY: test-sc64-t5-facing
test-sc64-t5-facing: build/test-sc64-t5-facing
	./build/test-sc64-t5-facing

build/starcraft-protoss-p7-typed-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_TYPED_COMPACT_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-typed-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Typed-Probe-GBA.gba: build/starcraft-protoss-p7-typed-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 TYPED' SC7T
.PHONY: protoss-p7-typed-probe
protoss-p7-typed-probe: build/StarCraft-Protoss-P7-Typed-Probe-GBA.gba

# Audio: codec and mixer/sequencer checks, a Python/C codec cross check on
# synthetic PCM, presentation hook fixtures, and a long controller run whose
# state hash must not change when the audio observer is compiled out
# (GAME_AUDIO_OFF).
AUDIO_TEST_DEPS := common/gba.c common/gba.h common/audio_mixer.h common/audio_codec.h $(STARCRAFT) $(wildcard starcraft/*.h) Makefile
AUDIO_CODEC_TEST_DEPS := common/audio_codec.h tests/audio_codec_ref.h common/gba.h
build/test-audio-codec: tests/audio_codec.c $(AUDIO_CODEC_TEST_DEPS) | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $< -o $@
build/test-audio-mixer: tests/audio_mixer.c common/audio_mixer.h $(AUDIO_CODEC_TEST_DEPS) | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $< -o $@

# Both directions on generated vectors: Python-encoded streams decode with the
# C decoder, C-encoded streams with the pack tool's reference decoder. The
# codec pads to whole 8-sample blocks with zeros (NAME.pcm8).
AUDIO_CROSS_DIR := build/audio-codec-cross
.PHONY: test-audio-codec-cross
test-audio-codec-cross: build/test-audio-codec tests/audio_codec_vectors.py tools/sc64_audio_pack.py
	rm -rf $(AUDIO_CROSS_DIR)
	$(PYTHON) tests/audio_codec_vectors.py $(AUDIO_CROSS_DIR)
	set -e; for pcm in $(AUDIO_CROSS_DIR)/*.pcm; do \
	  base=$${pcm%.pcm}; n=$$(wc -c < $$pcm | tr -d ' '); padded=$$(( (n + 7) / 8 * 8 )); \
	  $(PYTHON) tools/sc64_audio_pack.py --encode-pcm $$pcm --out $$base.py.stream > $$base.py.json; \
	  ./build/test-audio-codec decode $$base.py.stream $$padded $$base.c.pcm; \
	  cmp $$base.pcm8 $$base.c.pcm; \
	  ./build/test-audio-codec encode $$pcm $$base.c.stream > $$base.c.json; \
	  $(PYTHON) tools/sc64_audio_pack.py --decode-stream $$base.c.stream --samples $$padded --out $$base.py.pcm; \
	  cmp $$base.pcm8 $$base.py.pcm; \
	  if cmp -s $$base.py.stream $$base.c.stream; then same=identical; else same=different; fi; \
	  echo "  $$(basename $$base): $$n samples, python $$(wc -c < $$base.py.stream | tr -d ' ') bytes, C $$(wc -c < $$base.c.stream | tr -d ' ') bytes, streams $$same"; \
	done
	@echo "audio codec cross check: Python and C streams decode identically both ways"

# Optional private check, not part of ./dev test: the C decoder reproduces the
# generated bank's manifest pcm_sha256 (needs a format-2 bank in SC64_AUDIO_DIR).
build/test-audio-bank: tests/audio_bank.c common/audio_codec.h common/gba.h $(SC64_AUDIO_DIR)/sc64_audio_generated.h | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon -DSC64_AUDIO_IMPORTED -I$(SC64_AUDIO_DIR) $< -o $@
.PHONY: test-audio-bank
test-audio-bank: build/test-audio-bank
	./build/test-audio-bank $(SC64_AUDIO_DIR)/sc64_audio_generated.bin $(SC64_AUDIO_DIR)/sc64_audio_manifest.json

define GAME_AUDIO_PROFILE
build/test-game-audio-$(1)-on: tests/game_audio_run.c $$(AUDIO_TEST_DEPS) $(3) | build
	$$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(2) $$< common/gba.c -o $$@
build/test-game-audio-$(1)-off: tests/game_audio_run.c $$(AUDIO_TEST_DEPS) $(3) | build
	$$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(2) -DGAME_AUDIO_OFF $$< common/gba.c -o $$@
.PHONY: test-game-audio-$(1)
test-game-audio-$(1): build/test-game-audio-$(1)-on build/test-game-audio-$(1)-off
	./build/test-game-audio-$(1)-on hooks
	./build/test-game-audio-$(1)-on > build/test-game-audio-$(1)-on.txt
	./build/test-game-audio-$(1)-off > build/test-game-audio-$(1)-off.txt
	cat build/test-game-audio-$(1)-on.txt
	test "$$$$(grep '^state' build/test-game-audio-$(1)-on.txt)" = "$$$$(grep '^state' build/test-game-audio-$(1)-off.txt)"
	@echo "game audio $(1): audio-on and audio-off state hashes match"
endef
$(eval $(call GAME_AUDIO_PROFILE,default,,))
$(eval $(call GAME_AUDIO_PROFILE,unified,$(UNIFIED_FLAGS),$(RACES_INPUTS)))

.PHONY: test-audio
test-audio: build/test-audio-codec build/test-audio-mixer test-audio-codec-cross test-game-audio-default
	./build/test-audio-codec
	./build/test-audio-mixer
test-starcraft: test-audio
test-unified: test-game-audio-unified

# Stock profile facts are generated from the pinned public BWAPI default tables.
# The normal local test stays asset-free; pass SC64_STOCK_FACTS to run the full
# independent table-by-table comparison when the reference JSON is available.
build/test-sc64-stock-profiles: tests/sc64_stock_profiles_dump.c starcraft/sc64_stock_profiles.h | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-stock-combat: tests/sc64_stock_combat.c starcraft/sc64_stock_combat.h starcraft/sc64_stock_profiles.h | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-raynor-stock20: tests/sc64_raynor_stock_fixture.c starcraft/race_rules.h starcraft/sc64_stock_profiles.h | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-raynor-stock19: tests/sc64_raynor_stock_fixture.c starcraft/race_rules.h starcraft/sc64_stock_profiles.h | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_T4_RUNTIME $< -o $@
.PHONY: test-sc64-stock-profiles test-sc64-stock-profiles-facts
test-sc64-stock-profiles: build/test-sc64-stock-profiles build/test-sc64-stock-combat build/test-sc64-raynor-stock20 build/test-sc64-raynor-stock19
	./build/test-sc64-stock-profiles >/dev/null
	./build/test-sc64-stock-combat
	./build/test-sc64-raynor-stock20
	./build/test-sc64-raynor-stock19
SC64_STOCK_FACTS ?= .local/unit-specs/reference.json
test-sc64-stock-profiles-facts:
	@test -f "$(SC64_STOCK_FACTS)" || { echo "Set SC64_STOCK_FACTS to the extracted pinned BWAPI reference JSON"; exit 1; }
	$(PYTHON) tools/test_sc64_stock_profiles.py --facts "$(SC64_STOCK_FACTS)"
test: test-sc64-stock-profiles

# Exercise stock weapon fields through the actual campaign tick_combat path.
build/test-sc64-stock-combat-runtime: tests/sc64_stock_combat_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(T4_STORAGE_FLAGS) $< common/gba.c -o $@
build/test-sc64-stock-combat-runtime-base: tests/sc64_stock_combat_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-stock-combat-runtime
test-sc64-stock-combat-runtime: build/test-sc64-stock-combat-runtime build/test-sc64-stock-combat-runtime-base
	./build/test-sc64-stock-combat-runtime
	./build/test-sc64-stock-combat-runtime-base
test-unified: test-sc64-stock-motion-runtime test-sc64-stock-combat-runtime test-sc64-scv-contact test-sc64-scv-contact-zp test-sc64-construction-source

# Compare every compiled race-building identity against the independent public
# BWAPI numeric profile rows. Build the same fixture under each profile gate so
# optional campaign suffixes and their max-HP/shield/armor helpers stay covered.
SC64_BUILDING_STOCK_TEST_FLAGS := -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -Icommon
define BUILD_SC64_BUILDING_STOCK_PROFILE
build/test-sc64-building-stock-$(1): tests/sc64_building_stock_fixture.c $(wildcard starcraft/*.h) Makefile | build
	$$(CLANG) $$(SC64_BUILDING_STOCK_TEST_FLAGS) $(2) $$< -o $$@
endef
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,base,))
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,z3,-DSC64_Z3_ENABLED -DSC64_CAMPAIGN_RACE=1))
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,p3,-DSC64_P3_ENABLED -DSC64_P3_RUNTIME -DSC64_CAMPAIGN_RACE=2))
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,t4,-DSC64_T4_RUNTIME))
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,t5,-DSC64_T5_RUNTIME))
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,late,-DSC64_TERRAN_LATE_ACTORS))
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,terran-full,-DSC64_TERRAN_LATE_ACTORS -DSC64_T4_RUNTIME -DSC64_T5_RUNTIME -DSC64_T6_ENABLED))
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,zp-p7,-DSC64_ZP_CAMPAIGN_ACTORS -DSC64_P7_ACTORS))
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,z5-p6,-DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z5_ACTORS -DSC64_Z5_DOODADS -DSC64_P6_ACTORS))
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,z6,-DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z6_RUNTIME -DSC64_Z6_ACTORS))
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,z7,-DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z5_ACTORS -DSC64_Z5_DOODADS -DSC64_P6_ACTORS -DSC64_Z6_RUNTIME -DSC64_Z6_ACTORS -DSC64_Z7_RUNTIME -DSC64_Z7_ENABLED -DSC64_Z7_CAMPAIGN_ENTRY))
$(eval $(call BUILD_SC64_BUILDING_STOCK_PROFILE,combined,-DSC64_TERRAN_LATE_ACTORS -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_P7_ACTORS -DSC64_P6_ACTORS -DSC64_Z5_ACTORS -DSC64_Z5_DOODADS -DSC64_T4_RUNTIME -DSC64_Z6_RUNTIME -DSC64_T6_ENABLED))
.PHONY: test-sc64-building-stock
test-sc64-building-stock: build/test-sc64-building-stock-base build/test-sc64-building-stock-z3 build/test-sc64-building-stock-p3 build/test-sc64-building-stock-t4 build/test-sc64-building-stock-t5 build/test-sc64-building-stock-late build/test-sc64-building-stock-terran-full build/test-sc64-building-stock-zp-p7 build/test-sc64-building-stock-z5-p6 build/test-sc64-building-stock-z6 build/test-sc64-building-stock-z7 build/test-sc64-building-stock-combined
	./build/test-sc64-building-stock-base
	./build/test-sc64-building-stock-z3
	./build/test-sc64-building-stock-p3
	./build/test-sc64-building-stock-t4
	./build/test-sc64-building-stock-t5
	./build/test-sc64-building-stock-late
	./build/test-sc64-building-stock-terran-full
	./build/test-sc64-building-stock-zp-p7
	./build/test-sc64-building-stock-z5-p6
	./build/test-sc64-building-stock-z6
	./build/test-sc64-building-stock-z7
	./build/test-sc64-building-stock-combined
test: test-sc64-building-stock
test-unified: test-sc64-building-stock

# Exercise the worker-health branch through actual game_update() calls in both
# imported stock profiles and the original-art runtime.
build/test-sc64-worker-heal-imported: tests/sc64_worker_heal_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(RACES_FLAGS) $< common/gba.c -o $@
build/test-sc64-worker-heal-legacy: tests/sc64_worker_heal_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $< common/gba.c -o $@
.PHONY: test-sc64-worker-heal test-sc64-worker-heal-legacy test-sc64-worker-heal-imported
test-sc64-worker-heal-legacy: build/test-sc64-worker-heal-legacy
	./build/test-sc64-worker-heal-legacy
test-sc64-worker-heal-imported: build/test-sc64-worker-heal-imported
	./build/test-sc64-worker-heal-imported
test-sc64-worker-heal: test-sc64-worker-heal-legacy test-sc64-worker-heal-imported
test: test-sc64-worker-heal-legacy
test-unified: test-sc64-worker-heal

.PHONY: test-sc64-late-building-renderer
build/test-sc64-late-building-renderer: tests/sc64_late_building_renderer.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_FULL_FLAGS) $< common/gba.c -o $@
test-sc64-late-building-renderer: build/test-sc64-late-building-renderer
	./build/test-sc64-late-building-renderer
test-terran: test-sc64-late-building-renderer

.PHONY: test-sc64-vital-bars
VITAL_BAR_TEST_FLAGS := -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -Icommon
build/test-sc64-vital-bars: tests/sc64_vital_bars_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(VITAL_BAR_TEST_FLAGS) $< common/gba.c -o $@
test-sc64-vital-bars: build/test-sc64-vital-bars
	./build/test-sc64-vital-bars
test: test-sc64-vital-bars

# Requalify every carried chapter with the canonical six-entry Zerg pools and
# flags, rather than relying on wins from smaller or staged profiles.
ZERG_CARRIED_DEPS = $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z6_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile
build/test-sc64-zerg-carried-z1: tests/race_z1_controls.c $(ZERG_CARRIED_DEPS) | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-zerg-carried-z2: tests/race_campaign_run.c $(ZERG_CARRIED_DEPS) | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-zerg-carried-z3: tests/sc64_z3_campaign_run.c $(ZERG_CARRIED_DEPS) | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-zerg-carried-z4: tests/sc64_z4_campaign_run.c $(ZERG_CARRIED_DEPS) | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-zerg-carried-z5: tests/sc64_z5_campaign_run.c $(ZERG_CARRIED_DEPS) | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-zerg-carried-z6: tests/sc64_z6_campaign_run.c $(ZERG_CARRIED_DEPS) | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_ROM_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-zerg-carried-missions
test-sc64-zerg-carried-missions: build/test-sc64-zerg-carried-z1 build/test-sc64-zerg-carried-z2 build/test-sc64-zerg-carried-z3 build/test-sc64-zerg-carried-z4 build/test-sc64-zerg-carried-z5 build/test-sc64-zerg-carried-z6
	mkdir -p .local/sc64
	./build/test-sc64-zerg-carried-z1 --trace .local/sc64/z6-carried-z1-controller.json
	./build/test-sc64-zerg-carried-z2 --controller 1 --trace .local/sc64/z6-carried-z2-controller.json
	./build/test-sc64-zerg-carried-z3 --trace .local/sc64/z6-carried-z3-controller.json
	./build/test-sc64-zerg-carried-z4 --limit 150000 --trace .local/sc64/z6-carried-z4-controller.json
	./build/test-sc64-zerg-carried-z5 --limit 60000 --trace .local/sc64/z6-carried-z5-diagnostics.json --attempt .local/sc64/z6-carried-z5-attempt.json --verified .local/sc64/z6-carried-z5-controller.json
	./build/test-sc64-zerg-carried-z6 --trace .local/sc64/z6-carried-z6-controller.json
test-zerg: test-sc64-zerg-carried-missions

# P2's timed source roster must not become a false Fenix death when the
# bounded unit pool is full. Cover both the shared and canonical adapters.
.PHONY: test-sc64-p2-reinforcements
build/test-sc64-p2-reinforcements: tests/sc64_p2_reinforcements.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(UNIFIED_FLAGS) $< common/gba.c -o $@
build/test-sc64-p2-reinforcements-canonical: tests/sc64_p2_reinforcements.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
test-sc64-p2-reinforcements: build/test-sc64-p2-reinforcements build/test-sc64-p2-reinforcements-canonical
	./build/test-sc64-p2-reinforcements
	./build/test-sc64-p2-reinforcements-canonical
test-unified test-protoss: test-sc64-p2-reinforcements

build/test-sc64-protoss-carried-campaign: tests/race_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-protoss-carried-p3: tests/sc64_p3_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-protoss-carried-p5: tests/sc64_p5_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-protoss-carried-missions
test-sc64-protoss-carried-missions: build/test-sc64-protoss-carried-campaign build/test-sc64-protoss-carried-p3 build/test-sc64-protoss-carried-p5
	./build/test-sc64-protoss-carried-campaign --controller 2 --trace .local/sc64/p3-carried-p1-controller.json
	./build/test-sc64-protoss-carried-campaign --controller 3 --trace .local/sc64/p3-carried-p2-controller.json
	./build/test-sc64-protoss-carried-p3 --trace .local/sc64/p4-carried-p3-controller.json
	./build/test-sc64-protoss-carried-p5 --limit 180000 --attempt-tag canonical-p6-carried-p5
test-protoss: test-sc64-protoss-carried-missions

# The indexed and packed sprite paths must agree over the whole framebuffer,
# including transparent lanes, alignment and both clipping configurations.
SPRITE_WORD_TEST_FLAGS := -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_UNIT_WORD_SPRITES -Icommon
build/test-gfx-sprite-packed: tests/gfx_sprite_words.c common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(SPRITE_WORD_TEST_FLAGS) -DGBA_SCENE_CLIP $< common/gba.c -o $@
build/test-gfx-sprite-packed-no-clip: tests/gfx_sprite_words.c common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(SPRITE_WORD_TEST_FLAGS) $< common/gba.c -o $@
.PHONY: test-gfx-sprite-packed
test-gfx-sprite-packed: build/test-gfx-sprite-packed build/test-gfx-sprite-packed-no-clip
	./build/test-gfx-sprite-packed
	./build/test-gfx-sprite-packed-no-clip
test-starcraft: test-gfx-sprite-packed

.PHONY: test-gfx-unit-facing-clip
build/test-gfx-unit-facing-clip: tests/gfx_unit_facing_clip.c common/gba.c common/gba.h .local/sc64/facing/generated/sc64_unit_facing_generated.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -DGBA_SPRITE_WORDS -DSC64_UNIT_FACING -Icommon -I.local/sc64/facing/generated $< common/gba.c -o $@
test-gfx-unit-facing-clip: build/test-gfx-unit-facing-clip
	./build/test-gfx-unit-facing-clip

.PHONY: test-sc64-unit-facing-importer
test-sc64-unit-facing-importer:
	$(PYTHON) -m unittest discover -s tests -p 'test_sc64_unit_facing.py'


# Optional native weapon-art banks. Nothing in the asset-free default target
# prepares private data; canonical imported ROMs require both versioned banks.
$(WEAPON_SOURCE_HEADER): tools/sc64_weapon_source_sprites.py tools/sc64_weapon_source_audit.py tools/sc64_effect_palette.py tools/sc64_construction_sprites.py tools/sc64_sprites.py common/gba.c common/gba.h starcraft/sc64_display.h .local/sc64/backwater/generated/palette-remap.bin .local/sc64/backwater/generated/palette.json
	$(PYTHON) tools/sc64_weapon_source_sprites.py --out $(WEAPON_SOURCE_DIR)
$(ATTACK_POSE_HEADER): tools/sc64_unit_attack_sprites.py tools/sc64_construction_sprites.py tools/sc64_sprites.py .local/sc64/backwater/generated/palette-remap.bin
	$(PYTHON) tools/sc64_unit_attack_sprites.py --out $(ATTACK_POSE_DIR) --header $(ATTACK_POSE_HEADER)
build/starcraft-terran.elf build/starcraft-zerg-z4.elf build/starcraft-zerg-z6.elf build/starcraft-protoss-p5-shuttle.elf build/starcraft-protoss-p6.elf build/starcraft-showcase.elf: $(SOURCE_WEAPON_INPUTS)
build/test-terran build/test-zerg build/test-zerg-z4 build/test-sc64-zerg-z4-promoted-profile build/test-protoss build/test-sc64-terran-full-profile build/test-sc64-zerg-promoted-profile build/test-sc64-protoss-promoted-profile build/test-sc64-showcase-game build/test-sc64-showcase-game-packed build/test-sc64-showcase-game-indexed build/test-sc64-protoss-skirmish-pylons build/test-sc64-protoss-gas-placement build/test-sc64-worker-cargo-packed build/test-sc64-protoss-warp-color build/test-sc64-protoss-warp-color-no-clip build/test-sc64-terran-full-frame build/test-sc64-terran-full-hud build/test-sc64-z4-frame-coverage build/test-sc64-zerg-canonical-hud build/test-sc64-protoss-canonical-frame build/test-sc64-protoss-canonical-hud: $(SOURCE_WEAPON_INPUTS)

SOURCE_SAMPLER_TEST_FLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined
build/test-sc64-weapon-source-sampling: tests/sc64_weapon_source_vfx_sampling.c starcraft/sc64_weapon_source_vfx.h | build
	$(CLANG) $(SOURCE_SAMPLER_TEST_FLAGS) $< -o $@
build/test-sc64-weapon-source-raster: tests/sc64_weapon_source_vfx_render.c starcraft/sc64_weapon_source_vfx_render.h common/gba.h | build
	$(CLANG) $(SOURCE_SAMPLER_TEST_FLAGS) $< -o $@
build/test-sc64-weapon-source-runtime: tests/sc64_weapon_source_runtime.c starcraft/sc64_weapon_source_runtime.h starcraft/sc64_weapon_source_vfx.h tests/fixtures/weapon_source_mock.h | build
	$(CLANG) $(SOURCE_SAMPLER_TEST_FLAGS) -Itests/fixtures $< -o $@
build/test-sc64-weapon-source-resolved-trace: tests/sc64_weapon_source_resolved_trace.c starcraft/sc64_weapon_source_runtime.h starcraft/sc64_weapon_source_vfx_render.h starcraft/sc64_weapon_source_vfx_bounds.h | build
	$(CLANG) $(SOURCE_SAMPLER_TEST_FLAGS) -Icommon $< -o $@
.PHONY: test-sc64-weapon-source-sampling
build/test-gfx-reverse-bytes: tests/gfx_reverse_bytes.c common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $< common/gba.c -o $@
test-sc64-weapon-source-sampling: build/test-sc64-weapon-source-sampling build/test-sc64-weapon-source-raster build/test-sc64-weapon-source-runtime build/test-sc64-weapon-source-resolved-trace build/test-gfx-reverse-bytes
	./build/test-sc64-weapon-source-sampling
	./build/test-sc64-weapon-source-raster
	./build/test-sc64-weapon-source-runtime
	./build/test-sc64-weapon-source-resolved-trace
	./build/test-gfx-reverse-bytes
test: test-sc64-weapon-source-sampling

# The actual engine emitter/renderer, using only already prepared private art.
build/test-sc64-native-weapon-game: tests/sc64_native_weapon_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(SOURCE_WEAPON_INPUTS) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-native-weapon-game
test-sc64-native-weapon-game: build/test-sc64-native-weapon-game
	./build/test-sc64-native-weapon-game

# Source event/LO/trail timing against the already prepared private bank.
build/test-sc64-weapon-source-generated: tests/sc64_weapon_source_generated_compile.c starcraft/sc64_weapon_source_runtime.h starcraft/sc64_weapon_source_vfx.h starcraft/sc64_weapon_source_vfx_render.h $(WEAPON_SOURCE_HEADER) $(ATTACK_POSE_HEADER) common/gba.h Makefile | build
	$(CLANG) $(SOURCE_SAMPLER_TEST_FLAGS) -I$(WEAPON_SOURCE_DIR) -I$(ATTACK_POSE_DIR) -DSC64_UNIT_ATTACK_POSES_ENABLED $< -o $@
.PHONY: test-sc64-weapon-source-generated
test-sc64-weapon-source-generated: build/test-sc64-weapon-source-generated
	./build/test-sc64-weapon-source-generated

# Asset-free offscreen-cull envelope fixture and Make profile-flag contract.
build/test-sc64-weapon-source-vfx-bounds: tests/sc64_weapon_source_vfx_bounds.c starcraft/sc64_weapon_source_vfx_bounds.h | build
	$(CLANG) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
.PHONY: test-sc64-weapon-source-vfx-bounds test-sc64-vfx-cull-profile-flags test-sc64-vfx-cull
test-sc64-weapon-source-vfx-bounds: build/test-sc64-weapon-source-vfx-bounds
	./build/test-sc64-weapon-source-vfx-bounds
test-sc64-vfx-cull-profile-flags:
	$(PYTHON) tests/test_sc64_vfx_cull_profile_flags.py
test-sc64-vfx-cull: test-sc64-weapon-source-vfx-bounds test-sc64-vfx-cull-profile-flags
test: test-sc64-vfx-cull

# Asset-free test of transient duration caching through real engine functions.
build/test-sc64-trace-duration-cache-game: tests/sc64_trace_duration_cache_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $< common/gba.c -o $@
.PHONY: test-sc64-trace-duration-cache-game
test-sc64-trace-duration-cache-game: build/test-sc64-trace-duration-cache-game
	./build/test-sc64-trace-duration-cache-game
test: test-sc64-trace-duration-cache-game

# Optional qualification against an already-prepared source bank. This target
# never depends on the generated-header rule and skips cleanly when absent.
.PHONY: test-sc64-weapon-source-vfx-cull-bank
test-sc64-weapon-source-vfx-cull-bank: | build
	@if test ! -f "$(WEAPON_SOURCE_HEADER)"; then \
		echo "SKIP: prepared weapon source bank not present at $(WEAPON_SOURCE_HEADER)"; \
	else \
		$(CLANG) -std=c11 -O2 -Wall -Wextra -Werror -I"$(WEAPON_SOURCE_DIR)" tests/sc64_weapon_source_vfx_cull_bank.c -o build/test-sc64-weapon-source-vfx-cull-bank && \
		./build/test-sc64-weapon-source-vfx-cull-bank; \
	fi

# Canonical source death/corpse art; default original-art/unified remains asset-free.
build/starcraft-terran.elf build/starcraft-zerg-z4.elf build/starcraft-zerg-z6.elf build/starcraft-protoss-p5-shuttle.elf build/starcraft-protoss-p6.elf build/starcraft-showcase.elf: $(CANONICAL_DEATH_INPUTS)
build/test-terran build/test-sc64-terran-full-frame build/test-sc64-terran-full-hud build/test-sc64-z4-frame-coverage build/test-sc64-zerg-canonical-hud build/test-sc64-protoss-canonical-frame build/test-sc64-protoss-canonical-hud build/test-sc64-showcase-game build/test-sc64-showcase-game-packed build/test-sc64-showcase-game-indexed build/test-sc64-native-weapon-game build/test-sc64-protoss-skirmish-pylons build/test-sc64-protoss-gas-placement build/test-sc64-worker-cargo-packed build/test-sc64-protoss-warp-color build/test-sc64-protoss-warp-color-no-clip: $(CANONICAL_DEATH_INPUTS)

build/test-sc64-showcase-death-source: tests/sc64_showcase_death_source.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(SOURCE_WEAPON_INPUTS) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) $(DEATH_ART_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-showcase-death-source
test-sc64-showcase-death-source: build/test-sc64-showcase-death-source
	./build/test-sc64-showcase-death-source

# Packed source remains must match indexed pixels, including HUD-edge fallback.
build/test-sc64-death-source-packed: tests/sc64_death_source_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) $(DEATH_ART_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(DEATH_VFX_TEST_FLAGS) $(DEATH_ART_FLAGS) -DSC64_UNIT_WORD_SPRITES -DGBA_SPRITE_WORDS -DGBA_SCENE_CLIP $< -o $@
build/test-sc64-death-source-packed-no-clip: tests/sc64_death_source_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) $(DEATH_ART_HEADER) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(DEATH_VFX_TEST_FLAGS) $(DEATH_ART_FLAGS) -DSC64_UNIT_WORD_SPRITES -DGBA_SPRITE_WORDS $< -o $@
.PHONY: test-sc64-death-source-packed
test-sc64-death-source-packed: build/test-sc64-death-source-packed build/test-sc64-death-source-packed-no-clip
	./build/test-sc64-death-source-packed
	./build/test-sc64-death-source-packed-no-clip

# Full-campaign controller binaries compile the canonical presentation flags.
# Refresh them when their generated art changes as well as when code changes.
$(addprefix build/test-sc64-full-,t1-run t2-run t3-run t3-save t3-resume t4-run t4-save t5-save t5-run t6-run) $(addprefix build/test-sc64-t,7-campaign-run 8-campaign-run 9-campaign-run 10-campaign-run): $(CANONICAL_DEATH_INPUTS) $(SOURCE_WEAPON_INPUTS)

# Exercise mid-match SRAM continuation in the actual canonical race runtimes,
# rather than relying on the earlier Z3/P3 split-profile save fixtures.
CANONICAL_SKIRMISH_RESUME_ART_INPUTS = $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS)
CANONICAL_SKIRMISH_RESUME_HOST_FLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon
define CANONICAL_SKIRMISH_RESUME_PROFILE
build/test-sc64-skirmish-resume-$(1): tests/sc64_skirmish_resume.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $(3) $(if $(filter protoss,$(1)),$(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES)) $$(CANONICAL_SKIRMISH_RESUME_ART_INPUTS) common/gba.c common/gba.h Makefile | build
	$$(CLANG) $$(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $(2) $$< common/gba.c $(if $(filter protoss,$(1)),$(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES)) -o $$@
endef
$(eval $(call CANONICAL_SKIRMISH_RESUME_PROFILE,terran,$(TERRAN_ROM_FLAGS),$(TERRAN_FULL_INPUTS)))
$(eval $(call CANONICAL_SKIRMISH_RESUME_PROFILE,zerg,$(ZERG_ROM_FLAGS),$(ZERG_Z6_INPUTS)))
$(eval $(call CANONICAL_SKIRMISH_RESUME_PROFILE,protoss,$(PROTOSS_ROM_FLAGS),$(PROTOSS_CANONICAL_INPUTS)))
.PHONY: test-sc64-canonical-skirmish-resume
test-sc64-canonical-skirmish-resume: $(addprefix build/test-sc64-skirmish-resume-,terran zerg protoss)
	./build/test-sc64-skirmish-resume-terran
	./build/test-sc64-skirmish-resume-zerg
	./build/test-sc64-skirmish-resume-protoss
test-race-campaigns: test-sc64-canonical-skirmish-resume

# Public actor facts only: no generated maps, art, or ROM required.
COLLISION_BOUNDS_TEST_FLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -fno-omit-frame-pointer -Istarcraft -DGBA_HOST -DSC64_RACES -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_Z5_ACTORS -DSC64_Z6_ACTORS -DSC64_TERRAN_LATE_ACTORS -DSC64_P5_ACTORS -DSC64_P6_ACTORS -DSC64_P7_ACTORS -DSC64_P6_COMBAT_ENABLED -DSC64_NYDUS_ENABLED
define COLLISION_BOUNDS_TEST
build/test-sc64-collision-bounds-$(1): tests/sc64_collision_bounds.c $$(wildcard starcraft/*.h) Makefile | build
	$$(CLANG) $$(COLLISION_BOUNDS_TEST_FLAGS) $(2) $$< -o $$@
endef
$(eval $(call COLLISION_BOUNDS_TEST,cached-list,-DSC64_Z7_RUNTIME -DSC64_COLLISION_CLASS_CACHE -DSC64_COLLISION_STEER_LIST))
$(eval $(call COLLISION_BOUNDS_TEST,cached,-DSC64_Z7_RUNTIME -DSC64_COLLISION_CLASS_CACHE))
$(eval $(call COLLISION_BOUNDS_TEST,packed-list,-DSC64_Z7_RUNTIME -DSC64_COLLISION_CLASS_CACHE -DSC64_COLLISION_STEER_LIST -DSC64_COLLISION_PACKED_HIT))
$(eval $(call COLLISION_BOUNDS_TEST,packed,-DSC64_Z7_RUNTIME -DSC64_COLLISION_CLASS_CACHE -DSC64_COLLISION_PACKED_HIT))
$(eval $(call COLLISION_BOUNDS_TEST,hoisted,-DSC64_COLLISION_FACT_HOIST))
$(eval $(call COLLISION_BOUNDS_TEST,plain,))
.PHONY: test-sc64-collision-bounds
test-sc64-collision-bounds: $(addprefix build/test-sc64-collision-bounds-,cached-list cached packed-list packed hoisted plain)
	./build/test-sc64-collision-bounds-cached-list
	./build/test-sc64-collision-bounds-cached
	./build/test-sc64-collision-bounds-packed-list
	./build/test-sc64-collision-bounds-packed
	./build/test-sc64-collision-bounds-hoisted
	./build/test-sc64-collision-bounds-plain
test: test-sc64-collision-bounds test-sc64-contact-bucket-birth

# A post-build spawn's first move must not unlink an unrelated live bucket.
build/test-sc64-contact-bucket-birth: tests/sc64_contact_bucket_birth.c starcraft/sc64_collision_legacy.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST $< -o $@
.PHONY: test-sc64-contact-bucket-birth
test-sc64-contact-bucket-birth: build/test-sc64-contact-bucket-birth
	./build/test-sc64-contact-bucket-birth

# SC64_CONTACT_CACHE differential checks: the cached-fact bucket query, the
# one-pass blocked-steering mask and the stock-rate cache against the original
# query, unit_step_clear() and sc64_stock_motion_step(). Each variant needs its
# profile's private generated inputs, so none of them is part of `make test`.
CONTACT_CACHE_TEST_FLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon
build/test-sc64-contact-cache-races: tests/sc64_contact_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(CONTACT_CACHE_TEST_FLAGS) $(RACES_FLAGS) -DSC64_CONTACT_CACHE $< common/gba.c -o $@
build/test-sc64-contact-cache-p7: tests/sc64_contact_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_BATTERY_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(CONTACT_CACHE_TEST_FLAGS) $(P7_MOVE_BASE_FLAGS) $(P7_CONTACT_FLAGS) $< common/gba.c -o $@
build/test-sc64-contact-cache-terran: tests/sc64_contact_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(CANONICAL_DEATH_INPUTS) $(SOURCE_WEAPON_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(CONTACT_CACHE_TEST_FLAGS) $(TERRAN_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-contact-cache-nydus: tests/sc64_contact_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(Z7_PROBE_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(CONTACT_CACHE_TEST_FLAGS) $(NYDUS_PROBE_FLAGS) -DSC64_CONTACT_CACHE $< common/gba.c -o $@
# The canonical Zerg and Protoss vectors, including their ROM placement flags.
build/test-sc64-contact-cache-zerg: tests/sc64_contact_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z6_INPUTS) $(UNIT_FACING_HEADER) $(TANK_SOURCE_HEADER) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(CONTACT_CACHE_TEST_FLAGS) $(ZERG_ROM_FLAGS) $< common/gba.c -o $@
build/test-sc64-contact-cache-protoss: tests/sc64_contact_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) $(CONTACT_CACHE_TEST_FLAGS) $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-contact-cache test-sc64-contact-cache-races test-sc64-contact-cache-p7 test-sc64-contact-cache-terran test-sc64-contact-cache-nydus test-sc64-contact-cache-zerg test-sc64-contact-cache-protoss
test-sc64-contact-cache-zerg: build/test-sc64-contact-cache-zerg
	./build/test-sc64-contact-cache-zerg
test-sc64-contact-cache-protoss: build/test-sc64-contact-cache-protoss
	./build/test-sc64-contact-cache-protoss
test-sc64-contact-cache-races: build/test-sc64-contact-cache-races
	./build/test-sc64-contact-cache-races
test-sc64-contact-cache-p7: build/test-sc64-contact-cache-p7
	./build/test-sc64-contact-cache-p7
test-sc64-contact-cache-terran: build/test-sc64-contact-cache-terran
	./build/test-sc64-contact-cache-terran
test-sc64-contact-cache-nydus: build/test-sc64-contact-cache-nydus
	./build/test-sc64-contact-cache-nydus
test-sc64-contact-cache: test-sc64-contact-cache-races test-sc64-contact-cache-p7 test-sc64-contact-cache-terran test-sc64-contact-cache-nydus test-sc64-contact-cache-zerg test-sc64-contact-cache-protoss
test-unified: test-sc64-contact-cache-races
test-sc64-terran-full-campaign: test-sc64-contact-cache-terran
test-sc64-p7-contact: test-sc64-contact-cache-p7

# The optional CONTACT mask geometry kernel is generated authored math, not a
# ROM-derived asset. This target has no imported/native input prerequisites.
CONTACT_MASK_TABLE_HEADER = build/generated/sc64_contact_mask_table_generated.h
$(CONTACT_MASK_TABLE_HEADER): tools/sc64_contact_mask_table_gen.py | build
	$(PYTHON) $< --out $@
build/test-sc64-contact-mask-table: tests/sc64_contact_mask_table_generated.c $(CONTACT_MASK_TABLE_HEADER) Makefile | build
	$(CLANG) -std=c11 -O2 -Wall -Wextra -Werror -Ibuild/generated $< -o $@
.PHONY: test-sc64-contact-mask-table
test-sc64-contact-mask-table: build/test-sc64-contact-mask-table
	./build/test-sc64-contact-mask-table
test-zerg: test-sc64-contact-cache-zerg
test-protoss: test-sc64-contact-cache-protoss

# Optional P7 ordinary-input performance replay; excluded from default targets.
# Requires the same authenticated generated P7 data inputs as other P7 probes.
P7_NORMAL_INPUT_REPLAY := tests/sc64_p7_normal_input_replay.c
P7_NORMAL_INPUT_COMMON := common/start.s common/gba.c common/memory.c common/audio_bank.S

build/test-sc64-p7-normal-input-replay: $(P7_NORMAL_INPUT_REPLAY) tests/sc64_p7_normal_input_tape.h $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_INTERCEPTOR_FLIGHT_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-p7-normal-input-replay
test-sc64-p7-normal-input-replay: build/test-sc64-p7-normal-input-replay
	./build/test-sc64-p7-normal-input-replay

build/sc64-p7-normal-input-replay.elf: $(P7_NORMAL_INPUT_REPLAY) tests/sc64_p7_normal_input_tape.h $(P7_NORMAL_INPUT_COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_INTERCEPTOR_FLIGHT_FLAGS) $(P7_NORMAL_INPUT_COMMON) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/sc64-p7-normal-input-replay.map -lgcc -o $@

# Optional sight-radius axis-cull qualification targets; no default profile changes.
P7_SIGHT_AXIS_FLAGS := $(P7_INTERCEPTOR_FLIGHT_FLAGS) -DSC64_P7_SIGHT_AXIS_CULL
P7_SIGHT_AXIS_BOUNDS_TEST := tests/sc64_p7_sight_axis_cull.c

build/test-sc64-p7-sight-axis-bounds: $(P7_SIGHT_AXIS_BOUNDS_TEST) $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_SIGHT_AXIS_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-p7-sight-axis-bounds
test-sc64-p7-sight-axis-bounds: build/test-sc64-p7-sight-axis-bounds
	./build/test-sc64-p7-sight-axis-bounds

# Ordinary-input replay with the candidate enabled; remains opt-in.
build/test-sc64-p7-sight-axis-normal-input: $(P7_NORMAL_INPUT_REPLAY) tests/sc64_p7_normal_input_tape.h $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_SIGHT_AXIS_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-p7-sight-axis-normal-input
test-sc64-p7-sight-axis-normal-input: build/test-sc64-p7-sight-axis-normal-input
	./build/test-sc64-p7-sight-axis-normal-input

# Internal probe only; the canonical P7 targets stay unchanged.
build/starcraft-protoss-p7-sight-axis-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_SIGHT_AXIS_FLAGS) $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-sight-axis-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Sight-Axis-Probe-GBA.gba: build/starcraft-protoss-p7-sight-axis-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 AXIS' S7AX
.PHONY: protoss-p7-sight-axis-probe
protoss-p7-sight-axis-probe: build/StarCraft-Protoss-P7-Sight-Axis-Probe-GBA.gba

build/sc64-p7-sight-axis-normal-input.elf: $(P7_NORMAL_INPUT_REPLAY) tests/sc64_p7_normal_input_tape.h $(P7_NORMAL_INPUT_COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_SIGHT_AXIS_FLAGS) $(P7_NORMAL_INPUT_COMMON) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/sc64-p7-sight-axis-normal-input.map -lgcc -o $@

# Optional full P7 ordinary-input controller qualification; excluded from default targets.
P7_CAMPAIGN_CONTROLLER := tests/sc64_p7_campaign_run.c
P7_CAMPAIGN_TRACE := build/sc64-p7-campaign-controller.jsonl

build/test-sc64-p7-campaign-controller: $(P7_CAMPAIGN_CONTROLLER) $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_SIGHT_AXIS_FLAGS) -DP7_CONTROLLER_PROFILE=\"P7_SIGHT_AXIS_FLAGS\" $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-p7-campaign-controller
test-sc64-p7-campaign-controller: build/test-sc64-p7-campaign-controller tools/sc64_p7_campaign_trace.py
	./build/test-sc64-p7-campaign-controller $(P7_CAMPAIGN_TRACE)
	$(PYTHON) tools/sc64_p7_campaign_trace.py --require-win $(P7_CAMPAIGN_TRACE)

# Opt-in full P7 performance profile; canonical campaigns remain unchanged.
# The contact cache supersedes the older per-slot class/steering caches.
# V2 sight/rectangle facts use a separate path from existing V1 headers.
P7_FACT_V2_DIR := build/generated/p7-fact-v2
P7_FACT_V2_HEADER := $(P7_FACT_V2_DIR)/sc64_fact_tables_generated.h
P7_FACT_V2_FEATURES := -DSC64_FACT_SIGHT_TABLES -DSC64_P7_RECT_TABLES
P7_FACT_V2_FLAGS := $(filter-out -DSC64_COLLISION_CLASS_CACHE -DSC64_COLLISION_STEER_LIST,$(P7_SIGHT_AXIS_FLAGS)) -DSC64_CONTACT_CACHE $(P7_FACT_V2_FEATURES) -DSC64_FACT_TABLES -I$(P7_FACT_V2_DIR)
$(P7_FACT_V2_HEADER): tools/sc64_fact_tables_gen.c $(wildcard starcraft/*.h) $(COMMON_HEADERS) Makefile | build
	mkdir -p $(P7_FACT_V2_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(filter-out -DSC64_FACT_TABLES,$(P7_FACT_V2_FLAGS)) $< -o build/sc64-p7-fact-v2-gen
	./build/sc64-p7-fact-v2-gen > $@.tmp
	mv $@.tmp $@
.PHONY: test-sc64-fact-v2 test-sc64-p7-fact-v2-normal-input
test-sc64-fact-v2:
	$(PYTHON) -m unittest discover -s tests -p 'test_sc64_fact_v2.py'
	$(PYTHON) -m unittest discover -s tests -p 'test_sc64_p7_sight_runtime.py'
build/test-sc64-p7-fact-v2-normal-input: $(P7_NORMAL_INPUT_REPLAY) tests/sc64_p7_normal_input_tape.h $(P7_FACT_V2_HEADER) $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_FACT_V2_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
test-sc64-p7-fact-v2-normal-input: build/test-sc64-p7-fact-v2-normal-input
	./build/test-sc64-p7-fact-v2-normal-input
build/sc64-p7-fact-v2-normal-input.elf: $(P7_NORMAL_INPUT_REPLAY) tests/sc64_p7_normal_input_tape.h $(P7_FACT_V2_HEADER) $(P7_NORMAL_INPUT_COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_FACT_V2_FLAGS) $(P7_NORMAL_INPUT_COMMON) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/sc64-p7-fact-v2-normal-input.map -lgcc -o $@

build/starcraft-protoss-p7-fact-v2-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(P7_FACT_V2_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_FACT_V2_FLAGS) $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-fact-v2-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Fact-V2-Probe-GBA.gba: build/starcraft-protoss-p7-fact-v2-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 FACTV2' S7V2
.PHONY: protoss-p7-fact-v2-probe
protoss-p7-fact-v2-probe: build/StarCraft-Protoss-P7-Fact-V2-Probe-GBA.gba

P7_FACT_V2_CAMPAIGN_TRACE := build/sc64-p7-fact-v2-campaign-controller.jsonl
build/test-sc64-p7-fact-v2-campaign-controller: $(P7_CAMPAIGN_CONTROLLER) $(P7_FACT_V2_HEADER) $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_FACT_V2_FLAGS) -DP7_CONTROLLER_PROFILE=\"P7_FACT_V2_FLAGS\" $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-p7-fact-v2-campaign-controller
test-sc64-p7-fact-v2-campaign-controller: build/test-sc64-p7-fact-v2-campaign-controller tools/sc64_p7_campaign_trace.py
	./build/test-sc64-p7-fact-v2-campaign-controller $(P7_FACT_V2_CAMPAIGN_TRACE)
	$(PYTHON) tools/sc64_p7_campaign_trace.py --require-win $(P7_FACT_V2_CAMPAIGN_TRACE)

# Full P7 cache profile with the qualified mode-4 drawing paths. Keep the
# earlier FACT V2 flags and products available for exact replay comparisons.
P7_RENDER_FLAGS := $(P7_FACT_V2_FLAGS) $(CANONICAL_DRAW_FLAGS) -DGBA_WINDOW_BANDS $(CANONICAL_STATIC_FLAGS) $(CANONICAL_MINIMAP_FLAGS) -DSC64_UNIT_WORD_SPRITES -DSC64_P7_ARBITER_AURA_ONCE
P7_RENDER_FRAME_TEST := tests/sc64_p7_tile_render.c
define P7_RENDER_FRAME_FIXTURE
build/test-sc64-p7-render-$(1): $$(P7_RENDER_FRAME_TEST) $$(P7_FACT_V2_HEADER) $$(STARCRAFT) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) $$(wildcard starcraft/*.h) $$(P7_ARBITER_INPUTS) common/gba.c $$(COMMON_HEADERS) Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $(2) $$< common/gba.c $$(P7_PROJECTILE_SOURCES) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $$@
endef
$(eval $(call P7_RENDER_FRAME_FIXTURE,reference,$(P7_FACT_V2_FLAGS)))
$(eval $(call P7_RENDER_FRAME_FIXTURE,candidate,$(P7_RENDER_FLAGS)))
.PHONY: test-sc64-p7-render
test-sc64-p7-render: build/test-sc64-p7-render-reference build/test-sc64-p7-render-candidate
	./build/test-sc64-p7-render-reference > build/sc64-p7-render-reference.txt
	./build/test-sc64-p7-render-candidate > build/sc64-p7-render-candidate.txt
	cmp build/sc64-p7-render-reference.txt build/sc64-p7-render-candidate.txt

build/sc64-p7-render-normal-input.elf: $(P7_NORMAL_INPUT_REPLAY) tests/sc64_p7_normal_input_tape.h $(P7_FACT_V2_HEADER) $(P7_NORMAL_INPUT_COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_RENDER_FLAGS) $(P7_NORMAL_INPUT_COMMON) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/sc64-p7-render-normal-input.map -lgcc -o $@
build/starcraft-protoss-p7-render-probe.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(P7_FACT_V2_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(P7_RENDER_FLAGS) $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p7-render-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P7-Render-Probe-GBA.gba: build/starcraft-protoss-p7-render-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC P7 DRAW' S7DR
.PHONY: protoss-p7-render-probe
protoss-p7-render-probe: build/StarCraft-Protoss-P7-Render-Probe-GBA.gba

P7_RENDER_CAMPAIGN_TRACE := build/sc64-p7-render-campaign-controller.jsonl
build/test-sc64-p7-render-campaign-controller: $(P7_CAMPAIGN_CONTROLLER) $(P7_FACT_V2_HEADER) $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_RENDER_FLAGS) -DP7_CONTROLLER_PROFILE=\"P7_RENDER_FLAGS\" $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
.PHONY: test-sc64-p7-render-campaign-controller
test-sc64-p7-render-campaign-controller: build/test-sc64-p7-render-campaign-controller tools/sc64_p7_campaign_trace.py
	./build/test-sc64-p7-render-campaign-controller $(P7_RENDER_CAMPAIGN_TRACE)
	$(PYTHON) tools/sc64_p7_campaign_trace.py --require-win $(P7_RENDER_CAMPAIGN_TRACE)

# These canonical targets are declared before the optional P7 inputs above are
# assigned. Add their effective prerequisites here so clean builds generate
# the assets they consume and rebuild when those inputs change.
build/starcraft-protoss-p7.elf \
build/test-sc64-protoss-canonical-frame \
build/test-sc64-protoss-canonical-hud \
build/test-sc64-geyser-art \
build/test-protoss \
build/test-sc64-protoss-promoted-profile \
build/test-sc64-protoss-canonical-storm \
build/test-sc64-protoss-canonical-storage \
build/test-sc64-protoss-canonical-controller \
build/test-sc64-p4-census \
build/test-sc64-protoss-canonical-p7-controller \
build/test-sc64-protoss-canonical-p7-save \
build/test-sc64-protoss-warp-color \
build/test-sc64-protoss-warp-color-no-clip \
build/test-sc64-worker-cargo-packed \
build/test-sc64-p2-reinforcements-canonical \
build/test-sc64-protoss-carried-campaign \
build/test-sc64-protoss-carried-p3 \
build/test-sc64-protoss-carried-p5 \
build/test-sc64-skirmish-resume-protoss \
build/test-sc64-contact-cache-protoss: $(PROTOSS_P7_CANONICAL_INPUTS) $(P7_ARBITER_INPUTS) $(P7_FACT_V2_HEADER) $(P7_INTERCEPTOR_FLIGHT_SOURCES)

# Prepared-input regression checks for race construction travel and SRAM.
CONSTRUCTION_ARRIVAL_FLAGS_terran = $(TERRAN_ROM_FLAGS)
CONSTRUCTION_ARRIVAL_FLAGS_zerg-z6 = $(ZERG_ROM_FLAGS)
CONSTRUCTION_ARRIVAL_FLAGS_protoss-p7 = $(PROTOSS_ROM_FLAGS)
CONSTRUCTION_ARRIVAL_FLAGS_zerg-z7 = $(ZERG_Z7_RELEASE_FLAGS)
CONSTRUCTION_ARRIVAL_INPUTS_terran = $(TERRAN_FULL_INPUTS)
CONSTRUCTION_ARRIVAL_INPUTS_zerg-z6 = $(ZERG_Z6_INPUTS)
CONSTRUCTION_ARRIVAL_INPUTS_protoss-p7 = $(PROTOSS_P7_CANONICAL_INPUTS)
CONSTRUCTION_ARRIVAL_INPUTS_zerg-z7 = $(ZERG_Z7_CANONICAL_INPUTS)
CONSTRUCTION_ARRIVAL_SOURCES_protoss-p7 = $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES)

define CONSTRUCTION_ARRIVAL_TESTS
build/test-sc64-construction-arrival-$(1): tests/sc64_construction_arrival_game.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(CONSTRUCTION_ARRIVAL_INPUTS_$(1)) common/gba.c common/gba.h Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(CONSTRUCTION_ARRIVAL_FLAGS_$(1)) $$< common/gba.c $$(CONSTRUCTION_ARRIVAL_SOURCES_$(1)) -o $$@
build/test-sc64-construction-arrival-save-$(1): tests/sc64_construction_arrival_save_game.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(CONSTRUCTION_ARRIVAL_INPUTS_$(1)) common/gba.c common/gba.h Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(CONSTRUCTION_ARRIVAL_FLAGS_$(1)) $$< common/gba.c $$(CONSTRUCTION_ARRIVAL_SOURCES_$(1)) -o $$@
.PHONY: test-sc64-construction-arrival-$(1)
test-sc64-construction-arrival-$(1): build/test-sc64-construction-arrival-$(1) build/test-sc64-construction-arrival-save-$(1)
	./build/test-sc64-construction-arrival-$(1)
	./build/test-sc64-construction-arrival-save-$(1)
endef
$(foreach profile,terran zerg-z6 protoss-p7 zerg-z7,$(eval $(call CONSTRUCTION_ARRIVAL_TESTS,$(profile))))

# No-GDB ARM probe of the imported interface: an embedded key tape drives the
# menus, briefing, orders, pause, save and load; results go to the mGBA debug
# log (tests/sc64_ui_arm_probe.c). Run twice with one isolated save directory.
UI_ARM_COMMON := $(filter-out common/main.c,$(COMMON))
build/starcraft-showcase-ui-probe.elf: tests/sc64_ui_arm_probe.c $(UI_ARM_COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_HEADER) $(COMMAND_ICON_HEADER) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(SHOWCASE_FLAGS) $(UI_ARM_COMMON) $< $(filter-out starcraft/game.c,$(STARCRAFT)) $(LDFLAGS) -Wl,-Map,build/starcraft-showcase-ui-probe.map -lgcc -o $@
	$(SIZE) $@
build/starcraft64-races-ui-probe.elf: tests/sc64_ui_arm_probe.c $(UI_ARM_COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(RACES_FLAGS) $(UI_ARM_COMMON) $< $(filter-out starcraft/game.c,$(STARCRAFT)) $(LDFLAGS) -Wl,-Map,build/starcraft64-races-ui-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Showcase-UI-Probe-GBA.gba: build/starcraft-showcase-ui-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC UI PROBE' SCUP
build/StarCraft64-Races-UI-Probe-GBA.gba: build/starcraft64-races-ui-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC UI PROBE' SCUR
.PHONY: ui-arm-probe
ui-arm-probe: build/StarCraft-Showcase-UI-Probe-GBA.gba build/StarCraft64-Races-UI-Probe-GBA.gba

.PHONY: test-sc64-direct-firing-flags
test-sc64-direct-firing-flags:
	$(PYTHON) tests/test_sc64_direct_firing_flags.py

# Optional actual-engine regression for the staged, corridor-only fix. The
# fixture needs already prepared imported-race headers and invokes no assets.
define DIRECT_FIRING_GAME_FIXTURE
build/test-sc64-direct-firing-points-$(1): tests/sc64_direct_firing_points_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.h common/gba.c Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $$(RACES_FLAGS) $(2) $$< common/gba.c -o $$@
endef
$(eval $(call DIRECT_FIRING_GAME_FIXTURE,off,))
$(eval $(call DIRECT_FIRING_GAME_FIXTURE,on,-DSC64_DIRECT_FIRING_POINTS))
.PHONY: test-sc64-direct-firing-points-game
test-sc64-direct-firing-points-game: test-sc64-direct-firing-flags build/test-sc64-direct-firing-points-off build/test-sc64-direct-firing-points-on
	./build/test-sc64-direct-firing-points-off
	./build/test-sc64-direct-firing-points-on
