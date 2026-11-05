# Optional shared Cocoon renderer: focused checks

Source checkpoint `8ef2845e` remains opt-in and unhooked from game rendering.
No normal product flags changed and no private art bank was generated.

After disk space recovered, these checks passed locally under the shared native
job lock:

```sh
python3 -B -m unittest tests.test_sc64_shared_cocoon_bank
clang -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-variable \
  -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -Icommon \
  tests/sc64_shared_cocoon_render.c common/gba.c \
  -o /tmp/sc64-shared-cocoon-8ef2845e-clipped
/tmp/sc64-shared-cocoon-8ef2845e-clipped
```

The three Python tests cover deterministic timeline compilation, malformed
instructions and refusal to overwrite an existing output. The compiled fixture
uses authored pixels to check primary/underlay composition, both framebuffer
pages, visibility masking and scene clipping. These are asset-free tests.

An initial compile omitted `GBA_SCENE_CLIP` and failed because the clip API is
conditional. The command above explicitly uses the production clipping
capability; unused common-platform globals use the existing fixture convention.

Still required: authenticated fresh private-bank generation, source-frame and
palette verification, game hook/lifecycle integration, mid-morph SRAM
continuation, ARM linkage/stack/RAM/ROM accounting, crowded frame cost, and paired
ordinary N64 observation. The deterministic source-wait approximation is not a
validated N64 runtime clock or animation match. No screenshot, ROM, extracted
art, save or private bank belongs in the source repository.
