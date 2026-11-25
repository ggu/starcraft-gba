/* Optional private check (part of make test-audio-bank): play the whole
 * generated bank through the mixer and compare every frame it writes with a
 * plain model.
 *
 *   test-audio-bank-mix BLOB
 *
 * The model decodes each stream bit by bit, straight from the format's
 * description, and a frame of the mix is the clamped sum of the voices'
 * samples at their positions; a music frame is the track's samples. Voice
 * assignment is the mixer's own (audio_mixer_service is run first and the
 * voice table read), since only decoding, mixing and sequencing are under
 * test here.
 *
 * Part 1 plays every clip to its end, one to four at a time, while every
 * track plays through once. Part 2 issues pseudo-random requests of every
 * class, so voices are stolen and cut short, and changes looping tracks.
 * A digest of every frame written is printed for comparing two builds.
 *
 * Built with -DSC64_AUDIO_IMPORTED -I$(SC64_AUDIO_DIR), like test-audio-bank.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio_mixer.h"
#include "sc64_audio_generated.h"

_Static_assert(SC64_AUDIO_FORMAT == AUDIO_CODEC_FORMAT, "bank and decoder formats differ");
_Static_assert(SC64_AUDIO_SAMPLES_PER_FRAME == AUDIO_FRAME_SAMPLES, "bank and mixer frame sizes differ");
#define FRAME AUDIO_FRAME_SAMPLES
#define STRESS_FRAMES 30000u

static u32 bits(const u8 *s, u32 *pos, u32 n)
{
    u32 v = 0;
    for (u32 i = 0; i < n; ++i, ++*pos) v |= (u32)(s[*pos >> 3] >> (*pos & 7u) & 1u) << i;
    return v;
}

/* The format, as documented: a 2-bit predictor, then blocks of a width
 * header and eight residuals. */
static s8 *model_decode(const u8 *s, u32 n)
{
    s8 *pcm = malloc(n ? n : 1u);
    if (!pcm) abort();
    u32 pos = 0, pred = bits(s, &pos, 2), k = 0;
    int x1 = 0, x2 = 0;
    for (u32 at = 0; at < n; at += AUDIO_CODEC_BLOCK) {
        if (bits(s, &pos, 1)) {
            if (!bits(s, &pos, 1)) k -= 1u;
            else if (!bits(s, &pos, 1)) k += 1u;
            else k = bits(s, &pos, 4);
        }
        if (k > 8u) { fprintf(stderr, "FAIL audio bank mix: invalid width in a stream\n"); exit(1); }
        for (u32 i = 0; i < AUDIO_CODEC_BLOCK; ++i) {
            int r = k ? (int)bits(s, &pos, k) : 0;
            if (k && r >= 1 << (k - 1u)) r -= 1 << k;
            int v = pred == 0 ? x1 : pred == 1 ? 2 * x1 - x2 : 3 * x1 - x2;
            if (pred == 2) v = v >= 0 ? v / 2 : -((-v + 1) / 2);
            int x = ((v + r + 128) & 255) - 128;
            pcm[at + i] = (s8)x;
            x2 = x1;
            x1 = x;
        }
    }
    return pcm;
}

static u8 *read_file(const char *path, u32 *size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    u8 *data = len > 0 ? aligned_alloc(16, ((size_t)len + 15u) & ~(size_t)15u) : NULL;
    if (!data || fread(data, 1, (size_t)len, f) != (size_t)len) { fclose(f); free(data); return NULL; }
    fclose(f);
    *size = (u32)len;
    return data;
}

static s8 *clip_pcm[SC64_AUDIO_SAMPLE_COUNT], *track_pcm[SC64_AUDIO_MUSIC_TRACKS];
static u32 clip_len[SC64_AUDIO_SAMPLE_COUNT];
static _Alignas(4) u16 out[FRAME / 2], music_out[FRAME / 2], scratch[AUDIO_VOICES * FRAME / 2];
static AudioDecoder decoders[AUDIO_DECODERS];
static AudioMixer m;
static AudioBank bank;
static unsigned long long digest = 1469598103934665603ull, frames, voice_frames[AUDIO_VOICES + 1], music_frames_played, clipped;

static void digest_add(const void *p, u32 n)
{
    const u8 *b = p;
    for (u32 i = 0; i < n; ++i) digest = (digest ^ b[i]) * 1099511628211ull;
}

static int clamp8(int s) { return s < -128 ? -128 : s > 127 ? 127 : s; }

/* One frame through the mixer, checked against the model. silent mirrors
 * a caller with one output buffer. */
static int frame(void)
{
    static u8 silent;
    int want[FRAME] = { 0 };
    u32 active = 0;
    audio_mixer_service(&m, &bank);
    for (u32 v = 0; v < AUDIO_VOICES; ++v) {
        u32 left = m.voice[v].remaining, sound = m.voice[v].sound;
        if (!left) continue;
        ++active;
        for (u32 i = 0; i < FRAME && i < left; ++i) want[i] += clip_pcm[sound][clip_len[sound] - left + i];
    }
    ++voice_frames[active];
    /* Poison what must be written; a silent buffer may be left alone. */
    if (!(silent && !active)) memset(out, 0x5a, sizeof out);
    memset(music_out, 0x5a, sizeof music_out);
    int music = audio_mixer_frame(&m, &bank, (volatile u16 *)out, (volatile u16 *)scratch, (volatile u16 *)music_out, &silent);
    const s8 *got = (const s8 *)out, *got_music = (const s8 *)music_out;  /* little-endian host */
    for (u32 i = 0; i < FRAME; ++i) {
        if (want[i] != clamp8(want[i])) ++clipped;
        if (got[i] != clamp8(want[i])) {
            fprintf(stderr, "FAIL audio bank mix: frame %llu sample %u is %d, model %d (%u voices)\n", frames, i, got[i], clamp8(want[i]), active);
            return 0;
        }
    }
    if (silent != !active) { fprintf(stderr, "FAIL audio bank mix: frame %llu silence flag\n", frames); return 0; }
    if (m.music_track != AUDIO_MUSIC_NONE) {
        if (m.music_track >= SC64_AUDIO_MUSIC_TRACKS || m.music_frame >= sc64_audio_music_frames[m.music_track] ||
            memcmp(got_music, track_pcm[m.music_track] + m.music_frame * FRAME, FRAME)) {
            fprintf(stderr, "FAIL audio bank mix: frame %llu, track %u frame %u\n", frames, m.music_track, m.music_frame);
            return 0;
        }
        ++music_frames_played;
        digest_add(music_out, FRAME);
    } else {
        for (u32 i = 0; i < FRAME; ++i)
            if (got_music[i] != (music == AUDIO_MUSIC_SILENCE ? 0 : 0x5a)) {
                fprintf(stderr, "FAIL audio bank mix: frame %llu, stopped music wrote %d\n", frames, got_music[i]);
                return 0;
            }
    }
    digest_add(out, FRAME);
    ++frames;
    return 1;
}

static u32 rng_state = 0x1f123bb5u;
static u32 rng(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "usage: %s BLOB\n", argv[0]); return 2; }
    u32 size;
    u8 *blob = read_file(argv[1], &size);
    if (!blob) { fprintf(stderr, "test-audio-bank-mix: cannot read the blob\n"); return 2; }
    if (size != SC64_AUDIO_BLOB_BYTES) { fprintf(stderr, "test-audio-bank-mix: blob size differs from the header\n"); return 1; }
    bank = (AudioBank){ blob, sc64_audio_sample_offset, sc64_audio_sample_length, SC64_AUDIO_SAMPLE_COUNT,
                        sc64_audio_music_offset, sc64_audio_music_frames, SC64_AUDIO_MUSIC_TRACKS };
    u32 tracks = 0;
    for (u32 i = 0; i < SC64_AUDIO_SAMPLE_COUNT; ++i) {
        clip_len[i] = sc64_audio_sample_length[i] & ~(AUDIO_CODEC_BLOCK - 1u);
        clip_pcm[i] = model_decode(blob + sc64_audio_sample_offset[i], clip_len[i]);
    }
    for (u32 t = 0; t < SC64_AUDIO_MUSIC_TRACKS; ++t)
        if (sc64_audio_music_frames[t]) { track_pcm[t] = model_decode(blob + sc64_audio_music_offset[t], sc64_audio_music_frames[t] * FRAME); ++tracks; }

    /* Part 1: every clip in full, one to four at a time, never stolen (a
     * clip starts only on a free voice); every track once. */
    audio_mixer_reset(&m, decoders);
    u32 clip = 0, track = 0, started = 0;
    for (;;) {
        u32 active = 0, effect_free = 0;
        for (u32 v = 0; v < AUDIO_VOICES; ++v) {
            if (m.voice[v].remaining) ++active;
            else if (v != AUDIO_SPEECH_VOICE) ++effect_free;
        }
        u32 target = 1u + clip / 24u % AUDIO_VOICES;
        while (clip < SC64_AUDIO_SAMPLE_COUNT && active < target) {
            if (!clip_len[clip]) { ++clip; continue; }
            if (!m.voice[AUDIO_SPEECH_VOICE].remaining && (clip & 1u || !effect_free)) {
                audio_mixer_request(&m, (u16)clip, 1, AUDIO_CLASS_SPEECH);
            } else if (effect_free) {
                audio_mixer_request(&m, (u16)clip, 1, AUDIO_CLASS_EFFECT);
                --effect_free;
            } else break;
            ++clip; ++active; ++started;
            audio_mixer_service(&m, &bank);
        }
        while (track < SC64_AUDIO_MUSIC_TRACKS && !sc64_audio_music_frames[track]) ++track;
        if (m.music_track == AUDIO_MUSIC_NONE && track < SC64_AUDIO_MUSIC_TRACKS) audio_mixer_music(&m, (u8)track++, 0);
        if (!active && clip >= SC64_AUDIO_SAMPLE_COUNT && m.music_track == AUDIO_MUSIC_NONE && m.music_request == AUDIO_MUSIC_NONE) break;
        if (!frame()) return 1;
    }
    if (m.dropped) { fprintf(stderr, "FAIL audio bank mix: %u requests dropped in part 1\n", m.dropped); return 1; }
    unsigned long long part1 = frames, part1_music = music_frames_played, all_music = 0;
    for (u32 t = 0; t < SC64_AUDIO_MUSIC_TRACKS; ++t) all_music += sc64_audio_music_frames[t];
    if (part1_music != all_music) { fprintf(stderr, "FAIL audio bank mix: %llu of %llu music frames played\n", part1_music, all_music); return 1; }

    /* Part 2: random requests of every class and looping track changes. */
    for (u32 f = 0; f < STRESS_FRAMES; ++f) {
        u32 r = rng();
        for (u32 q = r & 3u; q; --q) {
            u32 pick = rng(), kind = pick >> 16 & 15u;
            audio_mixer_request(&m, (u16)(pick % SC64_AUDIO_SAMPLE_COUNT), (u8)(pick >> 24 & 3u),
                                kind < 5u ? AUDIO_CLASS_SPEECH : kind == 5u ? AUDIO_CLASS_ADVISOR : AUDIO_CLASS_EFFECT);
        }
        if ((r >> 8 & 1023u) == 0) {
            u32 pick = rng() % (SC64_AUDIO_MUSIC_TRACKS + 2u);
            audio_mixer_music(&m, pick < SC64_AUDIO_MUSIC_TRACKS ? (u8)pick : AUDIO_MUSIC_STOP, (u8)(pick & 1u));
        }
        if (!frame()) return 1;
    }
    printf("audio bank mix: %u clips and %u tracks in %llu frames, then %u random frames; voices per frame 0:%llu 1:%llu 2:%llu 3:%llu 4:%llu; %llu clamped samples; digest %016llx\n",
           started, tracks, part1, STRESS_FRAMES, voice_frames[0], voice_frames[1], voice_frames[2], voice_frames[3], voice_frames[4], clipped, digest);
    if (!voice_frames[2] || !voice_frames[3] || !voice_frames[4] || !clipped) { fprintf(stderr, "FAIL audio bank mix: a voice count or clamping was never exercised\n"); return 1; }
    puts("audio bank mix: every frame matches the model");
    return 0;
}
