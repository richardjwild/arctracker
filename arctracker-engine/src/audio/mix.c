#include "mix.h"
#include "memory/heap.h"

static const float POSITIVE_0dBFS = 1.0f;
static const float NEGATIVE_0dBFS = -1.0f;

static bool has_overflowed(stereo_frame_t);

stereo_frame_t *allocate_audio_buffer(const int no_of_frames)
{
    return allocate_array(AUDIO, no_of_frames, sizeof(stereo_frame_t));
}

bool mix(const stereo_frame_t *mix_buffer, stereo_frame_t *output_buffer, const int channels, int frames_to_mix)
{
    int in_index = 0;
    int out_index = 0;
    bool overflowed = false;
    while (frames_to_mix > 0)
    {
        stereo_frame_t output_frame = {
            .l = 0.0f,
            .r = 0.0f,
        };
        for (int channel = 0; channel < channels; channel++)
        {
            const stereo_frame_t stereo_frame = mix_buffer[in_index++];
            output_frame.l += stereo_frame.l;
            output_frame.r += stereo_frame.r;
        }
        output_buffer[out_index++] = output_frame;
        overflowed |= has_overflowed(output_frame);
        frames_to_mix--;
    }
    return overflowed;
}

static bool has_overflowed(stereo_frame_t const frame)
{
    return frame.l > POSITIVE_0dBFS || frame.r > POSITIVE_0dBFS || frame.l < NEGATIVE_0dBFS || frame.r < NEGATIVE_0dBFS;
}
