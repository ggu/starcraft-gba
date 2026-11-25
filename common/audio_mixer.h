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

/* Game-loop side: nonzero while an advisor line plays or waits in the queue.
 * A request made now would be refused, so callers hold their line instead.
 * A stale read of tail or a voice only delays or repeats one such check. */
static inline int audio_mixer_advisor_busy(const AudioMixer *m)
{
    for (u32 i = AUDIO_SPEECH_VOICE + 1u; i < AUDIO_VOICES; ++i)
        if (m->voice[i].remaining && m->voice[i].priority == AUDIO_PRIORITY_ADVISOR) return 1;
    for (u8 t = m->tail; t != m->head; t = (u8)((t + 1u) % AUDIO_QUEUE))
        if (m->queue[t].kind == AUDIO_CLASS_ADVISOR) return 1;
    return 0;
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

/* Sum n (2 to 4) voices four samples at a time. src interleaves them word by
 * word (word j of voice i at src[n * j + i]) as unsigned bytes, the sample
 * with its top bit flipped, which is how audio_decoder_run_column() writes
 * them. Each word is split into two 16-bit lanes (bytes 0/2 and 1/3), so one
 * load and four ALU operations add four samples; lanes cannot carry
 * (4 * 255 < 65536). Unclipped words repack in a few operations; only a
 * word with a clipping lane is clamped lane by lane.
 *
 * ROMs that link the bank run a hand-written Thumb body with one loop per n
 * and every constant in a register; the compiler's version of the C below
 * reloaded three constants from ROM for each word. It is an ordinary
 * function in its own .text section, so a link may place it in ROM or in
 * IWRAM. The C function is the reference and runs in host builds;
 * tests/audio_codec_arm_probe.c compares the two. */
#ifdef AUDIO_CODEC_HAND_WRITTEN
/* The rare word with a clipping lane, kept out of the loop's section. It is
 * part of audio_mixer_sum, with that loop's registers and no stack, not a C
 * function. In: r2, r3 the rebiased lanes, r10 and r11 as below. Out: r2
 * the finished word. Uses r3, r4, r5 and r7. Each lane is clamped to 0..255
 * from its exact sum, since a lane below zero has borrowed from its
 * neighbour. */
#define AUDIO_SUM_ASM_CLAMP                                                     \
    "    sub     r4, r5\n"                                                      \
    "    cmp     r4, #255\n"                                                    \
    "    bls     1f\n"                                                          \
    "    asr     r4, #31\n"                     /* below 0: 0; above 255: 255 */ \
    "    mvn     r4, r4\n"                                                      \
    "    lsl     r4, #24\n"                                                     \
    "    lsr     r4, #24\n"                                                     \
    "1:\n"
static void __attribute__((naked, noinline, noclone, used, target("thumb"))) audio_mixer_clip(void)
{
    __asm__ (
        "    add     r2, r11\n"                 /* the lane sums again */
        "    add     r3, r11\n"
        "    mov     r5, r11\n"
        "    lsl     r5, #16\n"
        "    lsr     r5, #16\n"                 /* 128 * (n - 1) */
        "    lsr     r4, r3, #16\n"             /* byte 3 */
        AUDIO_SUM_ASM_CLAMP
        "    lsl     r7, r4, #24\n"
        "    lsr     r4, r2, #16\n"             /* byte 2 */
        AUDIO_SUM_ASM_CLAMP
        "    lsl     r4, #16\n"
        "    orr     r7, r4\n"
        "    lsl     r4, r3, #16\n"             /* byte 1 */
        "    lsr     r4, #16\n"
        AUDIO_SUM_ASM_CLAMP
        "    lsl     r4, #8\n"
        "    orr     r7, r4\n"
        "    lsl     r4, r2, #16\n"             /* byte 0 */
        "    lsr     r4, #16\n"
        AUDIO_SUM_ASM_CLAMP
        "    orr     r7, r4\n"
        "    mov     r2, r10\n"
        "    eor     r2, r7\n"
        "    bx      lr\n");
}
#undef AUDIO_SUM_ASM_CLAMP
/*   r0 dst   r2 even lanes   r6 0x00ff00ff    r9  end of src
 *   r1 src   r3 odd lanes    r8 -rebias       r10 0x80808080
 *            r4, r5, r7 voices and scratch    r11 rebias
 *
 * A voice adds (word >> 8) & mask to the odd lanes and word & mask to the
 * even ones. The tail rebiases both to one byte per lane; a lane outside
 * 0..255 sets a bit above its byte. Otherwise the four bytes are packed
 * and their top bits flipped back. */
#define AUDIO_SUM_ASM_VOICE(w, t)                                               \
    "    lsr     " t ", " w ", #8\n"                                            \
    "    and     " t ", r6\n"                                                   \
    "    add     r3, " t "\n"                                                   \
    "    and     " w ", r6\n"                                                   \
    "    add     r2, " w "\n"
#define AUDIO_SUM_ASM_TAIL(n)                                                   \
    "    add     r2, r8\n"                                                      \
    "    add     r3, r8\n"                                                      \
    "    mov     r4, r2\n"                                                      \
    "    orr     r4, r3\n"                                                      \
    "    bic     r4, r6\n"                                                      \
    "    bne     .Laudio_sum_clip" n "\n"                                       \
    "    lsl     r3, #8\n"                                                      \
    "    orr     r2, r3\n"                                                      \
    "    mov     r4, r10\n"                                                     \
    "    eor     r2, r4\n"                                                      \
    ".Laudio_sum_store" n ":\n"                                                 \
    "    stmia   r0!, {r2}\n"                                                   \
    "    cmp     r1, r9\n"                                                      \
    "    bne     .Laudio_sum" n "\n"                                            \
    "    b       .Laudio_summed\n"                                              \
    ".Laudio_sum_clip" n ":\n"                                                  \
    "    bl      audio_mixer_clip\n"                                            \
    "    b       .Laudio_sum_store" n "\n"
static void __attribute__((noinline, noclone, unused, target("thumb")))
audio_mixer_sum(volatile u32 *dst, const u32 *src, u32 n, u32 words)
{
    if (!words) return;
    register volatile u32 *d __asm__("r0") = dst;
    register const u32 *s __asm__("r1") = src;
    register u32 voices __asm__("r2") = n;
    register u32 count __asm__("r3") = words;
    __asm__ volatile (
        "    mul     r3, r2\n"
        "    lsl     r3, #2\n"
        "    add     r3, r1\n"
        "    mov     r9, r3\n"
        "    mov     r6, #255\n"                /* 0x00ff00ff */
        "    lsl     r4, r6, #16\n"
        "    orr     r6, r4\n"
        "    mov     r7, #128\n"                /* 0x80808080 */
        "    lsl     r4, r7, #8\n"
        "    orr     r7, r4\n"
        "    lsl     r4, r7, #16\n"
        "    orr     r7, r4\n"
        "    mov     r10, r7\n"
        "    sub     r3, r2, #1\n"
        "    lsl     r3, #7\n"                  /* 128 * (n - 1), in both lanes */
        "    lsl     r4, r3, #16\n"
        "    orr     r3, r4\n"
        "    mov     r11, r3\n"
        "    neg     r3, r3\n"
        "    mov     r8, r3\n"
        "    cmp     r2, #3\n"
        "    beq     .Laudio_sum3\n"
        "    bhi     .Laudio_sum4\n"
        ".Laudio_sum2:\n"
        "    ldmia   r1!, {r2, r4}\n"
        "    lsr     r3, r2, #8\n"
        "    and     r3, r6\n"
        "    and     r2, r6\n"
        AUDIO_SUM_ASM_VOICE("r4", "r5")
        AUDIO_SUM_ASM_TAIL("2")
        ".Laudio_sum3:\n"
        "    ldmia   r1!, {r2, r4, r5}\n"
        "    lsr     r3, r2, #8\n"
        "    and     r3, r6\n"
        "    and     r2, r6\n"
        AUDIO_SUM_ASM_VOICE("r4", "r7")
        AUDIO_SUM_ASM_VOICE("r5", "r7")
        AUDIO_SUM_ASM_TAIL("3")
        ".Laudio_sum4:\n"
        "    ldmia   r1!, {r2, r4, r5, r7}\n"
        "    lsr     r3, r2, #8\n"
        "    and     r3, r6\n"
        "    and     r2, r6\n"
        "    mov     r12, r4\n"                 /* no register is free yet */
        "    and     r4, r6\n"
        "    add     r2, r4\n"
        "    mov     r4, r12\n"
        "    lsr     r4, #8\n"
        "    and     r4, r6\n"
        "    add     r3, r4\n"
        AUDIO_SUM_ASM_VOICE("r5", "r4")
        AUDIO_SUM_ASM_VOICE("r7", "r4")
        AUDIO_SUM_ASM_TAIL("4")
        ".Laudio_summed:\n"
        : "+l"(d), "+l"(s), "+l"(voices), "+l"(count)
        :
        : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "lr", "cc", "memory");
}
#undef AUDIO_SUM_ASM_TAIL
#undef AUDIO_SUM_ASM_VOICE
#endif
#if !defined(AUDIO_CODEC_HAND_WRITTEN) || defined(AUDIO_CODEC_REFERENCE)
#ifdef AUDIO_CODEC_HAND_WRITTEN
#define AUDIO_MIXER_SUM audio_mixer_sum_reference
#else
#define AUDIO_MIXER_SUM audio_mixer_sum
#endif
static __attribute__((noinline, unused)) void AUDIO_MIXER_SUM(volatile u32 *dst, const u32 *src, u32 n, u32 words)
{
    const u32 mask = 0x00ff00ffu;
    const s32 bias = (s32)(128u * n);
    const u32 rebias = 128u * (n - 1u) * 0x00010001u;
    for (u32 k = 0; k < words; ++k) {
        u32 even = 0, odd = 0;
        for (u32 i = 0; i < n; ++i) {
            u32 w = *src++;
            even += w & mask;
            odd += (w >> 8) & mask;
        }
        /* Rebias every lane to one unsigned byte at once. A lane outside
         * 0..255 sets bits 8-15 or 24-31 (an underflowing low lane does so
         * too, before its borrow reaches the high lane). */
        u32 e = even - rebias, o = odd - rebias;
        if (!((e | o) & ~mask)) {
            dst[k] = (e | o << 8) ^ 0x80808080u;
            continue;
        }
        s32 s0 = audio_clip8((s32)(even & 0xffffu) - bias), s1 = audio_clip8((s32)(odd & 0xffffu) - bias);
        s32 s2 = audio_clip8((s32)(even >> 16) - bias), s3 = audio_clip8((s32)(odd >> 16) - bias);
        dst[k] = (u32)(u8)s0 | (u32)(u8)s1 << 8 | (u32)(u8)s2 << 16 | (u32)(u8)s3 << 24;
    }
}
#undef AUDIO_MIXER_SUM
#endif

/* Mix one frame into out with word stores (VRAM accepts them). A lone voice
 * decodes straight into out. Otherwise the n active voices decode into the
 * columns of an n-wide table in scratch (AUDIO_VOICES * AUDIO_FRAME_SAMPLES
 * bytes, word aligned), a voice that ends inside the frame is padded with
 * silence, and the table is summed in one pass. Lengths are multiples of
 * eight, so every part is a whole number of words.
 *
 * *silent says whether out already holds a whole frame of silence: then a
 * frame without voices writes nothing. It is updated to say whether out
 * holds silence now. A caller with several output buffers keeps one flag
 * for each. */
static inline void audio_mixer_mix(AudioMixer *m, volatile u16 *out, volatile u16 *scratch, u8 *silent)
{
    u32 count = 0, lone = 0;
    for (u32 i = 0; i < AUDIO_VOICES; ++i) {
        AudioVoice *v = &m->voice[i];
        if (v->age != 0xff) ++v->age;
        if (v->remaining) { ++count; lone = i; }
    }
    volatile u32 *dst = (volatile u32 *)(volatile void *)out;
    if (count < 2) {
        u32 n = 0;
        if (count) {
            AudioVoice *v = &m->voice[lone];
            n = v->remaining < AUDIO_FRAME_SAMPLES ? v->remaining : AUDIO_FRAME_SAMPLES;
            audio_decoder_run(&m->dec[lone], out, n);
            v->remaining -= n;
        } else if (*silent) {
            return;
        }
        for (u32 k = n / 4u; k < AUDIO_FRAME_SAMPLES / 4u; ++k) dst[k] = 0;
        *silent = !count;
        return;
    }
    u32 column = 0;
    for (u32 i = 0; i < AUDIO_VOICES; ++i) {
        AudioVoice *v = &m->voice[i];
        if (!v->remaining) continue;
        u32 n = v->remaining < AUDIO_FRAME_SAMPLES ? v->remaining : AUDIO_FRAME_SAMPLES;
        volatile u16 *slot = scratch + 2u * column++;
        audio_decoder_run_column(&m->dec[i], slot, n, count);
        v->remaining -= n;
        volatile u32 *pad = (volatile u32 *)(volatile void *)slot;
        for (u32 k = n / 4u; k < AUDIO_FRAME_SAMPLES / 4u; ++k) pad[count * k] = 0x80808080u;
    }
    /* The table is read back below as words. */
    __asm__ volatile ("" ::: "memory");
    audio_mixer_sum(dst, (const u32 *)(const void *)scratch, count, AUDIO_FRAME_SAMPLES / 4u);
    *silent = 0;
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
 * frame; mix_silent is out's flag for audio_mixer_mix(). Returns the music
 * step outcome. */
static inline int audio_mixer_frame(AudioMixer *m, const AudioBank *bank, volatile u16 *out,
                                    volatile u16 *scratch, volatile u16 *music_out, u8 *mix_silent)
{
    audio_mixer_service(m, bank);
    audio_mixer_mix(m, out, scratch, mix_silent);
    ++m->frames;
    return audio_mixer_music_step(m, bank, music_out);
}
#endif
