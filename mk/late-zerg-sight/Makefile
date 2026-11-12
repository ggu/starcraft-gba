# SPDX-License-Identifier: GPL-3.0-or-later
# Shared by the late Zerg cartridge, controller, replay and test vectors.
LATE_ZERG_PACKED_SIGHT ?= 0
ifneq ($(words $(filter 0 1,$(LATE_ZERG_PACKED_SIGHT))),1)
$(error LATE_ZERG_PACKED_SIGHT must be 0 or 1)
endif
ifneq ($(filter 0 1,$(LATE_ZERG_PACKED_SIGHT)),$(LATE_ZERG_PACKED_SIGHT))
$(error LATE_ZERG_PACKED_SIGHT must be 0 or 1)
endif
LATE_ZERG_PACKED_SIGHT_FLAGS = $(if $(filter 1,$(LATE_ZERG_PACKED_SIGHT)),-DSC64_Z7_VISIBILITY_PACKED_ROWS)

SC64_SIGHT_ROW_LUT_HEADER = build/generated/sc64_sight_row_lut_generated.h
$(SC64_SIGHT_ROW_LUT_HEADER): tools/sc64_sight_row_lut.py | build
	$(PYTHON) $< $@

LATE_ZERG_SIGHT_STAMP := build/late-zerg-sight-config
LATE_ZERG_SIGHT_FORCE := $(if $(shell printf '%s\n' '$(LATE_ZERG_PACKED_SIGHT)' | cmp -s - $(LATE_ZERG_SIGHT_STAMP) && printf matched),,force-late-zerg-sight-rebuild)
LATE_ZERG_SIGHT_INPUTS = mk/late-zerg-sight/Makefile $(LATE_ZERG_SIGHT_STAMP) $(LATE_ZERG_SIGHT_FORCE) $(if $(LATE_ZERG_PACKED_SIGHT_FLAGS),$(SC64_SIGHT_ROW_LUT_HEADER))
.PHONY: force-late-zerg-sight-config force-late-zerg-sight-rebuild
force-late-zerg-sight-config:
force-late-zerg-sight-rebuild:
$(LATE_ZERG_SIGHT_STAMP): force-late-zerg-sight-config | build
	@printf '%s\n' '$(LATE_ZERG_PACKED_SIGHT)' > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@
