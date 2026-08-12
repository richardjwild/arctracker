#ifndef ARCTRACKER_PORTAUDIO_H
#define ARCTRACKER_PORTAUDIO_H

#include "api.h"

typedef struct {
    int device_index;
    char name[AUDIO_DEVICE_NAME_SIZE];
    char host_api_name[HOST_API_NAME_SIZE];
} audio_device_info_t;

audio_api_t get_output(int device_index, const char *name, const char *host_api_name);

audio_api_t get_default_output(void);

bool start_portaudio(void);

int get_available_output_count(void);

bool get_available_outputs(audio_device_info_t *available_outputs, int requested_outputs);

bool stop_portaudio(void);

#endif //ARCTRACKER_PORTAUDIO_H
