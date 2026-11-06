# Explicit non-shipping ARM qualification; invoke with the top-level Makefile.
# No target alters the normal release flags or output paths.
ARMQ_OUT ?= /tmp/sc64-shared-actor-arm-qualification
ARMQ_FACTS ?= .local/arm-qual-facts/p10
ARMQ_STAGE = -DSC64_SHARED_ACTOR_ARM_QUALIFICATION -DSC64_SHARED_EGG_CUSTOM_SERVICE \
 -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE \
 -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE \
 -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE \
 -DSC64_SHARED_ACTOR_FLIGHT_HOST_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE \
 -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE \
 -DSC64_SCARAB_WEAPON_PROBE -DSC64_SCARAB_HUMAN_CONTROL_PROBE \
 -DSC64_SCARAB_TECH_PATH_HOST_PROBE -DSC64_SCARAB_AI_STOCK_PROBE
ARMQ_PROFILE = -DSC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE
ARMQ_P10_FLAGS = $(patsubst -I$(P10_FACT_V2_DIR),-I$(ARMQ_FACTS),$(PROTOSS_P10_FLAGS)) -DSC64_GUARDIAN_FULL_FACTS_STAGE
ARMQ_SOURCES = $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
.PHONY: armq-terran armq-z7 armq-p10
armq-terran:
	mkdir -p $(ARMQ_OUT)/terran
	$(CC) $(CFLAGS) -fstack-usage $(ARMQ_STAGE) $(TERRAN_ROM_FLAGS) $(ARMQ_SOURCES) $(LDFLAGS) -Wl,-Map,$(ARMQ_OUT)/terran/game.map -lgcc -o $(ARMQ_OUT)/terran/game.elf
	$(OBJCOPY) -O binary $(ARMQ_OUT)/terran/game.elf $(ARMQ_OUT)/terran/game.gba
	$(PYTHON) tools/fix_rom.py $(ARMQ_OUT)/terran/game.gba 'SC ARMQ T' 'SQT1'
armq-z7:
	mkdir -p $(ARMQ_OUT)/z7
	$(CC) $(CFLAGS) -fstack-usage $(ARMQ_STAGE) $(ARMQ_PROFILE) $(BC_FLAGS) $(ZERG_Z7_RELEASE_FLAGS) -DSC64_Z7_DERIVED_SAVE $(ARMQ_SOURCES) $(LDFLAGS) -Wl,-Map,$(ARMQ_OUT)/z7/game.map -lgcc -o $(ARMQ_OUT)/z7/game.elf
	$(OBJCOPY) -O binary $(ARMQ_OUT)/z7/game.elf $(ARMQ_OUT)/z7/game.gba
	$(PYTHON) tools/fix_rom.py $(ARMQ_OUT)/z7/game.gba 'SC ARMQ Z' 'SQZ7'
armq-p10:
	mkdir -p $(ARMQ_OUT)/p10
	$(CC) $(CFLAGS) -fstack-usage $(ARMQ_STAGE) $(ARMQ_PROFILE) $(BC_FLAGS) $(ARMQ_P10_FLAGS) $(ARMQ_SOURCES) $(LDFLAGS) -Wl,-Map,$(ARMQ_OUT)/p10/game.map -lgcc -o $(ARMQ_OUT)/p10/game.elf
	$(OBJCOPY) -O binary $(ARMQ_OUT)/p10/game.elf $(ARMQ_OUT)/p10/game.gba
	$(PYTHON) tools/fix_rom.py $(ARMQ_OUT)/p10/game.gba 'SC ARMQ P' 'SQP0'
