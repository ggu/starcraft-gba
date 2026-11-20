# SPDX-License-Identifier: GPL-3.0-or-later

# Pack aligned linear point and facility fields with whole-word stores.
CANONICAL_ROUTE_LINEAR_WORD_PACK ?= 1
ifneq ($(words $(CANONICAL_ROUTE_LINEAR_WORD_PACK)),1)
$(error CANONICAL_ROUTE_LINEAR_WORD_PACK must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_ROUTE_LINEAR_WORD_PACK)),)
$(error CANONICAL_ROUTE_LINEAR_WORD_PACK must be 0 or 1)
endif
ROUTE_PACK_FLAGS = $(if $(and $(filter 1,$(CANONICAL_ROUTE_LINEAR_WORD_PACK)),$(strip $(CANONICAL_ROUTE_LINEAR_FLAGS) $(CANONICAL_ROUTE_FACILITY_FIFO_FLAGS))),-DSC64_ROUTE_LINEAR_WORD_PACK_PROBE)
LINEAR_WORD_PACK_KEY = $(if $(ROUTE_PACK_FLAGS),-linearwordpack)$(if $(LINEAR_SPARSE_PACK_FLAGS),-linearsparsepack)$(POINT_RETENTION_KEY)

# Reuse the complete, uncompacted frontier when a small point field can be
# packed by visiting only its discovered cells.
CANONICAL_ROUTE_LINEAR_SPARSE_PACK ?= 1
ifneq ($(words $(CANONICAL_ROUTE_LINEAR_SPARSE_PACK)),1)
$(error CANONICAL_ROUTE_LINEAR_SPARSE_PACK must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_ROUTE_LINEAR_SPARSE_PACK)),)
$(error CANONICAL_ROUTE_LINEAR_SPARSE_PACK must be 0 or 1)
endif
LINEAR_SPARSE_PACK_FLAGS = $(if $(and $(filter 1,$(CANONICAL_ROUTE_LINEAR_SPARSE_PACK)),$(filter 1,$(CANONICAL_ROUTE_LINEAR)),$(filter 1,$(CANONICAL_ROUTE_NEW_CELL))),-DSC64_ROUTE_LINEAR_SPARSE_PACK_PROBE)

# Select an experimental backing only when requested: a dedicated EWRAM
# plane (1), or the borrowed tail of the route/save queue (tail).
CANONICAL_ROUTE_POINT_RETENTION ?= tail
ifneq ($(words $(CANONICAL_ROUTE_POINT_RETENTION)),1)
$(error CANONICAL_ROUTE_POINT_RETENTION must be 0, 1 or tail)
endif
ifeq ($(filter 0 1 tail,$(CANONICAL_ROUTE_POINT_RETENTION)),)
$(error CANONICAL_ROUTE_POINT_RETENTION must be 0, 1 or tail)
endif
POINT_RETENTION_FLAGS = $(if $(filter 1 tail,$(CANONICAL_ROUTE_POINT_RETENTION)),-DSC64_ROUTE_POINT_POINTER_PROBE)$(if $(filter tail,$(CANONICAL_ROUTE_POINT_RETENTION)), -DSC64_ROUTE_POINT_TAIL_PROBE)
POINT_RETENTION_KEY = $(if $(filter 1,$(CANONICAL_ROUTE_POINT_RETENTION)),-pointretention)$(if $(filter tail,$(CANONICAL_ROUTE_POINT_RETENTION)),-pointretentiontail)
