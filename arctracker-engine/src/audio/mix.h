#ifndef ARCTRACKER_MIX_H
#define ARCTRACKER_MIX_H

#include <stdatomic.h>
#include <stdbool.h>
#include "audio_api/api.h"

stereo_frame_t *allocate_audio_buffer(int no_of_frames);

bool mix(const stereo_frame_t *mix_buffer, stereo_frame_t *output_buffer, atomic_uint *peak_l, atomic_uint *peak_r, int channels, int frames_to_mix);

#endif // ARCTRACKER_MIX_H
