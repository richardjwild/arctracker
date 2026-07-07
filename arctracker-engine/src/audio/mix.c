#include "mix.h"
#include "memory/heap.h"
#include <math.h>
#include <stdatomic.h>

static const float POSITIVE_0dBFS = 1.0f;
static const float NEGATIVE_0dBFS = -1.0f;

static void atomic_peak_max(atomic_uint *peak, float value);
static bool has_overflowed(stereo_frame_t);

stereo_frame_t *allocate_audio_buffer(const int no_of_frames)
{
    return allocate_array(AUDIO, no_of_frames, sizeof(stereo_frame_t));
}

bool mix(const stereo_frame_t *mix_buffer, stereo_frame_t *output_buffer, atomic_uint *peak_l, atomic_uint *peak_r, const int channels, int frames_to_mix)
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
        atomic_peak_max(peak_l, output_frame.l);
        atomic_peak_max(peak_r, output_frame.r);
        overflowed |= has_overflowed(output_frame);
        frames_to_mix--;
    }
    return overflowed;
}

static void atomic_peak_max(atomic_uint *peak, float value)
{
    if (value < 0.0f) value = -value;
    if (value > 1.0f) value = 1.0f;
    const unsigned scaled = (unsigned) (value * 65535.0f);
    unsigned current = atomic_load_explicit(peak, memory_order_relaxed);
    while (scaled > current && !atomic_compare_exchange_weak_explicit(peak, &current, scaled, memory_order_relaxed, memory_order_relaxed))
    {
        /* current is updated by compare_exchange */
    }
}

static bool has_overflowed(stereo_frame_t const frame)
{
    return frame.l > POSITIVE_0dBFS || frame.r > POSITIVE_0dBFS || frame.l < NEGATIVE_0dBFS || frame.r < NEGATIVE_0dBFS;
}
