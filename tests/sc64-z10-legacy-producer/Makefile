# SPDX-License-Identifier: GPL-3.0-or-later
# Caller supplies an existing, verified normal Z10 source checkout and inputs.
Z10_LEGACY_SOURCE ?=
Z10_LEGACY_COVER_DIR ?= .local/hardware-feedback/verified-cover
Z10_LEGACY_OUT ?= build/z10-legacy-producer
.PHONY: test-z10-normal-legacy-producer
test-z10-normal-legacy-producer:
	@test -n "$(Z10_LEGACY_SOURCE)"
	@mkdir -p $(Z10_LEGACY_OUT)
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable \
	  -I$(Z10_LEGACY_COVER_DIR) $(ZERG_Z10_CANONICAL_FLAGS) \
	  '-DSC64_Z10_OLD_GBA="$(Z10_LEGACY_SOURCE)/common/gba.c"' \
	  '-DSC64_Z10_OLD_GAME="$(Z10_LEGACY_SOURCE)/starcraft/game.c"' \
	  tests/sc64_z10_normal_legacy_producer.c \
	  $(addprefix $(Z10_LEGACY_SOURCE)/,$(filter-out starcraft/game.c,$(STARCRAFT))) \
	  $(addprefix $(Z10_LEGACY_SOURCE)/,$(P7_INTERCEPTOR_FLIGHT_SOURCES)) -o $(Z10_LEGACY_OUT)/producer
	$(Z10_LEGACY_OUT)/producer write $(Z10_LEGACY_OUT)/z10-normal.sav
	$(Z10_LEGACY_OUT)/producer read $(Z10_LEGACY_OUT)/z10-normal.sav
