#ifndef ARCTRACKER_AUDIO_API_H
#define ARCTRACKER_AUDIO_API_H

#include <stdbool.h>
#include "audio/interpolation_type.h"

#define AUDIO_DEVICE_NAME_SIZE 256
#define HOST_API_NAME_SIZE 256
#define MAX_FILE_PATH_LENGTH 4096

static const int SAMPLE_RATE = 44100;
static const int AUDIO_BUFFER_SIZE_FRAMES = 512;

typedef struct {
    float l;
    float r;
} stereo_frame_t;

typedef struct {
    int device_index;
    char name[AUDIO_DEVICE_NAME_SIZE];
    char host_api_name[HOST_API_NAME_SIZE];
} portaudio_config_t;

typedef struct {
    char output_filename[MAX_FILE_PATH_LENGTH];
} wav_config_t;

typedef union {
    portaudio_config_t portaudio;
    wav_config_t wav;
} audio_backend_config_t;

typedef struct {
    int buffer_size_frames;
    int sample_rate;
    bool bouncing;
    bool healthy;
    interpolation_type_t interpolation_type;
    audio_backend_config_t config;
} audio_api_info_t;

typedef struct {
    audio_api_info_t info;
    bool (*init)(const audio_api_info_t *info);
    bool (*write)(const stereo_frame_t *audio_buffer, int frames_in_buffer);
    void (*finish)(const audio_api_info_t *info);
} audio_api_t;

#endif //ARCTRACKER_AUDIO_API_H
