#include "write_audio.h"
#include "mix.h"
#include "resample.h"
#include "memory/heap.h"

#define AUDIO_OUT_SUCCESS (audio_out_result_t) {\
    .success = true\
}

const float PAN_L[] = {1.0f, 0.828f, 0.672f, 0.5f, 0.328f, 0.172f, 0.0f};
const float PAN_R[] = {0.0f, 0.172f, 0.328f, 0.5f, 0.672f, 0.828f, 1.0f};

static int can_be_filled(audio_out_t *, int);
static audio_api_result_t fill_frames(audio_out_t *, voice_t *, int);
static void write_frames_for_channel(audio_out_t *, voice_t *, int, int);
static stereo_frame_t to_stereo_frame(float, voice_t *, audio_out_t *);
static audio_api_result_t mix_and_send(const audio_out_t *);

audio_out_result_t initialise_audio(audio_out_t *audio_out, audio_api_t audio_api, int num_channels, float master_gain, float *gain_curve)
{
    audio_out->api = audio_api;
    audio_out->num_channels = num_channels;
    audio_out->master_gain = master_gain;
    audio_out->gain_curve = gain_curve;
    audio_out->phase_increments = calculate_phase_increments(audio_api.sample_rate);
    audio_out->resample_buffer = allocate_resample_buffer(audio_api.buffer_size_frames);
    audio_out->channel_buffer = allocate_array(AUDIO, audio_api.buffer_size_frames * num_channels, sizeof(stereo_frame_t));
    audio_out->output_buffer = allocate_audio_buffer(audio_api.buffer_size_frames);
    audio_out->frames_filled = 0;
    audio_out->healthy = true;
    audio_api_result_t result = audio_out->api.init();
    if (result.success)
        return AUDIO_OUT_SUCCESS;
    else
        return (audio_out_result_t) {
            .success = false,
            .error_message = result.error_message
        };
}

audio_out_result_t write_audio_data(audio_out_t *audio_out, voice_t *voices, int samples_to_write)
{
    int frames_left = samples_to_write;
    while (frames_left)
    {
        const int to_fill = can_be_filled(audio_out, frames_left);
        const audio_api_result_t result = fill_frames(audio_out, voices, to_fill);
        if (!result.success)
        {
            audio_out->healthy = false;
            return (audio_out_result_t) {
                .success = false,
                .error_message = result.error_message
            };
        }
        frames_left -= to_fill;
    }
    return AUDIO_OUT_SUCCESS;
}

static int can_be_filled(audio_out_t *audio_out, const int frames_requested)
{
    const int buffer_frames_unfilled = audio_out->api.buffer_size_frames - audio_out->frames_filled;
    return frames_requested > buffer_frames_unfilled
        ? buffer_frames_unfilled
        : frames_requested;
}

static audio_api_result_t fill_frames(audio_out_t *audio_out, voice_t *voices, const int frames_to_fill)
{
    for (int channel = 0; channel < audio_out->num_channels; channel++)
        write_frames_for_channel(audio_out, voices, channel, frames_to_fill);
    audio_out->frames_filled += frames_to_fill;
    if (audio_out->frames_filled == audio_out->api.buffer_size_frames)
    {
        audio_api_result_t result = mix_and_send(audio_out);
        audio_out->frames_filled = 0;
        return result;
    }
    return AUDIO_API_SUCCESS;
}

static void write_frames_for_channel(audio_out_t *audio_out, voice_t *voices, const int channel, const int frames_to_fill)
{
    const int channels = audio_out->num_channels;
    voice_t *voice = voices + channel;
    int offset = (audio_out->frames_filled * channels) + channel;
    const float *resample_buffer = resample(voice, audio_out->resample_buffer, audio_out->api.buffer_size_frames * sizeof(float), audio_out->phase_increments, frames_to_fill);
    stereo_frame_t *channel_buffer = audio_out->channel_buffer;
    for (int frame = 0; frame < frames_to_fill; frame++)
    {
        const float pcm = *(resample_buffer + frame);
        *(channel_buffer + offset) = to_stereo_frame(pcm, voice, audio_out);
        offset += channels;
    }
}

static stereo_frame_t to_stereo_frame(float pcm, voice_t *voice, audio_out_t *audio_out)
{
    stereo_frame_t stereo_frame;
    const float voice_gain = *(audio_out->gain_curve + voice->gain);
    const float adjusted_pcm = audio_out->master_gain * voice_gain * pcm;
    if (voice->panning >= 0 && voice->panning <= 6)
    {
        stereo_frame.l = adjusted_pcm * PAN_L[voice->panning];
        stereo_frame.r = adjusted_pcm * PAN_R[voice->panning];
        return stereo_frame;
    }
    stereo_frame.l = stereo_frame.r = adjusted_pcm / 2.0f;
    return stereo_frame;
}

static audio_api_result_t mix_and_send(const audio_out_t *audio_out)
{
    mix(audio_out->channel_buffer, audio_out->output_buffer, audio_out->num_channels, audio_out->api.buffer_size_frames);
    return audio_out->api.write(audio_out->output_buffer, audio_out->frames_filled);
}

void send_remaining_audio(audio_out_t *audio_out)
{
    if (audio_out->frames_filled > 0)
    {
        if (!mix_and_send(audio_out).success)
            audio_out->healthy = false;
    }
}

void destroy_audio_resources(audio_out_t *audio_out)
{
    audio_out->api.finish(audio_out->healthy);
    deallocate(AUDIO, audio_out->phase_increments);
    deallocate(AUDIO, audio_out->resample_buffer);
    deallocate(AUDIO, audio_out->channel_buffer);
    deallocate(AUDIO, audio_out->output_buffer);
}
