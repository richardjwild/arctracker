#include "write_audio.h"
#include "audio_api.h"
#include "mix.h"
#include "resample.h"
#include "log_lin_tab.h"
#include "heap.h"
#include "play_mod.h"

void initialise_audio(audio_api_t audio_api, long number_of_channels)
{
    channels = (int) number_of_channels;
    channel_buffer = allocate_array(audio_api.buffer_size_frames * channels * 2, sizeof(long));
    calculate_phase_increments(audio_api.sample_rate);
    allocate_resample_buffer(audio_api.buffer_size_frames);
    allocate_audio_buffer(audio_api.buffer_size_frames);
    channel_buffer_stride_length = (channels - 1) * 2;
    frames_filled = 0;
}

unsigned char adjust_gain(unsigned char mlaw, unsigned char gain)
{
    unsigned char adjustment = ((unsigned char) 255) - gain;
    if (mlaw > adjustment)
        return mlaw - adjustment;
    else
        return 0;
}

void write_channel_audio_data(
        channel_info* voice,
        long frames_to_write,
        long channel_buffer_index,
        unsigned char master_gain,
        int stride_length)
{
    unsigned char* resample_buffer = resample(voice, frames_to_write);
    for (long frame = 0; frame < frames_to_write; frame++)
    {
        unsigned char mu_law = resample_buffer[frame];
        mu_law = adjust_gain(mu_law, voice->gain);
        long linear_signed = log_lin_tab[mu_law];

        long left = (linear_signed * voice->left_channel_multiplier) >> 16;
        long right = (linear_signed * voice->right_channel_multiplier) >> 16;

        channel_buffer[channel_buffer_index++] = left * master_gain;
        channel_buffer[channel_buffer_index++] = right * master_gain;
        channel_buffer_index += stride_length;
    }
}

static inline
long channel_buffer_offset(long frames_filled, int channel)
{
    return ((frames_filled * channels) + channel) * 2;
}

static inline
long frames_unfilled(audio_api_t audio_output)
{
    return audio_output.buffer_size_frames - frames_filled;
}

static inline
long frames_can_be_written(audio_api_t audio_output, long frames_requested)
{
    const long frames_left_in_buffer = frames_unfilled(audio_output);
    return frames_requested > frames_left_in_buffer
           ? frames_left_in_buffer
           : frames_requested;
}

void write_audio_data(
        audio_api_t audio_output,
        channel_info *voices,
        unsigned char master_gain,
        long frames_requested)
{
    while (frames_requested > 0)
    {
        const long frames_to_write = frames_can_be_written(audio_output, frames_requested);
        for (int channel = 0; channel < channels; channel++)
        {
            write_channel_audio_data(
                    &voices[channel],
                    frames_to_write,
                    channel_buffer_offset(frames_filled, channel),
                    master_gain,
                    channel_buffer_stride_length);
        }
        frames_filled += frames_to_write;
        if (frames_filled == audio_output.buffer_size_frames)
        {
            __int16_t *audio_buffer = mix(channel_buffer, channels);
            audio_output.write(audio_buffer);
            frames_filled = 0;
        }
        frames_requested -= frames_to_write;
    }
}
