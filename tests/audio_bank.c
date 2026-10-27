/* Optional private check (make test-audio-bank): decode every stream of the
 * generated format-2 bank with the C decoder, frame by frame as the mixer
 * does, and compare the SHA-256 of the concatenated PCM with the manifest's
 * pcm_sha256. Order matches the pack tool: SFX clips in index order, then
 * imported music tracks by offset. Each stream is copied into an exactly
 * sized allocation (it ends where the next stream begins), so ASan reports
 * any read past it. Built with -DSC64_AUDIO_IMPORTED -I$(SC64_AUDIO_DIR);
 * the blob and manifest paths are arguments.
 *
 *   test-audio-bank BLOB MANIFEST
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio_codec.h"
#include "sc64_audio_generated.h"

_Static_assert(SC64_AUDIO_FORMAT == AUDIO_CODEC_FORMAT, "bank and decoder formats differ");
#define FRAME SC64_AUDIO_SAMPLES_PER_FRAME
#define GUARD (FRAME + 32u)

/* Plain SHA-256 (FIPS 180-4). */
typedef struct { u32 h[8]; u8 buf[64]; u32 used; unsigned long long bytes; } Sha256;
static u32 ror32(u32 x, u32 n) { return x >> n | x << (32u - n); }
static void sha256_block(Sha256 *s, const u8 *p)
{
    static const u32 k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    };
    u32 w[64], v[8];
    for (u32 i = 0; i < 16; ++i) w[i] = (u32)p[4 * i] << 24 | (u32)p[4 * i + 1] << 16 | (u32)p[4 * i + 2] << 8 | p[4 * i + 3];
    for (u32 i = 16; i < 64; ++i) {
        u32 s0 = ror32(w[i - 15], 7) ^ ror32(w[i - 15], 18) ^ w[i - 15] >> 3;
        u32 s1 = ror32(w[i - 2], 17) ^ ror32(w[i - 2], 19) ^ w[i - 2] >> 10;
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    memcpy(v, s->h, sizeof v);
    for (u32 i = 0; i < 64; ++i) {
        u32 t1 = v[7] + (ror32(v[4], 6) ^ ror32(v[4], 11) ^ ror32(v[4], 25)) + ((v[4] & v[5]) ^ (~v[4] & v[6])) + k[i] + w[i];
        u32 t2 = (ror32(v[0], 2) ^ ror32(v[0], 13) ^ ror32(v[0], 22)) + ((v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]));
        memmove(v + 1, v, 7 * sizeof v[0]);
        v[4] += t1;
        v[0] = t1 + t2;
    }
    for (u32 i = 0; i < 8; ++i) s->h[i] += v[i];
}
static void sha256_init(Sha256 *s)
{
    static const u32 h0[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    memcpy(s->h, h0, sizeof h0);
    s->used = 0;
    s->bytes = 0;
}
static void sha256_update(Sha256 *s, const u8 *p, u32 n)
{
    s->bytes += n;
    while (n--) {
        s->buf[s->used++] = *p++;
        if (s->used == 64) { sha256_block(s, s->buf); s->used = 0; }
    }
}
static void sha256_hex(Sha256 *s, char hex[65])
{
    unsigned long long bits = s->bytes * 8u;
    u8 pad = 0x80, zero = 0, len[8];
    sha256_update(s, &pad, 1);
    while (s->used != 56) sha256_update(s, &zero, 1);
    for (u32 i = 0; i < 8; ++i) len[i] = (u8)(bits >> (56 - 8 * i));
    sha256_update(s, len, 8);
    for (u32 i = 0; i < 8; ++i) snprintf(hex + 8 * i, 9, "%08x", s->h[i]);
}

static u8 *read_file(const char *path, u32 *size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    u8 *data = malloc(len > 0 ? (size_t)len + 1u : 1u);
    if (!data || len < 0 || fread(data, 1, (size_t)len, f) != (size_t)len) { fclose(f); free(data); return NULL; }
    fclose(f);
    data[len] = 0;
    *size = (u32)len;
    return data;
}

typedef struct { u32 offset, samples; } Stream;

static int cmp_stream(const void *a, const void *b)
{
    const Stream *x = a, *y = b;
    return x->offset < y->offset ? -1 : x->offset > y->offset;
}

int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "usage: %s BLOB MANIFEST\n", argv[0]); return 2; }
    u32 size, msize;
    u8 *blob = read_file(argv[1], &size), *manifest = read_file(argv[2], &msize);
    if (!blob || !manifest) { fprintf(stderr, "test-audio-bank: cannot read inputs\n"); return 2; }
    const char *key = strstr((const char *)manifest, "\"pcm_sha256\"");
    char want[65] = { 0 };
    /* A space in the format matches any (or no) whitespace around the colon. */
    if (!key || sscanf(key, "\"pcm_sha256\" : \"%64[0-9a-f]\"", want) != 1 || strlen(want) != 64) {
        fprintf(stderr, "test-audio-bank: manifest has no pcm_sha256\n");
        return 2;
    }
    if (size != SC64_AUDIO_BLOB_BYTES || size < GUARD) { fprintf(stderr, "test-audio-bank: blob size differs from the header\n"); return 1; }

    static Stream streams[SC64_AUDIO_SAMPLE_COUNT + SC64_AUDIO_MUSIC_TRACKS];
    u32 count = 0, sfx = SC64_AUDIO_SAMPLE_COUNT, music = 0;
    for (u32 i = 0; i < SC64_AUDIO_SAMPLE_COUNT; ++i)
        streams[count++] = (Stream){ sc64_audio_sample_offset[i], sc64_audio_sample_length[i] };
    for (u32 t = 0; t < SC64_AUDIO_MUSIC_TRACKS; ++t)
        if (sc64_audio_music_frames[t]) streams[count++] = (Stream){ sc64_audio_music_offset[t], sc64_audio_music_frames[t] * FRAME };
    music = count - sfx;
    qsort(streams + sfx, music, sizeof streams[0], cmp_stream);

    Sha256 sha;
    sha256_init(&sha);
    u16 out[FRAME / 2];
    unsigned long long samples = 0;
    for (u32 i = 0; i < count; ++i) {
        /* A stream ends where the next one in the blob begins (or at the guard). */
        u32 end = size - GUARD;
        for (u32 j = 0; j < count; ++j)
            if (streams[j].offset > streams[i].offset && streams[j].offset < end) end = streams[j].offset;
        if (streams[i].offset % 4u || streams[i].samples % AUDIO_CODEC_BLOCK || streams[i].offset >= end) {
            fprintf(stderr, "FAIL audio bank: stream %u misplaced\n", i);
            return 1;
        }
        u32 bytes = end - streams[i].offset;
        u8 *exact = malloc(bytes);
        if (!exact) abort();
        memcpy(exact, blob + streams[i].offset, bytes);
        AudioDecoder d;
        audio_decoder_start(&d, exact);
        for (u32 at = 0; at < streams[i].samples; at += FRAME) {
            u32 n = streams[i].samples - at < FRAME ? streams[i].samples - at : FRAME;
            audio_decoder_run(&d, (volatile u16 *)out, n);
            sha256_update(&sha, (const u8 *)out, n);  /* little-endian halfwords = PCM order */
        }
        samples += streams[i].samples;
        free(exact);
    }
    char got[65];
    sha256_hex(&sha, got);
    printf("audio bank: %u SFX + %u music streams, %llu samples from %u bytes; pcm_sha256 %s\n",
           sfx, music, samples, size, got);
    if (strcmp(got, want)) { fprintf(stderr, "FAIL audio bank: manifest pcm_sha256 is %s\n", want); return 1; }
    puts("audio bank: C decoder matches the manifest");
    free(blob);
    free(manifest);
    return 0;
}
