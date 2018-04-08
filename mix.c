#include <stdlib.h>
#include "heap.h"

__int16_t *audio_buffer;

void allocate_audio_buffer(int no_of_frames)
{
    const size_t stereo_frame_size = 2 * sizeof(__int16_t);
    audio_buffer = (__int16_t *) allocate_array(no_of_frames, stereo_frame_size);
}

__int16_t clip(__int16_t sample)
{
    if (sample > 32767)
        return 32767;
    else if (sample < -32768)
        return -32768;
    else
        return sample;
}

__int16_t *mix(const long *channel_buffer, const long channels_to_mix, const int frames_to_mix)
{
    int channel_buffer_index = 0;
    int audio_buffer_index = 0;
    for (int frame = 0; frame < frames_to_mix; frame++)
    {
        __int16_t lval = 0, rval = 0;
        for (int channel = 0; channel < channels_to_mix; channel++)
        {
            lval += channel_buffer[channel_buffer_index++];
            rval += channel_buffer[channel_buffer_index++];
        }
        audio_buffer[audio_buffer_index++] = clip(lval);
        audio_buffer[audio_buffer_index++] = clip(rval);
    }
    return audio_buffer;
}
