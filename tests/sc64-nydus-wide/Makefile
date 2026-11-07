# Optional prepared-input composition check. No asset preparation is implied.
NYDUS_WIDE_OUT ?= build/nydus-wide
NYDUS_WIDE_COVER_DIR ?= .local/hardware-feedback/verified-cover
.PHONY: test-sc64-nydus-wide
test-sc64-nydus-wide:
	@mkdir -p $(NYDUS_WIDE_OUT)
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -I$(NYDUS_WIDE_COVER_DIR) \
	  $(BC_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) tests/sc64_nydus_wide_save.c tests/sc64_yamato_host_sram_shim.c \
	  $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(NYDUS_WIDE_OUT)/test
	$(NYDUS_WIDE_OUT)/test write $(NYDUS_WIDE_OUT)/wide.sav
	$(NYDUS_WIDE_OUT)/test read $(NYDUS_WIDE_OUT)/wide.sav

# Supply an unchanged earlier checkout's game.c after running the target above.
# This producer retains the earlier ENERGY/YAMATO-only BC closure; it writes
# its own journal through the old implementation, without retagging a record.
NYDUS_WIDE_LEGACY_SOURCE ?=
NYDUS_WIDE_LEGACY_BC_FLAGS = $(filter-out -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_PRODUCTION_EXPERIMENT -DSC64_BATTLECRUISER_FLIGHT_EXPERIMENT -DSC64_BATTLECRUISER_RESEARCH_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT,$(BC_FLAGS))
.PHONY: test-sc64-nydus-wide-predecessor
test-sc64-nydus-wide-predecessor:
	@test -n "$(NYDUS_WIDE_LEGACY_SOURCE)" && test -f "$(NYDUS_WIDE_LEGACY_SOURCE)"
	@test -x $(NYDUS_WIDE_OUT)/test
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -I$(NYDUS_WIDE_COVER_DIR) \
	  $(NYDUS_WIDE_LEGACY_BC_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) -DSC64_NYDUS_WIDE_LEGACY_WRITER \
	  '-DSC64_NYDUS_GAME_SOURCE="$(NYDUS_WIDE_LEGACY_SOURCE)"' tests/sc64_nydus_wide_save.c tests/sc64_yamato_host_sram_shim.c \
	  $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -Wno-unused-parameter -o $(NYDUS_WIDE_OUT)/legacy-writer
	$(NYDUS_WIDE_OUT)/legacy-writer write $(NYDUS_WIDE_OUT)/legacy.sav legacy
	$(NYDUS_WIDE_OUT)/test read $(NYDUS_WIDE_OUT)/legacy.sav legacy
