/* Host checks for the bounded Direct Sound mixer and music sequencer. */
#include <stdio.h>
#include <string.h>
#include "audio_mixer.h"
#include "audio_codec_ref.h"

#define CHECK(cond, msg) do { if (!(cond)) { fprintf(stderr, "FAIL audio mixer: %s (line %d)\n", msg, __LINE__); return 1; } } while (0)

/* PCM fixtures, encoded into format-2 streams by the reference encoder.
 * Samples: 0 = +10 x 400, 1 = +100 x 100, 2 = -100 x 100 (both padded to 104
 * with zeros, as the pack tool pads to 8), 3 = ramp x 8, 4 = +60 x 352,
 * 5 = noise x 184 (not a whole frame). Music: track 0 two frames, track 1
 * three, track 2 absent. */
#define FIXTURES 5
#define MUSIC_TRACKS 2
static s8 pcm[1 + FIXTURES][400];
static s8 music_pcm[MUSIC_TRACKS][3 * AUDIO_FRAME_SAMPLES];
static _Alignas(4) u8 blob[16384];
static u32 blob_used;
static u32 offsets[6], lengths[6];
static u32 music_offset[3], music_frames[3] = { 2, 3, 0 };
static const AudioBank bank = { blob, offsets, lengths, 6, music_offset, music_frames, 3 };
static _Alignas(4) u16 out[AUDIO_FRAME_SAMPLES / 2];
static _Alignas(4) u16 scratch[AUDIO_VOICES * AUDIO_FRAME_SAMPLES / 2];
static _Alignas(4) u16 music_out[AUDIO_FRAME_SAMPLES / 2];
static AudioDecoder decoders[AUDIO_DECODERS];

/* Append one encoded stream at a 4-byte boundary; returns its offset. */
static u32 blob_add(u8 *base, u32 *used, u32 cap, const s8 *samples, u32 n)
{
    u32 bytes, at = *used;
    u8 *s = audio_ref_encode_alloc(samples, n, audio_ref_choose(samples, n), &bytes, NULL);
    if (at + bytes > cap) abort();
    memcpy(base + at, s, bytes);
    free(s);
    *used = at + bytes;
    return at;
}

static int sample_at(u32 i)
{
    return (s8)(u8)(i & 1 ? out[i / 2] >> 8 : out[i / 2]);
}

static int music_at(u32 i)
{
    return (s8)(u8)(i & 1 ? music_out[i / 2] >> 8 : music_out[i / 2]);
}

static void fill(void)
{
    static const u32 n[6] = { 400, 104, 104, 8, 352, 184 };
    u32 lcg = 3;
    memset(pcm, 0, sizeof pcm);
    for (int i = 0; i < 400; ++i) pcm[0][i] = 10;
    for (int i = 0; i < 100; ++i) { pcm[1][i] = 100; pcm[2][i] = -100; }
    for (int i = 0; i < 8; ++i) pcm[3][i] = (s8)(i + 1);
    for (int i = 0; i < 352; ++i) pcm[4][i] = 60;
    for (int i = 0; i < 184; ++i) { lcg = lcg * 1103515245u + 12345u; pcm[5][i] = (s8)((lcg >> 16 & 63u) - 32u); }
    for (u32 t = 0; t < MUSIC_TRACKS; ++t)
        for (u32 i = 0; i < sizeof music_pcm[t]; ++i)
            music_pcm[t][i] = (s8)(t ? (s32)(i / AUDIO_FRAME_SAMPLES * 40u) - 50 + (s32)(i % 7u) : (s32)(i % 200u) - 100);
    memset(blob, 0, sizeof blob);
    blob_used = 0;
    for (u32 i = 0; i < 6; ++i) {
        offsets[i] = blob_add(blob, &blob_used, sizeof blob, pcm[i], n[i]);
        lengths[i] = n[i];
    }
    for (u32 t = 0; t < MUSIC_TRACKS; ++t)
        music_offset[t] = blob_add(blob, &blob_used, sizeof blob, music_pcm[t], music_frames[t] * AUDIO_FRAME_SAMPLES);
}

static void mix(AudioMixer *m)
{
    audio_mixer_service(m, &bank);
    audio_mixer_mix(m, (volatile u16 *)out, (volatile u16 *)scratch);
}

static int music_frame_is(u32 track, u32 frame)
{
    for (u32 i = 0; i < AUDIO_FRAME_SAMPLES; ++i)
        if (music_at(i) != music_pcm[track][frame * AUDIO_FRAME_SAMPLES + i]) return 0;
    return 1;
}

static int music_silent(void)
{
    for (u32 i = 0; i < AUDIO_FRAME_SAMPLES; ++i) if (music_at(i)) return 0;
    return 1;
}

int main(void)
{
    AudioMixer m;
    fill();

    /* Silence: an idle mixer writes a full zero frame. */
    audio_mixer_reset(&m, decoders);
    memset(out, 0x55, sizeof out);
    mix(&m);
    for (u32 i = 0; i < AUDIO_FRAME_SAMPLES; ++i) CHECK(sample_at(i) == 0, "idle frame is silent");

    /* One voice is copied verbatim; a short voice ends mid-frame in zeros. */
    audio_mixer_reset(&m, decoders);
    CHECK(audio_mixer_request(&m, 3, 1, AUDIO_CLASS_EFFECT), "request accepted");
    mix(&m);
    for (u32 i = 0; i < 8; ++i) CHECK(sample_at(i) == (int)i + 1, "single voice copied");
    for (u32 i = 8; i < AUDIO_FRAME_SAMPLES; ++i) CHECK(sample_at(i) == 0, "voice tail silent");
    CHECK(!m.voice[1].remaining, "short voice retired");

    /* Two and three voices sum with saturation; lengths advance exactly. */
    audio_mixer_reset(&m, decoders);
    audio_mixer_request(&m, 1, 1, AUDIO_CLASS_EFFECT);
    audio_mixer_request(&m, 1, 1, AUDIO_CLASS_SPEECH);
    audio_mixer_request(&m, 0, 1, AUDIO_CLASS_EFFECT);
    mix(&m);
    for (u32 i = 0; i < 100; ++i) CHECK(sample_at(i) == 127, "three-voice sum saturates high");
    for (u32 i = 100; i < AUDIO_FRAME_SAMPLES; ++i) CHECK(sample_at(i) == 10, "remaining voice continues");
    CHECK(m.voice[2].remaining == 400 - AUDIO_FRAME_SAMPLES || m.voice[1].remaining == 400 - AUDIO_FRAME_SAMPLES,
          "long voice advanced one frame");
    audio_mixer_reset(&m, decoders);
    audio_mixer_request(&m, 2, 1, AUDIO_CLASS_EFFECT);
    audio_mixer_request(&m, 2, 1, AUDIO_CLASS_SPEECH);
    mix(&m);
    for (u32 i = 0; i < 100; ++i) CHECK(sample_at(i) == -128, "two-voice sum saturates low");
    audio_mixer_reset(&m, decoders);
    audio_mixer_request(&m, 1, 1, AUDIO_CLASS_EFFECT);
    audio_mixer_request(&m, 2, 1, AUDIO_CLASS_SPEECH);
    audio_mixer_request(&m, 0, 1, AUDIO_CLASS_EFFECT);
    mix(&m);
    CHECK(sample_at(0) == 10 && sample_at(99) == 10, "three opposite voices cancel");

    /* Packed 2-4 voice sums match a scalar reference on random samples,
     * including staggered ends inside a frame. */
    {
        static s8 rnd[4096];
        static _Alignas(4) u8 rblob[8192];
        static u32 roff[4];
        static const u32 rlen[4] = { 520, 176, 352, 1000 };
        static const u32 none[1] = { 0 };
        const AudioBank rbank = { rblob, roff, rlen, 4, none, none, 0 };
        u32 lcg = 7, rused = 0;
        for (u32 i = 0; i < sizeof rnd; ++i) { lcg = lcg * 1103515245u + 12345u; rnd[i] = (s8)(lcg >> 16); }
        for (u32 v = 0; v < 4; ++v) roff[v] = blob_add(rblob, &rused, sizeof rblob, rnd + 1024u * v, rlen[v]);
        for (u32 voices = 2; voices <= 4; ++voices) {
            audio_mixer_reset(&m, decoders);
            audio_mixer_request(&m, 0, 1, AUDIO_CLASS_SPEECH);
            for (u32 v = 1; v < voices; ++v) audio_mixer_request(&m, (u16)v, 1, AUDIO_CLASS_EFFECT);
            audio_mixer_service(&m, &rbank);
            for (u32 f = 0; f < 4; ++f) {
                audio_mixer_mix(&m, (volatile u16 *)out, (volatile u16 *)scratch);
                for (u32 i = 0; i < AUDIO_FRAME_SAMPLES; ++i) {
                    s32 ref = 0;
                    u32 at = f * AUDIO_FRAME_SAMPLES + i;
                    for (u32 v = 0; v < voices; ++v) if (at < rlen[v]) ref += rnd[1024u * v + at];
                    CHECK(sample_at(i) == audio_clip8(ref), "packed sum matches reference");
                }
            }
        }
    }

    /* Staggered starts over several frames, including lone-voice frames
     * decoded straight into out and voices ending mid-frame: the mix equals
     * a clipped scalar sum of the original PCM, sample by sample. */
    {
        static const u16 sounds[6] = { 0, 5, 4, 3, 1, 2 };
        static const u32 start_frame[6] = { 0, 1, 2, 3, 3, 6 };
        static const u32 n[6] = { 400, 104, 104, 8, 352, 184 };
        audio_mixer_reset(&m, decoders);
        for (u32 f = 0; f < 8; ++f) {
            for (u32 j = 0; j < 6; ++j)
                if (start_frame[j] == f) audio_mixer_request(&m, sounds[j], 1, sounds[j] == 4 ? AUDIO_CLASS_SPEECH : AUDIO_CLASS_EFFECT);
            mix(&m);
            for (u32 i = 0; i < AUDIO_FRAME_SAMPLES; ++i) {
                s32 ref = 0;
                for (u32 j = 0; j < 6; ++j) {
                    u32 at = (f - start_frame[j]) * AUDIO_FRAME_SAMPLES + i;
                    if (f >= start_frame[j] && at < n[sounds[j]]) ref += pcm[sounds[j]][at];
                }
                CHECK(sample_at(i) == audio_clip8(ref), "staggered mix matches reference");
            }
        }
        CHECK(!m.dropped, "staggered requests all played");
        for (u32 v = 0; v < AUDIO_VOICES; ++v) CHECK(!m.voice[v].remaining, "staggered voices retired");
    }

    /* A 184-sample voice retires exactly: one full frame, then eight samples. */
    audio_mixer_reset(&m, decoders);
    audio_mixer_request(&m, 5, 1, AUDIO_CLASS_EFFECT);
    mix(&m);
    CHECK(m.voice[1].remaining == 8, "first frame of an odd-length voice");
    mix(&m);
    CHECK(!m.voice[1].remaining, "odd-length voice retired");
    for (u32 i = 0; i < 8; ++i) CHECK(sample_at(i) == pcm[5][AUDIO_FRAME_SAMPLES + i], "odd-length tail");
    for (u32 i = 8; i < AUDIO_FRAME_SAMPLES; ++i) CHECK(sample_at(i) == 0, "silence after tail");

    /* A voice spanning frames plays every byte exactly once. */
    audio_mixer_reset(&m, decoders);
    audio_mixer_request(&m, 4, 1, AUDIO_CLASS_EFFECT);
    u32 total = 0;
    for (int f = 0; f < 3; ++f) {
        mix(&m);
        for (u32 i = 0; i < AUDIO_FRAME_SAMPLES; ++i) total += sample_at(i) == 60;
    }
    CHECK(total == 352, "multi-frame voice length preserved");

    /* Speech always takes voice 0 and interrupts the previous response. */
    audio_mixer_reset(&m, decoders);
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
    audio_mixer_reset(&m, decoders);
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

    /* An advisor line outranks every effect, never shares the speech voice,
     * plays one line at a time and replays once it has ended. */
    audio_mixer_reset(&m, decoders);
    for (u16 s = 0; s < 3; ++s) audio_mixer_request(&m, s, 3, AUDIO_CLASS_EFFECT);
    audio_mixer_request(&m, 3, 1, AUDIO_CLASS_SPEECH);
    audio_mixer_service(&m, &bank);
    m.voice[2].age = 9;
    audio_mixer_request(&m, 4, 0, AUDIO_CLASS_ADVISOR);
    audio_mixer_service(&m, &bank);
    CHECK(m.voice[2].sound == 4 && m.voice[2].priority == AUDIO_PRIORITY_ADVISOR &&
          m.voice[AUDIO_SPEECH_VOICE].sound == 3, "advisor takes the oldest effect voice");
    dropped = m.dropped;
    m.voice[2].age = AUDIO_RETRIGGER_FRAMES + 5;
    audio_mixer_request(&m, 4, 0, AUDIO_CLASS_ADVISOR);
    audio_mixer_request(&m, 1, 0, AUDIO_CLASS_ADVISOR);
    audio_mixer_request(&m, 5, 3, AUDIO_CLASS_EFFECT);
    audio_mixer_service(&m, &bank);
    CHECK(m.dropped == dropped + 2 && m.voice[2].sound == 4 && m.voice[2].remaining == 352,
          "advisor never cuts itself off or overlaps another line");
    CHECK(m.voice[1].sound == 5 && m.voice[3].sound == 2, "effects cannot take the advisor voice");
    mix(&m);
    mix(&m);
    CHECK(!m.voice[1].remaining && !m.voice[2].remaining && !m.voice[3].remaining, "advisor line ends");
    audio_mixer_request(&m, 4, 0, AUDIO_CLASS_ADVISOR);
    audio_mixer_service(&m, &bank);
    CHECK(m.voice[1].sound == 4 && m.voice[1].remaining == 352, "advisor replays after it ends");

    /* The ring holds AUDIO_QUEUE-1 requests and rejects the rest. */
    audio_mixer_reset(&m, decoders);
    for (u32 i = 0; i < AUDIO_QUEUE - 1; ++i) CHECK(audio_mixer_request(&m, 3, 1, AUDIO_CLASS_EFFECT), "queue slot");
    CHECK(!audio_mixer_request(&m, 3, 1, AUDIO_CLASS_EFFECT) && m.dropped == 1, "full queue refuses");
    audio_mixer_service(&m, &bank);
    CHECK(m.head == m.tail, "service drains queue");
    for (u32 round = 0; round < 3 * AUDIO_QUEUE; ++round) {
        CHECK(audio_mixer_request(&m, 3, 1, AUDIO_CLASS_SPEECH), "ring wraps");
        audio_mixer_service(&m, &bank);
    }

    /* Music: start, count frames, loop or end, change, stop, reject absent.
     * Every playing step decodes the next frame of the track into music_out;
     * a silent, unchanged step leaves it untouched. */
#define STEP(expect) (memset(music_out, 0x55, sizeof music_out), audio_mixer_music_step(&m, &bank, (volatile u16 *)music_out) == (expect))
#define UNTOUCHED() (music_out[0] == 0x5555 && music_out[AUDIO_FRAME_SAMPLES / 2 - 1] == 0x5555)
    audio_mixer_reset(&m, decoders);
    CHECK(STEP(AUDIO_MUSIC_CONTINUE) && UNTOUCHED(), "silent idle writes nothing");
    audio_mixer_music(&m, 0, 1);
    CHECK(STEP(AUDIO_MUSIC_RESTART) && music_frame_is(0, 0), "track starts at frame 0");
    CHECK(STEP(AUDIO_MUSIC_CONTINUE) && music_frame_is(0, 1), "frame 1");
    CHECK(STEP(AUDIO_MUSIC_RESTART) && music_frame_is(0, 0), "loops to frame 0 after two frames");
    audio_mixer_music(&m, 0, 1);
    CHECK(STEP(AUDIO_MUSIC_CONTINUE) && music_frame_is(0, 1), "same track continues in step");
    CHECK(STEP(AUDIO_MUSIC_RESTART) && music_frame_is(0, 0), "and loops on time");
    for (u32 f = 0; f < 7; ++f)
        CHECK(STEP(f % 2 ? AUDIO_MUSIC_RESTART : AUDIO_MUSIC_CONTINUE) && music_frame_is(0, (f + 1) % 2), "repeated loops");
    audio_mixer_music(&m, 1, 0);
    CHECK(STEP(AUDIO_MUSIC_RESTART) && music_frame_is(1, 0), "track change");
    CHECK(STEP(AUDIO_MUSIC_CONTINUE) && music_frame_is(1, 1), "three-frame track, frame 1");
    CHECK(STEP(AUDIO_MUSIC_CONTINUE) && music_frame_is(1, 2), "three-frame track, frame 2");
    CHECK(STEP(AUDIO_MUSIC_SILENCE) && m.music_track == AUDIO_MUSIC_NONE && music_silent(), "one-shot ends in a zero frame");
    CHECK(STEP(AUDIO_MUSIC_CONTINUE) && UNTOUCHED(), "stays silent");
    audio_mixer_music(&m, 2, 1);
    CHECK(STEP(AUDIO_MUSIC_CONTINUE) && m.music_track == AUDIO_MUSIC_NONE && UNTOUCHED(), "absent track ignored");
    audio_mixer_music(&m, AUDIO_MUSIC_STOP, 0);
    CHECK(STEP(AUDIO_MUSIC_CONTINUE) && UNTOUCHED(), "stop while silent");
    audio_mixer_music(&m, 0, 1);
    CHECK(STEP(AUDIO_MUSIC_RESTART) && music_frame_is(0, 0), "restart after silence");
    audio_mixer_music(&m, AUDIO_MUSIC_STOP, 0);
    CHECK(STEP(AUDIO_MUSIC_SILENCE) && music_silent(), "stop writes a zero frame");
    audio_mixer_music(&m, 1, 1);
    CHECK(STEP(AUDIO_MUSIC_RESTART) && music_frame_is(1, 0), "start after stop");
    audio_mixer_music(&m, 0, 1);
    CHECK(STEP(AUDIO_MUSIC_RESTART) && music_frame_is(0, 0), "change mid-track restarts the decoder");
    audio_mixer_music(&m, 2, 1);
    CHECK(STEP(AUDIO_MUSIC_SILENCE) && music_silent(), "absent track stops the playing one");
#undef STEP
#undef UNTOUCHED

    /* The frame entry point mixes, counts and steps music. */
    audio_mixer_reset(&m, decoders);
    audio_mixer_music(&m, 1, 0);
    audio_mixer_request(&m, 3, 1, AUDIO_CLASS_EFFECT);
    CHECK(audio_mixer_frame(&m, &bank, (volatile u16 *)out, (volatile u16 *)scratch, (volatile u16 *)music_out) ==
          AUDIO_MUSIC_RESTART && m.frames == 1 && music_frame_is(1, 0) && sample_at(7) == 8,
          "frame entry point");
    puts("audio mixer checks passed");
    return 0;
}
