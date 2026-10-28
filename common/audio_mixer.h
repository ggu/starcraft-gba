#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H
/* Bounded sampled-audio mixer shared by the GBA VBlank handler and host tests.
 *
 * Hardware owns no logic here: one call to audio_mixer_frame() per video frame
 * drains the request queue, decodes and mixes AUDIO_FRAME_SAMPLES signed 8-bit
 * samples for Direct Sound A and decodes the next music frame for Direct
 * Sound B. Every sample and track is a format-2 stream (audio_codec.h).
 * Storage is fixed: AUDIO_VOICES voices, an AUDIO_QUEUE request ring and no
 * allocation. The AUDIO_DECODERS stream decoders (one per voice, then music)
 * live in caller-supplied halfword-writable memory (VRAM on hardware) given
 * to audio_mixer_reset(). Arithmetic is integer only.
 *
 * Concurrency: the game loop is the only writer of queue[] and head, the
 * interrupt the only writer of tail, of every voice and of every decoder.
 * Music requests are a single volatile word written by the game loop and
 * cleared by the interrupt.
 */
#include "gba.h"
#include "audio_codec.h"

#define AUDIO_FRAME_SAMPLES 176u
#define AUDIO_VOICES 4u
#define AUDIO_SPEECH_VOICE 0u
#define AUDIO_QUEUE 8u
/* An effect is not retriggered while the same sound started this recently. */
#define AUDIO_RETRIGGER_FRAMES 4u
#define AUDIO_MUSIC_NONE 0xffu
#define AUDIO_MUSIC_STOP 0xfeu
/* Decoders: voice i uses dec[i], music dec[AUDIO_VOICES]. */
#define AUDIO_DECODERS (AUDIO_VOICES + 1u)

/* Request classes (AUDIO_CLASS_*) are declared in gba.h for the game side.
 * Advisor lines outrank every effect, so only another advisor line could
 * take their voice, and that is refused while one plays. */
#define AUDIO_PRIORITY_ADVISOR 0xffu

typedef struct {
    const u8 *blob;            /* Word-aligned base of all encoded streams. */
    const u32 *sample_offset;  /* Byte offset of each sample's stream; multiple of 4. */
    const u32 *sample_length;  /* Sample count of each sample; multiple of 8. */
    u32 sample_count;
    const u32 *music_offset;   /* Byte offset of each track's stream; multiple of 4. */
    const u32 *music_frames;   /* Track length in AUDIO_FRAME_SAMPLES frames. */
    u32 music_count;
} AudioBank;

typedef struct {
    u32 remaining;  /* Samples still to play, a multiple of eight; zero when idle. */
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
    u32 music_frame;     /* Frame the music decoder produced last. */
    AudioDecoder *dec;   /* AUDIO_DECODERS decoders: voices, then music. */
    u32 frames;
    u32 dropped;         /* Diagnostic count of refused requests (both sides). */
} AudioMixer;

/* Music step outcomes for the hardware layer. */
enum { AUDIO_MUSIC_CONTINUE, AUDIO_MUSIC_RESTART, AUDIO_MUSIC_SILENCE };

/* decoders: AUDIO_DECODERS entries; their contents need no initialisation
 * (every use starts a stream first). */
static inline void audio_mixer_reset(AudioMixer *m, AudioDecoder *decoders)
{
    u8 *p = (u8 *)m;
    for (u32 i = 0; i < sizeof *m; ++i) p[i] = 0;
    m->dec = decoders;
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

static inline void audio_mixer_start_voice(AudioMixer *m, AudioVoice *v, const AudioBank *bank,
                                           const AudioRequest *r, u8 priority)
{
    audio_decoder_start(&m->dec[v - m->voice], bank->blob + bank->sample_offset[r->sound]);
    v->remaining = bank->sample_length[r->sound] & ~(AUDIO_CODEC_BLOCK - 1u);
    v->sound = r->sound;
    v->priority = priority;
    v->age = 0;
}

/* Interrupt side: assign queued requests to voices. Speech always replaces
 * the speech voice, like a new unit response interrupting the last one.
 * Effects take a free voice, else the lowest-priority (then oldest) voice
 * whose priority does not exceed the request. An advisor line is an effect
 * at AUDIO_PRIORITY_ADVISOR, refused while any advisor line still plays, so
 * it never cuts itself off and a later request replays it once it ends. */
static inline void audio_mixer_service(AudioMixer *m, const AudioBank *bank)
{
    u8 tail = m->tail;
    while (tail != m->head) {
        const AudioRequest *r = &m->queue[tail];
        tail = (u8)((tail + 1u) % AUDIO_QUEUE);
        if (r->sound >= bank->sample_count) { ++m->dropped; continue; }
        if (r->kind == AUDIO_CLASS_SPEECH) {
            audio_mixer_start_voice(m, &m->voice[AUDIO_SPEECH_VOICE], bank, r, r->priority);
            continue;
        }
        int advisor = r->kind == AUDIO_CLASS_ADVISOR;
        u8 priority = advisor ? AUDIO_PRIORITY_ADVISOR : r->priority;
        AudioVoice *best = 0;
        int duplicate = 0;
        for (u32 i = AUDIO_SPEECH_VOICE + 1u; i < AUDIO_VOICES; ++i) {
            AudioVoice *v = &m->voice[i];
            if (v->remaining && ((v->sound == r->sound && v->age < AUDIO_RETRIGGER_FRAMES) ||
                                 (advisor && v->priority == AUDIO_PRIORITY_ADVISOR))) {
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
        if (duplicate || !best || (best->remaining && best->priority > priority)) {
            ++m->dropped;
            continue;
        }
        audio_mixer_start_voice(m, best, bank, r, priority);
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
 * (4 * 255 < 65536). Unclipped words repack in a few operations; only a
 * word with a clipping lane is clamped lane by lane. */
static inline void audio_mixer_sum(volatile u32 *dst, const u32 *const *src, u32 n, u32 words)
{
    const u32 *a = src[0], *b = src[1], *c = n > 2 ? src[2] : 0, *d = n > 3 ? src[3] : 0;
    const s32 bias = (s32)(128u * n);
    const u32 rebias = 128u * (n - 1u) * 0x00010001u;
    for (u32 k = 0; k < words; ++k) {
        u32 w = a[k] ^ 0x80808080u, even = w & 0x00ff00ffu, odd = (w >> 8) & 0x00ff00ffu;
        w = b[k] ^ 0x80808080u; even += w & 0x00ff00ffu; odd += (w >> 8) & 0x00ff00ffu;
        if (n > 2) { w = c[k] ^ 0x80808080u; even += w & 0x00ff00ffu; odd += (w >> 8) & 0x00ff00ffu; }
        if (n > 3) { w = d[k] ^ 0x80808080u; even += w & 0x00ff00ffu; odd += (w >> 8) & 0x00ff00ffu; }
        /* Rebias every lane to one unsigned byte at once. A lane outside
         * 0..255 sets bits 8-15 or 24-31 (an underflowing low lane does so
         * too, before its borrow reaches the high lane). */
        u32 e = even - rebias, o = odd - rebias;
        if (!((e | o) & 0xff00ff00u)) {
            dst[k] = (e | o << 8) ^ 0x80808080u;
            continue;
        }
        s32 s0 = audio_clip8((s32)(even & 0xffffu) - bias), s1 = audio_clip8((s32)(odd & 0xffffu) - bias);
        s32 s2 = audio_clip8((s32)(even >> 16) - bias), s3 = audio_clip8((s32)(odd >> 16) - bias);
        dst[k] = (u32)(u8)s0 | (u32)(u8)s1 << 8 | (u32)(u8)s2 << 16 | (u32)(u8)s3 << 24;
    }
}

/* Mix one frame into out with word stores (VRAM accepts them). A lone voice
 * decodes straight into out. Otherwise each voice decodes its part of the
 * frame into its own AUDIO_FRAME_SAMPLES-byte slot of scratch (halfword
 * stores, word-aligned slots) and the slots are summed span by span. Lengths
 * are multiples of eight, so every span is a whole number of words. */
static inline void audio_mixer_mix(AudioMixer *m, volatile u16 *out, volatile u16 *scratch)
{
    const u32 *src[AUDIO_VOICES];
    u32 count = 0, pos = 0;
    u32 lone = 0;
    for (u32 i = 0; i < AUDIO_VOICES; ++i) {
        AudioVoice *v = &m->voice[i];
        if (v->age != 0xff) ++v->age;
        if (v->remaining) { ++count; lone = i; }
    }
    if (count == 1) {
        AudioVoice *v = &m->voice[lone];
        u32 n = v->remaining < AUDIO_FRAME_SAMPLES ? v->remaining : AUDIO_FRAME_SAMPLES;
        audio_decoder_run(&m->dec[lone], out, n);
        v->remaining -= n;
        volatile u32 *dst = (volatile u32 *)(volatile void *)out;
        for (u32 k = n / 4u; k < AUDIO_FRAME_SAMPLES / 4u; ++k) dst[k] = 0;
        return;
    }
    for (u32 i = 0; i < AUDIO_VOICES; ++i) {
        AudioVoice *v = &m->voice[i];
        if (!v->remaining) continue;
        u32 n = v->remaining < AUDIO_FRAME_SAMPLES ? v->remaining : AUDIO_FRAME_SAMPLES;
        audio_decoder_run(&m->dec[i], scratch + i * (AUDIO_FRAME_SAMPLES / 2u), n);
    }
    /* The slots are read back below as words. */
    __asm__ volatile ("" ::: "memory");
    while (pos < AUDIO_FRAME_SAMPLES) {
        /* Mix the span where the active set is constant, then retire. */
        u32 span = AUDIO_FRAME_SAMPLES - pos;
        count = 0;
        for (u32 i = 0; i < AUDIO_VOICES; ++i) {
            AudioVoice *v = &m->voice[i];
            if (!v->remaining) continue;
            src[count++] = (const u32 *)(const void *)(scratch + i * (AUDIO_FRAME_SAMPLES / 2u) + pos / 2u);
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
            if (v->remaining) v->remaining -= span;
        }
        pos += span;
    }
}

/* Advance music by one frame and decode it: whenever a track is playing
 * after the step, exactly one AUDIO_FRAME_SAMPLES frame is written to
 * music_out (halfword packed); RESTART first restarts the decoder at the
 * track's stream; SILENCE writes one zero frame; while silent and unchanged
 * nothing is written. A request for the playing track only updates the loop
 * flag and keeps the decoder in step with music_frame. */
static inline int audio_mixer_music_step(AudioMixer *m, const AudioBank *bank, volatile u16 *music_out)
{
    int result = AUDIO_MUSIC_CONTINUE;
    u8 request = m->music_request;
    if (request != AUDIO_MUSIC_NONE) m->music_request = AUDIO_MUSIC_NONE;
    if (request != AUDIO_MUSIC_NONE && request != m->music_track) {
        if (request == AUDIO_MUSIC_STOP || request >= bank->music_count ||
            !bank->music_frames[request]) {
            if (m->music_track == AUDIO_MUSIC_NONE) return AUDIO_MUSIC_CONTINUE;
            m->music_track = AUDIO_MUSIC_NONE;
            result = AUDIO_MUSIC_SILENCE;
        } else {
            m->music_track = request;
            m->music_frame = 0;
            result = AUDIO_MUSIC_RESTART;
        }
    } else {
        if (m->music_track == AUDIO_MUSIC_NONE) return AUDIO_MUSIC_CONTINUE;
        if (++m->music_frame >= bank->music_frames[m->music_track]) {
            m->music_frame = 0;
            if (m->music_loop) {
                result = AUDIO_MUSIC_RESTART;
            } else {
                m->music_track = AUDIO_MUSIC_NONE;
                result = AUDIO_MUSIC_SILENCE;
            }
        }
    }
    if (result == AUDIO_MUSIC_SILENCE) {
        volatile u32 *dst = (volatile u32 *)(volatile void *)music_out;
        for (u32 k = 0; k < AUDIO_FRAME_SAMPLES / 4u; ++k) dst[k] = 0;
        return result;
    }
    if (result == AUDIO_MUSIC_RESTART)
        audio_decoder_start(&m->dec[AUDIO_VOICES], bank->blob + bank->music_offset[m->music_track]);
    audio_decoder_run(&m->dec[AUDIO_VOICES], music_out, AUDIO_FRAME_SAMPLES);
    return result;
}

/* One video frame of interrupt work: out receives the mix, scratch is the
 * AUDIO_VOICES * AUDIO_FRAME_SAMPLES-byte decode area, music_out the music
 * frame. Returns the music step outcome. */
static inline int audio_mixer_frame(AudioMixer *m, const AudioBank *bank, volatile u16 *out,
                                    volatile u16 *scratch, volatile u16 *music_out)
{
    audio_mixer_service(m, bank);
    audio_mixer_mix(m, out, scratch);
    ++m->frames;
    return audio_mixer_music_step(m, bank, music_out);
}
#endif
