# SPDX-License-Identifier: GPL-3.0-or-later
# Included late (from mk/protoss-research/Makefile), after the shared fixture
# macros exist: the paid Scarab HOST checks, and the Protoss profiles of the
# skirmish AI-progress and scripted-human harnesses rebuilt from the shipped P10
# product vector (research, Carriers, paid Scarabs) instead of PROTOSS_ROM_FLAGS.
# The top-level Makefile is at its size limit, so the override lives here; make
# warns that it replaces the earlier recipes, which is intended.
$(eval $(call SKIRMISH_CONTROLLER_PROFILE,protoss-p10,PROTOSS_P10_FLAGS,PROTOSS_P10_INPUTS,P7_INTERCEPTOR_FLIGHT_SOURCES))
$(eval $(call SKIRMISH_AI_PROGRESS_PROFILE,protoss-p10,PROTOSS_P10_FLAGS,PROTOSS_P10_INPUTS,SKIRMISH_AI_PROTOSS_SOURCES))

# HOST checks for the product (sanitized). Needs the prepared P10 inputs.
# The pre-service save is written by a checkout of the unchanged default
# branch: make test-sc64-paid-scarab PSC_BASE_TREE=<path to that checkout>.
PSC_DIR := build/paid-scarab
PSC_SRC := tests/sc64_paid_scarab_product.c
PSC_DEPS := $(PSC_SRC) $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) mk/paid-scarab-checks/Makefile
$(PSC_DIR)/product: $(PSC_DEPS) | build
	@mkdir -p $(PSC_DIR)
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) $(PSC_SRC) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-paid-scarab test-sc64-paid-scarab-old
test-sc64-paid-scarab: $(PSC_DIR)/product
	for t in economy combat splash loss validate save-process campaign placed cold retrain recharge shuttle birth restock0 lift trigger; do ./$(PSC_DIR)/product $$t || exit 1; done
	rm -f $(PSC_DIR)/scarab.sav $(PSC_DIR)/scarab.sav.state
	./$(PSC_DIR)/product save-write $(PSC_DIR)/scarab.sav
	./$(PSC_DIR)/product save-read $(PSC_DIR)/scarab.sav
	rm -f $(PSC_DIR)/camp.sav $(PSC_DIR)/camp.sav.state
	./$(PSC_DIR)/product campaign-write $(PSC_DIR)/camp.sav
	./$(PSC_DIR)/product campaign-read $(PSC_DIR)/camp.sav
	$(MAKE) test-sc64-paid-scarab-old
# A save written by the unchanged base code must load here. PSC_BASE_TREE is an unchanged
# default-branch checkout; it is only read: the fixture is compiled from a scratch directory
# that holds the product source and links to the checkout's own sources and generated inputs.
ifeq ($(strip $(PSC_BASE_TREE)),)
test-sc64-paid-scarab-old:
	@echo "test-sc64-paid-scarab-old: set PSC_BASE_TREE to an unchanged default-branch checkout; skipped"
else
PSC_OLD_DIR := $(PSC_DIR)/old-tree
test-sc64-paid-scarab-old: $(PSC_DIR)/product
	rm -rf $(PSC_OLD_DIR)
	mkdir -p $(PSC_OLD_DIR)/tests
	cp $(PSC_SRC) $(PSC_OLD_DIR)/tests/
	for d in starcraft common build .local; do ln -s $(abspath $(PSC_BASE_TREE))/$$d $(PSC_OLD_DIR)/$$d; done
	cd $(PSC_OLD_DIR) && $(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(patsubst -I$(P10_SCARAB_FACT_DIR),-I$(P10_FACT_V2_DIR),$(filter-out $(PROTOSS_SCARAB_FLAGS),$(PROTOSS_P10_FLAGS))) $(PSC_SRC) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $(CURDIR)/$(PSC_DIR)/psc-old-writer
	rm -f $(PSC_DIR)/old.sav $(PSC_DIR)/old.sav.state
	./$(PSC_DIR)/psc-old-writer old-write $(PSC_DIR)/old.sav
	./$(PSC_DIR)/product old-read $(PSC_DIR)/old.sav
endif

# Ordinary controls end to end (pad input only): chain, Reaver, Scarabs, attack,
# real pause-menu save with a Scarab in flight, fresh-process load and continuation.
PSC_ORD_SRC := tests/sc64_paid_scarab_ordinary.c
$(PSC_DIR)/ordinary: $(PSC_ORD_SRC) tests/sc64_skirmish_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) mk/paid-scarab-checks/Makefile | build
	@mkdir -p $(PSC_DIR)
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) $(PSC_ORD_SRC) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-paid-scarab-ordinary
test-sc64-paid-scarab-ordinary: $(PSC_DIR)/ordinary
	rm -f $(PSC_DIR)/ordinary.sav $(PSC_DIR)/ordinary.sav.state
	./$(PSC_DIR)/ordinary run $(PSC_DIR)/ordinary.sav $(PSC_DIR)/ordinary-tape.json
	./$(PSC_DIR)/ordinary resume $(PSC_DIR)/ordinary.sav

# Paid Scarabs on Protoss mission 2 through the P2 controller's own pad tape: the human has
# Reavers there, so a Scarab flies with no fixture. See tests/sc64_paid_scarab_p2.c.
PSC_P2_SRC := tests/sc64_paid_scarab_p2.c
$(PSC_DIR)/p2: $(PSC_P2_SRC) tests/sc64_text_box_replay.h $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) mk/paid-scarab-checks/Makefile | build
	@mkdir -p $(PSC_DIR)
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_RESEARCH_COVER_DIR) $(PROTOSS_P10_FLAGS) $(PSC_P2_SRC) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
# A stock P10 save with a Scarab in flight, read by the staged probes in a fresh process. Stock P10 numbers the
# Scarab SC_RU_LEGACY_COUNT; the research/Egg probe and the active nuke probe keep their staged identity there
# and put U_SCARAB behind it, so their codecs translate the stock pair (type SC_RU_LEGACY_COUNT, raw 85) only.
# The nuke probe needs the private prepared nuke inputs and is skipped, said so, without them.
PSC_STAGED_SRC := tests/sc64_psc_stock_row.c
.PHONY: test-sc64-paid-scarab-staged
$(PSC_DIR)/stock-row-egg: $(PSC_STAGED_SRC) $(SC64_EGG_PREVIEW_DEPS) $(SC64_P10_RESEARCH_EGG_INPUTS) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) mk/paid-scarab-checks/Makefile | build
	@mkdir -p $(PSC_DIR)
	$(CLANG) $(P8_HOST_FLAGS) $(SC64_P10_RESEARCH_EGG_FLAGS) $(PSC_STAGED_SRC) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-sc64-paid-scarab-staged: $(PSC_DIR)/product $(PSC_DIR)/stock-row-egg
	rm -f $(PSC_DIR)/stock.sav $(PSC_DIR)/stock.sav.state $(PSC_DIR)/stock-bad.sav
	./$(PSC_DIR)/product save-write $(PSC_DIR)/stock.sav
	./$(PSC_DIR)/product save-write-badraw $(PSC_DIR)/stock-bad.sav
	./$(PSC_DIR)/stock-row-egg pairs
	./$(PSC_DIR)/stock-row-egg read $(PSC_DIR)/stock.sav
	./$(PSC_DIR)/stock-row-egg reject $(PSC_DIR)/stock-bad.sav
	@if [ -f $(SC64_NUKE_ASSET_DIR)/sc64_nuke_source_generated.h ]; then set -e; \
		$(MAKE) $(SC64_NUKE_STRIKE_P10_FACT_HEADER) $(SC64_NUKE_RUNTIME_ASSETS); \
		$(CLANG) $(SC64_NUKE_FIXTURE_FLAGS) $(SC64_NUKE_RUNTIME_FLAGS) $(SC64_NUKE_STRIKE_FLAGS) $(patsubst -I$(P10_SCARAB_FACT_DIR),-I$(SC64_NUKE_STRIKE_P10_FACT_DIR),$(PROTOSS_P10_FLAGS)) $(PSC_STAGED_SRC) $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(PSC_DIR)/stock-row-nuke; \
		./$(PSC_DIR)/stock-row-nuke pairs; ./$(PSC_DIR)/stock-row-nuke read $(PSC_DIR)/stock.sav; \
		./$(PSC_DIR)/stock-row-nuke reject $(PSC_DIR)/stock-bad.sav; \
	else echo "test-sc64-paid-scarab-staged: the prepared nuke inputs are absent; the nuke-probe reader was not built or run"; fi
.PHONY: test-sc64-paid-scarab-p2 check-sc64-paid-scarab-p2-arm
test-sc64-paid-scarab-p2: $(PSC_DIR)/p2 build/protoss-px/p2/trace.jsonl tools/sc64_paid_scarab_p2_host.py tools/sc64_paid_scarab_p2_tape.py
	$(PYTHON) tools/sc64_paid_scarab_p2_host.py $(PSC_DIR)/p2 build/protoss-px/p2/trace.jsonl.raw $(PSC_DIR)/p2run

# The same pad tape on the P10 ROM in mGBA (no-GDB physical-button runner), with a fresh process
# loading the exported SRAM: make check-sc64-paid-scarab-p2-arm PSC_ARM_RUNNER=<sc64-headless-replay>
check-sc64-paid-scarab-p2-arm: test-sc64-paid-scarab-p2 build/StarCraft-Protoss-P10-Transport1-GBA.gba tools/sc64_paid_scarab_p2_arm.py
	@if [ -z "$(PSC_ARM_RUNNER)" ]; then echo "set PSC_ARM_RUNNER to the headless mGBA runner"; exit 2; fi
	$(PYTHON) tools/sc64_paid_scarab_p2_arm.py build/StarCraft-Protoss-P10-Transport1-GBA.gba build/starcraft-protoss-p10.elf $(PSC_ARM_RUNNER) $(PSC_DIR)/p2run/p2-save.json $(PSC_DIR)/p2run/run.out $(PSC_DIR)/p2arm
