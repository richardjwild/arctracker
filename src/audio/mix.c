#include "mix.h"
#include <memory/heap.h>
#include <io/console.h>

static const __int16_t DIGITAL_PCM_MAX = 32767;
static const __int16_t DIGITAL_PCM_MIN = -32768;
static const float POSITIVE_0dBFS = 1.0f;
static const float NEGATIVE_0dBFS = -1.0f;
static const size_t STEREO_FRAME_SIZE = 2 * sizeof(__int16_t);

static __int16_t *audio_buffer;
static int audio_buffer_frames;

void allocate_audio_buffer(const int no_of_frames)
{
    audio_buffer = (__int16_t *) allocate_array(no_of_frames, STEREO_FRAME_SIZE);
    audio_buffer_frames = no_of_frames;
}

static inline
__int16_t clip(const float sample)
{
    if (sample > POSITIVE_0dBFS)
    {
        warn_clip();
        return DIGITAL_PCM_MAX;
    }
    else if (sample < NEGATIVE_0dBFS)
    {
        warn_clip();
        return DIGITAL_PCM_MIN;
    }
    else
        return (__int16_t) (sample * DIGITAL_PCM_MAX);
}

__int16_t *mix(const stereo_frame_t *channel_buffer, const int channels_to_mix)
{
    int input_i = 0;
    int output_i = 0;
    for (int frame = 0; frame < audio_buffer_frames; frame++)
    {
        float l_sample = 0.0, r_sample = 0.0;
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
