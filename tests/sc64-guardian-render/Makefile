.PHONY: guardian-render-fake guardian-render-private
guardian-render-fake:
	clang -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -Icommon tests/sc64_guardian_cocoon_render.c common/gba.c -o /tmp/sc64-guardian-render-fake
	/tmp/sc64-guardian-render-fake
guardian-render-private:
	clang -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -Icommon -I.local/sc64/guardian-cocoon/bank tests/sc64_guardian_cocoon_private.c common/gba.c -o /tmp/sc64-guardian-render-private
	/tmp/sc64-guardian-render-private
