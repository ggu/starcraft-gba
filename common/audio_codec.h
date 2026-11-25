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
 * variable-length loop. Streams start word aligned and are padded to whole
 * words, so the halfword stream is read a word (two halfwords) at a time; a
 * word is fetched only when the next field needs one of its bits, so a run
 * never reads past the stream's padded end. Header bits are taken one at a
 * time. Streams come from the pack tool, which verifies them; reserved
 * codes are not rejected here, only kept memory-safe (an invalid width
 * decodes as 8). Output is written as packed halfwords, the only store size
 * VRAM accepts.
 *
 * There are two decoders. Streams with a linear predictor (P = 0 or 1, all
 * the pack tool writes by default) go through audio_decoder_linear(), one
 * small loop for every width that ROMs run as ARM code from IWRAM. P = 2
 * keeps a state machine with an unrolled block body per width and constant
 * shifts, sized for Thumb-1 code running from ROM, where residuals are taken
 * one word per block (k <= 4) or two (k >= 5).
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

/* The third predictor, (3*x1 - x2) >> 1, keeps the state machine above, as
 * Thumb code in ROM: the pack tool does not use it by default and no stream
 * of the current bank does. GCC's -Os lays it out with fewer instructions
 * per sample than -O2. */
#if defined(__GNUC__) && !defined(__clang__)
#define AUDIO_CODEC_SIZE_OPT __attribute__((optimize("Os")))
#else
#define AUDIO_CODEC_SIZE_OPT
#endif
static void AUDIO_CODEC_SIZE_OPT __attribute__((noinline, unused)) audio_decoder_run_p2(AudioCodecRun *d, volatile u16 *out, u32 samples)
{
    audio_decoder_blocks(d, out, samples, 2);
}

/* The two linear predictors (every stream of the default bank) share one
 * compact loop that takes the width as a variable. Samples are sums modulo
 * 256, so the registers run unwrapped and only low bytes are stored. Each
 * block's residuals are taken as two fields of 4k bits (at most one word
 * each); a residual is rotated to the top of its field word and added with
 * an arithmetic shift. Words are fetched exactly when the state machine
 * above fetches them, so the two may continue each other's streams.
 *
 * columns is 1 for packed output as above. With 2 to 4 the run writes one
 * column of a table that interleaves that many streams word by word: its
 * word j (samples 4j..4j+3) goes to out + 2 * columns * j halfwords. The
 * mixer sums such a table with one pointer.
 *
 * ROMs that link the bank run the loop as ARM code from IWRAM, where an
 * instruction costs one cycle instead of the two a Thumb instruction costs
 * from ROM, and a rotate or an add with a shifted operand is one
 * instruction. Its body is written by hand: the compiler's version of the
 * C below needs twice the code and spills half its values. The C function
 * is the reference and runs in host builds; tests/audio_codec_arm_probe.c
 * compares the two on the whole bank. samples is a nonzero multiple of 8
 * there.
 *
 * Only ROMs that link the bank get the hand-written code, so every other
 * object, the default ROM's included, is what it was without it. */
#if defined(__arm__) && !defined(GBA_HOST) && defined(SC64_AUDIO_IMPORTED)
#define AUDIO_CODEC_HAND_WRITTEN 1
#endif
#ifdef AUDIO_CODEC_HAND_WRITTEN
/*   r0  bits in half a block (4 * width)   r7  x1 (low byte)
 *   r1  out                                r8  x1 - x2 (low byte)
 *   r2  end of this run's output           r9  residual word
 *   r3  next stream word                   r10 32 - width
 *   r4  bit accumulator                    r11 predictor (0x400), half (bit 31)
 *   r5  bits held in it                    r12 scratch
 *   r6  width                              lr  4 * columns - 2
 *
 * Width 0 needs no case of its own: the block takes no bits, the top bit of
 * the accumulator is clear after any header bit, and so every residual is
 * that word rotated by 0 and shifted right by 32, which is 0. Shifts by 32
 * also end a field that uses up a word. */
#define AUDIO_CODEC_ASM_BIT                                                     \
    "    subs    r5, r5, #1\n"                                                  \
    "    ldrmi   r4, [r3], #4\n"                                                \
    "    movmi   r5, #31\n"                                                     \
    "    movs    r4, r4, lsr #1\n"
#define AUDIO_CODEC_ASM_FIRST(step)             /* x1 += r, twice, and store */ \
    "    mov     r9, r9, ror r6\n"                                              \
    "    add     r7, r7, r9, asr r10\n"                                         \
    "    and     r12, r7, #255\n"                                               \
    "    mov     r9, r9, ror r6\n"                                              \
    "    add     r7, r7, r9, asr r10\n"                                         \
    "    orr     r12, r12, r7, lsl #8\n"                                        \
    "    strh    r12, [r1], " step "\n"
#define AUDIO_CODEC_ASM_SECOND(step)            /* d += r, x1 += d */           \
    "    mov     r9, r9, ror r6\n"                                              \
    "    add     r8, r8, r9, asr r10\n"                                         \
    "    add     r7, r7, r8\n"                                                  \
    "    and     r12, r7, #255\n"                                               \
    "    mov     r9, r9, ror r6\n"                                              \
    "    add     r8, r8, r9, asr r10\n"                                         \
    "    add     r7, r7, r8\n"                                                  \
    "    orr     r12, r12, r7, lsl #8\n"                                        \
    "    strh    r12, [r1], " step "\n"
static void __attribute__((noinline, noclone, unused, section(".iwram"), long_call, target("arm")))
audio_decoder_linear(AudioDecoder *dec, volatile u16 *out, u32 samples, u32 columns)
{
    register volatile u16 *o __asm__("r1") = out;
    register u32 end __asm__("r2") = (u32)out + samples * columns;
    register const AudioCodecWord *p __asm__("r3") = (const AudioCodecWord *)(const void *)dec->next;
    register u32 acc __asm__("r4") = dec->acc;
    /* state in the low half, x in the high half; both come back that way. */
    register u32 state __asm__("r5") = *(const AudioCodecWord *)(const void *)&dec->state;
    register u32 step __asm__("lr") = 4u * columns - 2u;
    __asm__ volatile (
        "    mov     r12, r5\n"
        "    mov     r7, r12, lsr #16\n"
        "    sub     r8, r7, r7, lsr #8\n"
        "    and     r5, r12, #63\n"
        "    and     r11, r12, #0x400\n"
        "    mov     r6, r12, lsl #22\n"
        "    mov     r6, r6, lsr #28\n"
        "    rsb     r10, r6, #32\n"
        "    mov     r0, r6, lsl #2\n"
        ".Laudio_block:\n"                      /* header: "0" keeps the width */
        AUDIO_CODEC_ASM_BIT
        "    bcs     .Laudio_changed\n"
        ".Laudio_half:\n"                       /* four residuals: 4 * width bits */
        "    subs    r5, r5, r0\n"
        "    bmi     .Laudio_refill\n"
        "    mov     r9, r4\n"
        "    mov     r4, r4, lsr r0\n"
        ".Laudio_taken:\n"
        "    tst     r11, #0x400\n"
        "    bne     .Laudio_second\n"
        AUDIO_CODEC_ASM_FIRST("#2")
        AUDIO_CODEC_ASM_FIRST("lr")
        "    eors    r11, r11, #0x80000000\n"
        "    bmi     .Laudio_half\n"
        "    cmp     r1, r2\n"
        "    bne     .Laudio_block\n"
        "    b       .Laudio_done\n"
        ".Laudio_second:\n"
        AUDIO_CODEC_ASM_SECOND("#2")
        AUDIO_CODEC_ASM_SECOND("lr")
        "    eors    r11, r11, #0x80000000\n"
        "    bmi     .Laudio_half\n"
        "    cmp     r1, r2\n"
        "    bne     .Laudio_block\n"
        "    b       .Laudio_done\n"
        ".Laudio_refill:\n"                     /* r5 = held - wanted < 0 */
        "    ldr     r12, [r3], #4\n"
        "    add     r5, r5, r0\n"
        "    orr     r9, r4, r12, lsl r5\n"
        "    sub     r5, r0, r5\n"              /* bits used of the new word: 1..32 */
        "    mov     r4, r12, lsr r5\n"
        "    rsb     r5, r5, #32\n"
        "    b       .Laudio_taken\n"
        ".Laudio_changed:\n"                    /* "10" one less, "110" one more */
        AUDIO_CODEC_ASM_BIT
        "    subcc   r6, r6, #1\n"
        "    bcc     .Laudio_width\n"
        AUDIO_CODEC_ASM_BIT
        "    addcc   r6, r6, #1\n"
        "    bcc     .Laudio_width\n"
        "    mov     r6, #8\n"                  /* "111": four bits, lowest first */
        ".Laudio_explicit:\n"
        AUDIO_CODEC_ASM_BIT
        "    movs    r6, r6, rrx\n"             /* the marker bit ends the loop */
        "    bcc     .Laudio_explicit\n"
        "    mov     r6, r6, lsr #28\n"
        ".Laudio_width:\n"
        "    cmp     r6, #8\n"
        "    movhi   r6, #8\n"                  /* an invalid width decodes as 8 */
        "    rsb     r10, r6, #32\n"
        "    mov     r0, r6, lsl #2\n"
        "    b       .Laudio_half\n"
        ".Laudio_done:\n"
        "    orr     r5, r5, r6, lsl #6\n"
        "    orr     r5, r5, r11\n"
        "    sub     r8, r7, r8\n"              /* x2 */
        "    and     r7, r7, #255\n"
        "    and     r8, r8, #255\n"
        "    orr     r7, r7, r8, lsl #8\n"
        "    orr     r5, r5, r7, lsl #16\n"
        : "+r"(o), "+r"(p), "+r"(acc), "+r"(state)
        : "r"(end), "r"(step)
        : "r0", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "cc", "memory");
    dec->next = (const u8 *)(const void *)p;
    dec->acc = acc;
    *(AudioCodecWord *)(void *)&dec->state = state;
}
#undef AUDIO_CODEC_ASM_SECOND
#undef AUDIO_CODEC_ASM_FIRST
#undef AUDIO_CODEC_ASM_BIT
#endif
#if !defined(AUDIO_CODEC_HAND_WRITTEN) || defined(AUDIO_CODEC_REFERENCE)
#ifdef AUDIO_CODEC_HAND_WRITTEN
#define AUDIO_CODEC_LINEAR audio_decoder_linear_reference
#else
#define AUDIO_CODEC_LINEAR audio_decoder_linear
#endif
/* The next header bit (nonzero when set). */
#define AUDIO_CODEC_BIT(bit)                                                    \
    do {                                                                        \
        if (!nb) { acc = *p++; nb = 32u; }                                      \
        nb -= 1u;                                                               \
        (bit) = acc & 1u;                                                       \
        acc >>= 1;                                                              \
    } while (0)
/* One sample of a block: the next kk bits of w are its residual. */
#define AUDIO_CODEC_NEXT()                                                      \
    do {                                                                        \
        w = w >> kk | w << sh;                                                  \
        u32 r = (u32)((s32)w >> sh);                                            \
        if (second) { d += r; x += d; } else x += r;                            \
    } while (0)
static void __attribute__((noinline, unused)) AUDIO_CODEC_LINEAR(AudioDecoder *dec, volatile u16 *out, u32 samples, u32 columns)
{
    const AudioCodecWord *p = (const AudioCodecWord *)(const void *)dec->next;
    u32 acc = dec->acc, state = dec->state, x = dec->x;
    u32 nb = state & 63u, k = state >> 6 & 15u;
    const u32 second = state >> 10 & 1u;  /* 0: x1, 1: 2*x1 - x2 */
    u32 d = x - (x >> 8);                 /* x1 - x2 */
    const u32 stride = 2u * columns;      /* halfwords between a column's words */
    /* at is this run's halfword index: no pointer is formed past the table. */
    for (u32 at = 0, last = samples / 4u * stride; at != last; ) {
        /* Header: "0" same width, "10" one less, "110" one more, "111" and
         * four bits (lowest first) explicit. An invalid width decodes as 8. */
        u32 bit;
        AUDIO_CODEC_BIT(bit);
        if (bit) {
            AUDIO_CODEC_BIT(bit);
            if (!bit) k -= 1u;
            else {
                AUDIO_CODEC_BIT(bit);
                if (!bit) k += 1u;
                else {
                    k = 0;
                    for (u32 i = 0; i < 4u; ++i) { AUDIO_CODEC_BIT(bit); k |= bit << i; }
                }
            }
            if (k > 8u) k = 8u;
        }
        /* Two halves of four residuals. With k == 0 nothing is taken and the
         * zero word decodes as zeros: the first-order sample repeats and the
         * second-order one keeps moving by d. */
        const u32 n = 4u * k, kk = k ? k : 1u, sh = 32u - kk;
        u32 w = 0;
        for (u32 half = 0; half < 2u; ++half) {
            if (n) {
                if (nb >= n) {
                    w = acc;
                    acc = acc >> (n - 1u) >> 1;
                    nb -= n;
                } else {
                    u32 h = *p++;
                    w = acc | h << nb;
                    acc = h >> (n - 1u - nb) >> 1;
                    nb += 32u - n;
                }
            }
            u32 a;
            AUDIO_CODEC_NEXT(); a = x & 0xffu;
            AUDIO_CODEC_NEXT(); out[at] = (u16)(a | x << 8);
            AUDIO_CODEC_NEXT(); a = x & 0xffu;
            AUDIO_CODEC_NEXT(); out[at + 1u] = (u16)(a | x << 8);
            at += stride;
        }
    }
    dec->next = (const u8 *)(const void *)p;
    dec->acc = acc;
    dec->state = AUDIO_CODEC_STATE(nb, k, second);
    dec->x = (u16)((x & 0xffu) | ((x - d) & 0xffu) << 8);
}
#undef AUDIO_CODEC_NEXT
#undef AUDIO_CODEC_BIT
#undef AUDIO_CODEC_LINEAR
#endif

/* The third predictor through the state machine. */
static void __attribute__((unused, noinline)) audio_decoder_run_halved(AudioDecoder *d, volatile u16 *out, u32 samples)
{
    u32 state = d->state, x = d->x;
    AudioCodecRun r;
    r.next = d->next;
    r.acc = d->acc;
    r.nb = state & 63u;
    r.k = state >> 6 & 15u;
    r.x1 = (u32)((s32)(x << 24) >> 24);
    r.d = r.x1 - (u32)((s32)(x << 16) >> 24);
    audio_decoder_run_p2(&r, out, samples);
    d->next = r.next;
    d->acc = r.acc;
    d->state = AUDIO_CODEC_STATE(r.nb, r.k, 2u);
    d->x = (u16)((r.x1 & 0xffu) | ((r.x1 - r.d) & 0xffu) << 8);
}

/* Decode `samples` (a multiple of 8) into out as packed halfwords: sample 2j in the low
 * byte of out[j], sample 2j+1 in the high byte (little-endian PCM order, VRAM-safe). */
static inline void audio_decoder_run(AudioDecoder *d, volatile u16 *out, u32 samples)
{
    if (!samples) return;
    if (d->state >> 10 < 2u) audio_decoder_linear(d, out, samples, 1u);
    else audio_decoder_run_halved(d, out, samples);
}

/* Decode `samples` (a multiple of 8) as one column of `columns` (2 to 4)
 * interleaved streams, each sample with its top bit flipped: the unsigned
 * bytes the mixer adds. The linear predictors produce them by decoding from
 * a flipped state, since a constant offset passes through both; the third
 * predictor decodes a block at a time and converts. */
static inline void audio_decoder_run_column(AudioDecoder *d, volatile u16 *out, u32 samples, u32 columns)
{
    if (!samples) return;
    if (d->state >> 10 < 2u) {
        d->x ^= 0x8080u;
        audio_decoder_linear(d, out, samples, columns);
        d->x ^= 0x8080u;
        return;
    }
    for (u32 at = 0, last = samples / 2u * columns; at != last; at += 4u * columns) {
        volatile u16 block[AUDIO_CODEC_BLOCK / 2u];
        audio_decoder_run_halved(d, block, AUDIO_CODEC_BLOCK);
        out[at] = (u16)(block[0] ^ 0x8080u); out[at + 1u] = (u16)(block[1] ^ 0x8080u);
        out[at + 2u * columns] = (u16)(block[2] ^ 0x8080u); out[at + 2u * columns + 1u] = (u16)(block[3] ^ 0x8080u);
    }
}
#endif
