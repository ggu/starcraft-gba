# SPDX-License-Identifier: GPL-3.0-or-later
# Compact Unit profiles can reject a coincident actor with one word load.
CANONICAL_CONTACT_COINCIDENT ?= 1
ifneq ($(words $(CANONICAL_CONTACT_COINCIDENT)),1)
$(error CANONICAL_CONTACT_COINCIDENT must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_CONTACT_COINCIDENT)),)
$(error CANONICAL_CONTACT_COINCIDENT must be 0 or 1)
endif
# Preserve row-disjoint memo entries only in explicitly measured builds.
CANONICAL_CONTACT_LOCAL_MEMO ?= 0
ifneq ($(words $(CANONICAL_CONTACT_LOCAL_MEMO)),1)
$(error CANONICAL_CONTACT_LOCAL_MEMO must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_CONTACT_LOCAL_MEMO)),)
$(error CANONICAL_CONTACT_LOCAL_MEMO must be 0 or 1)
endif
CS_FLAGS = $(if $(filter 1,$(CANONICAL_CONTACT_COINCIDENT)),-DSC64_CONTACT_COINCIDENT_PROBE) $(if $(filter 1,$(CANONICAL_CONTACT_LOCAL_MEMO)),-DSC64_CONTACT_LOCAL_MEMO_PROBE)
CS_KEY = $(if $(filter 1,$(CANONICAL_CONTACT_COINCIDENT)),-contactxy)$(if $(filter 1,$(CANONICAL_CONTACT_LOCAL_MEMO)),-contactrows)
