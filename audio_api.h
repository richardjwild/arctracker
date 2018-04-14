#ifndef INCLUDE_AUDIO_API_H

#include <bits/types.h>

typedef struct {
    void (*write)(__int16_t *audio_buffer);
    void (*finish)(void);
    int buffer_size_frames;
    long sample_rate;
} audio_api_t;

#define INCLUDE_AUDIO_API_H

#endif