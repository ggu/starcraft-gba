# SPDX-License-Identifier: GPL-3.0-or-later

# Exact opt-in compositor for the linear point and facility route probes.
CANONICAL_ROUTE_LINEAR_WORD_PACK ?= 0
ifneq ($(words $(CANONICAL_ROUTE_LINEAR_WORD_PACK)),1)
$(error CANONICAL_ROUTE_LINEAR_WORD_PACK must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_ROUTE_LINEAR_WORD_PACK)),)
$(error CANONICAL_ROUTE_LINEAR_WORD_PACK must be 0 or 1)
endif
ROUTE_PACK_FLAGS = $(if $(and $(filter 1,$(CANONICAL_ROUTE_LINEAR_WORD_PACK)),$(strip $(CANONICAL_ROUTE_LINEAR_FLAGS) $(CANONICAL_ROUTE_FACILITY_FIFO_FLAGS))),-DSC64_ROUTE_LINEAR_WORD_PACK_PROBE)
LINEAR_WORD_PACK_KEY = $(if $(ROUTE_PACK_FLAGS),-linearwordpack)$(if $(POINT_RETENTION_FLAGS),-pointretention)

# Shares the wide route bank only when explicitly requested. It needs one
# extra physical plane, so keep a low-memory production control available.
CANONICAL_ROUTE_POINT_RETENTION ?= 0
ifneq ($(words $(CANONICAL_ROUTE_POINT_RETENTION)),1)
$(error CANONICAL_ROUTE_POINT_RETENTION must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_ROUTE_POINT_RETENTION)),)
$(error CANONICAL_ROUTE_POINT_RETENTION must be 0 or 1)
endif
POINT_RETENTION_FLAGS = $(if $(filter 1,$(CANONICAL_ROUTE_POINT_RETENTION)),-DSC64_ROUTE_POINT_POINTER_PROBE)
