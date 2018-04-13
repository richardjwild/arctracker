#include "alsa.h"
#include "error.h"
#include "config.h"

#ifndef HAVE_LIBASOUND

audio_api_t initialise_alsa(long sample_rate, int audio_buffer_frames)
{
    error("ALSA playback is not available");
}

#else // HAVE_LIBASOUND

#include <alsa/asoundlib.h>

static snd_pcm_t *audio_handle;
static int audio_buffer_size_bytes;
static audio_api_t alsa_audio_api;
static int err;

static
void write_audio(__int16_t *audio_buffer)
{
    snd_pcm_sframes_t err = snd_pcm_writei(
            audio_handle,
            audio_buffer,
            alsa_audio_api.buffer_size_frames);
    if (err < 0)
        error_with_detail("audio write failed", snd_strerror(err));
}

static
void open_device()
{
    if ((err = snd_pcm_open(&audio_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
        error_with_detail("Cannot open audio device", snd_strerror(err));
}

static
snd_pcm_hw_params_t *initialise_hardware_params()
{
    snd_pcm_hw_params_t *hw_params;
    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
        error_with_detail("Cannot allocate hardware parameter structure", snd_strerror(err));
    if ((err = snd_pcm_hw_params_any(audio_handle, hw_params)) < 0)
        error_with_detail("Cannot initialize hardware parameter structure", snd_strerror(err));
    return hw_params;
}

static
void set_access_type(snd_pcm_hw_params_t *hw_params)
{
    if ((err = snd_pcm_hw_params_set_access(audio_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
        error_with_detail("Cannot set access type", snd_strerror(err));
}

static
void set_sample_format(snd_pcm_hw_params_t *hw_params)
{
    if ((err = snd_pcm_hw_params_set_format(audio_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0)
        error_with_detail("Cannot set sample format", snd_strerror(err));
}

static
unsigned int set_sample_rate(long sample_rate_in, snd_pcm_hw_params_t *hw_params)
{
    unsigned int sample_rate = (unsigned int) sample_rate_in;
    if ((err = snd_pcm_hw_params_set_rate_near(audio_handle, hw_params, &sample_rate, 0)) < 0)
        error_with_detail("Cannot set sample rate", snd_strerror(err));
    else
        return sample_rate;
}

static
void set_number_of_channels(snd_pcm_hw_params_t *hw_params)
{
    if ((err = snd_pcm_hw_params_set_channels(audio_handle, hw_params, 2)) < 0)
        error_with_detail("Cannot set channel count", snd_strerror(err));
}

static
void set_parameters(snd_pcm_hw_params_t *hw_params)
{
    if ((err = snd_pcm_hw_params(audio_handle, hw_params)) < 0)
        error_with_detail("Cannot set parameters", snd_strerror(err));
    else
        snd_pcm_hw_params_free(hw_params);
}

static
void prepare_audio_device()
{
    if ((err = snd_pcm_prepare(audio_handle)) < 0)
        error_with_detail("Cannot prepare audio interface for use", snd_strerror(err));
}

static
audio_api_t audio_api(int audio_buffer_frames, int sample_rate)
{
    alsa_audio_api.buffer_size_frames = audio_buffer_frames;
    alsa_audio_api.sample_rate = (long) sample_rate;
    alsa_audio_api.write = &write_audio;
    return alsa_audio_api;
}

audio_api_t initialise_alsa(long sample_rate_in, int audio_buffer_frames)
{
    open_device();
    snd_pcm_hw_params_t *hw_params = initialise_hardware_params();
    set_access_type(hw_params);
    set_sample_format(hw_params);
    unsigned int sample_rate = set_sample_rate(sample_rate_in, hw_params);
    set_number_of_channels(hw_params);
    set_parameters(hw_params);
    prepare_audio_device();
    return audio_api(audio_buffer_frames, sample_rate);
}

#endif