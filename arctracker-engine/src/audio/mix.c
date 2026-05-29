#include "mix.h"
#include "memory/heap.h"

static const int16_t DIGITAL_PCM_MAX = 32767;
static const int16_t DIGITAL_PCM_MIN = -32768;
static const float POSITIVE_0dBFS = 1.0f;
static const float NEGATIVE_0dBFS = -1.0f;
static const float DIGITAL_PCM_MAX_FLOAT = 32767.0f;
static const size_t STEREO_FRAME_SIZE = 2 * sizeof(int16_t);

int16_t *allocate_audio_buffer(const int no_of_frames)
{
    return allocate_array(AUDIO, no_of_frames, STEREO_FRAME_SIZE);
}

int16_t clip(const float sample)
{
    if (sample > POSITIVE_0dBFS)
    {
        return DIGITAL_PCM_MAX;
    }
    if (sample < NEGATIVE_0dBFS)
    {
        return DIGITAL_PCM_MIN;
    }
    return (int16_t) (sample * DIGITAL_PCM_MAX_FLOAT);
}

void mix(const stereo_frame_t *channel_buffer, int16_t *output_buffer, int channels_to_mix, int no_of_frames)
{
    int input_i = 0;
    int output_i = 0;
    for (int frame = 0; frame < no_of_frames; frame++)
    {
        float l_sample = 0.0f, r_sample = 0.0f;
        for (int channel = 0; channel < channels_to_mix; channel++)
        {
            const stereo_frame_t stereo_frame = channel_buffer[input_i++];
            l_sample += stereo_frame.l;
            r_sample += stereo_frame.r;
        }
        output_buffer[output_i++] = clip(l_sample);
        output_buffer[output_i++] = clip(r_sample);
    }
}
