#ifndef AUDIO_CODEC_REF_H
#define AUDIO_CODEC_REF_H
/* Test-only reference encoder for SC64 audio codec format 2 (see
 * common/audio_codec.h). Deliberately plain: greedy minimal width per block,
 * the shortest valid header code, and a per-stream predictor chosen by
 * encoding with all three. Host code only; never linked into a ROM. */
#include <stdlib.h>
#include <string.h>
#include "audio_codec.h"

typedef struct {
    u8 *buf;      /* NULL: count bits only. */
    u32 bits;
} AudioRefWriter;

/* Append `count` bits of value, least significant bit first. */
static void audio_ref_put(AudioRefWriter *w, u32 value, u32 count)
{
    for (u32 i = 0; i < count; ++i, ++w->bits) {
        if (w->buf && (value >> i & 1u)) w->buf[w->bits >> 3] |= (u8)(1u << (w->bits & 7u));
    }
}

static s32 audio_ref_predict(u32 pred, s32 x1, s32 x2)
{
    if (pred == 0) return x1;
    if (pred == 1) return 2 * x1 - x2;
    s32 v = 3 * x1 - x2;
    return v >= 0 ? v / 2 : -((-v + 1) / 2);  /* floor(v / 2) without >> on negatives */
}

static s32 audio_ref_wrap8(s32 v) { return ((v + 128) & 255) - 128; }

/* Minimal two's-complement width holding r: 0 only for r == 0. */
static u32 audio_ref_width(s32 r)
{
    if (!r) return 0;
    for (u32 k = 1; k <= 8; ++k)
        if (r >= -(1 << (k - 1)) && r < (1 << (k - 1))) return k;
    return 8;
}

/* Header code for k after prev (the shortest valid one). */
static void audio_ref_header(AudioRefWriter *w, u32 prev, u32 k)
{
    if (k == prev) audio_ref_put(w, 0u, 1);
    else if (prev > 0 && k == prev - 1u) audio_ref_put(w, 1u, 2);
    else if (prev < 8 && k == prev + 1u) audio_ref_put(w, 3u, 3);
    else audio_ref_put(w, 7u | k << 3, 7);
}

/* Bytes of a stream with `bits` bits: whole halfwords, then a 4-byte multiple. */
static u32 audio_ref_stream_bytes(u32 bits) { return ((bits + 15u) / 16u * 2u + 3u) & ~3u; }

/* Encode n samples (a multiple of 8) with predictor pred. With w->buf NULL
 * this only counts; w->buf must hold audio_ref_stream_bytes() zeroed bytes.
 * hist, when given, counts blocks per width. Returns the bit length. */
static u32 audio_ref_encode_with(const s8 *pcm, u32 n, u32 pred, u8 *buf, u32 hist[9])
{
    AudioRefWriter w = { buf, 0 };
    s32 x1 = 0, x2 = 0;
    u32 prev = 0;
    audio_ref_put(&w, pred, 2);
    for (u32 b = 0; b < n; b += AUDIO_CODEC_BLOCK) {
        s32 r[AUDIO_CODEC_BLOCK];
        s32 y1 = x1, y2 = x2;
        u32 k = 0;
        for (u32 i = 0; i < AUDIO_CODEC_BLOCK; ++i) {
            s32 x = pcm[b + i];
            r[i] = audio_ref_wrap8(x - audio_ref_predict(pred, y1, y2));
            u32 need = audio_ref_width(r[i]);
            if (need > k) k = need;
            y2 = y1;
            y1 = x;
        }
        audio_ref_header(&w, prev, k);
        for (u32 i = 0; i < AUDIO_CODEC_BLOCK && k; ++i) audio_ref_put(&w, (u32)r[i] & ((1u << k) - 1u), k);
        if (hist) ++hist[k];
        prev = k;
        x1 = y1;
        x2 = y2;
    }
    return w.bits;
}

/* Smallest of the three predictors (ties go to the lower one). */
static u32 audio_ref_choose(const s8 *pcm, u32 n)
{
    u32 best = 0, best_bits = audio_ref_encode_with(pcm, n, 0, NULL, NULL);
    for (u32 p = 1; p < 3; ++p) {
        u32 bits = audio_ref_encode_with(pcm, n, p, NULL, NULL);
        if (bits < best_bits) { best = p; best_bits = bits; }
    }
    return best;
}

/* Encode into a new exactly sized allocation; returns it and its size. */
static u8 *audio_ref_encode_alloc(const s8 *pcm, u32 n, u32 pred, u32 *bytes, u32 hist[9])
{
    u32 size = audio_ref_stream_bytes(audio_ref_encode_with(pcm, n, pred, NULL, NULL));
    u8 *buf = calloc(size, 1);
    if (!buf) abort();
    audio_ref_encode_with(pcm, n, pred, buf, hist);
    *bytes = size;
    return buf;
}
#endif
