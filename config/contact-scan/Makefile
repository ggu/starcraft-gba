# SPDX-License-Identifier: GPL-3.0-or-later
# Compact Unit profiles can reject a coincident actor with one word load.
CANONICAL_CONTACT_COINCIDENT ?= 1
ifneq ($(words $(CANONICAL_CONTACT_COINCIDENT)),1)
$(error CANONICAL_CONTACT_COINCIDENT must be 0 or 1)
endif
ifeq ($(filter 0 1,$(CANONICAL_CONTACT_COINCIDENT)),)
$(error CANONICAL_CONTACT_COINCIDENT must be 0 or 1)
endif
CS_FLAGS = $(if $(filter 1,$(CANONICAL_CONTACT_COINCIDENT)),-DSC64_CONTACT_COINCIDENT_PROBE)
CS_KEY = $(if $(CS_FLAGS),-contactxy)
