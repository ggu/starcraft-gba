# Prepared canonical P7/P8 candidate preflight binding fixture.
build/test-sc64-p7-preflight-binding: tests/sc64_protoss_preflight_binding_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P7_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(PROTOSS_P7_CANONICAL_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-p8-preflight-binding: tests/sc64_protoss_preflight_binding_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P8_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P8_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-protoss-preflight-binding
test-sc64-protoss-preflight-binding: build/test-sc64-p7-preflight-binding build/test-sc64-p8-preflight-binding
	./build/test-sc64-p7-preflight-binding
	./build/test-sc64-p8-preflight-binding
