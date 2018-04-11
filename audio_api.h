#ifndef INCLUDE_AUDIO_API_H

typedef struct {
    void (*write_audio)(__int16_t *audio_buffer);
    int audio_buffer_frames;
} audio_api_t;

#define INCLUDE_AUDIO_API_H

#endif