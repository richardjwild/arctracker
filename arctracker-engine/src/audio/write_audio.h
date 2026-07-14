#ifndef ARCTRACKER_WRITE_AUDIO_H
#define ARCTRACKER_WRITE_AUDIO_H

#include <stdatomic.h>

#include "mix.h"
#include "voice.h"
#include "audio_api/api.h"

typedef struct
{
    int num_channels;
    float master_gain;
    float gain_curve[256];
    float *phase_increments;
    float *resample_buffer;
    stereo_frame_t *mix_buffer;
    stereo_frame_t *output_buffer;
    int frames_filled;
    atomic_uint peak_l;
    atomic_uint peak_r;
    audio_api_t api;
    bool healthy;
} audio_out_t;

typedef struct {
    bool success;
    const char *error_message;
} audio_out_result_t;

audio_out_result_t initialise_audio(audio_out_t *audio_out, audio_api_t audio_api, int num_channels, float master_gain);

audio_out_result_t write_audio_data(audio_out_t *, voice_t *, int);

void send_remaining_audio(audio_out_t *);

void destroy_audio_resources(audio_out_t *);

#endif // ARCTRACKER_WRITE_AUDIO_H
