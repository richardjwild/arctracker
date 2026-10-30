#include <string.h>
#include <stdatomic.h>
#include "audio_out.h"
#include <math.h>
#include "memory/heap.h"

static void write_audio_for_channel(const audio_out_t *audio_out, audio_channel_t *channel, unsigned int frames_to_fill, float *output_buffer);
static void mix_to_output_buffer(const stereo_frame_t *channel_buffer, float *output_buffer, unsigned int frames, float gain);
static void apply_master_gain(float, unsigned int, float *);
static void find_peak_levels(atomic_uint *peak_l, atomic_uint *peak_r, unsigned int, const float *);
static void atomic_peak_max(atomic_uint *peak, float value);

bool initialise_audio(audio_out_t *audio_out, const int num_channels, const double sample_rate, const float master_gain)
{
    audio_out->num_channels = num_channels;
    audio_out->sample_rate = sample_rate;
    audio_out->master_gain = master_gain;
    audio_out->peak_l = 0;
    audio_out->peak_r = 0;
    audio_out->mono_channel_buffer = allocate_array(AUDIO, AUDIO_BUFFER_SIZE_FRAMES, sizeof(float));
    if (audio_out->mono_channel_buffer == NULL)
    {
        return false;
    }
    audio_out->stereo_channel_buffer = allocate_array(AUDIO, AUDIO_BUFFER_SIZE_FRAMES, sizeof(stereo_frame_t));
    if (audio_out->stereo_channel_buffer == NULL)
    {
        deallocate(AUDIO, audio_out->mono_channel_buffer);
        return false;
    }
    return true;
}

void write_audio_data(audio_out_t *audio_out, audio_channel_t *channels, const unsigned int samples_requested, float *output_buffer)
{
    memset(output_buffer, 0, samples_requested * 2 * sizeof(float));
    //
    // The player has called us to output one buffer's worth of samples, which is given by samples_to_write. Our
    // internal channel buffers may or may not be large enough; if not then we will need more than one pass until all
    // the requested samples are written.
    //
    int samples_to_write = samples_requested;
    int buffer_offset = 0;
    while (samples_to_write)
    {
        int samples_to_write_now = samples_to_write;
        if (samples_to_write_now > AUDIO_BUFFER_SIZE_FRAMES)
        {
            samples_to_write_now = AUDIO_BUFFER_SIZE_FRAMES;
        }
        for (int channel = 0; channel < audio_out->num_channels; channel++)
        {
            write_audio_for_channel(audio_out, &channels[channel], samples_to_write_now, output_buffer + buffer_offset);
        }
        samples_to_write -= samples_to_write_now;
        buffer_offset += samples_to_write_now * 2;
    }
    apply_master_gain(audio_out->master_gain, samples_requested, output_buffer);
    find_peak_levels(&audio_out->peak_l, &audio_out->peak_r, samples_requested, output_buffer);
}

static void write_audio_for_channel(const audio_out_t *audio_out, audio_channel_t *channel, const unsigned int frames_to_fill, float *output_buffer)
{
    audio_generator_t *audio_generator = &channel->audio_generator;
    float *mono_channel_buffer = audio_out->mono_channel_buffer;
    const bool has_more_audio = audio_generator->generate_audio(&audio_generator->state, mono_channel_buffer, frames_to_fill);
    if (!has_more_audio)
    {
        audio_channel_silence(channel);
    }
    //
    // This is the point where we may apply mono effects: filtering, compression, distortion, etc.
    //
    audio_spatialiser_t *spatialiser = &channel->audio_spatialiser;
    stereo_frame_t *stereo_channel_buffer = audio_out->stereo_channel_buffer;
    spatialiser->spatialise(&spatialiser->state, mono_channel_buffer, stereo_channel_buffer, frames_to_fill);
    //
    // This is the point where we may apply stereo effects, such as delay.
    //
    if (!channel->muted)
    {
        mix_to_output_buffer(stereo_channel_buffer, output_buffer, frames_to_fill, channel->gain);
    }
}

static void mix_to_output_buffer(const stereo_frame_t *channel_buffer, float *output_buffer, const unsigned int frames, const float gain)
{
    for (int frame = 0; frame < frames; frame++)
    {
        output_buffer[frame * 2] += channel_buffer[frame].l * gain;
        output_buffer[frame * 2 + 1] += channel_buffer[frame].r * gain;
    }
}

static void apply_master_gain(const float master_gain, const unsigned int frames_filled, float *output_buffer)
{
    for (int frame = 0; frame < frames_filled; frame++)
    {
        output_buffer[frame * 2] *= master_gain;
        output_buffer[1 + frame * 2] *= master_gain;
    }
}

static void find_peak_levels(atomic_uint *peak_l, atomic_uint *peak_r, const unsigned int frames_filled, const float *output_buffer)
{
    for (int frame = 0; frame < frames_filled; frame++)
    {
        atomic_peak_max(peak_l, output_buffer[frame * 2]);
        atomic_peak_max(peak_r, output_buffer[1 + frame * 2]);
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

void destroy_audio_resources(const audio_out_t *audio_out)
{
    deallocate(AUDIO, audio_out->mono_channel_buffer);
    deallocate(AUDIO, audio_out->stereo_channel_buffer);
}
