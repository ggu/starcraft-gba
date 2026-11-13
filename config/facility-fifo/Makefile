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
