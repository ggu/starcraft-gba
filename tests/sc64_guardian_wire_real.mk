include Makefile
.PHONY: guardian-real
GUARD_HOST = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -DSC64_GUARDIAN_COCOON_STAGE
GUARD_EXTRA = common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
guardian-real:
	$(CLANG) $(GUARD_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-terran
	/tmp/guardian-real-terran
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-zerg
	/tmp/guardian-real-zerg
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-protoss
	/tmp/guardian-real-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-showcase
	/tmp/guardian-real-showcase
.PHONY: guardian-real-remaining
guardian-real-remaining:
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-protoss
	/tmp/guardian-real-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o /tmp/guardian-real-showcase
	/tmp/guardian-real-showcase
