#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H
/* Bounded sampled-audio mixer shared by the GBA VBlank handler and host tests.
 *
 * Hardware owns no logic here: one call to audio_mixer_frame() per video frame
 * drains the request queue, mixes AUDIO_FRAME_SAMPLES signed 8-bit samples for
 * Direct Sound A and advances the music stream that DMA plays on Direct Sound
 * B. Storage is fixed: AUDIO_VOICES voices, an AUDIO_QUEUE request ring and no
 * allocation. Arithmetic is integer only.
 *
 * Concurrency: the game loop is the only writer of queue[] and head, the
 * interrupt the only writer of tail and of every voice. Music requests are a
 * single volatile word written by the game loop and cleared by the interrupt.
 */
#include "gba.h"

#define AUDIO_FRAME_SAMPLES 176u
#define AUDIO_VOICES 4u
#define AUDIO_SPEECH_VOICE 0u
#define AUDIO_QUEUE 8u
/* An effect is not retriggered while the same sound started this recently. */
#define AUDIO_RETRIGGER_FRAMES 4u
#define AUDIO_MUSIC_NONE 0xffu
#define AUDIO_MUSIC_STOP 0xfeu

enum { AUDIO_CLASS_EFFECT, AUDIO_CLASS_SPEECH };

typedef struct {
    const s8 *blob;            /* Word-aligned base of all PCM data. */
    const u32 *sample_offset;  /* Byte offset of each sample; multiple of 4. */
    const u32 *sample_length;  /* Byte length of each sample; multiple of 4. */
    u32 sample_count;
    const u32 *music_offset;   /* Word-aligned track starts. */
    const u32 *music_frames;   /* Track length in AUDIO_FRAME_SAMPLES frames. */
    u32 music_count;
} AudioBank;

typedef struct {
    const s8 *data;
    u32 remaining;  /* Bytes still to play, a multiple of four; zero when idle. */
    u16 sound;
    u8 priority;
    u8 age;         /* Frames since start, saturating. */
} AudioVoice;

typedef struct {
    u16 sound;
    u8 priority;
    u8 kind;
} AudioRequest;

typedef struct {
    AudioVoice voice[AUDIO_VOICES];
    AudioRequest queue[AUDIO_QUEUE];
    volatile u8 head, tail;
    volatile u8 music_request;
    u8 music_loop;
    u8 music_track;      /* AUDIO_MUSIC_NONE while silent. */
    u32 music_frame;
    u32 frames;
    u32 dropped;         /* Diagnostic count of refused requests (both sides). */
} AudioMixer;

/* Music step outcomes for the hardware layer. */
enum { AUDIO_MUSIC_CONTINUE, AUDIO_MUSIC_RESTART, AUDIO_MUSIC_SILENCE };

static inline void audio_mixer_reset(AudioMixer *m)
{
    u8 *p = (u8 *)m;
    for (u32 i = 0; i < sizeof *m; ++i) p[i] = 0;
    m->music_request = AUDIO_MUSIC_NONE;
    m->music_track = AUDIO_MUSIC_NONE;
}

/* Game-loop side. Returns 0 when the bounded queue is full. */
static inline int audio_mixer_request(AudioMixer *m, u16 sound, u8 priority, u8 kind)
{
    u8 head = m->head, next = (u8)((head + 1u) % AUDIO_QUEUE);
    if (next == m->tail) { ++m->dropped; return 0; }
    m->queue[head].sound = sound;
    m->queue[head].priority = priority;
    m->queue[head].kind = kind;
    /* The entry must be complete before the interrupt can see the new head. */
    __asm__ volatile ("" ::: "memory");
    m->head = next;
    return 1;
}

static inline void audio_mixer_music(AudioMixer *m, u8 track, u8 loop)
{
    m->music_loop = loop;
    __asm__ volatile ("" ::: "memory");
    m->music_request = track;
}

static inline void audio_mixer_start_voice(AudioVoice *v, const AudioBank *bank,
                                           const AudioRequest *r)
{
    v->data = bank->blob + bank->sample_offset[r->sound];
    v->remaining = bank->sample_length[r->sound] & ~3u;
    v->sound = r->sound;
    v->priority = r->priority;
    v->age = 0;
}

/* Interrupt side: assign queued requests to voices. Speech always replaces
 * the speech voice, like a new unit response interrupting the last one.
 * Effects take a free voice, else the lowest-priority (then oldest) voice
 * whose priority does not exceed the request. */
static inline void audio_mixer_service(AudioMixer *m, const AudioBank *bank)
{
    u8 tail = m->tail;
    while (tail != m->head) {
        const AudioRequest *r = &m->queue[tail];
        tail = (u8)((tail + 1u) % AUDIO_QUEUE);
        if (r->sound >= bank->sample_count) { ++m->dropped; continue; }
        if (r->kind == AUDIO_CLASS_SPEECH) {
            audio_mixer_start_voice(&m->voice[AUDIO_SPEECH_VOICE], bank, r);
            continue;
        }
        AudioVoice *best = 0;
        int duplicate = 0;
        for (u32 i = AUDIO_SPEECH_VOICE + 1u; i < AUDIO_VOICES; ++i) {
            AudioVoice *v = &m->voice[i];
            if (v->remaining && v->sound == r->sound && v->age < AUDIO_RETRIGGER_FRAMES) {
                duplicate = 1;
                break;
            }
            if (!v->remaining) {
                if (!best || best->remaining) best = v;
            } else if (!best || (best->remaining &&
                       (v->priority < best->priority ||
                        (v->priority == best->priority && v->age > best->age)))) {
                best = v;
            }
        }
        if (duplicate || !best || (best->remaining && best->priority > r->priority)) {
            ++m->dropped;
            continue;
        }
        audio_mixer_start_voice(best, bank, r);
    }
    m->tail = tail;
}

static inline s32 audio_clip8(s32 s)
{
    if ((u32)(s + 128) > 255u) s = s < 0 ? -128 : 127;
    return s;
}

/* Sum n voices four samples at a time. Each source word is biased to
 * unsigned bytes and split into two 16-bit lanes (bytes 0/2 and 1/3), so one
 * ROM word read and four ALU operations add four samples; lanes cannot carry
 * (4 * 255 < 65536). Saturation branches only when a lane clips. */
static inline void audio_mixer_sum(volatile u32 *dst, const u32 *const *src, u32 n, u32 words)
{
    const u32 *a = src[0], *b = src[1], *c = n > 2 ? src[2] : 0, *d = n > 3 ? src[3] : 0;
    const s32 bias = (s32)(128u * n);
    for (u32 k = 0; k < words; ++k) {
        u32 w = a[k] ^ 0x80808080u, even = w & 0x00ff00ffu, odd = (w >> 8) & 0x00ff00ffu;
        w = b[k] ^ 0x80808080u; even += w & 0x00ff00ffu; odd += (w >> 8) & 0x00ff00ffu;
        if (n > 2) { w = c[k] ^ 0x80808080u; even += w & 0x00ff00ffu; odd += (w >> 8) & 0x00ff00ffu; }
        if (n > 3) { w = d[k] ^ 0x80808080u; even += w & 0x00ff00ffu; odd += (w >> 8) & 0x00ff00ffu; }
        s32 s0 = audio_clip8((s32)(even & 0xffffu) - bias), s1 = audio_clip8((s32)(odd & 0xffffu) - bias);
        s32 s2 = audio_clip8((s32)(even >> 16) - bias), s3 = audio_clip8((s32)(odd >> 16) - bias);
        dst[k] = (u32)(u8)s0 | (u32)(u8)s1 << 8 | (u32)(u8)s2 << 16 | (u32)(u8)s3 << 24;
    }
}

/* Mix one frame into out with word stores (VRAM accepts them). Voice
 * positions and lengths are multiples of four by construction, so every
 * span is too and ROM samples are read as aligned words. */
static inline void audio_mixer_mix(AudioMixer *m, volatile u16 *out)
{
    const u32 *src[AUDIO_VOICES];
    u32 count, pos = 0;
    for (u32 i = 0; i < AUDIO_VOICES; ++i)
        if (m->voice[i].age != 0xff) ++m->voice[i].age;
    while (pos < AUDIO_FRAME_SAMPLES) {
        /* Mix the span where the active set is constant, then retire. */
        u32 span = AUDIO_FRAME_SAMPLES - pos;
        count = 0;
        for (u32 i = 0; i < AUDIO_VOICES; ++i) {
            AudioVoice *v = &m->voice[i];
            if (!v->remaining) continue;
            src[count++] = (const u32 *)(const void *)v->data;
            if (v->remaining < span) span = v->remaining;
        }
        volatile u32 *dst = (volatile u32 *)(volatile void *)(out + pos / 2u);
        u32 words = span / 4u;
        switch (count) {
        case 0:
            for (u32 k = 0; k < words; ++k) dst[k] = 0;
            break;
        case 1: {
            const u32 *a = src[0];
            for (u32 k = 0; k < words; ++k) dst[k] = a[k];
            break;
        }
        case 2: audio_mixer_sum(dst, src, 2, words); break;
        case 3: audio_mixer_sum(dst, src, 3, words); break;
        default: audio_mixer_sum(dst, src, 4, words); break;
        }
        for (u32 i = 0; i < AUDIO_VOICES; ++i) {
            AudioVoice *v = &m->voice[i];
            if (!v->remaining) continue;
            v->data += span;
            v->remaining -= span;
        }
        pos += span;
    }
}

/* Advance music by one frame. The caller restarts its DMA stream at
 * audio_mixer_music_data() on RESTART and silences it on SILENCE. */
static inline int audio_mixer_music_step(AudioMixer *m, const AudioBank *bank)
{
    u8 request = m->music_request;
    if (request != AUDIO_MUSIC_NONE) {
        m->music_request = AUDIO_MUSIC_NONE;
        if (request == AUDIO_MUSIC_STOP || request >= bank->music_count ||
            !bank->music_frames[request]) {
            int was_playing = m->music_track != AUDIO_MUSIC_NONE;
            m->music_track = AUDIO_MUSIC_NONE;
            return was_playing ? AUDIO_MUSIC_SILENCE : AUDIO_MUSIC_CONTINUE;
        }
        if (request == m->music_track) return AUDIO_MUSIC_CONTINUE;
        m->music_track = request;
        m->music_frame = 0;
        return AUDIO_MUSIC_RESTART;
    }
    if (m->music_track == AUDIO_MUSIC_NONE) return AUDIO_MUSIC_CONTINUE;
    if (++m->music_frame < bank->music_frames[m->music_track]) return AUDIO_MUSIC_CONTINUE;
    m->music_frame = 0;
    if (m->music_loop) return AUDIO_MUSIC_RESTART;
    m->music_track = AUDIO_MUSIC_NONE;
    return AUDIO_MUSIC_SILENCE;
}

static inline const s8 *audio_mixer_music_data(const AudioMixer *m, const AudioBank *bank)
{
    return bank->blob + bank->music_offset[m->music_track];
}

/* One video frame of interrupt work. Returns the music step outcome. */
static inline int audio_mixer_frame(AudioMixer *m, const AudioBank *bank, volatile u16 *out)
{
    audio_mixer_service(m, bank);
    audio_mixer_mix(m, out);
    ++m->frames;
    return audio_mixer_music_step(m, bank);
}
#endif
