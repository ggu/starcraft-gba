# SPDX-License-Identifier: GPL-3.0-or-later

# Run each race cartridge's hottest ROM functions from the part of IWRAM that
# its stack cannot reach. Only the link changes: the functions in
# config/iwram-hot/<cartridge>.txt keep their code and move from ROM to IWRAM
# through common/gba_hot.ld. tools/gba_iwram_hot.py then bounds the stack depth
# of the linked program and fails the ROM unless the stack still fits above
# the IWRAM code. Set SC64_IWRAM_HOT=0 for the previous link (gba.ld, IWRAM
# code below 0x03005000) and for matched comparisons on the same inputs.
SC64_IWRAM_HOT ?= 1
ifneq ($(words $(SC64_IWRAM_HOT)),1)
$(error SC64_IWRAM_HOT must be 0 or 1)
endif
ifeq ($(filter 0 1,$(SC64_IWRAM_HOT)),)
$(error SC64_IWRAM_HOT must be 0 or 1)
endif
IWRAM_HOT_TOOL := tools/gba_iwram_hot.py
IWRAM_HOT_ELFS := build/starcraft-terran.elf build/starcraft-zerg-z10.elf build/starcraft-protoss-p10.elf
.PHONY: force-iwram-hot-config test-sc64-iwram-hot
force-iwram-hot-config:
build/iwram-hot-config: force-iwram-hot-config | build
	@printf '%s\n' '$(SC64_IWRAM_HOT)' > $@.tmp
	@if cmp -s $@.tmp $@; then rm $@.tmp; else mv $@.tmp $@; fi
$(IWRAM_HOT_ELFS): build/iwram-hot-config

# $(1) list name, $(2) ELF, $(3) ROM. The ROM waits for the stack check, so a
# cartridge whose stack could reach its IWRAM code is never produced.
define iwram_hot_rules
# One command writes both fragments, the limit last. The second rule runs
# only when the limit fragment is missing or older than the section list.
build/iwram-hot/$(1)/iwram_hot_sections.ld: config/iwram-hot/$(1).txt $$(IWRAM_HOT_TOOL) | build
	$$(PYTHON) $$(IWRAM_HOT_TOOL) fragments $$< $$(@D)
build/iwram-hot/$(1)/iwram_hot_limit.ld: build/iwram-hot/$(1)/iwram_hot_sections.ld
	$$(PYTHON) $$(IWRAM_HOT_TOOL) fragments config/iwram-hot/$(1).txt $$(@D)
# The dumps are GCC's call graph with frame sizes (.ci) and its record of
# which functions have their address taken (.cgraph). They do not change the
# generated code. `override` keeps the script and the dumps when LDFLAGS is
# set on the command line, because the stack check needs both.
$(2): override LDFLAGS := $$(filter-out -Tcommon/gba.ld,$$(LDFLAGS)) -Tcommon/gba_hot.ld -Lbuild/iwram-hot/$(1) -fcallgraph-info=su -fdump-ipa-cgraph -dumpdir build/iwram-hot/$(1)/
$(2): build/iwram-hot/$(1)/iwram_hot_sections.ld build/iwram-hot/$(1)/iwram_hot_limit.ld common/gba_hot.ld mk/iwram-hot/Makefile
build/iwram-hot/$(1)/stack.json: $(2) $$(IWRAM_HOT_TOOL) config/iwram-hot/$(1).txt
	$$(PYTHON) $$(IWRAM_HOT_TOOL) check --elf $$< --dump-dir build/iwram-hot/$(1) --config config/iwram-hot/$(1).txt --json $$@.tmp
	mv $$@.tmp $$@
$(3): build/iwram-hot/$(1)/stack.json
endef
ifeq ($(SC64_IWRAM_HOT),1)
$(eval $(call iwram_hot_rules,terran,build/starcraft-terran.elf,build/StarCraft-Terran-Transport1-GBA.gba))
$(eval $(call iwram_hot_rules,zerg-z10,build/starcraft-zerg-z10.elf,build/StarCraft-Zerg-Z10-Transport1-GBA.gba))
$(eval $(call iwram_hot_rules,protoss-p10,build/starcraft-protoss-p10.elf,build/StarCraft-Protoss-P10-Transport1-GBA.gba))
endif

# test-starcraft already discovers tests/test_sc64*.py; this runs the one file.
test-sc64-iwram-hot:
	$(PYTHON) -m unittest discover -s tests -p 'test_sc64_iwram_hot.py'
