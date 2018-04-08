#include <stdlib.h>
#include "heap.h"

unsigned char *audio_buffer;

void allocate_audio_buffer(int no_of_frames)
{
    audio_buffer = (unsigned char *) allocate_array(no_of_frames * 4, sizeof(unsigned char));
}

short clip(short sample)
{
    if (sample > 32767)
        return 32767;
    else if (sample < -32768)
        return -32768;
    else
        return sample;
}

unsigned char *mix(long *channel_buffer, long channels_to_mix, int frames_to_mix)
{
    int channel_buffer_index = 0;
    int audio_buffer_index = 0;
    for (int frame = 0; frame < frames_to_mix; frame++)
    {
        long lval = 0, rval = 0;
        for (int channel = 0; channel < channels_to_mix; channel++)
        {
            lval += channel_buffer[channel_buffer_index++];
            rval += channel_buffer[channel_buffer_index++];
        }
        lval = clip(lval);
        rval = clip(rval);
        audio_buffer[audio_buffer_index++] = (unsigned char) (lval & 0xff);
        audio_buffer[audio_buffer_index++] = (unsigned char) ((lval >> 8) & 0xff);
        audio_buffer[audio_buffer_index++] = (unsigned char) (rval & 0xff);
        audio_buffer[audio_buffer_index++] = (unsigned char) ((rval >> 8) & 0xff);
    }
    return audio_buffer;
}
