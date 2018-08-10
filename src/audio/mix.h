#include "gain.h"

static int audio_buffer_frames;

void allocate_audio_buffer(int no_of_frames);

__int16_t *mix(const stereo_frame_t *channel_buffer, int channels_to_mix);

__int16_t *silence(int no_of_frames);