#include "write_audio.h"
#include "mix.h"
#include "resample.h"
#include "memory/heap.h"
#include "pcm/mu_law.h"

#define AUDIO_OUT_SUCCESS (audio_out_result_t) {\
    .success = true\
}

static const float PAN_HARD_LEFT = 1.0f;
static const float PAN_HARD_RIGHT = 255.0f;

static void calculate_gain_curve(float *gain_curve);
static bool fill_audio_buffer(audio_out_t *, voice_t *, int);
static void write_audio_for_channel(audio_out_t *, voice_t *, int, int);
static bool mix_and_send(audio_out_t *);

bool initialise_audio(audio_out_t *audio_out, audio_api_t audio_api, int num_channels, float master_gain)
{
    audio_out->api = audio_api;
    audio_out->num_channels = num_channels;
    audio_out->master_gain = master_gain;
    audio_out->phase_increments = calculate_phase_increments(audio_api.info.sample_rate);
    audio_out->resample_buffer = allocate_resample_buffer(audio_api.info.buffer_size_frames);
    audio_out->mix_buffer = allocate_array(AUDIO, audio_api.info.buffer_size_frames * num_channels, sizeof(stereo_frame_t));
    audio_out->output_buffer = allocate_audio_buffer(audio_api.info.buffer_size_frames);
    audio_out->frames_filled = 0;
    audio_out->peak_l = 0;
    audio_out->peak_r = 0;
    calculate_gain_curve(audio_out->gain_curve);
    audio_out->api.info.healthy = true;
    return audio_out->api.init(&audio_api.info);
}

static void calculate_gain_curve(float *gain_curve)
{
    for (int i = 0; i <= 127; i++)
    {
        gain_curve[(i * 2) + 1] = mu_law_to_linear(255 - i);
        if (i >= 1)
            gain_curve[i * 2] = (gain_curve[(i * 2) - 1] + gain_curve[(i * 2) + 1]) / 2;
    }
    gain_curve[0] = 0.0f;
    gain_curve[1] = gain_curve[2] / 2;
}

bool write_audio_data(audio_out_t *audio_out, voice_t *voices, int samples_to_write)
{
    // The player has called us to output one tick's worth of samples, which is given by samples_to_write.
    // We will write it all to the audio buffer, sending to the device whenever the buffer becomes full.
    audio_out_result_t result = AUDIO_OUT_SUCCESS;
    while (samples_to_write)
    {
        const int buffer_frames_unfilled = audio_out->api.info.buffer_size_frames - audio_out->frames_filled;
        const int samples_to_write_now = samples_to_write > buffer_frames_unfilled ? buffer_frames_unfilled : samples_to_write;
        if (!fill_audio_buffer(audio_out, voices, samples_to_write_now))
        {
            audio_out->api.info.healthy = false;
            return false;
        }
        samples_to_write -= samples_to_write_now;
    }
    return true;
}

static bool fill_audio_buffer(audio_out_t *audio_out, voice_t *voices, const int frames_to_fill)
{
    for (int channel = 0; channel < audio_out->num_channels; channel++)
    {
        write_audio_for_channel(audio_out, voices, channel, frames_to_fill);
    }
    audio_out->frames_filled += frames_to_fill;
    if (audio_out->frames_filled < audio_out->api.info.buffer_size_frames)
    {
        // Audio buffer is not yet filled, but we have to break now because it is time for the player to tick.
        return true;
    }
    const bool result = mix_and_send(audio_out);
    audio_out->frames_filled = 0;
    return result;
}

static void write_audio_for_channel(audio_out_t *audio_out, voice_t *voices, const int channel, const int frames_to_fill)
{
    voice_t *voice = voices + channel;
    resample(voice, audio_out->resample_buffer, audio_out->phase_increments, frames_to_fill);
    const float voice_gain = audio_out->gain_curve[voice->volume];
    const float gain = voice->muted ? 0.0f : voice_gain * audio_out->master_gain;
    const float left_gain = gain * (PAN_HARD_RIGHT - (float) voice->panning) / 254.0f;
    const float right_gain = gain * ((float) voice->panning - PAN_HARD_LEFT) / 254.0f;
    const int channels = audio_out->num_channels;
    stereo_frame_t *mix_buffer = audio_out->mix_buffer;
    const float *resample_buffer = audio_out->resample_buffer;
    int offset = (audio_out->frames_filled * channels) + channel;
    for (int frame = 0; frame < frames_to_fill; frame++)
    {
        const float pcm = resample_buffer[frame];
        mix_buffer[offset] = (stereo_frame_t) {
            .l = pcm * left_gain,
            .r = pcm * right_gain,
        };
        offset += channels;
    }
}

static bool mix_and_send(audio_out_t *audio_out)
{
    mix(audio_out->mix_buffer, audio_out->output_buffer, &audio_out->peak_l, &audio_out->peak_r, audio_out->num_channels, audio_out->api.info.buffer_size_frames);
    return audio_out->api.write(audio_out->output_buffer, audio_out->frames_filled);
}

void send_remaining_audio(audio_out_t *audio_out)
{
    if (audio_out->frames_filled > 0)
    {
        if (!mix_and_send(audio_out))
            audio_out->api.info.healthy = false;
    }
}

void destroy_audio_resources(audio_out_t *audio_out)
{
    audio_out->api.finish(&audio_out->api.info);
    deallocate(AUDIO, audio_out->phase_increments);
    deallocate(AUDIO, audio_out->resample_buffer);
    deallocate(AUDIO, audio_out->mix_buffer);
    deallocate(AUDIO, audio_out->output_buffer);
}
