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

typedef struct {
    float *audio_data;
    int frames;
    int sample_rate;
} audio_t;

bool wav_write_pcm16(const char *filename, const wav_pcm16_t *wav);

audio_t wav_read_audio(const uint8_t *wav_data, size_t loaded_data_size);

#endif //ARCTRACKER_ENGINE_WAV_H
