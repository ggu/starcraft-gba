# SPDX-License-Identifier: GPL-3.0-or-later
# Shared by the late Zerg cartridge, controller, replay and test vectors.
# The earlier race products use a different sight-span path.
LATE_ZERG_PACKED_SIGHT ?= 1
LATE_ZERG_SPARSE_SIGHT ?= $(LATE_ZERG_PACKED_SIGHT)
ifneq ($(words $(filter 0 1,$(LATE_ZERG_PACKED_SIGHT))),1)
$(error LATE_ZERG_PACKED_SIGHT must be 0 or 1)
endif
ifneq ($(filter 0 1,$(LATE_ZERG_PACKED_SIGHT)),$(LATE_ZERG_PACKED_SIGHT))
$(error LATE_ZERG_PACKED_SIGHT must be 0 or 1)
endif
ifneq ($(words $(filter 0 1,$(LATE_ZERG_SPARSE_SIGHT))),1)
$(error LATE_ZERG_SPARSE_SIGHT must be 0 or 1)
endif
ifneq ($(filter 0 1,$(LATE_ZERG_SPARSE_SIGHT)),$(LATE_ZERG_SPARSE_SIGHT))
$(error LATE_ZERG_SPARSE_SIGHT must be 0 or 1)
endif
ifeq ($(LATE_ZERG_PACKED_SIGHT):$(LATE_ZERG_SPARSE_SIGHT),0:1)
$(error LATE_ZERG_SPARSE_SIGHT requires LATE_ZERG_PACKED_SIGHT=1)
endif
LATE_ZERG_SPARSE_SIGHT_FLAGS = $(if $(filter 1,$(LATE_ZERG_SPARSE_SIGHT)),-DSC64_Z7_VISIBILITY_SPARSE_ROWS)
LATE_ZERG_PACKED_SIGHT_FLAGS = $(if $(filter 1,$(LATE_ZERG_PACKED_SIGHT)),-DSC64_Z7_VISIBILITY_PACKED_ROWS $(LATE_ZERG_SPARSE_SIGHT_FLAGS))

SC64_SIGHT_ROW_LUT_HEADER = build/generated/sc64_sight_row_lut_generated.h
$(SC64_SIGHT_ROW_LUT_HEADER): tools/sc64_sight_row_lut.py | build
	$(PYTHON) $< $@
SC64_SIGHT_ROW_MASK_HEADER = build/generated/sc64_sight_row_mask_generated.h
$(SC64_SIGHT_ROW_MASK_HEADER): tools/sc64_sight_row_lut.py | build
	$(PYTHON) $< --sparse-mask $@

LATE_ZERG_SIGHT_STAMP := build/late-zerg-sight-config
LATE_ZERG_SIGHT_KEY := $(LATE_ZERG_PACKED_SIGHT):$(LATE_ZERG_SPARSE_SIGHT)
LATE_ZERG_SIGHT_FORCE := $(if $(shell printf '%s\n' '$(LATE_ZERG_SIGHT_KEY)' | cmp -s - $(LATE_ZERG_SIGHT_STAMP) && printf matched),,force-late-zerg-sight-rebuild)
LATE_ZERG_SIGHT_INPUTS = mk/late-zerg-sight/Makefile $(LATE_ZERG_SIGHT_STAMP) $(LATE_ZERG_SIGHT_FORCE) $(if $(LATE_ZERG_PACKED_SIGHT_FLAGS),$(SC64_SIGHT_ROW_LUT_HEADER)) $(if $(LATE_ZERG_SPARSE_SIGHT_FLAGS),$(SC64_SIGHT_ROW_MASK_HEADER))
.PHONY: force-late-zerg-sight-config force-late-zerg-sight-rebuild
force-late-zerg-sight-config:
force-late-zerg-sight-rebuild:
$(LATE_ZERG_SIGHT_STAMP): force-late-zerg-sight-config | build
	@printf '%s\n' '$(LATE_ZERG_SIGHT_KEY)' > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@
