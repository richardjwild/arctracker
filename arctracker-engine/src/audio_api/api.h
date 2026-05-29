#ifndef ARCTRACKER_AUDIO_API_H
#define ARCTRACKER_AUDIO_API_H

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_API_SUCCESS (audio_api_result_t) {\
    .success = true\
}

static const int SAMPLE_RATE = 44100;
static const int AUDIO_BUFFER_SIZE_FRAMES = 512;

typedef struct {
    bool success;
    char *error_message;
} audio_api_result_t;

typedef struct {
    int buffer_size_frames;
    int sample_rate;
    bool bouncing;
    audio_api_result_t (*init)(void);
    audio_api_result_t (*write)(int16_t *audio_buffer, int frames_in_buffer);
    void (*finish)(bool);
} audio_api_t;

audio_api_t create_audio_api(bool bounce, char *output_filename);

audio_api_result_t audio_api_failure(char *error_message);

#endif //ARCTRACKER_AUDIO_API_H
