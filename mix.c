#include <stdlib.h>
#include "heap.h"
#include "mix.h"

void allocate_audio_buffer(const int no_of_frames)
{
    const size_t stereo_frame_size = 2 * sizeof(__int16_t);
    audio_buffer = (__int16_t *) allocate_array(no_of_frames, stereo_frame_size);
    audio_buffer_frames = no_of_frames;
}

static inline
__int16_t clip(const __int16_t sample)
{
    if (sample > POSITIVE_0dBFS)
        return POSITIVE_0dBFS;
    else if (sample < NEGATIVE_0dBFS)
        return NEGATIVE_0dBFS;
    else
        return sample;
}

__int16_t *mix(const stereo_frame_t *channel_buffer, const int channels_to_mix)
{
    int input_i = 0;
    int output_i = 0;
    for (int frame = 0; frame < audio_buffer_frames; frame++)
    {
        __int16_t l_sample = 0, r_sample = 0;
        for (int channel = 0; channel < channels_to_mix; channel++)
        {
            const stereo_frame_t stereo_frame = channel_buffer[input_i++];
            l_sample += stereo_frame.l;
            r_sample += stereo_frame.r;
        }
        audio_buffer[output_i++] = clip(l_sample);
        audio_buffer[output_i++] = clip(r_sample);
    }
    return audio_buffer;
}
