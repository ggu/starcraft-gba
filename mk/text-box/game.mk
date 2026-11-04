# SPDX-License-Identifier: GPL-3.0-or-later
# The modal behaviour on the real engine; needs the prepared Terran inputs.
build/test-sc64-text-box-game: tests/sc64_text_box_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/text-box/game.mk | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-text-box-game
test-sc64-text-box-game: build/test-sc64-text-box-game
	./build/test-sc64-text-box-game
test-sc64-terran-full-campaign: test-sc64-text-box-game

# One ARM probe per product, built with that product's own flags.
# $(1) name, $(2) flag variable, $(3) global mission id, $(4) extra sources
define TEXT_BOX_ARM_PROBE
build/starcraft-text-box-$(1).elf: tests/sc64_text_box_arm_probe.c $$(filter-out common/main.c,$$(COMMON)) $$(COMMON_HEADERS) $$(STARCRAFT) $$(wildcard starcraft/*.h) common/gba.ld Makefile mk/text-box/game.mk | build
	$$(CC) $$(CFLAGS) $$($(2)) -DTB_MISSION=$(3) $$(filter-out common/main.c,$$(COMMON)) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $(4) $$(LDFLAGS) -lgcc -o $$@
build/StarCraft-Text-Box-$(1).gba: build/starcraft-text-box-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC TEXT BOX' SCTB
TEXT_BOX_ARM_ROMS += build/StarCraft-Text-Box-$(1).gba
endef
$(eval $(call TEXT_BOX_ARM_PROBE,t1,TERRAN_ROM_FLAGS,4,))
$(eval $(call TEXT_BOX_ARM_PROBE,z1,ZERG_Z10_RELEASE_FLAGS,0,))
$(eval $(call TEXT_BOX_ARM_PROBE,z10,ZERG_Z10_RELEASE_FLAGS,21 -DTB_LIMIT=12000,))
$(eval $(call TEXT_BOX_ARM_PROBE,p2,PROTOSS_P10_FLAGS,3,$$(P7_INTERCEPTOR_FLIGHT_SOURCES)))
.PHONY: sc64-text-box-arm
sc64-text-box-arm: $(TEXT_BOX_ARM_ROMS)
