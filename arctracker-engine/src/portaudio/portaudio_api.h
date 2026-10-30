#ifndef ARCTRACKER_PORTAUDIO_H
#define ARCTRACKER_PORTAUDIO_H

#include "portaudio.h"
#include <stdbool.h>

#define AUDIO_DEVICE_NAME_SIZE 256
#define HOST_API_NAME_SIZE 256
#define DEFAULT_PORTAUDIO_DEVICE_INDEX -1

typedef struct {
    int device_index;
    char name[AUDIO_DEVICE_NAME_SIZE];
    char host_api_name[HOST_API_NAME_SIZE];
    double sample_rate;
} portaudio_config_t;

typedef struct {
    int device_index;
    char name[AUDIO_DEVICE_NAME_SIZE];
    char host_api_name[HOST_API_NAME_SIZE];
} audio_device_info_t;

typedef struct {
    bool success;
    double sample_rate;
    double reported_latency;
} open_stream_result_t;

open_stream_result_t open_portaudio_stream(const portaudio_config_t *config, PaStream **pa_stream, PaStreamCallback *callback, void *user_data);

bool start_portaudio_stream(PaStream *pa_stream);

void close_portaudio_stream(PaStream *pa_stream);

bool start_portaudio(void);

bool stop_portaudio(void);

int get_available_output_count(void);

bool get_available_outputs(audio_device_info_t *available_outputs, int requested_outputs);

#endif //ARCTRACKER_PORTAUDIO_H
