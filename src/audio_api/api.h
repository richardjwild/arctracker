#ifndef ARCTRACKER_AUDIO_API_H
#define ARCTRACKER_AUDIO_API_H

#include <io/configuration.h>
#include <stdint.h>

static const int SAMPLE_RATE = 44100;
static const int AUDIO_BUFFER_SIZE_FRAMES = 1024;

typedef struct {
    void (*write)(int16_t *audio_buffer, long frames_in_buffer);
    void (*finish)(void);
    int buffer_size_frames;
    int sample_rate;
} audio_api_t;

audio_api_t initialise_audio_api(args_t config);

#endif //ARCTRACKER_AUDIO_API_H
