#ifndef ARCTRACKER_MIX_H
#define ARCTRACKER_MIX_H

#include <audio/gain.h>

void allocate_audio_buffer(int no_of_frames);

int16_t *mix(const stereo_frame_t *channel_buffer, int channels_to_mix);

#endif // ARCTRACKER_MIX_H