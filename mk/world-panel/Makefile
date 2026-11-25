build/chrome-fixture/sc64_chrome_generated.h: tests/sc64_chrome_fixture.h | build
	mkdir -p $(@D)
	cp $< $@
build/test-sc64-chrome-panel: tests/sc64_chrome_world_panel.c starcraft/sc64_chrome.h build/chrome-fixture/sc64_chrome_generated.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Ibuild/chrome-fixture $< -o $@
.PHONY: test-sc64-chrome-panel
test-sc64-chrome-panel: build/test-sc64-chrome-panel
	./build/test-sc64-chrome-panel
test-starcraft: test-sc64-chrome-panel
