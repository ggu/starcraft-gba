/* Links the private sampled-audio bank into SC64 builds that imported one.
 * The blob is generated under .local/ by tools/sc64_audio_pack.py; default
 * builds assemble this file to nothing. */
#ifdef SC64_AUDIO_IMPORTED
    .section .rodata.sc64_audio_blob,"a",%progbits
    .balign 4
    .global sc64_audio_blob
sc64_audio_blob:
    .incbin SC64_AUDIO_BLOB
#endif
