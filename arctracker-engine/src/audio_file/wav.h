#ifndef ARCTRACKER_ENGINE_WAV_H
#define ARCTRACKER_ENGINE_WAV_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int sample_rate;
    int channels;
    int bits_per_sample;
    int frames;
    int16_t *pcm16;
} wav_pcm16_t;

bool wav_write_pcm16(const char *filename, const wav_pcm16_t *wav);

#endif //ARCTRACKER_ENGINE_WAV_H
