# SPDX-License-Identifier: GPL-3.0-or-later
# Compact Unit profiles can reject a coincident actor with one word load.
CANONICAL_CONTACT_COINCIDENT ?= 1
ifneq ($(words $(CANONICAL_CONTACT_COINCIDENT)),1)
$(error CANONICAL_CONTACT_COINCIDENT must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_CONTACT_COINCIDENT)),)
$(error CANONICAL_CONTACT_COINCIDENT must be 0 or 1)
endif
# Retain disjoint rows; setting this to zero keeps global invalidation.
CANONICAL_CONTACT_LOCAL_MEMO ?= 1
ifneq ($(words $(CANONICAL_CONTACT_LOCAL_MEMO)),1)
$(error CANONICAL_CONTACT_LOCAL_MEMO must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_CONTACT_LOCAL_MEMO)),)
$(error CANONICAL_CONTACT_LOCAL_MEMO must be 0 or 1)
endif
CS_MEMO_FLAGS = $(if $(filter 1,$(CANONICAL_CONTACT_LOCAL_MEMO)),-DSC64_CONTACT_LOCAL_MEMO_PROBE)
CANONICAL_CONTACT_DEFER_STEERING ?= 1
ifneq ($(words $(CANONICAL_CONTACT_DEFER_STEERING)),1)
$(error CANONICAL_CONTACT_DEFER_STEERING must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_CONTACT_DEFER_STEERING)),)
$(error CANONICAL_CONTACT_DEFER_STEERING must be 0 or 1)
endif
CS_STEER_FLAGS = $(if $(filter 1,$(CANONICAL_CONTACT_DEFER_STEERING)),-DSC64_CONTACT_DEFER_STEERING_MEMO)
# Check the candidate axes against the proven radius bound before reading facts.
CANONICAL_CONTACT_EARLY_DIRECT ?= 1
ifneq ($(words $(CANONICAL_CONTACT_EARLY_DIRECT)),1)
$(error CANONICAL_CONTACT_EARLY_DIRECT must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_CONTACT_EARLY_DIRECT)),)
$(error CANONICAL_CONTACT_EARLY_DIRECT must be 0 or 1)
endif
CS_DIRECT_FLAGS = $(if $(filter 1,$(CANONICAL_CONTACT_EARLY_DIRECT)),-DSC64_CONTACT_EARLY_DIRECT)
CS_FLAGS = $(if $(filter 1,$(CANONICAL_CONTACT_COINCIDENT)),-DSC64_CONTACT_COINCIDENT_PROBE) $(CS_MEMO_FLAGS) $(CS_STEER_FLAGS) $(CS_DIRECT_FLAGS)
CS_KEY = $(if $(filter 1,$(CANONICAL_CONTACT_COINCIDENT)),-contactxy)$(if $(CS_MEMO_FLAGS),-contactrows)$(if $(CS_STEER_FLAGS),-defersteering)$(if $(CS_DIRECT_FLAGS),-contactdirect)
