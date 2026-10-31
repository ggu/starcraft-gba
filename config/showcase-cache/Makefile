# SPDX-License-Identifier: GPL-3.0-or-later
# Showcase uses the same page-band and minimap capabilities as the three
# campaign products. Set to 0 for the original EWRAM-band comparison.
SHOWCASE_HUD_CACHE ?= 1
ifneq ($(words $(filter 0 1,$(SHOWCASE_HUD_CACHE))),1)
$(error SHOWCASE_HUD_CACHE must be 0 or 1)
endif
ifneq ($(filter $(SHOWCASE_HUD_CACHE),0 1),$(SHOWCASE_HUD_CACHE))
$(error SHOWCASE_HUD_CACHE must be 0 or 1)
endif
SHOWCASE_HUD_CACHE_FLAGS = $(if $(filter 1,$(SHOWCASE_HUD_CACHE)),-DSC64_HUD_BUFFER_REUSE -DSC64_HUD_VRAM_BANDS -DSC64_MINIMAP_RASTER)
# Optional Showcase collision comparison; the contact cache replaces the
# older class-cache and per-direction steering-list implementations.
SHOWCASE_CONTACT_CACHE ?= 1
ifneq ($(words $(filter 0 1,$(SHOWCASE_CONTACT_CACHE))),1)
$(error SHOWCASE_CONTACT_CACHE must be 0 or 1)
endif
ifneq ($(filter $(SHOWCASE_CONTACT_CACHE),0 1),$(SHOWCASE_CONTACT_CACHE))
$(error SHOWCASE_CONTACT_CACHE must be 0 or 1)
endif
SHOWCASE_CONTACT_BASE_FLAGS = $(if $(filter 1,$(SHOWCASE_CONTACT_CACHE)),$(filter-out -DSC64_COLLISION_CLASS_CACHE -DSC64_COLLISION_STEER_LIST,$(P7_TYPED_RECT_FLAGS)) -DSC64_CONTACT_CACHE,$(P7_TYPED_RECT_FLAGS))
# Existing split cache storage; gameplay and cache dimensions are unchanged.
SHOWCASE_TERRAIN_CACHE_VRAM ?= 1
ifneq ($(words $(filter 0 1,$(SHOWCASE_TERRAIN_CACHE_VRAM))),1)
$(error SHOWCASE_TERRAIN_CACHE_VRAM must be 0 or 1)
endif
ifneq ($(filter $(SHOWCASE_TERRAIN_CACHE_VRAM),0 1),$(SHOWCASE_TERRAIN_CACHE_VRAM))
$(error SHOWCASE_TERRAIN_CACHE_VRAM must be 0 or 1)
endif
SHOWCASE_TERRAIN_CACHE_FLAGS = $(if $(filter 1,$(SHOWCASE_TERRAIN_CACHE_VRAM)),-DSC64_TERRAIN_CACHE_VRAM)
