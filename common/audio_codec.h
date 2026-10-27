#ifndef AUDIO_CODEC_H
#define AUDIO_CODEC_H
/* Lossless decoder for SC64 audio codec format 2 (written by
 * tools/sc64_audio_pack.py). Header-only; shared by the VBlank mixer and the
 * host tests.
 *
 * A stream is a sequence of little-endian halfwords read LSB first: a 2-bit
 * predictor P, then blocks of 8 samples. Each block header sets the residual
 * width k (0..8) relative to the previous block ("0" same, "10" k-1, "110"
 * k+1, "111"+4 bits explicit), followed by eight k-bit two's-complement
 * residuals. The sample is (s8)(pred + r) with pred x1 (P=0), 2*x1-x2 (P=1)
 * or (3*x1-x2)>>1 (P=2).
 *
 * Work is constant per sample: no tables, no division and no per-sample
 * variable-length loop. Each predictor has its own function and each width
 * its own unrolled block body with constant shifts, sized for Thumb-1 code
 * running from ROM. Streams start word aligned and are padded to whole
 * words, so the halfword stream is read a word (two halfwords, ldmia) at a
 * time; a word is fetched only when the next field needs one of its bits, so
 * a run never reads past the stream's padded end. Residuals are taken one
 * word per block (k <= 4) or two (k >= 5); header bits are taken one at a
 * time. Streams come from the pack tool, which verifies them; reserved
 * codes are not rejected here, only kept memory-safe (an invalid width
 * decodes as 8). Output is written as packed halfwords, the only store size
 * VRAM accepts.
 */
#include "gba.h"

#define AUDIO_CODEC_FORMAT 2
#define AUDIO_CODEC_BLOCK 8u

/* Decoder state between runs: word and halfword fields only, so it can live
 * in VRAM (12 bytes on the GBA). A run loads it into registers and stores it
 * back once. */
typedef struct {
    const u8 *next;   /* Next word to load (4-byte aligned). */
    u32 acc;          /* Bit accumulator, LSB-first, nb valid bits. */
    u16 state;        /* nb (0..32) | previous block's width k << 6 | predictor << 10 */
    u16 x;            /* Last sample x1 in the low byte, the one before (x2) in the high byte. */
} AudioDecoder;

#define AUDIO_CODEC_STATE(nb, k, pred) ((u16)((nb) | (k) << 6 | (pred) << 10))

typedef u32 __attribute__((may_alias)) AudioCodecWord;

/* Start decoding a stream (reads the predictor field). stream is 4-byte aligned. */
static inline void audio_decoder_start(AudioDecoder *d, const u8 *stream)
{
    u32 first = *(const AudioCodecWord *)(const void *)stream;
    d->next = stream + 4;
    d->acc = first >> 2;
    d->state = AUDIO_CODEC_STATE(30u, 0u, first & 3u);
    d->x = 0;
}

#define AUDIO_CODEC_INLINE static inline __attribute__((always_inline))

/* Decoder state unpacked into words for one run (a stack copy made by
 * audio_decoder_run(); the VRAM copy is only read and written as words and
 * halfwords). */
typedef struct {
    const u8 *next;
    u32 acc, nb, k, x1, d;   /* d = x1 - x2 */
} AudioCodecRun;

/* Decoder registers while a run is in progress. */
typedef struct {
    /* Kept in memory (a stack slot): it is used only when a word is loaded,
     * and the block bodies need every low register. */
    const AudioCodecWord *volatile next;
    u32 acc, nb, x1, d;   /* nb <= 32 valid bits in acc; d = x1 - x2 */
} AudioCodecRegs;

/* Take the next n bits (1 <= n <= 32, a constant) as the low bits of the
 * result (bits above n are garbage). A word is loaded only when the bits
 * held fall short (nb < n), and then its leftover nb + 32 - n bits fit. */
AUDIO_CODEC_INLINE u32 audio_codec_take(AudioCodecRegs *s, u32 n)
{
    u32 w = s->acc, nb = s->nb;
    if (nb >= n) {
        s->acc = n < 32u ? w >> (n & 31u) : 0u;
        s->nb = nb - n;
        return w;
    }
    const AudioCodecWord *p = s->next;
    u32 h = *p;
    s->next = p + 1;
    w |= h << nb;
    /* h >> (n - nb); for n == 32 the shift may be 32, giving 0. */
    s->acc = n < 32u ? h >> ((n - nb) & 31u) : (h >> 1) >> ((31u - nb) & 31u);
    s->nb = nb + 32u - n;
    return w;
}

/* One header bit; a word is fetched only when no bit is held. */
AUDIO_CODEC_INLINE u32 audio_codec_bit(AudioCodecRegs *s)
{
    if (!s->nb) { const AudioCodecWord *p = s->next; s->acc = *p; s->next = p + 1; s->nb = 32u; }
    u32 b = s->acc << 31;  /* nonzero iff the bit is set; tested by sign */
    s->acc >>= 1;
    s->nb -= 1u;
    return b;
}

/* Sample registers are u32. P=0 and P=1 are linear, so their sums may run
 * unwrapped (only the low byte is ever stored); P=2 halves a sum and keeps
 * exact sign-extended samples. */
AUDIO_CODEC_INLINE u32 audio_codec_sample(AudioCodecRegs *s, u32 r, u32 pred)
{
    /* With d = x1 - x2: 2*x1 - x2 = x1 + d and (3*x1 - x2) >> 1 = x1 + (d >> 1). */
    if (pred == 0) {
        s->x1 += r;
    } else if (pred == 1) {
        s->d += r;
        s->x1 += s->d;
    } else {
        u32 x = s->x1 + (u32)((s32)s->d >> 1) + r;
        x = (u32)((s32)(x << 24) >> 24);
        s->d = x - s->x1;
        s->x1 = x;
    }
    return s->x1;
}

/* One block of width k (a constant). Its residual bits are taken as one
 * word (k <= 4: 8k <= 32 bits) or two (4k bits each); every residual is then
 * a constant shift pair out of that word. */
AUDIO_CODEC_INLINE void audio_codec_block(AudioCodecRegs *s, volatile u16 *out, u32 pred, u32 k)
{
    if (!k && !pred) {
        u32 v = s->x1 & 0xffu;
        v |= v << 8;
        out[0] = (u16)v; out[1] = (u16)v; out[2] = (u16)v; out[3] = (u16)v;
        return;
    }
    const u32 g = k <= 4u ? 8u : 4u;
    u32 w = 0;
#define AUDIO_CODEC_R(i) (k ? (u32)((s32)(w << ((32u - k * ((i) % g + 1u)) & 31u)) >> ((32u - k) & 31u)) : 0u)
#define AUDIO_CODEC_PAIR(j)                                                     \
    do {                                                                        \
        if (k && (2u * (j)) % g == 0) w = audio_codec_take(s, g * k);           \
        u32 a = audio_codec_sample(s, AUDIO_CODEC_R(2u * (j)), pred);           \
        u32 b = audio_codec_sample(s, AUDIO_CODEC_R(2u * (j) + 1u), pred);      \
        out[j] = (u16)((a & 0xffu) | b << 8);  /* low byte first */            \
    } while (0)
    AUDIO_CODEC_PAIR(0);
    AUDIO_CODEC_PAIR(1);
    AUDIO_CODEC_PAIR(2);
    AUDIO_CODEC_PAIR(3);
#undef AUDIO_CODEC_PAIR
#undef AUDIO_CODEC_R
}

/* Decode blocks with a fixed predictor: a state machine with one loop per
 * width. After each block the next header is decoded in place: "same" loops,
 * "k-1" and "k+1" jump straight to the neighbouring width and only the
 * explicit code goes through the switch. */
AUDIO_CODEC_INLINE void audio_decoder_blocks(AudioCodecRun *d, volatile u16 *out, u32 samples, const u32 pred)
{
    AudioCodecRegs s;
    volatile u16 *const end = out + samples / 2u;
    u32 k = d->k;
    s.next = (const AudioCodecWord *)(const void *)d->next;
    s.acc = d->acc;
    s.nb = d->nb;
    s.x1 = d->x1;
    s.d = d->d;
    if (out == end) goto done;
    /* First header of this run, relative to the width saved in d->k. */
    if (audio_codec_bit(&s)) {
        if (!audio_codec_bit(&s)) k -= 1u;
        else if (!audio_codec_bit(&s)) k += 1u;
        else k = audio_codec_take(&s, 4u) & 15u;
    }
dispatch:
    switch (k) {
    case 0: goto w0;
    case 1: goto w1;
    case 2: goto w2;
    case 3: goto w3;
    case 4: goto w4;
    case 5: goto w5;
    case 6: goto w6;
    case 7: goto w7;
    default: goto w8;  /* 8; reserved widths never occur. */
    }
#define AUDIO_CODEC_WIDTH(K, DOWN, UP)                                          \
w##K:                                                                           \
    audio_codec_block(&s, out, pred, K);                                        \
    out += 4;                                                                   \
    if (out == end) { k = K; goto done; }                                       \
    if (!audio_codec_bit(&s)) goto w##K;                                        \
    if (!audio_codec_bit(&s)) goto DOWN;                                        \
    if (!audio_codec_bit(&s)) goto UP;                                          \
    k = audio_codec_take(&s, 4u) & 15u;                                         \
    goto dispatch;
    AUDIO_CODEC_WIDTH(0, w8, w1)  /* "k-1" from 0 is invalid; any width is safe. */
    AUDIO_CODEC_WIDTH(1, w0, w2)
    AUDIO_CODEC_WIDTH(2, w1, w3)
    AUDIO_CODEC_WIDTH(3, w2, w4)
    AUDIO_CODEC_WIDTH(4, w3, w5)
    AUDIO_CODEC_WIDTH(5, w4, w6)
    AUDIO_CODEC_WIDTH(6, w5, w7)
    AUDIO_CODEC_WIDTH(7, w6, w8)
    AUDIO_CODEC_WIDTH(8, w7, w8)  /* "k+1" from 8 is invalid. */
#undef AUDIO_CODEC_WIDTH
done:
    d->next = (const u8 *)(const void *)s.next;
    d->acc = s.acc;
    d->nb = s.nb;
    d->k = k;
    d->x1 = s.x1;
    d->d = s.d;
}

/* One function per predictor keeps each state machine smaller (shorter
 * branches) and gives each its own register allocation. GCC's -Os lays these
 * state machines out with fewer instructions per sample than -O2 (measured
 * on real-bank streams with a Thumb instruction counter: about 10% fewer for
 * P=0 and P=1), so the ARM build compiles just these functions for size. */
#if defined(__GNUC__) && !defined(__clang__)
#define AUDIO_CODEC_SIZE_OPT __attribute__((optimize("Os")))
#else
#define AUDIO_CODEC_SIZE_OPT
#endif
static void AUDIO_CODEC_SIZE_OPT __attribute__((noinline, unused)) audio_decoder_run_p0(AudioCodecRun *d, volatile u16 *out, u32 samples)
{
    audio_decoder_blocks(d, out, samples, 0);
}
static void AUDIO_CODEC_SIZE_OPT __attribute__((noinline, unused)) audio_decoder_run_p1(AudioCodecRun *d, volatile u16 *out, u32 samples)
{
    audio_decoder_blocks(d, out, samples, 1);
}
static void AUDIO_CODEC_SIZE_OPT __attribute__((noinline, unused)) audio_decoder_run_p2(AudioCodecRun *d, volatile u16 *out, u32 samples)
{
    audio_decoder_blocks(d, out, samples, 2);
}

/* Decode `samples` (a multiple of 8) into out as packed halfwords: sample 2j in the low
 * byte of out[j], sample 2j+1 in the high byte (little-endian PCM order, VRAM-safe). */
static void __attribute__((unused, noinline)) audio_decoder_run(AudioDecoder *d, volatile u16 *out, u32 samples)
{
    u32 state = d->state, x = d->x, pred = state >> 10;
    AudioCodecRun r;
    r.next = d->next;
    r.acc = d->acc;
    r.nb = state & 63u;
    r.k = state >> 6 & 15u;
    r.x1 = (u32)((s32)(x << 24) >> 24);
    r.d = r.x1 - (u32)((s32)(x << 16) >> 24);
    if (pred == 0) audio_decoder_run_p0(&r, out, samples);
    else if (pred == 1) audio_decoder_run_p1(&r, out, samples);
    else audio_decoder_run_p2(&r, out, samples);
    d->next = r.next;
    d->acc = r.acc;
    d->state = AUDIO_CODEC_STATE(r.nb, r.k, pred);
    d->x = (u16)((r.x1 & 0xffu) | ((r.x1 - r.d) & 0xffu) << 8);
}
#endif
