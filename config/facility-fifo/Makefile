# SPDX-License-Identifier: GPL-3.0-or-later
# Independent opt-in for bounded depot and perimeter floods.
CANONICAL_ROUTE_FACILITY_FIFO ?= 0
ifneq ($(words $(CANONICAL_ROUTE_FACILITY_FIFO)),1)
$(error CANONICAL_ROUTE_FACILITY_FIFO must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_ROUTE_FACILITY_FIFO)),)
$(error CANONICAL_ROUTE_FACILITY_FIFO must be 0 or 1)
endif
CANONICAL_ROUTE_FACILITY_FIFO_FLAGS = $(if $(filter 1,$(CANONICAL_ROUTE_FACILITY_FIFO)),-DSC64_ROUTE_FACILITY_FIFO_PROBE)
FACILITY_FIFO_KEY = $(if $(CANONICAL_ROUTE_FACILITY_FIFO_FLAGS),-facilityfifo)
include config/route-pack/Makefile
include config/command-census/Makefile

# Keep the old indexed FIFO available for measured comparisons.
CANONICAL_ROUTE_POINTER ?= 1
ifneq ($(filter $(CANONICAL_ROUTE_POINTER),0 1),$(CANONICAL_ROUTE_POINTER))
$(error CANONICAL_ROUTE_POINTER must be 0 or 1)
endif
ifneq ($(words $(CANONICAL_ROUTE_POINTER)),1)
$(error CANONICAL_ROUTE_POINTER must be 0 or 1)
endif
LINEAR_POINTER_FLAGS = $(if $(filter 1,$(CANONICAL_ROUTE_POINTER)),-DSC64_ROUTE_LINEAR_POINTER_FIFO_PROBE)
LINEAR_POINTER_KEY = $(if $(LINEAR_POINTER_FLAGS),-ptr)
CANONICAL_ROUTE_LINEAR_FLAGS = $(if $(filter 1,$(CANONICAL_ROUTE_LINEAR)),-DSC64_ROUTE_LINEAR_FLOOD_PROBE $(LINEAR_POINTER_FLAGS))
