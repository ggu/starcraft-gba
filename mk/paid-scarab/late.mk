# SPDX-License-Identifier: GPL-3.0-or-later
# Included late (from mk/protoss-research/Makefile), after the shared fixture
# macros exist: the paid Scarab HOST checks, and the Protoss profiles of the
# skirmish AI-progress and scripted-human harnesses rebuilt from the shipped P10
# product vector (research, Carriers, paid Scarabs) instead of PROTOSS_ROM_FLAGS.
# The top-level Makefile is at its size limit, so the override lives here; make
# warns that it replaces the earlier recipes, which is intended.
$(eval $(call SKIRMISH_CONTROLLER_PROFILE,protoss,PROTOSS_P10_FLAGS,PROTOSS_P10_INPUTS,P7_INTERCEPTOR_FLIGHT_SOURCES))
$(eval $(call SKIRMISH_AI_PROGRESS_PROFILE,protoss,PROTOSS_P10_FLAGS,PROTOSS_P10_INPUTS,SKIRMISH_AI_PROTOSS_SOURCES))

# HOST checks for the product (sanitized). Needs the prepared P10 inputs.
# The pre-service save is written by a checkout of the unchanged default
# branch: make test-sc64-paid-scarab PSC_BASE_TREE=<path to that checkout>.
PSC_DIR := build/paid-scarab
PSC_SRC := tests/sc64_paid_scarab_product.c
PSC_DEPS := $(PSC_SRC) $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) mk/paid-scarab/Makefile
$(PSC_DIR)/product: $(PSC_DEPS) | build
	@mkdir -p $(PSC_DIR)
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) $(PSC_SRC) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-paid-scarab test-sc64-paid-scarab-old
test-sc64-paid-scarab: $(PSC_DIR)/product
	for t in economy combat splash loss validate save-process; do ./$(PSC_DIR)/product $$t || exit 1; done
	rm -f $(PSC_DIR)/scarab.sav $(PSC_DIR)/scarab.sav.state
	./$(PSC_DIR)/product save-write $(PSC_DIR)/scarab.sav
	./$(PSC_DIR)/product save-read $(PSC_DIR)/scarab.sav
	$(MAKE) test-sc64-paid-scarab-old
test-sc64-paid-scarab-old: $(PSC_DIR)/product
	@if [ -z "$(PSC_BASE_TREE)" ]; then echo "test-sc64-paid-scarab-old: set PSC_BASE_TREE to an unchanged default-branch checkout; skipped"; exit 0; fi
	cp $(PSC_SRC) $(PSC_BASE_TREE)/tests/
	cd $(PSC_BASE_TREE) && $(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_BASE_FLAGS) $(PROTOSS_RESEARCH_FLAGS) $(PSC_SRC) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o ../psc-old-writer
	rm -f $(PSC_DIR)/old.sav $(PSC_DIR)/old.sav.state
	$(PSC_BASE_TREE)/../psc-old-writer old-write $(CURDIR)/$(PSC_DIR)/old.sav
	./$(PSC_DIR)/product old-read $(PSC_DIR)/old.sav
