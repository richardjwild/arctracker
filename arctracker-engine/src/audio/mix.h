#ifndef ARCTRACKER_MIX_H
#define ARCTRACKER_MIX_H

#include <stdint.h>

typedef struct
{
    float l;
    float r;
} stereo_frame_t;

int16_t *allocate_audio_buffer(int no_of_frames);

void mix(const stereo_frame_t *channel_buffer, int16_t *output_buffer, int channels_to_mix, int no_of_frames);

#endif // ARCTRACKER_MIX_H
