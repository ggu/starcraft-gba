include Makefile
.PHONY: shared-egg-identities
EGG_HOST = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -DSC64_SHARED_EGG_WIRE_HOST_STAGE
EGG_EXTRA = common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
shared-egg-identities:
	$(CLANG) $(EGG_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-identity-terran
	/tmp/shared-egg-identity-terran
	$(CLANG) $(EGG_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-identity-zerg
	/tmp/shared-egg-identity-zerg
	$(CLANG) $(EGG_HOST) $(PROTOSS_P10_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-identity-protoss
	/tmp/shared-egg-identity-protoss
	$(CLANG) $(EGG_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-identity-showcase
	/tmp/shared-egg-identity-showcase
