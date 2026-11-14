# SPDX-License-Identifier: GPL-3.0-or-later
# The current Zerg and Protoss cartridges carry all four campaigns that use
# the shared Command provider. Retired standalone recipes keep their old flags.
CANONICAL_COMMAND_CENSUS ?= 1
ifneq ($(words $(CANONICAL_COMMAND_CENSUS)),1)
$(error CANONICAL_COMMAND_CENSUS must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_COMMAND_CENSUS)),)
$(error CANONICAL_COMMAND_CENSUS must be 0 or 1)
endif
CENSUS_FLAGS = $(if $(filter 1,$(CANONICAL_COMMAND_CENSUS)),-DSC64_Z8_DIRECT_COMMAND_CENSUS)
CENSUS_KEY = $(if $(CENSUS_FLAGS),-census)
