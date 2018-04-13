#include "write_audio.h"
#include "audio_api.h"
#include "mix.h"
#include "resample.h"
#include "heap.h"
#include "play_mod.h"
#include "gain.h"

static long *channel_buffer;
static audio_api_t audio_output;
static long frames_filled;
static int channels;
static int channel_buffer_stride_length;

void initialise_audio(audio_api_t audio_output_in, long channels_in)
{
    audio_output = audio_output_in;
    channels = (int) channels_in;
    channel_buffer = allocate_array(audio_output_in.buffer_size_frames * channels * 2, sizeof(long));
    calculate_phase_increments(audio_output_in.sample_rate);
    allocate_resample_buffer(audio_output_in.buffer_size_frames);
    allocate_audio_buffer(audio_output_in.buffer_size_frames);
    channel_buffer_stride_length = (channels - 1) * 2;
    frames_filled = 0;
}

static inline
long buffer_offset_for(int channel)
{
    return ((frames_filled * channels) + channel) * 2;
}

void write_frames_for_channel(channel_info *voices, const int channel, const long frames_to_fill)
{
    channel_info *voice = voices + channel;
    long offset = buffer_offset_for(channel);
    unsigned char* resample_buffer = resample(voice, frames_to_fill);
    for (long frame = 0; frame < frames_to_fill; frame++)
    {
        unsigned char mu_law = resample_buffer[frame];
        stereo_frame_t stereo_frame = apply_gain(mu_law, voice);
        channel_buffer[offset++] = stereo_frame.l;
        channel_buffer[offset++] = stereo_frame.r;
        offset += channel_buffer_stride_length;
    }
}

static inline
bool buffer_filled()
{
    return frames_filled == audio_output.buffer_size_frames;
}

void fill_frames(channel_info *voices, const long frames_to_fill)
{
    for (int channel = 0; channel < channels; channel++)
    {
        write_frames_for_channel(voices, channel, frames_to_fill);
    }
    frames_filled += frames_to_fill;
    if (buffer_filled())
    {
        __int16_t *audio_buffer = mix(channel_buffer, channels);
        audio_output.write(audio_buffer);
        frames_filled = 0;
    }
}

static inline
long frames_unfilled()
{
    return audio_output.buffer_size_frames - frames_filled;
}

static inline
long can_be_filled(const long frames_requested)
{
    const long buffer_frames_unfilled = frames_unfilled();
    if (frames_requested > buffer_frames_unfilled)
        return buffer_frames_unfilled;
    else
        return frames_requested;
}

void write_audio_data(channel_info *voices, const long frames_requested)
{
    long frames_left = frames_requested;
    while (frames_left)
    {
        const long to_fill = can_be_filled(frames_left);
        fill_frames(voices, to_fill);
        frames_left -= to_fill;
    }
}
