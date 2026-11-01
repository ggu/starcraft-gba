# Included by the top-level Makefile after the zerg-z8 rules, whose
# variables these recipes extend.

# Asset-free late-Zerg scheduler extensions and compact save state.
build/test-sc64-zerg-late-state: tests/sc64_zerg_late_state.c starcraft/sc64_zerg_late_state.h starcraft/sc64_zerg_late_triggers.h starcraft/sc64_z8_state.h starcraft/sc64_z8_triggers.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-zerg-late-state
test-sc64-zerg-late-state: build/test-sc64-zerg-late-state
	./build/test-sc64-zerg-late-state
test-starcraft: test-sc64-zerg-late-state

# Z9 candidate: the Z8 vector plus the authenticated Z9 program, rendered by
# `sc64_zerg_late_program_pack.py` from the private descriptor written by
# `sc64_zerg_late_pack.py pack --mission Z9`. ZERG_Z9_BANK must hold every Z9
# actor's art; it defaults to the Z8 bank (see the Z9 candidate audit).
ZERG_Z9_BANK ?= $(ZERG_Z8_BANK)
Z9_DESCRIPTOR ?= .local/build/z9-source/descriptor.json
Z9_PROGRAM_HEADER := build/generated/z9/sc64_z9_program_generated.h
$(Z9_PROGRAM_HEADER): $(Z9_DESCRIPTOR) tools/sc64_zerg_late_program_pack.py tools/sc64_zerg_late_pack.py tools/sc64_z8_program_pack.py | build
	$(PYTHON) tools/sc64_zerg_late_program_pack.py $< $@
ZERG_Z9_FEATURES = -DSC64_Z9_DATA_ENABLED -DSC64_Z9_RUNTIME -DSC64_Z9_CAMPAIGN_ENTRY -Ibuild/generated/z9
ZERG_Z9_CANONICAL_FLAGS = $(patsubst -I$(ZERG_Z8_BANK),-I$(ZERG_Z9_BANK),$(ZERG_Z8_CANONICAL_FLAGS)) $(ZERG_Z9_FEATURES)
ZERG_Z9_RELEASE_FLAGS = $(ZERG_Z9_CANONICAL_FLAGS) -DGBA_NO_DEBUG_INPUTS
ZERG_Z9_CANONICAL_INPUTS = $(filter-out $(ZERG_Z8_BANK)/sc64_race_sprites_generated.h,$(ZERG_Z8_CANONICAL_INPUTS)) $(ZERG_Z9_BANK)/sc64_race_sprites_generated.h $(Z9_PROGRAM_HEADER)
build/starcraft-zerg-z9.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(ZERG_Z9_RELEASE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z9.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z9-Transport1-GBA.gba: build/starcraft-zerg-z9.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z9' SCZ9
.PHONY: verify-zerg-z9 zerg-z9
verify-zerg-z9: build/StarCraft-Zerg-Z9-Transport1-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z9
zerg-z9: verify-zerg-z9
build/test-sc64-z9-game: tests/sc64_z9_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z9_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-zerg-z9
test-zerg-z9: build/test-sc64-z9-game
	./build/test-sc64-z9-game

# Z10 candidate: the Z9 vector plus the authenticated Z10 program, from
# `sc64_zerg_late_pack.py pack --mission Z10`. It carries Z9, so its test runs
# the Z9 fixture again under the Z10 flags.
ZERG_Z10_BANK ?= $(ZERG_Z9_BANK)
Z10_DESCRIPTOR ?= .local/build/z10-source/descriptor.json
Z10_PROGRAM_HEADER := build/generated/z10/sc64_z10_program_generated.h
$(Z10_PROGRAM_HEADER): $(Z10_DESCRIPTOR) tools/sc64_zerg_late_program_pack.py tools/sc64_zerg_late_pack.py tools/sc64_z8_program_pack.py | build
	$(PYTHON) tools/sc64_zerg_late_program_pack.py $< $@
ZERG_Z10_FEATURES = -DSC64_Z10_DATA_ENABLED -DSC64_Z10_RUNTIME -DSC64_Z10_CAMPAIGN_ENTRY -Ibuild/generated/z10
ZERG_Z10_CANONICAL_FLAGS = $(patsubst -I$(ZERG_Z9_BANK),-I$(ZERG_Z10_BANK),$(ZERG_Z9_CANONICAL_FLAGS)) $(ZERG_Z10_FEATURES)
ZERG_Z10_RELEASE_FLAGS = $(ZERG_Z10_CANONICAL_FLAGS) -DGBA_NO_DEBUG_INPUTS
ZERG_Z10_CANONICAL_INPUTS = $(filter-out $(ZERG_Z9_BANK)/sc64_race_sprites_generated.h,$(ZERG_Z9_CANONICAL_INPUTS)) $(ZERG_Z10_BANK)/sc64_race_sprites_generated.h $(Z10_PROGRAM_HEADER)
build/starcraft-zerg-z10.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(ZERG_Z10_RELEASE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z10.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z10-Transport1-GBA.gba: build/starcraft-zerg-z10.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z10' SCZA
.PHONY: verify-zerg-z10 zerg-z10
verify-zerg-z10: build/StarCraft-Zerg-Z10-Transport1-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z10
zerg-z10: verify-zerg-z10
build/test-sc64-z10-game: tests/sc64_z10_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z10_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-z10-carried-z9: tests/sc64_z9_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z10_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-zerg-z10
test-zerg-z10: build/test-sc64-z10-game build/test-sc64-z10-carried-z9
	./build/test-sc64-z10-carried-z9
	./build/test-sc64-z10-game
