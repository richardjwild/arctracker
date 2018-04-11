#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include "oss.h"
#include "error.h"

void write_audio(__int16_t *audio_buffer)
{
    if (write(audio_handle, audio_buffer, audio_buffer_size_bytes) == -1)
        system_error("audio write");
}

void open_device()
{
    if ((audio_handle = open("/dev/dsp", O_WRONLY, 0)) == -1)
        system_error("/dev/dsp");
}

void set_sample_format()
{
    int sample_format = AFMT_S16_LE;
    if (ioctl(audio_handle, SNDCTL_DSP_SETFMT, &sample_format) == -1)
        system_error("SNDCTL_DSP_SETFMT");
    else if (sample_format != AFMT_S16_LE)
        error("Could not set audio device to suitable sample format (16-bit signed little-endian)");
}

void set_number_of_channels()
{
    int channels = 2;
    if (ioctl(audio_handle, SNDCTL_DSP_CHANNELS, &channels) == -1)
        system_error("SNDCTL_DSP_CHANNELS");
    else if (channels != 2)
        error("Could not set stereo output");
}

void set_sample_rate(long sample_rate)
{
    if (ioctl(audio_handle, SNDCTL_DSP_SPEED, &sample_rate) == -1)
        system_error("SNDCTL_DSP_SPEED");
}

void set_audio_buffer_size(int audio_buffer_frames)
{
    audio_buffer_size_bytes = audio_buffer_frames * 2 * sizeof(__int16_t);
}

audio_api_t audio_api(int audio_buffer_frames)
{
    oss_audio_api.write_audio = &write_audio;
    oss_audio_api.audio_buffer_frames = audio_buffer_frames;
    return oss_audio_api;
}

audio_api_t initialise_oss(long sample_rate, int audio_buffer_frames)
{
    open_device();
    set_sample_format();
    set_number_of_channels();
    set_sample_rate(sample_rate);
    set_audio_buffer_size(audio_buffer_frames);
    return audio_api(audio_buffer_frames);
}
