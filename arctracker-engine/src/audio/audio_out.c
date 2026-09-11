#include <string.h>
#include <stdatomic.h>
#include "audio_out.h"
#include "sample_player.h"
#include "memory/heap.h"
#include "pcm/mu_law.h"

static const float PAN_HARD_LEFT = 1.0f;
static const float PAN_HARD_RIGHT = 255.0f;

static void calculate_gain_curve(float *, volume_mapping_type_t);
static bool fill_audio_buffer(audio_out_t *, audio_channel_t *, int);
static void write_audio_for_channel(const audio_out_t *, audio_channel_t *, int);
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
    calculate_gain_curve(audio_out->gain_curve, audio_out->volume_mapping_type);
    audio_out->api.info.healthy = true;
    return audio_out->api.init(&audio_api.info);
}

static void calculate_gain_curve(float *gain_curve, const volume_mapping_type_t volume_mapping)
{
    if (volume_mapping == VOLUME_AMIGA)
    {
        for (int i = 0; i <= 255; i++)
            gain_curve[i] = (float) i / 255;
    }
    else
    {
        for (int i = 0; i <= 127; i++)
        {
            gain_curve[i * 2 + 1] = mu_law_to_linear(255 - i);
            if (i >= 1)
                gain_curve[i * 2] = (gain_curve[i * 2 - 1] + gain_curve[i * 2 + 1]) / 2;
        }
        gain_curve[0] = 0.0f;
        gain_curve[1] = gain_curve[2] / 2;
    }
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
    float *mono_channel_buffer = audio_out->mono_channel_buffer;
    //
    // Generate channel audio.
    //
    if (channel->playing)
    {
        audio_generator_t *audio_generator = &channel->audio_generator;
        channel->playing = audio_generator->generate_audio(&audio_generator->state, mono_channel_buffer, frames_to_fill);
    }
    else
    {
        memset(mono_channel_buffer, 0, frames_to_fill * sizeof(float));
    }
    //
    // This is the point where we would apply mono effects: filtering, compression, distortion, etc.
    //
    float left_gain = 0.0f;
    float right_gain = 0.0f;
    if (!channel->muted)
    {
        left_gain = (PAN_HARD_RIGHT - (float) channel->panning) / 254.0f;
        right_gain = ((float) channel->panning - PAN_HARD_LEFT) / 254.0f;
    }
    //
    // Copy the mono channel buffer to the stereo channel buffer, applying panning as we go.
    //
    stereo_frame_t *stereo_channel_buffer = audio_out->stereo_channel_buffer;
    for (int frame = 0; frame < frames_to_fill; frame++)
    {
        const float pcm = mono_channel_buffer[frame];
        stereo_channel_buffer[frame].l = pcm * left_gain;
        stereo_channel_buffer[frame].r = pcm * right_gain;
    }
    //
    // This is the point where we would apply stereo effects, such as delay.
    //
    // Now mix the stereo channel buffer into the master output buffer.
    //
    stereo_frame_t *output_buffer = audio_out->output_buffer + audio_out->frames_filled;
    const float channel_gain = channel->gain;
    for (int frame = 0; frame < frames_to_fill; frame++)
    {
        output_buffer[frame].l += stereo_channel_buffer[frame].l * channel_gain;
        output_buffer[frame].r += stereo_channel_buffer[frame].r * channel_gain;
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
