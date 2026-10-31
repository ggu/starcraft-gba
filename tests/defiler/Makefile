.PHONY: test-shared-payload-mixed
build/test-shared-payload-mixed: tests/sc64_shared_payload_mixed_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-shared-payload-mixed: build/test-shared-payload-mixed
	./build/test-shared-payload-mixed

.PHONY: test-dark-swarm-unit-field
build/test-dark-swarm-unit-field: tests/sc64_defiler_field_unit_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_UNIT_FIELD_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-unit-field: build/test-dark-swarm-unit-field
	./build/test-dark-swarm-unit-field

.PHONY: test-dark-swarm-actor-union
build/test-dark-swarm-actor-union: tests/sc64_defiler_field_unit_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_UNIT_FIELD_PROBE -DSC64_SHARED_ACTOR_STATE_UNION_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-actor-union: build/test-dark-swarm-actor-union
	./build/test-dark-swarm-actor-union
