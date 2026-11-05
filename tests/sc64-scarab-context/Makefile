include Makefile

# Isolated authenticated Scarab codec fixture; shipping cargo/transport fences
# remain intact. No production flag vector is changed by these tests.
define SCARAB_CONTEXT_FIXTURE
.PHONY: test-scarab-context-$(1)
test-scarab-context-$(1):
	@set -eu; mkdir -p build/generated/scarab-context; \
	run=$$$$(mktemp -d build/generated/scarab-context/$(1).XXXXXX); \
	$(if $(filter protoss,$(1)),$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wno-unused-function -Istarcraft -DSC64_SCARAB_RUNTIME $(SCARAB_P7_BASE_FLAGS) -USC64_WORKER_CARGO_KIND -USC64_SHARED_TRANSPORT tools/sc64_fact_tables_gen.c -o "$$$$run/facts"; "$$$$run/facts" > "$$$$run/sc64_fact_tables_generated.h",:); \
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wno-unused-function -Wno-unused-variable \
	  -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_NAMESPACE_STAGE $(2) \
	  -USC64_WORKER_CARGO_KIND -USC64_SHARED_TRANSPORT \
	  tests/sc64_scarab_candidate_context_game.c common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(3) -o "$$$$run/context"; \
	"$$$$run/context"
endef
$(eval $(call SCARAB_CONTEXT_FIXTURE,terran,$(TERRAN_ROM_FLAGS),))
$(eval $(call SCARAB_CONTEXT_FIXTURE,zerg,$(ZERG_ROM_FLAGS),))
$(eval $(call SCARAB_CONTEXT_FIXTURE,protoss,$(SCARAB_P7_BASE_FLAGS) -DSC64_FACT_TABLES -I"$$$$run",$(P7_INTERCEPTOR_FLIGHT_SOURCES)))
$(eval $(call SCARAB_CONTEXT_FIXTURE,showcase,$(SHOWCASE_FLAGS),))
.PHONY: test-scarab-context
test-scarab-context: test-scarab-context-terran test-scarab-context-zerg test-scarab-context-protoss test-scarab-context-showcase
