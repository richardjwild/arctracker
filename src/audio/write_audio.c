#include "write_audio.h"
#include "audio_api.h"
#include "mix.h"
#include "resample.h"
#include "gain.h"
#include <chrono/clock.h>
#include <memory/heap.h>

static stereo_frame_t *channel_buffer;
static audio_api_t audio_output;
static long frames_filled;
static int channels;

void initialise_audio(audio_api_t audio_output_in, long channels_in)
{
    audio_output = audio_output_in;
    channels = (int) channels_in;
    channel_buffer = allocate_array(audio_output.buffer_size_frames * channels, sizeof(stereo_frame_t));
    calculate_phase_increments(audio_output.sample_rate);
    allocate_resample_buffer(audio_output.buffer_size_frames);
    allocate_audio_buffer(audio_output.buffer_size_frames);
    frames_filled = 0;
}

static inline
long buffer_offset_for(int channel)
{
    return (frames_filled * channels) + channel;
}

void write_frames_for_channel(voice_t *voices, const int channel, const long frames_to_fill)
{
    voice_t *voice = voices + channel;
    long offset = buffer_offset_for(channel);
    float *resample_buffer = resample(voice, frames_to_fill);
    for (long frame = 0; frame < frames_to_fill; frame++)
    {
        float pcm = resample_buffer[frame];
        stereo_frame_t stereo_frame = apply_gain(pcm, voice);
        channel_buffer[offset] = stereo_frame;
        offset += channels;
    }
}

static inline
bool buffer_filled()
{
    return frames_filled == audio_output.buffer_size_frames;
}

void mix_and_send()
{
    __int16_t *audio_buffer = mix(channel_buffer, channels);
    audio_output.write(audio_buffer, frames_filled);
}

void fill_frames(voice_t *voices, const long frames_to_fill)
{
    for (int channel = 0; channel < channels; channel++)
    {
        write_frames_for_channel(voices, channel, frames_to_fill);
    }
    frames_filled += frames_to_fill;
    if (buffer_filled())
    {
        mix_and_send();
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

void write_audio_data(voice_t *voices)
{
    long frames_left = frames_to_write();
    while (frames_left)
    {
        const long to_fill = can_be_filled(frames_left);
        fill_frames(voices, to_fill);
        frames_left -= to_fill;
    }
    all_frames_written();
}

void send_remaining_audio()
{
    if (frames_filled > 0)
        mix_and_send();
}
