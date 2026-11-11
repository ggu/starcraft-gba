# Normal-menu opening parity across the four products .
define SKIRMISH_OPENING_FIXTURE
build/test-skirmish-opening-$(1): tests/sc64_skirmish_opening_menu.c tests/sc64_skirmish_campaign_run.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$($(3)) common/gba.c $$(COMMON_HEADERS) Makefile tests/sc64-skirmish-opening/Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $$($(2)) $$< common/gba.c starcraft/sc64_bc_body_art.c $(4) -o $$@
endef
$(eval $(call SKIRMISH_OPENING_FIXTURE,terran,TERRAN_ROM_FLAGS,TERRAN_FULL_INPUTS))
$(eval $(call SKIRMISH_OPENING_FIXTURE,zerg,ZERG_Z7_CANONICAL_FLAGS,ZERG_Z7_CANONICAL_INPUTS))
$(eval $(call SKIRMISH_OPENING_FIXTURE,protoss,PROTOSS_P10_BASE_FLAGS,PROTOSS_P10_INPUTS,$(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES)))
$(eval $(call SKIRMISH_OPENING_FIXTURE,showcase,SHOWCASE_FLAGS,SHOWCASE_INPUTS,$(P7_INTERCEPTOR_FLIGHT_SOURCES)))
.PHONY: test-skirmish-openings
test-skirmish-openings: $(addprefix build/test-skirmish-opening-,terran zerg protoss showcase)
	@for t in $^; do ./$$t || exit $$?; done
