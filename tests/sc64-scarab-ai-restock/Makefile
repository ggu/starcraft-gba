# Explicit HOST-only integration stage; not part of default products/tests.
# Invoke with make -f Makefile -f tests/sc64-scarab-ai-restock/Makefile test-scarab-ai-restock
.PHONY: test-scarab-ai-restock
test-scarab-ai-restock: $(CANONICAL_LOOKUP_INPUTS)
	mkdir -p build/scarab-ai-restock
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wno-unused-function -Wno-unused-variable \
	  -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_NAMESPACE_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE \
	  -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE \
	  -DSC64_SCARAB_HUMAN_CONTROL_PROBE -DSC64_SCARAB_AI_RESTOCK_HOST_STAGE \
	  $(TERRAN_ROM_FLAGS) tests/sc64_scarab_ai_restock_game.c tests/sc64_yamato_host_sram_shim.c \
	  $(filter-out starcraft/game.c,$(STARCRAFT)) -o build/scarab-ai-restock/test
	./build/scarab-ai-restock/test
