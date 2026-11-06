# Nonshipping diagnostic, same product flags and unchanged linker guards.
STACKQ_OUT ?= .local/stack-qualification/diagnostic
.PHONY: stackq-z7
stackq-z7:
	mkdir -p $(STACKQ_OUT)
	$(CC) $(CFLAGS) -fstack-usage -DSC64_ARM_STACK_PAINT_QUALIFICATION -DSC64_RARE_ACTOR_DISPATCH_PROBE -DGBA_DIAMOND_ROM $(ARMQ_STAGE) $(ARMQ_PROFILE) $(BC_FLAGS) $(ZERG_Z7_RELEASE_FLAGS) -DSC64_Z7_DERIVED_SAVE $(ARMQ_SOURCES) $(LDFLAGS) -Wl,-Map,$(STACKQ_OUT)/game.map -lgcc -o $(STACKQ_OUT)/game.elf
	$(OBJCOPY) -O binary $(STACKQ_OUT)/game.elf $(STACKQ_OUT)/game.gba
	$(PYTHON) tools/fix_rom.py $(STACKQ_OUT)/game.gba 'SC ARMQ Z' 'SQZ7'
