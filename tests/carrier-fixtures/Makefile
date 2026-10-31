build/test-sc64-p8-owner-mask: tests/sc64_p7_owner_mask_game.c $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P8_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile tests/carrier-fixtures/Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(PROTOSS_P8_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-p8-owner-mask
test-sc64-p8-owner-mask: build/test-sc64-p8-owner-mask
	./build/test-sc64-p8-owner-mask

# Compare the Carrier validators to their eager reference under product flags.
CARRIER_PARENT_FAST_DEPS = tests/sc64_carrier_parent_fast.c $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) common/gba.c $(COMMON_HEADERS) Makefile tests/carrier-fixtures/Makefile
build/test-sc64-carrier-parent-fast-p7: $(CARRIER_PARENT_FAST_DEPS) $(PROTOSS_P7_CANONICAL_INPUTS) | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(PROTOSS_P7_CANONICAL_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-carrier-parent-fast-p8: $(CARRIER_PARENT_FAST_DEPS) $(PROTOSS_P8_INPUTS) | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(PROTOSS_P8_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-carrier-parent-fast-p7 test-sc64-carrier-parent-fast-p8
test-sc64-carrier-parent-fast-p7: build/test-sc64-carrier-parent-fast-p7
	./build/test-sc64-carrier-parent-fast-p7
test-sc64-carrier-parent-fast-p8: build/test-sc64-carrier-parent-fast-p8
	./build/test-sc64-carrier-parent-fast-p8
