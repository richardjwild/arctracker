#include "api_portaudio.h"
#include <config.h>

#if HAVE_LIBPORTAUDIO

#include <portaudio.h>
#include <stddef.h>
#include <io/error.h>
#include <stdio.h>

static const char *NO_OUTPUT_DEVICE_AVAILABLE = "No output device available";
static const char *FAILED_TO_INITIALIZE_PORTAUDIO = "Failed to initialise portaudio";
static const char *FAILED_TO_OPEN_STREAM = "Failed to open stream";
static const char *FAILED_TO_START_STREAM = "Failed to start stream";
static const char *FAILED_TO_WRITE_AUDIO = "Failed to write audio";

static PaStream *stream;

static PaStreamParameters get_output_parameters()
{
    const PaDeviceIndex output_device = Pa_GetDefaultOutputDevice();
    if (output_device == paNoDevice)
    {
        error(NO_OUTPUT_DEVICE_AVAILABLE);
    }
    else
    {
        const PaDeviceInfo *device_info = Pa_GetDeviceInfo(output_device);
        PaStreamParameters output_parameters = {
                .device = output_device,
                .channelCount = 2,
                .sampleFormat = paInt16,
                .suggestedLatency = device_info->defaultHighOutputLatency,
                .hostApiSpecificStreamInfo = NULL
        };
        return output_parameters;
    }
}

static void open_stream()
{
    PaStreamParameters output_parameters = get_output_parameters();
    PaError err = Pa_OpenStream(
            &stream,
            NULL,
            &output_parameters,
            SAMPLE_RATE,
            AUDIO_BUFFER_SIZE_FRAMES,
            paClipOff,
            NULL,
            NULL);
    if (err != paNoError)
        error_with_detail(FAILED_TO_OPEN_STREAM, Pa_GetErrorText(err));
}

static void start_stream()
{
    PaError err = Pa_StartStream(stream);
    if (err != paNoError)
        error_with_detail(FAILED_TO_START_STREAM, Pa_GetErrorText(err));
}

static void write_audio(int16_t *audio_buffer, long frames_in_buffer)
{
    PaError err = Pa_WriteStream(stream, audio_buffer, (unsigned) frames_in_buffer);
    if (err != paNoError)
        error_with_detail(FAILED_TO_WRITE_AUDIO, Pa_GetErrorText(err));
}

static void terminate_portaudio()
{
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}

audio_api_t initialise_portaudio()
{
    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        error_with_detail(FAILED_TO_INITIALIZE_PORTAUDIO, Pa_GetErrorText(err));
    }
    else
    {
        open_stream();
        start_stream();
        audio_api_t audio_api = {
                .buffer_size_frames = AUDIO_BUFFER_SIZE_FRAMES,
                .sample_rate = SAMPLE_RATE,
                .write = write_audio,
                .finish = terminate_portaudio
        };
        return audio_api;
    }
}

#endif // HAVE_LIBPORTAUDIO