.PHONY: test-sc64-padded-impact-save
test: test-sc64-padded-impact-save
test-sc64-padded-impact-save: build/test-sc64-padded-impact-save
	./build/test-sc64-padded-impact-save
build/test-sc64-padded-impact-save: tests/sc64_padded_impact_save.c starcraft/sc64_padded_impact_payload.h starcraft/sc64_impact_save_types.h starcraft/sc64_impact_events.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
