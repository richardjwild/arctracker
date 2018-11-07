#ifndef ARCTRACKER_MIX_H
#define ARCTRACKER_MIX_H

#include <audio/gain.h>

#define PAN_LEFT_FULL 1
#define PAN_LEFT_MEDIUM 2
#define PAN_LEFT_SLIGHT 3
#define PAN_MIDDLE 4
#define PAN_RIGHT_SLIGHT 5
#define PAN_RIGHT_MEDIUM 6
#define PAN_RIGHT_FULL 7

void allocate_audio_buffer(int no_of_frames);

int16_t *mix(const stereo_frame_t *channel_buffer, int channels_to_mix);

#endif // ARCTRACKER_MIX_H