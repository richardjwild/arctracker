#include <string.h>
#include <stdatomic.h>
#include "audio_out.h"
#include <math.h>
#include "memory/heap.h"

static bool fill_audio_buffer(audio_out_t *, audio_channel_t *, int);
static void write_audio_for_channel(const audio_out_t *, audio_channel_t *, int);
static void mix_to_output_buffer(const stereo_frame_t *, stereo_frame_t *, int, float);
static void apply_master_gain(const audio_out_t *);
static void find_peak_levels(const audio_out_t *, atomic_uint *, atomic_uint *);
static void atomic_peak_max(atomic_uint *, float);

bool initialise_audio(audio_out_t *audio_out, const audio_api_t audio_api, const int num_channels, const float master_gain, const volume_mapping_type_t volume_mapping_type)
{
    audio_out->api = audio_api;
    audio_out->num_channels = num_channels;
    audio_out->master_gain = master_gain;
    audio_out->interpolation_type = audio_api.info.interpolation_type;
    audio_out->mono_channel_buffer = allocate_array(AUDIO, audio_api.info.buffer_size_frames, sizeof(float));
    audio_out->stereo_channel_buffer = allocate_array(AUDIO, audio_api.info.buffer_size_frames, sizeof(stereo_frame_t));
    audio_out->output_buffer = allocate_array(AUDIO, audio_api.info.buffer_size_frames, sizeof(stereo_frame_t));
    audio_out->frames_filled = 0;
    audio_out->peak_l = 0;
    audio_out->peak_r = 0;
    audio_out->volume_mapping_type = volume_mapping_type;
    audio_out->api.info.healthy = true;
    return audio_out->api.init(&audio_api.info);
}

bool write_audio_data(audio_out_t *audio_out, audio_channel_t *channels, int samples_to_write)
{
    //
    // The player has called us to output one tick's worth of samples, which is given by samples_to_write.
    // We will write it all to the audio buffer, sending to the device whenever the buffer becomes full.
    //
    while (samples_to_write)
    {
        const int buffer_frames_unfilled = audio_out->api.info.buffer_size_frames - audio_out->frames_filled;
        const int samples_to_write_now = samples_to_write > buffer_frames_unfilled ? buffer_frames_unfilled : samples_to_write;
        if (!fill_audio_buffer(audio_out, channels, samples_to_write_now))
        {
            audio_out->api.info.healthy = false;
            return false;
        }
        samples_to_write -= samples_to_write_now;
    }
    return true;
}

static bool fill_audio_buffer(audio_out_t *audio_out, audio_channel_t *channels, const int frames_to_fill)
{
    for (int channel = 0; channel < audio_out->num_channels; channel++)
    {
        write_audio_for_channel(audio_out, &channels[channel], frames_to_fill);
    }
    audio_out->frames_filled += frames_to_fill;
    if (audio_out->frames_filled < audio_out->api.info.buffer_size_frames)
    {
        // Audio buffer is not yet filled, but we have to break now because it is time for the player to tick.
        return true;
    }
    apply_master_gain(audio_out);
    find_peak_levels(audio_out, &audio_out->peak_l, &audio_out->peak_r);
    //
    // Now we have a full output buffer ready to send to the device.
    //
    const bool result = audio_out->api.write(audio_out->output_buffer, audio_out->api.info.buffer_size_frames);
    memset(audio_out->output_buffer, 0, audio_out->frames_filled * sizeof(stereo_frame_t));
    audio_out->frames_filled = 0;
    return result;
}

static void write_audio_for_channel(const audio_out_t *audio_out, audio_channel_t *channel, const int frames_to_fill)
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
        stereo_frame_t *output_buffer = audio_out->output_buffer + audio_out->frames_filled;
        mix_to_output_buffer(stereo_channel_buffer, output_buffer, frames_to_fill, channel->gain);
    }
}

static void mix_to_output_buffer(const stereo_frame_t *channel_buffer, stereo_frame_t *output_buffer, const int frames, const float gain)
{
    for (int frame = 0; frame < frames; frame++)
    {
        output_buffer[frame].l += channel_buffer[frame].l * gain;
        output_buffer[frame].r += channel_buffer[frame].r * gain;
    }
}

static void apply_master_gain(const audio_out_t *audio_out)
{
    stereo_frame_t *output_buffer = audio_out->output_buffer;
    const float master_gain = audio_out->master_gain;
    for (int frame = 0; frame < audio_out->frames_filled; frame++)
    {
        output_buffer[frame].l *= master_gain;
        output_buffer[frame].r *= master_gain;
    }
}

static void find_peak_levels(const audio_out_t *audio_out, atomic_uint *peak_l, atomic_uint *peak_r)
{
    const stereo_frame_t *output_buffer = audio_out->output_buffer;
    for (int frame = 0; frame < audio_out->frames_filled; frame++)
    {
        const stereo_frame_t output_frame = output_buffer[frame];
        atomic_peak_max(peak_l, output_frame.l);
        atomic_peak_max(peak_r, output_frame.r);
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

void send_remaining_audio(audio_out_t *audio_out)
{
    if (audio_out->frames_filled > 0)
    {
        if (!audio_out->api.write(audio_out->output_buffer, audio_out->frames_filled))
            audio_out->api.info.healthy = false;
    }
}

void destroy_audio_resources(const audio_out_t *audio_out)
{
    audio_out->api.finish(&audio_out->api.info);
    deallocate(AUDIO, audio_out->mono_channel_buffer);
    deallocate(AUDIO, audio_out->stereo_channel_buffer);
    deallocate(AUDIO, audio_out->output_buffer);
}
