#include "api_portaudio.h"
#include <portaudio.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "messages.h"
#include "io/error.h"

static PaStream *stream;

static audio_api_result_t get_output_parameters(PaStreamParameters *output_parameters)
{
    const PaDeviceIndex output_device = Pa_GetDefaultOutputDevice();
    if (output_device == paNoDevice)
    {
        return (audio_api_result_t) {
            .success = false,
            .error_message = NO_OUTPUT_DEVICE_AVAILABLE,
        };
    }
    const PaDeviceInfo *device_info = Pa_GetDeviceInfo(output_device);
    output_parameters->device = output_device;
    output_parameters->channelCount = 2;
    output_parameters->sampleFormat = paInt16;
    output_parameters->suggestedLatency = device_info->defaultLowOutputLatency;
    output_parameters->hostApiSpecificStreamInfo = NULL;
    return AUDIO_API_SUCCESS;
}

static audio_api_result_t open_stream(void)
{
    PaStreamParameters output_parameters;
    audio_api_result_t result = get_output_parameters(&output_parameters);
    if (!result.success)
    {
        return result;
    }
    const PaError err = Pa_OpenStream(
        &stream,
        NULL,
        &output_parameters,
        SAMPLE_RATE,
        AUDIO_BUFFER_SIZE_FRAMES,
        paClipOff,
        NULL,
        NULL);
    if (err == paNoError)
    {
        PaStreamInfo *stream_info = Pa_GetStreamInfo(stream);
        printf("Actual latency: %f\n", stream_info->outputLatency);
        return AUDIO_API_SUCCESS;
    }
    return (audio_api_result_t) {
        .success = false,
        .error_message = Pa_GetErrorText(err)
    };
}

static audio_api_result_t start_stream(void)
{
    const PaError err = Pa_StartStream(stream);
    if (err == paNoError)
    {
        return AUDIO_API_SUCCESS;
    }
    return (audio_api_result_t) {
        .success = false,
        .error_message = Pa_GetErrorText(err)
    };
}

static audio_api_result_t init_portaudio(void)
{
    const PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        return (audio_api_result_t) {
            .success = false,
            .error_message = Pa_GetErrorText(err)
        };
    }
    audio_api_result_t open_result = open_stream();
    if (!open_result.success)
    {
        return open_result;
    }
    return start_stream();
}

static audio_api_result_t write_audio(int16_t *audio_buffer, int frames_in_buffer)
{
    const PaError err = Pa_WriteStream(stream, audio_buffer, (unsigned) frames_in_buffer);
    if (err != paNoError)
    {
        return audio_api_failure(Pa_GetErrorText(err));
    }
    return AUDIO_API_SUCCESS;
}

static void terminate_portaudio(bool healthy)
{
    if (stream != NULL)
    {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
}

audio_api_t create_portaudio_api(void)
{
    return (audio_api_t) {
        .buffer_size_frames = AUDIO_BUFFER_SIZE_FRAMES,
        .sample_rate = SAMPLE_RATE,
        .bouncing = false,
        .init = init_portaudio,
        .write = write_audio,
        .finish = terminate_portaudio
    };
}
