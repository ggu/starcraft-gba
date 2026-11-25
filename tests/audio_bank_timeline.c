/* Optional private check: replay a request timeline through the mixer with
 * the generated bank and write what the VBlank handler would leave in the two
 * DMA double buffers after every video frame.
 *
 *   test-audio-bank-timeline BLOB TRACE OUT [STATS]
 *
 * TRACE has one line per VBlank since reset, as seen on entry to the handler:
 *   <music_request> <music_loop> <n> { <sound> <priority> <kind> } x n
 * (the requests waiting in the queue, oldest first). OUT receives
 * TIMELINE_BYTES per VBlank: the mix double buffer with its zero guard, then
 * the music double buffer with its guard, exactly as audio_vblank() in
 * common/gba.c lays them out in VRAM. A capture of that VRAM range from an
 * ARM run with the same timeline must be byte-identical. STATS, if given,
 * receives one line per VBlank: active voices, samples decoded for effects
 * and for music with predictor 0 and 1, and samples summed by the mixer.
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

#define DOUBLE_BYTES (2u * AUDIO_FRAME_SAMPLES + 64u)
#define TIMELINE_BYTES (2u * DOUBLE_BYTES)

static _Alignas(4) u16 arena[TIMELINE_BYTES / 2];
static _Alignas(4) u16 scratch[AUDIO_VOICES * AUDIO_FRAME_SAMPLES / 2];
static AudioDecoder decoders[AUDIO_DECODERS];

static u8 *read_file(const char *path, u32 *size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    /* Word aligned, as the linked blob is. */
    u8 *data = len > 0 ? aligned_alloc(16, ((size_t)len + 15u) & ~(size_t)15u) : NULL;
    if (!data || fread(data, 1, (size_t)len, f) != (size_t)len) { fclose(f); free(data); return NULL; }
    fclose(f);
    *size = (u32)len;
    return data;
}

int main(int argc, char **argv)
{
    if (argc != 4 && argc != 5) { fprintf(stderr, "usage: %s BLOB TRACE OUT [STATS]\n", argv[0]); return 2; }
    u32 size;
    u8 *blob = read_file(argv[1], &size);
    FILE *trace = fopen(argv[2], "r"), *out = fopen(argv[3], "wb"), *stats = argc == 5 ? fopen(argv[4], "w") : NULL;
    if (!blob || !trace || !out || (argc == 5 && !stats)) { fprintf(stderr, "audio timeline: cannot open inputs\n"); return 2; }
    if (size != SC64_AUDIO_BLOB_BYTES) { fprintf(stderr, "audio timeline: blob size differs from the header\n"); return 1; }
    const AudioBank bank = {
        blob, sc64_audio_sample_offset, sc64_audio_sample_length, SC64_AUDIO_SAMPLE_COUNT,
        sc64_audio_music_offset, sc64_audio_music_frames, SC64_AUDIO_MUSIC_TRACKS
    };
    AudioMixer m;
    audio_mixer_reset(&m, decoders);
    volatile u16 *mix_buffer = (volatile u16 *)arena, *music_buffer = (volatile u16 *)(arena + DOUBLE_BYTES / 2);
    u32 audio_half = 0, music_clear = 0, frames = 0;
    u8 mix_silent[2] = { 1, 1 };  /* the arena starts zeroed */
    unsigned long long total[5] = { 0 };
    char line[256];
    while (fgets(line, sizeof line, trace)) {
        /* One whole record per line; anything else is a damaged capture. */
        unsigned music_request, music_loop, n, sound, priority, kind;
        int used = 0, more = 0;
        size_t len = strlen(line);
        if (!len || line[len - 1] != '\n' ||
            sscanf(line, "%u %u %u%n", &music_request, &music_loop, &n, &used) != 3 ||
            n >= AUDIO_QUEUE || music_request > 255u || music_loop > 1u) {
            fprintf(stderr, "audio timeline: bad line %u\n", frames + 1u);
            return 2;
        }
        for (unsigned i = 0; i < n; ++i) {
            if (sscanf(line + used, "%u %u %u%n", &sound, &priority, &kind, &more) != 3 ||
                sound > 65535u || priority > 255u || kind > 255u) {
                fprintf(stderr, "audio timeline: bad line %u\n", frames + 1u);
                return 2;
            }
            used += more;
            if (!audio_mixer_request(&m, (u16)sound, (u8)priority, (u8)kind)) { fprintf(stderr, "audio timeline: queue full\n"); return 2; }
        }
        if (line[used] != '\n') { fprintf(stderr, "audio timeline: bad line %u\n", frames + 1u); return 2; }
        m.music_loop = (u8)music_loop;
        m.music_request = (u8)music_request;

        /* What audio_vblank() does. Servicing first only empties the queue
         * early, so the voice table can be read before the mix. */
        u32 half = 0;
        if (audio_half) half = AUDIO_FRAME_SAMPLES / 2u;
        audio_half ^= 1u;
        audio_mixer_service(&m, &bank);
        u32 voices = 0, fx[2] = { 0, 0 }, mu[2] = { 0, 0 }, summed = 0;
        for (u32 i = 0; i < AUDIO_VOICES; ++i) {
            if (!m.voice[i].remaining) continue;
            u32 k = m.voice[i].remaining < AUDIO_FRAME_SAMPLES ? m.voice[i].remaining : AUDIO_FRAME_SAMPLES;
            ++voices;
            fx[decoders[i].state >> 10 & 1u] += k;
            summed += k;
        }
        if (voices < 2) summed = 0;
        int music = audio_mixer_frame(&m, &bank, mix_buffer + half, (volatile u16 *)scratch, music_buffer + half, &mix_silent[half != 0]);
        if (music_clear && m.music_track == AUDIO_MUSIC_NONE) memset(arena + DOUBLE_BYTES / 2 + half, 0, AUDIO_FRAME_SAMPLES);
        music_clear = music == AUDIO_MUSIC_SILENCE;
        if (m.music_track != AUDIO_MUSIC_NONE) mu[decoders[AUDIO_VOICES].state >> 10 & 1u] = AUDIO_FRAME_SAMPLES;

        if (fwrite(arena, 1, TIMELINE_BYTES, out) != TIMELINE_BYTES) { fprintf(stderr, "audio timeline: write failed\n"); return 2; }
        if (stats) fprintf(stats, "%u %u %u %u %u %u\n", voices, fx[0], fx[1], mu[0], mu[1], summed);
        total[0] += voices; total[1] += fx[0]; total[2] += fx[1]; total[3] += mu[0]; total[4] += mu[1];
        ++frames;
    }
    if (ferror(trace) || !feof(trace)) { fprintf(stderr, "audio timeline: read failed after line %u\n", frames); return 2; }
    if (fclose(out)) { fprintf(stderr, "audio timeline: write failed\n"); return 2; }
    if (stats) fclose(stats);
    fclose(trace);
    free(blob);
    printf("audio timeline: %u frames, %llu voice frames, effect samples %llu + %llu, music samples %llu + %llu (predictor 0 + 1), %u dropped\n",
           frames, total[0], total[1], total[2], total[3], total[4], m.dropped);
    return 0;
}
