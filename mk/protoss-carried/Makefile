# SPDX-License-Identifier: GPL-3.0-or-later
# Included from the top-level Makefile after the P10 candidate rules; it is not
# a standalone makefile. Run these targets from the repository root.

# The rest of the carried Protoss missions under the canonical flags. P1, P2, P3
# and P5 run from test-sc64-protoss-carried-missions in the top-level Makefile.
build/test-sc64-protoss-carried-p4: tests/sc64_p4_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-carried/Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-protoss-carried-p6: tests/sc64_p6_campaign_run.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-carried/Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(PROTOSS_ROM_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-protoss-carried-p4-p6
test-sc64-protoss-carried-p4-p6: build/test-sc64-protoss-carried-p4 build/test-sc64-protoss-carried-p6
	./build/test-sc64-protoss-carried-p4 --attempt-tag canonical-carried-p4
	./build/test-sc64-protoss-carried-p6 --trace .local/sc64/carried-p6-controller.json
test-sc64-protoss-carried-missions: test-sc64-protoss-carried-p4-p6

# Every Protoss mission under the P10 flags, which the default ROM ships: controller,
# tape, host replay and an input-only ARM replay ROM for each.
PX_DIR := build/protoss-px
PX_TOOL = $(PYTHON) tools/sc64_p8_arm_replay.py --mission $(1) --prefix PX
PX_CLANG = $(CLANG) -std=c11 -O1 -g -w -fsanitize=address,undefined -DGBA_HOST -Icommon $(P10_PRODUCT_FLAGS)
PX_SOURCES = $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c
# $(1) name, $(2) controller source, $(3) extra source, $(4) mission, $(5) recipe writing $@.raw
define PX_MISSION
build/px-$(1)-controller: tests/$(2).c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(filter-out $$(P10_FACT_V2_HEADER),$$(PROTOSS_P10_INPUTS)) $$(P10_EGG_FACT_HEADER) common/gba.c $$(COMMON_HEADERS) mk/protoss-carried/Makefile | build
	$$(PX_CLANG) -DPX_TAPE_TRACE $$< $(3) $$(PX_SOURCES) -o $$@
$$(PX_DIR)/$(1)/trace.jsonl: build/px-$(1)-controller tools/sc64_protoss_tape.py
	mkdir -p $$(PX_DIR)/$(1)
	$(5)
	$$(PYTHON) tools/sc64_protoss_tape.py $$@.raw $$@ --mission $(4)
$$(PX_DIR)/$(1)/sc64_px_tape_generated.h: $$(PX_DIR)/$(1)/trace.jsonl tools/sc64_p8_arm_replay.py
	$$(call PX_TOOL,$(4)) tape $$< $$@
PX_$(1)_DEPS = tests/sc64_protoss_arm_replay.c $$(PX_DIR)/$(1)/sc64_px_tape_generated.h $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(filter-out $$(P10_FACT_V2_HEADER),$$(PROTOSS_P10_INPUTS)) $$(P10_EGG_FACT_HEADER) $$(COMMON_HEADERS) mk/protoss-carried/Makefile
build/px-$(1)-replay: $$(PX_$(1)_DEPS) | build
	$$(PX_CLANG) -I$$(PX_DIR)/$(1) -DPX_MISSION=$(4) $$< $$(PX_SOURCES) -o $$@
$$(PX_DIR)/$(1)/host-full.txt: build/px-$(1)-replay
	./$$< > $$@
build/starcraft-px-$(1).elf: $$(PX_$(1)_DEPS) $$(P8_REPLAY_COMMON) common/gba.ld | build
	$$(CC) $$(CFLAGS) $$(P10_PRODUCT_FLAGS) -I$$(PX_DIR)/$(1) -DPX_MISSION=$(4) $$(P8_REPLAY_COMMON) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) $$(LDFLAGS) -lgcc -o $$@
build/StarCraft-Protoss-PX-$(1).gba: build/starcraft-px-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC PX REPLAY' SCPA
PX_HOST += $$(PX_DIR)/$(1)/host-full.txt
PX_ROMS += build/StarCraft-Protoss-PX-$(1).gba
endef
$(eval $(call PX_MISSION,p1,race_campaign_run,common/gba.c,2,./build/px-p1-controller --controller 2 --trace $$@.raw))
$(eval $(call PX_MISSION,p2,race_campaign_run,common/gba.c,3,./build/px-p2-controller --controller 3 --trace $$@.raw))
$(eval $(call PX_MISSION,p3,sc64_p3_campaign_run,common/gba.c,22,./build/px-p3-controller --trace $$@.raw))
$(eval $(call PX_MISSION,p4,sc64_p4_campaign_run,common/gba.c,23,./build/px-p4-controller --attempt-tag px-p4 && cp "$$$$(ls -t .local/sc64/p4-controller-px-p4-*-win.json | head -1)" $$@.raw))
$(eval $(call PX_MISSION,p5,sc64_p5_campaign_run,common/gba.c,24,./build/px-p5-controller --limit 180000 --attempt-tag px-p5 && cp .local/sc64/p5-controller-win.json $$@.raw))
$(eval $(call PX_MISSION,p6,sc64_p6_campaign_run,common/gba.c,25,./build/px-p6-controller --trace $$@.raw))
$(eval $(call PX_MISSION,p7,sc64_p7_campaign_run,common/gba.c,26,./build/px-p7-controller $$@.raw))
$(eval $(call PX_MISSION,p8,sc64_p8_campaign_run,,27,./build/px-p8-controller $$@.raw))
$(eval $(call PX_MISSION,p9,sc64_p9_campaign_run,,28,./build/px-p9-controller $$@.raw))
$(eval $(call PX_MISSION,p10,sc64_p10_campaign_run,,29,./build/px-p10-controller $$@.raw))
# P8 and P9 tapes hold a pause-menu save, so they also get a fresh-process
# continuation under these flags. P10's own qualification covers it there.
# $(1) name, $(2) mission
define PX_SAVE_MISSION
build/px-$(1)-prefix: $$(PX_$(1)_DEPS) | build
	$$(PX_CLANG) -I$$(PX_DIR)/$(1) -DPX_MISSION=$(2) -DPX_REPLAY_PREFIX $$< $$(PX_SOURCES) -o $$@
build/px-$(1)-resume: $$(PX_$(1)_DEPS) | build
	$$(PX_CLANG) -I$$(PX_DIR)/$(1) -DPX_MISSION=$(2) -DPX_REPLAY_RESUME $$< $$(PX_SOURCES) -o $$@
$$(PX_DIR)/$(1)/host-resume.txt: build/px-$(1)-prefix build/px-$(1)-resume $$(PX_DIR)/$(1)/host-full.txt
	rm -f $$(PX_DIR)/$(1)/host-prefix.sav
	./build/px-$(1)-prefix $$(PX_DIR)/$(1)/host-prefix.sav > $$(PX_DIR)/$(1)/host-prefix.txt || true
	./build/px-$(1)-resume $$(PX_DIR)/$(1)/host-prefix.sav > $$@
	$$(call PX_TOOL,$(2)) compare --from-tick $$$$(sed -n 's/^#define PX_TAPE_RESUME_TICK \([0-9]*\)u$$$$/\1/p' $$(PX_DIR)/$(1)/sc64_px_tape_generated.h) $$(PX_DIR)/$(1)/host-full.txt $$@
build/starcraft-px-$(1)-prefix.elf: $$(PX_$(1)_DEPS) $$(P8_REPLAY_COMMON) common/gba.ld | build
	$$(CC) $$(CFLAGS) $$(P10_PRODUCT_FLAGS) -I$$(PX_DIR)/$(1) -DPX_MISSION=$(2) -DPX_REPLAY_PREFIX $$(P8_REPLAY_COMMON) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) $$(LDFLAGS) -lgcc -o $$@
build/starcraft-px-$(1)-resume.elf: $$(PX_$(1)_DEPS) $$(P8_REPLAY_COMMON) common/gba.ld | build
	$$(CC) $$(CFLAGS) $$(P10_PRODUCT_FLAGS) -I$$(PX_DIR)/$(1) -DPX_MISSION=$(2) -DPX_REPLAY_RESUME $$(P8_REPLAY_COMMON) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) $$(LDFLAGS) -lgcc -o $$@
build/StarCraft-Protoss-PX-$(1)-Prefix.gba: build/starcraft-px-$(1)-prefix.elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC PX REPLAY' SCPA
build/StarCraft-Protoss-PX-$(1)-Resume.gba: build/starcraft-px-$(1)-resume.elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC PX REPLAY' SCPA
PX_SAVE_HOST += $$(PX_DIR)/$(1)/host-resume.txt
PX_SAVE_ROMS += build/StarCraft-Protoss-PX-$(1)-Prefix.gba build/StarCraft-Protoss-PX-$(1)-Resume.gba
endef
$(eval $(call PX_SAVE_MISSION,p8,27))
$(eval $(call PX_SAVE_MISSION,p9,28))
test-protoss: test-sc64-protoss-px
.PHONY: test-sc64-protoss-px sc64-protoss-px-arm
test-sc64-protoss-px: $(PX_HOST) $(PX_SAVE_HOST)
	grep -L ' won=1 ' $(PX_HOST) | sed 's/^/no win: /'
	! grep -L ' won=1 ' $(PX_HOST) | grep -q .
sc64-protoss-px-arm: $(PX_ROMS) $(PX_SAVE_ROMS)
