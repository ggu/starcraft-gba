# SPDX-License-Identifier: GPL-3.0-or-later
include mk/carrier-release/flags/Makefile
# The normal Terran cartridge uses the shared compact Custom Game journal.
TERRAN_CARRIER_RELEASE ?= 1
ifneq ($(TERRAN_CARRIER_RELEASE),0)
ifneq ($(TERRAN_CARRIER_RELEASE),1)
$(error TERRAN_CARRIER_RELEASE must be 0 or 1)
endif
endif
TERRAN_CARRIER_FLAGS := $(if $(filter 1,$(TERRAN_CARRIER_RELEASE)),$(SC64_CARRIER_RELEASE_FLAGS))
TERRAN_CARRIER_SOURCES := $(if $(TERRAN_CARRIER_FLAGS),$(P7_INTERCEPTOR_FLIGHT_SOURCES))
TERRAN_PROJECTILE_SOURCES := $(filter starcraft/sc64_interceptor_%.c,$(STARCRAFT))
TERRAN_ARM_SOURCES := $(filter-out $(TERRAN_PROJECTILE_SOURCES),$(STARCRAFT)) $(TERRAN_CARRIER_SOURCES) $(TERRAN_PROJECTILE_SOURCES)
TERRAN_CARRIER_CONFIG := $(if $(shell test -f build/terran-carrier-config && printf '%s\n' '$(TERRAN_CARRIER_RELEASE)' | cmp -s - build/terran-carrier-config && printf matched),,force-terran-carrier-rebuild)
.PHONY: force-terran-carrier-config force-terran-carrier-rebuild
force-terran-carrier-config:
force-terran-carrier-rebuild:
build/terran-carrier-config: force-terran-carrier-config | build
	@printf '%s\n' '$(TERRAN_CARRIER_RELEASE)' > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

build/starcraft-terran.elf: $(COMMON) $(COMMON_HEADERS) $(TERRAN_ARM_SOURCES) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.ld Makefile mk/combat-perf/Makefile mk/carrier-release/flags/Makefile mk/carrier-release/terran-normal/Makefile build/terran-carrier-config $(TERRAN_CARRIER_CONFIG) | build
	$(CC) $(TERRAN_CARRIER_FLAGS) $(CFLAGS) $(TERRAN_ROM_FLAGS) $(CANONICAL_DEFENSE_FLAGS) $(CANONICAL_TARGET_INDEX_FLAGS) $(CANONICAL_TARGET_POLICY_FLAGS) $(COMMON) $(TERRAN_ARM_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-terran.map -lgcc -o $@
	$(SIZE) $@
