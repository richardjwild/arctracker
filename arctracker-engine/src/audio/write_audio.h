#ifndef ARCTRACKER_WRITE_AUDIO_H
#define ARCTRACKER_WRITE_AUDIO_H

#include <stdatomic.h>
#include "voice.h"
#include "audio_api/api.h"
#include "audio/volume_mapping_type.h"

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
    interpolation_type_t interpolation_type;
    volume_mapping_type_t volume_mapping_type;
    atomic_uint peak_l;
    atomic_uint peak_r;
    audio_api_t api;
} audio_out_t;

typedef struct {
    bool success;
    const char *error_message;
} audio_out_result_t;

bool initialise_audio(audio_out_t *audio_out, audio_api_t audio_api, int num_channels, float master_gain, volume_mapping_type_t volume_mapping_type);

bool write_audio_data(audio_out_t *, voice_t *, int);

void send_remaining_audio(audio_out_t *);

void destroy_audio_resources(audio_out_t *);

#endif // ARCTRACKER_WRITE_AUDIO_H
