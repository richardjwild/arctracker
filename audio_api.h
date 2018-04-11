#ifndef INCLUDE_AUDIO_API_H

typedef struct {
    void (*write)(__int16_t *audio_buffer);
    int buffer_size_frames;
} audio_api_t;

#define INCLUDE_AUDIO_API_H

#endif