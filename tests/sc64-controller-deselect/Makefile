# Optional focused helper check; requires prepared P10 inputs.
build/test-sc64-zerg-controller-safe-deselect-p10: tests/sc64_zerg_controller_safe_deselect.c tests/sc64_zerg_late_controller.h tests/sc64_worker_gather_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-controller-deselect/Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(PROTOSS_P10_BASE_FLAGS) -DSC64_WORKER_CARGO_KIND $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-zerg-controller-safe-deselect-p10
test-sc64-zerg-controller-safe-deselect-p10: build/test-sc64-zerg-controller-safe-deselect-p10
	./build/test-sc64-zerg-controller-safe-deselect-p10
