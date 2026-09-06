#include "mix.h"
#include "memory/heap.h"
#include <stdatomic.h>
#include <stdint.h>

static void atomic_peak_max(atomic_uint *peak, float value);

stereo_frame_t *allocate_audio_buffer(const int no_of_frames)
{
    return allocate_array(AUDIO, no_of_frames, sizeof(stereo_frame_t));
}

void mix(const stereo_frame_t *mix_buffer, stereo_frame_t *output_buffer, atomic_uint *peak_l, atomic_uint *peak_r, const int channels, int frames_to_mix)
{
    int in_index = 0;
    int out_index = 0;
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
        frames_to_mix--;
    }
}

static void atomic_peak_max(atomic_uint *peak, float value)
{
    if (value < 0.0f) value = -value;
    const uint32_t scaled = (uint32_t) (value * 65535.0f);
    uint32_t current = atomic_load_explicit(peak, memory_order_relaxed);
    while (scaled > current && !atomic_compare_exchange_weak_explicit(peak, &current, scaled, memory_order_relaxed, memory_order_relaxed))
    {
        /* current is updated by compare_exchange */
    }
}
