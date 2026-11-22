# SPDX-License-Identifier: GPL-3.0-or-later
# The current Zerg and Protoss cartridges carry all four campaigns that use
# the shared Command provider. Retired standalone recipes keep their old flags.
CANONICAL_COMMAND_CENSUS ?= 1
CANONICAL_COMMAND_VALIDATION ?= 1
ifneq ($(words $(CANONICAL_COMMAND_CENSUS)),1)
$(error CANONICAL_COMMAND_CENSUS must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_COMMAND_CENSUS)),)
$(error CANONICAL_COMMAND_CENSUS must be 0 or 1)
endif
ifneq ($(words $(CANONICAL_COMMAND_VALIDATION)),1)
$(error CANONICAL_COMMAND_VALIDATION must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_COMMAND_VALIDATION)),)
$(error CANONICAL_COMMAND_VALIDATION must be 0 or 1)
endif
CENSUS_FLAGS = $(if $(filter 1,$(CANONICAL_COMMAND_CENSUS)),-DSC64_Z8_DIRECT_COMMAND_CENSUS $(if $(filter 1,$(CANONICAL_COMMAND_VALIDATION)),-DSC64_COMMAND_RAW_VALIDATION_CACHE))
CENSUS_KEY = $(if $(filter 1,$(CANONICAL_COMMAND_CENSUS)),-census$(if $(filter 1,$(CANONICAL_COMMAND_VALIDATION)),-raw-validation))
