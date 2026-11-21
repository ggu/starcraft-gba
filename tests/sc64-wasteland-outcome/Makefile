# Invoke with the repository Makefile to supply prepared product flags.
# make -f Makefile -f tests/sc64-wasteland-outcome/Makefile test-wasteland-outcome
WASTELAND_OUT ?= build/wasteland-outcome
.PHONY: test-wasteland-outcome
test-wasteland-outcome:
	mkdir -p $(WASTELAND_OUT)
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(BACKWATER_FLAGS) tests/sc64_wasteland_outcome_game.c common/gba.c -o $(WASTELAND_OUT)/fallback
	$(WASTELAND_OUT)/fallback
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(RACE_CAMPAIGN_FLAGS) -DSC64_CAMPAIGN_RACE=0 tests/sc64_wasteland_outcome_game.c common/gba.c -o $(WASTELAND_OUT)/unified
	$(WASTELAND_OUT)/unified
