#ifndef INCLUDE_AUDIO_API_H

#include <bits/types.h>

#define DEFAULT_SAMPLERATE 44100
#define AUDIO_BUFFER_SIZE_FRAMES 1024

typedef struct {
    void (*write)(__int16_t *audio_buffer, long frames_in_buffer);
    void (*finish)(void);
    int buffer_size_frames;
    int sample_rate;
} audio_api_t;

#define INCLUDE_AUDIO_API_H

#endif