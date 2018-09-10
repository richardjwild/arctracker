#include "api_oss.h"
#include <config.h>

#if HAVE_SYS_SOUNDCARD_H

#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <io/error.h>

static const char *DEVICE_FILE_NAME = "/dev/dsp";

static int audio_handle;

static
void write_audio(int16_t *audio_buffer, long frames_in_buffer)
{
    const size_t bytes_to_write = frames_in_buffer * 2 * sizeof(int16_t);
    if (write(audio_handle, audio_buffer, bytes_to_write) == -1)
        system_error("audio write failed");
}

static
void close_oss()
{
    close(audio_handle);
}

static
void open_device()
{
    if ((audio_handle = open(DEVICE_FILE_NAME, O_WRONLY, 0)) == -1)
        system_error(DEVICE_FILE_NAME);
}

static
void set_sample_format()
{
    int sample_format = AFMT_S16_LE;
    if (ioctl(audio_handle, SNDCTL_DSP_SETFMT, &sample_format) == -1)
        system_error("SNDCTL_DSP_SETFMT");
    else if (sample_format != AFMT_S16_LE)
        error("Could not set audio device to suitable sample format (16-bit signed little-endian)");
}

static
void set_number_of_channels()
{
    int channels = 2;
    if (ioctl(audio_handle, SNDCTL_DSP_CHANNELS, &channels) == -1)
        system_error("SNDCTL_DSP_CHANNELS");
    else if (channels != 2)
        error("Could not set stereo output");
}

static
void set_sample_rate(long sample_rate)
{
    if (ioctl(audio_handle, SNDCTL_DSP_SPEED, &sample_rate) == -1)
        system_error("SNDCTL_DSP_SPEED");
}

static
audio_api_t audio_api(int audio_buffer_frames, long sample_rate)
{
    audio_api_t oss_audio_api = {
            .write = write_audio,
            .finish = close_oss,
            .buffer_size_frames = audio_buffer_frames,
            .sample_rate = sample_rate
    };
    return oss_audio_api;
}

audio_api_t initialise_oss()
{
    open_device();
    set_sample_format();
    set_number_of_channels();
    set_sample_rate(SAMPLE_RATE);
    return audio_api(AUDIO_BUFFER_SIZE_FRAMES, SAMPLE_RATE);
}

#endif //HAVE_SYS_SOUNDCARD_H