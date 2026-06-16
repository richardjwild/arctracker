#ifndef ARCTRACKER_AUDIO_API_H
#define ARCTRACKER_AUDIO_API_H

#include <stdbool.h>

#define AUDIO_API_SUCCESS (audio_api_result_t) {\
    .success = true,\
    .overflowed = false\
}

static const int SAMPLE_RATE = 44100;
static const int AUDIO_BUFFER_SIZE_FRAMES = 512;

typedef struct
{
    float l;
    float r;
} stereo_frame_t;

typedef struct {
    bool success;
    bool overflowed;
    const char *error_message;
} audio_api_result_t;

typedef struct {
    int buffer_size_frames;
    int sample_rate;
    bool bouncing;
    audio_api_result_t (*init)(void);
    audio_api_result_t (*write)(const stereo_frame_t *audio_buffer, int frames_in_buffer);
    void (*finish)(bool);
} audio_api_t;

audio_api_t create_audio_api(bool bounce, char *output_filename);

audio_api_result_t audio_api_failure(const char *error_message);

#endif //ARCTRACKER_AUDIO_API_H
