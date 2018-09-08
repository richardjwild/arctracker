#ifndef ARCTRACKER_AUDIO_API_H
#define ARCTRACKER_AUDIO_API_H

#include <bits/types.h>
#include <io/configuration.h>

static const int DEFAULT_SAMPLERATE = 44100;
static const int AUDIO_BUFFER_SIZE_FRAMES = 1024;

typedef struct {
    void (*write)(__int16_t *audio_buffer, long frames_in_buffer);
    void (*finish)(void);
    int buffer_size_frames;
    int sample_rate;
} audio_api_t;

audio_api_t initialise_audio_api(output_api api);

#endif //ARCTRACKER_AUDIO_API_H
