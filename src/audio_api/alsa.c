#include "alsa.h"
#include <config.h>
#include <io/error.h>

#ifndef HAVE_LIBASOUND

audio_api_t initialise_alsa(long sample_rate, int audio_buffer_frames)
{
    error("ALSA playback is not available");
}

#else // HAVE_LIBASOUND

#include <alsa/asoundlib.h>

static const char *PCM_DEVICE = "default";
static const int SILENT = 1;

static snd_pcm_t *pcm_handle;
static int err;

static
void write_audio(__int16_t *audio_buffer, long frames_in_buffer)
{
    snd_pcm_sframes_t frames_written = snd_pcm_writei(
            pcm_handle,
            audio_buffer,
            (snd_pcm_uframes_t) frames_in_buffer);
    if (frames_written < 0)
        snd_pcm_recover(pcm_handle, (int) frames_written, SILENT);
}

static
void close_alsa()
{
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
}

static
void open_device()
{
    if ((err = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
        error_with_detail("Cannot open audio device", snd_strerror(err));
}

static
snd_pcm_hw_params_t *initialise_hardware_params()
{
    snd_pcm_hw_params_t *hw_params;
    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
        error_with_detail("Cannot allocate hardware parameter structure", snd_strerror(err));
    if ((err = snd_pcm_hw_params_any(pcm_handle, hw_params)) < 0)
        error_with_detail("Cannot initialize hardware parameter structure", snd_strerror(err));
    return hw_params;
}

static
void set_access_type(snd_pcm_hw_params_t *hw_params)
{
    if ((err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
        error_with_detail("Cannot set access type", snd_strerror(err));
}

static
void set_sample_format(snd_pcm_hw_params_t *hw_params)
{
    if ((err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0)
        error_with_detail("Cannot set sample format", snd_strerror(err));
}

static
unsigned int set_sample_rate(long sample_rate_in, snd_pcm_hw_params_t *hw_params)
{
    unsigned int sample_rate = (unsigned int) sample_rate_in;
    if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &sample_rate, 0)) < 0)
        error_with_detail("Cannot set sample rate", snd_strerror(err));
    else
        return sample_rate;
}

static
void set_number_of_channels(snd_pcm_hw_params_t *hw_params)
{
    if ((err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, 2)) < 0)
        error_with_detail("Cannot set channel count", snd_strerror(err));
}

static
void set_parameters(snd_pcm_hw_params_t *hw_params)
{
    if ((err = snd_pcm_hw_params(pcm_handle, hw_params)) < 0)
        error_with_detail("Cannot set parameters", snd_strerror(err));
    else
        snd_pcm_hw_params_free(hw_params);
}

static
void prepare_audio_device()
{
    if ((err = snd_pcm_prepare(pcm_handle)) < 0)
        error_with_detail("Cannot prepare audio interface for use", snd_strerror(err));
}

static
audio_api_t audio_api(int audio_buffer_frames, int sample_rate)
{
    audio_api_t alsa_audio_api = {
            .write = write_audio,
            .finish = close_alsa,
            .buffer_size_frames = audio_buffer_frames,
            .sample_rate = (long) sample_rate
    };
    return alsa_audio_api;
}

audio_api_t initialise_alsa()
{
    open_device();
    snd_pcm_hw_params_t *hw_params = initialise_hardware_params();
    set_access_type(hw_params);
    set_sample_format(hw_params);
    unsigned int sample_rate = set_sample_rate(SAMPLE_RATE, hw_params);
    set_number_of_channels(hw_params);
    set_parameters(hw_params);
    prepare_audio_device();
    return audio_api(AUDIO_BUFFER_SIZE_FRAMES, sample_rate);
}

#endif