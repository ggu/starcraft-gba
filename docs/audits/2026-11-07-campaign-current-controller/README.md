# Experimental campaign current-controller stage

`SC64_CAMPAIGN_CONTROLLER_HOST_STAGE` is a default-off HOST and shared-races-only experiment. It keeps bounded per-actor transient current-controller bytes separate from `raw_owner`, which remains source origin. Descriptor-authenticated P1 and Z9/Z10 are the only admitted campaign contexts. Other campaign starts fail closed; unknown or inconsistent controller identities are not inferred from origin. P1 rescue uses the authenticated unique human slot, and creator inheritance follows the tracked live creator. Z9/Z10 town production and waves use current controller, with direct wallet admission rejecting an unknown or wrong-town producer. The same-engine-owner transfer API is an authored HOST boundary, not evidence of an existing late-Zerg GiveUnit callback.

This stage has no serialized controller state: every save and load is refused, including from the title screen. It is not enabled in an ARM ROM, and it does not qualify other campaign transfers, old-save migration, active research or paid Scarabs. HOST fixtures use authored setup; they are not ordinary-input wins or native parity.

With the existing verified optional P10/Z10 generated inputs and audio bank already prepared, run:

```
make -f Makefile -f tests/sc64-campaign-controller/Makefile \
  SC64_AUDIO_DIR=<prepared-audio-dir> CONTROLLER_COVER_DIR=<prepared-cover-dir> \
  test-campaign-controller
```

The recipe neither fetches nor generates inputs. Keep generated/private inputs and outputs outside version control. P1 exercises actual rescue to Nexus production and Probe construction; Z10 covers descriptor identity, same-engine-owner transfer, unknown and wrong-town wallet refusal, creator inheritance, slot reuse and the save fence. Bounds and unsupported-context checks are included. All coverage remains scoped to these fixtures and contexts.
