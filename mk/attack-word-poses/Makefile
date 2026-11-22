# SPDX-License-Identifier: GPL-3.0-or-later

# Both headers come from the same generator run. Put the paired byte bank
# first on the include path so its content identifier matches the sidecar.
ATTACK_POSE_WORD_ACTORS ?=
ATTACK_WORD_DIR := build/generated/attack-words
ATTACK_WORD_HEADER := $(ATTACK_WORD_DIR)/sc64_unit_attack_words_generated.h
ATTACK_WORD_BANK := $(ATTACK_WORD_DIR)/sc64_unit_attack_source_generated.h
ATTACK_WORD_INPUTS := $(if $(ATTACK_POSE_WORD_ACTORS),$(ATTACK_WORD_HEADER))
ATTACK_WORD_FLAGS := $(if $(ATTACK_POSE_WORD_ACTORS),-DSC64_UNIT_ATTACK_WORDS -I$(ATTACK_WORD_DIR))
ATTACK_WORD_MISSING := $(if $(wildcard $(ATTACK_WORD_BANK)),,force-attack-word-rebuild)

CANONICAL_LOOKUP_CONFIG_KEY := $(CANONICAL_LOOKUP_CONFIG_KEY)$(if $(ATTACK_POSE_WORD_ACTORS),-attackwords$(ATTACK_POSE_WORD_ACTORS))
CANONICAL_LOOKUP_REBUILD := $(if $(shell test -f build/canonical-lookup-config && printf '%s\n' $(call sc64_lookup_quote,$(CANONICAL_LOOKUP_CONFIG_KEY)) | cmp -s - build/canonical-lookup-config && printf matched),,force-canonical-lookup-rebuild)

.PHONY: force-attack-word-config force-attack-word-rebuild
force-attack-word-rebuild:
force-attack-word-config:

$(ATTACK_WORD_DIR)/config: force-attack-word-config
	@mkdir -p $(@D)
	@printf '%s\n' $(call sc64_lookup_quote,$(ATTACK_POSE_WORD_ACTORS)) > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

$(ATTACK_WORD_HEADER): $(ATTACK_POSE_HEADER) $(ATTACK_WORD_DIR)/config tools/sc64_unit_facing.py $(ATTACK_WORD_MISSING)
	$(PYTHON) tools/sc64_unit_attack_sprites.py --out $(ATTACK_WORD_DIR) --header $(ATTACK_WORD_BANK) --word-header $@ --word-actors $(call sc64_lookup_quote,$(ATTACK_POSE_WORD_ACTORS))
