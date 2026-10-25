/* Host checks for the bounded Direct Sound mixer and music sequencer. */
#include <stdio.h>
#include <string.h>
#include "audio_mixer.h"

#define CHECK(cond, msg) do { if (!(cond)) { fprintf(stderr, "FAIL audio mixer: %s (line %d)\n", msg, __LINE__); return 1; } } while (0)

/* Samples: 0 = +10 x 400, 1 = +100 x 100, 2 = -100 x 100, 3 = ramp x 8,
 * 4 = +60 x 352 (ramp padded to a word). Music: track 0 two frames, track 1 three, track 2 absent. */
static _Alignas(4) s8 blob[400 + 100 + 100 + 6 + 2 + 352 + 4 * 176];
static const u32 offsets[5] = { 0, 400, 500, 600, 608 };
static const u32 lengths[5] = { 400, 100, 100, 8, 352 };
static const u32 music_offset[3] = { 960, 960 + 2 * 176, 0 };
static const u32 music_frames[3] = { 2, 3, 0 };
static const AudioBank bank = { blob, offsets, lengths, 5, music_offset, music_frames, 3 };
static _Alignas(4) u16 out[AUDIO_FRAME_SAMPLES / 2];

static int sample_at(u32 i)
{
    return (s8)(u8)(i & 1 ? out[i / 2] >> 8 : out[i / 2]);
}

static void fill(void)
{
    memset(blob, 0, sizeof blob);
    for (int i = 0; i < 400; ++i) blob[i] = 10;
    for (int i = 0; i < 100; ++i) { blob[400 + i] = 100; blob[500 + i] = -100; }
    for (int i = 0; i < 8; ++i) blob[600 + i] = (s8)(i + 1);
    for (int i = 0; i < 352; ++i) blob[608 + i] = 60;
}

static void mix(AudioMixer *m)
{
    audio_mixer_service(m, &bank);
    audio_mixer_mix(m, (volatile u16 *)out);
}

int main(void)
{
    AudioMixer m;
    fill();

    /* Silence: an idle mixer writes a full zero frame. */
    audio_mixer_reset(&m);
    memset(out, 0x55, sizeof out);
    mix(&m);
    for (u32 i = 0; i < AUDIO_FRAME_SAMPLES; ++i) CHECK(sample_at(i) == 0, "idle frame is silent");

    /* One voice is copied verbatim; a short voice ends mid-frame in zeros. */
    audio_mixer_reset(&m);
    CHECK(audio_mixer_request(&m, 3, 1, AUDIO_CLASS_EFFECT), "request accepted");
    mix(&m);
    for (u32 i = 0; i < 8; ++i) CHECK(sample_at(i) == (int)i + 1, "single voice copied");
    for (u32 i = 8; i < AUDIO_FRAME_SAMPLES; ++i) CHECK(sample_at(i) == 0, "voice tail silent");
    CHECK(!m.voice[1].remaining, "short voice retired");

    /* Two and three voices sum with saturation; lengths advance exactly. */
    audio_mixer_reset(&m);
    audio_mixer_request(&m, 1, 1, AUDIO_CLASS_EFFECT);
    audio_mixer_request(&m, 1, 1, AUDIO_CLASS_SPEECH);
    audio_mixer_request(&m, 0, 1, AUDIO_CLASS_EFFECT);
    mix(&m);
    for (u32 i = 0; i < 100; ++i) CHECK(sample_at(i) == 127, "three-voice sum saturates high");
    for (u32 i = 100; i < AUDIO_FRAME_SAMPLES; ++i) CHECK(sample_at(i) == 10, "remaining voice continues");
    CHECK(m.voice[2].remaining == 400 - AUDIO_FRAME_SAMPLES || m.voice[1].remaining == 400 - AUDIO_FRAME_SAMPLES,
          "long voice advanced one frame");
    audio_mixer_reset(&m);
    audio_mixer_request(&m, 2, 1, AUDIO_CLASS_EFFECT);
    audio_mixer_request(&m, 2, 1, AUDIO_CLASS_SPEECH);
    mix(&m);
    for (u32 i = 0; i < 100; ++i) CHECK(sample_at(i) == -128, "two-voice sum saturates low");
    audio_mixer_reset(&m);
    audio_mixer_request(&m, 1, 1, AUDIO_CLASS_EFFECT);
    audio_mixer_request(&m, 2, 1, AUDIO_CLASS_SPEECH);
    audio_mixer_request(&m, 0, 1, AUDIO_CLASS_EFFECT);
    mix(&m);
    CHECK(sample_at(0) == 10 && sample_at(99) == 10, "three opposite voices cancel");

    /* Packed 2-4 voice sums match a scalar reference on random samples,
     * including staggered ends inside a frame. */
    {
        static _Alignas(4) s8 rnd[4096];
        static const u32 roff[4] = { 0, 1024, 2048, 3072 };
        static const u32 rlen[4] = { 520, 176, 352, 1000 };
        static const u32 none[1] = { 0 };
        const AudioBank rbank = { rnd, roff, rlen, 4, none, none, 0 };
        u32 lcg = 7;
        for (u32 i = 0; i < sizeof rnd; ++i) { lcg = lcg * 1103515245u + 12345u; rnd[i] = (s8)(lcg >> 16); }
        for (u32 voices = 2; voices <= 4; ++voices) {
            audio_mixer_reset(&m);
            audio_mixer_request(&m, 0, 1, AUDIO_CLASS_SPEECH);
            for (u32 v = 1; v < voices; ++v) audio_mixer_request(&m, (u16)v, 1, AUDIO_CLASS_EFFECT);
            audio_mixer_service(&m, &rbank);
            for (u32 f = 0; f < 4; ++f) {
                audio_mixer_mix(&m, (volatile u16 *)out);
                for (u32 i = 0; i < AUDIO_FRAME_SAMPLES; ++i) {
                    s32 ref = 0;
                    u32 at = f * AUDIO_FRAME_SAMPLES + i;
                    for (u32 v = 0; v < voices; ++v) if (at < rlen[v]) ref += rnd[roff[v] + at];
                    CHECK(sample_at(i) == audio_clip8(ref), "packed sum matches reference");
                }
            }
        }
    }

    /* A voice spanning frames plays every byte exactly once. */
    audio_mixer_reset(&m);
    audio_mixer_request(&m, 4, 1, AUDIO_CLASS_EFFECT);
    u32 total = 0;
    for (int f = 0; f < 3; ++f) {
        mix(&m);
        for (u32 i = 0; i < AUDIO_FRAME_SAMPLES; ++i) total += sample_at(i) == 60;
    }
    CHECK(total == 352, "multi-frame voice length preserved");

    /* Speech always takes voice 0 and interrupts the previous response. */
    audio_mixer_reset(&m);
    audio_mixer_request(&m, 0, 1, AUDIO_CLASS_SPEECH);
    audio_mixer_service(&m, &bank);
    audio_mixer_request(&m, 4, 0, AUDIO_CLASS_SPEECH);
    audio_mixer_service(&m, &bank);
    CHECK(m.voice[AUDIO_SPEECH_VOICE].sound == 4 && m.voice[AUDIO_SPEECH_VOICE].remaining == 352,
          "speech replaces speech");
    CHECK(!m.voice[1].remaining && !m.voice[2].remaining && !m.voice[3].remaining,
          "speech never uses effect voices");

    /* Effects fill free voices, suppress quick duplicates, then steal the
     * lowest-priority oldest voice and refuse when every voice outranks. */
    audio_mixer_reset(&m);
    audio_mixer_request(&m, 0, 1, AUDIO_CLASS_EFFECT);
    audio_mixer_request(&m, 0, 1, AUDIO_CLASS_EFFECT);
    audio_mixer_service(&m, &bank);
    CHECK(m.voice[1].remaining && !m.voice[2].remaining && m.dropped == 1, "duplicate suppressed");
    m.voice[1].age = AUDIO_RETRIGGER_FRAMES;
    audio_mixer_request(&m, 0, 1, AUDIO_CLASS_EFFECT);
    audio_mixer_service(&m, &bank);
    CHECK(m.voice[2].sound == 0 && m.voice[2].remaining, "retrigger after window");
    audio_mixer_request(&m, 4, 3, AUDIO_CLASS_EFFECT);
    audio_mixer_service(&m, &bank);
    CHECK(m.voice[3].sound == 4, "third effect voice");
    audio_mixer_request(&m, 1, 2, AUDIO_CLASS_EFFECT);
    audio_mixer_service(&m, &bank);
    CHECK(m.voice[1].sound == 1 && m.voice[2].sound == 0 && m.voice[3].sound == 4,
          "steals the oldest lowest-priority voice");
    u32 dropped = m.dropped;
    audio_mixer_request(&m, 2, 0, AUDIO_CLASS_EFFECT);
    audio_mixer_service(&m, &bank);
    CHECK(m.dropped == dropped + 1 && m.voice[1].sound == 1, "lower priority refused");
    audio_mixer_request(&m, 9, 9, AUDIO_CLASS_EFFECT);
    audio_mixer_service(&m, &bank);
    CHECK(m.dropped == dropped + 2, "invalid sample refused");

    /* The ring holds AUDIO_QUEUE-1 requests and rejects the rest. */
    audio_mixer_reset(&m);
    for (u32 i = 0; i < AUDIO_QUEUE - 1; ++i) CHECK(audio_mixer_request(&m, 3, 1, AUDIO_CLASS_EFFECT), "queue slot");
    CHECK(!audio_mixer_request(&m, 3, 1, AUDIO_CLASS_EFFECT) && m.dropped == 1, "full queue refuses");
    audio_mixer_service(&m, &bank);
    CHECK(m.head == m.tail, "service drains queue");
    for (u32 round = 0; round < 3 * AUDIO_QUEUE; ++round) {
        CHECK(audio_mixer_request(&m, 3, 1, AUDIO_CLASS_SPEECH), "ring wraps");
        audio_mixer_service(&m, &bank);
    }

    /* Music: start, count frames, loop or end, change, stop, reject absent. */
    audio_mixer_reset(&m);
    CHECK(audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_CONTINUE, "silent idle");
    audio_mixer_music(&m, 0, 1);
    CHECK(audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_RESTART, "track starts");
    CHECK(audio_mixer_music_data(&m, &bank) == blob + 960, "track data");
    CHECK(audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_CONTINUE, "frame 1");
    CHECK(audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_RESTART, "loops after two frames");
    audio_mixer_music(&m, 0, 1);
    CHECK(audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_CONTINUE, "same track continues");
    audio_mixer_music(&m, 1, 0);
    CHECK(audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_RESTART, "track change");
    CHECK(audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_CONTINUE &&
          audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_CONTINUE, "three-frame track");
    CHECK(audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_SILENCE && m.music_track == AUDIO_MUSIC_NONE,
          "one-shot ends");
    CHECK(audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_CONTINUE, "stays silent");
    audio_mixer_music(&m, 2, 1);
    CHECK(audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_CONTINUE && m.music_track == AUDIO_MUSIC_NONE,
          "absent track ignored");
    audio_mixer_music(&m, 0, 1);
    audio_mixer_music_step(&m, &bank);
    audio_mixer_music(&m, AUDIO_MUSIC_STOP, 0);
    CHECK(audio_mixer_music_step(&m, &bank) == AUDIO_MUSIC_SILENCE, "stop silences");
    CHECK(audio_mixer_frame(&m, &bank, (volatile u16 *)out) == AUDIO_MUSIC_CONTINUE && m.frames == 1,
          "frame entry point");
    puts("audio mixer checks passed");
    return 0;
}
