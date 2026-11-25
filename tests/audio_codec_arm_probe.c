/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * ARM probe for the two hand-written routines of the sampled-audio
 * interrupt: audio_decoder_linear, the IWRAM stream decoder
 * (common/audio_codec.h), and audio_mixer_sum, the voice sum
 * (common/audio_mixer.h). Host tests run their C references; this ROM runs
 * both on the ARM7TDMI and compares them.
 *
 * - Every stream of the linked bank is decoded twice, frame by frame as the
 *   mixer does, by the hand-written routine and by the C reference. Each
 *   frame uses the next of the ten output layouts in rotation (packed, and
 *   each column of 2, 3 and 4 wide tables), so a stream of ten frames or
 *   more is seen in all of them. The whole tables and the decoder states
 *   must be equal after every frame.
 * - A two-word running sum of the decoded samples (clips in index order,
 *   then tracks in index order) is printed; `make test-audio-bank` prints
 *   the same sum from the host decode that reproduces the manifest hash.
 * - Pseudo-random words are decoded the same way. Any bit sequence is a
 *   stream, so these reach what the pack tool never writes: reserved width
 *   codes, width 0 on a moving second-order stream, long runs of 32-bit
 *   fields. The last two spell out width 8 in every block, the most bits a
 *   stream can take, and no stream pointer may pass the end of the words.
 * - The voice sum runs on pseudo-random tables for 2, 3 and 4 voices, half
 *   of them built from extreme bytes so that lanes clip in both directions.
 *
 * It replaces common/main.c and common/gba.c: no interrupt runs. Results go
 * to the mGBA debug log (run with -l 7), ending in PASS or FAIL, and to
 * audio_probe_result for a debugger. Emulator evidence of arithmetic only:
 * it says nothing about timing or what a person hears.
 */
#define AUDIO_CODEC_REFERENCE
#include "audio_mixer.h"
#include "sc64_audio_generated.h"

_Static_assert(SC64_AUDIO_FORMAT == AUDIO_CODEC_FORMAT, "bank and decoder formats differ");
extern const u8 sc64_audio_blob[];

#define DBG_ENABLE (*(volatile u16 *)0x04FFF780)
#define DBG_FLAGS (*(volatile u16 *)0x04FFF700)
#define DBG_TEXT ((volatile char *)0x04FFF600)
#define FRAME AUDIO_FRAME_SAMPLES
#define SUM_WORDS (FRAME / 4u)
#define SUM_TABLES 3000u
#define RANDOM_STREAMS 400u
#define RANDOM_WORDS 512u
/* A block takes at most 7 header and 64 residual bits. */
#define RANDOM_SAMPLES ((RANDOM_WORDS * 32u - 2u) / 71u * AUDIO_CODEC_BLOCK / FRAME * FRAME)

/* 0: running, 1: pass, 2: fail; streams, linear streams, samples, sum low,
 * sum high, decoder mismatches (bank and random), mixer mismatches. */
volatile u32 audio_probe_result[8];

static char line[200];
static unsigned line_len;
static _Alignas(4) u32 table_asm[AUDIO_VOICES * SUM_WORDS], table_ref[AUDIO_VOICES * SUM_WORDS];
static _Alignas(4) u32 sum_asm[SUM_WORDS], sum_ref[SUM_WORDS];
static _Alignas(4) u32 random_words[RANDOM_WORDS];

static void out_text(const char *s) { while (*s && line_len < sizeof(line) - 1) line[line_len++] = *s++; }
static void out_num(u32 v)
{
    char tmp[11]; int n = 0;
    do { tmp[n++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (n) { char c[2] = { tmp[--n], 0 }; out_text(c); }
}
static void out_hex(u32 v)
{
    for (int i = 28; i >= 0; i -= 4) { char c[2] = { "0123456789abcdef"[v >> i & 15u], 0 }; out_text(c); }
}
static void out_flush(void)
{
    for (unsigned i = 0; i < line_len; ++i) DBG_TEXT[i] = line[i];
    DBG_TEXT[line_len] = 0;
    DBG_FLAGS = 0x100 | 2;
    line_len = 0;
}

static u32 rng_state = 0x2545f491u;
static u32 rng(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

/* A breakpoint target for debuggers; the result is final here. */
__attribute__((noinline)) void audio_probe_done(void) { __asm__ volatile(""); }

static u32 sum_low, sum_high, decoder_bad, samples_done, layouts_used;
/* When set, no decoder may leave its stream pointer past this address. */
static const u8 *stream_limit;

/* Both decoders over one stream, frame by frame. */
static void decode_stream(u32 stream, const u8 *start, u32 samples)
{
    AudioDecoder a, r;
    audio_decoder_start(&a, start);
    audio_decoder_start(&r, start);
    for (u32 at = 0, frame = 0; at < samples; at += FRAME, ++frame) {
        u32 n = samples - at < FRAME ? samples - at : FRAME;
        /* The ten layouts in turn: packed, then each column of each table. */
        static const u8 layout[10][2] = { { 1, 0 }, { 2, 0 }, { 2, 1 }, { 3, 0 }, { 3, 1 }, { 3, 2 }, { 4, 0 }, { 4, 1 }, { 4, 2 }, { 4, 3 } };
        u32 turn = layouts_used++ % 10u, columns = layout[turn][0], column = layout[turn][1];
        audio_decoder_linear(&a, (volatile u16 *)(table_asm + column), n, columns);
        audio_decoder_linear_reference(&r, (volatile u16 *)(table_ref + column), n, columns);
        u32 bad = a.next != r.next || a.acc != r.acc || a.state != r.state || a.x != r.x;
        if (stream_limit && a.next > stream_limit) bad = 1;
        for (u32 i = 0; i < AUDIO_VOICES * SUM_WORDS; ++i) bad |= table_asm[i] != table_ref[i];
        if (bad && !decoder_bad++) {
            out_text("audio arm probe: decoder differs, stream "); out_num(stream);
            out_text(" sample "); out_num(at); out_text(" columns "); out_num(columns); out_flush();
        }
        for (u32 j = 0; j < n / 4u; ++j) { sum_low += table_asm[columns * j + column]; sum_high += sum_low; }
    }
    samples_done += samples;
}

int main(void)
{
    u32 streams = 0, linear = 0, mixer_bad = 0, clipped = 0;
    DBG_ENABLE = 0xC0DE;
    for (u32 i = 0; i < AUDIO_VOICES * SUM_WORDS; ++i) table_asm[i] = table_ref[i] = 0x5a5a5a5au;

    for (u32 i = 0; i < SC64_AUDIO_SAMPLE_COUNT + SC64_AUDIO_MUSIC_TRACKS; ++i) {
        u32 clip = i < SC64_AUDIO_SAMPLE_COUNT, t = i - SC64_AUDIO_SAMPLE_COUNT;
        u32 samples = clip ? sc64_audio_sample_length[i] & ~(AUDIO_CODEC_BLOCK - 1u) : sc64_audio_music_frames[t] * FRAME;
        const u8 *start = sc64_audio_blob + (clip ? sc64_audio_sample_offset[i] : sc64_audio_music_offset[t]);
        if (!samples) continue;
        ++streams;
        /* The other predictor has no hand-written routine. */
        if ((*(const AudioCodecWord *)(const void *)start & 3u) >= 2u) continue;
        ++linear;
        decode_stream(i, start, samples);
    }
    out_text("audio arm probe: decoder streams="); out_num(streams); out_text(" linear="); out_num(linear);
    out_text(" samples="); out_num(samples_done); out_text(" sum="); out_hex(sum_low); out_text(" "); out_hex(sum_high);
    out_text(" mismatches="); out_num(decoder_bad); out_flush();

    u32 bank_bad = decoder_bad, bank_samples = samples_done, keep_low = sum_low, keep_high = sum_high;
    for (u32 round = 0; round < RANDOM_STREAMS; ++round) {
        for (u32 i = 0; i < RANDOM_WORDS; ++i) {
            u32 w = rng();
            if (round & 2u) w &= rng();          /* fewer set bits: widths persist */
            if ((round & 12u) == 12u) w &= rng();
            random_words[i] = w;
        }
        random_words[0] = (random_words[0] & ~3u) | (round & 1u);  /* the predictor */
        if (round >= RANDOM_STREAMS - 2u) {
            /* The longest a stream can be: every block spells its width out
             * as 8 ("111", then 0001 lowest bit first) before 64 residual
             * bits. RANDOM_SAMPLES of it must fit the buffer. */
            u32 at = 2;
            for (u32 block = 0; block < RANDOM_SAMPLES / AUDIO_CODEC_BLOCK; ++block, at += 71u)
                for (u32 bit = 0; bit < 7u; ++bit) {
                    u32 mask = 1u << ((at + bit) & 31u), *w = &random_words[(at + bit) >> 5];
                    *w = bit == 3u || bit == 4u || bit == 5u ? *w & ~mask : *w | mask;
                }
        }
        stream_limit = (const u8 *)(random_words + RANDOM_WORDS);
        decode_stream(SC64_AUDIO_SAMPLE_COUNT + SC64_AUDIO_MUSIC_TRACKS + round, (const u8 *)random_words, RANDOM_SAMPLES);
        stream_limit = 0;
    }
    out_text("audio arm probe: random streams="); out_num(RANDOM_STREAMS); out_text(" samples="); out_num(samples_done - bank_samples);
    out_text(" mismatches="); out_num(decoder_bad - bank_bad); out_flush();
    samples_done = bank_samples; sum_low = keep_low; sum_high = keep_high;

    for (u32 round = 0; round < SUM_TABLES; ++round) {
        u32 n = 2u + round % 3u;
        for (u32 i = 0; i < n * SUM_WORDS; ++i) {
            u32 w = rng();
            if (round & 1u) {
                /* Mostly extreme bytes: 0x00, 0x01, 0x7f, 0x80, 0xfe, 0xff. */
                static const u8 edge[8] = { 0x00, 0x01, 0x7f, 0x80, 0xfe, 0xff, 0x00, 0xff };
                u32 pick = rng();
                w = (u32)edge[pick & 7u] | (u32)edge[pick >> 3 & 7u] << 8 | (u32)edge[pick >> 6 & 7u] << 16 | (u32)edge[pick >> 9 & 7u] << 24;
            }
            table_asm[i] = w;
        }
        audio_mixer_sum((volatile u32 *)sum_asm, table_asm, n, SUM_WORDS);
        audio_mixer_sum_reference((volatile u32 *)sum_ref, table_asm, n, SUM_WORDS);
        u32 bad = 0;
        for (u32 j = 0; j < SUM_WORDS; ++j) {
            bad |= sum_asm[j] != sum_ref[j];
            /* A clamped sample is 0x7f or 0x80; count words holding one. */
            u32 hi = (sum_ref[j] ^ 0x7f7f7f7fu), lo = (sum_ref[j] ^ 0x80808080u);
            clipped += (((hi - 0x01010101u) & ~hi) | ((lo - 0x01010101u) & ~lo)) & 0x80808080u ? 1u : 0u;
        }
        if (bad && !mixer_bad++) { out_text("audio arm probe: voice sum differs, table "); out_num(round); out_flush(); }
    }
    /* No words: nothing is read or written. */
    sum_asm[0] = 0x5a5a5a5au;
    audio_mixer_sum((volatile u32 *)sum_asm, table_asm, 4, 0);
    if (sum_asm[0] != 0x5a5a5a5au && !mixer_bad++) { out_text("audio arm probe: voice sum wrote for no words"); out_flush(); }
    out_text("audio arm probe: mixer tables="); out_num(SUM_TABLES); out_text(" words_at_a_limit="); out_num(clipped);
    out_text(" mismatches="); out_num(mixer_bad); out_flush();

    u32 ok = streams && linear && !decoder_bad && !mixer_bad && clipped;
    audio_probe_result[1] = streams; audio_probe_result[2] = linear; audio_probe_result[3] = samples_done;
    audio_probe_result[4] = sum_low; audio_probe_result[5] = sum_high;
    audio_probe_result[6] = decoder_bad; audio_probe_result[7] = mixer_bad;
    audio_probe_result[0] = ok ? 1u : 2u;
    out_text(ok ? "audio arm probe: PASS" : "audio arm probe: FAIL"); out_flush();
    audio_probe_done();
    for (;;) { }
}
