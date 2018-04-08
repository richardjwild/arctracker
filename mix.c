#include <stdlib.h>
#include "heap.h"
#include "mix.h"

static const int DBFS_0_POSITIVE = 32767;
static const int DBFS_0_NEGATIVE = -32768;

__int16_t *audio_buffer;

void allocate_audio_buffer(int no_of_frames)
{
    const size_t stereo_frame_size = 2 * sizeof(__int16_t);
    audio_buffer = (__int16_t *) allocate_array(no_of_frames, stereo_frame_size);
}

__int16_t clip(__int16_t sample)
{
    if (sample > DBFS_0_POSITIVE)
        return DBFS_0_POSITIVE;
    else if (sample < DBFS_0_NEGATIVE)
        return DBFS_0_NEGATIVE;
    else
        return sample;
}

__int16_t *mix(const long *channel_buffer, const long channels_to_mix, const int frames_to_mix)
{
    int input_i = 0;
    int output_i = 0;
    for (int frame = 0; frame < frames_to_mix; frame++)
    {
        __int16_t l_sample = 0, r_sample = 0;
        for (int channel = 0; channel < channels_to_mix; channel++)
        {
            l_sample += channel_buffer[input_i++];
            r_sample += channel_buffer[input_i++];
        }
        audio_buffer[output_i++] = clip(l_sample);
        audio_buffer[output_i++] = clip(r_sample);
    }
    return audio_buffer;
}
