#include "portaudio_api.h"
#include "portaudio.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "messages.h"
#include "audio_out/audio_out.h"
#include "io/error.h"
#include "memory/heap.h"

static bool get_output_parameters(const portaudio_config_t *config, PaStreamParameters *output_parameters);
static bool get_device_index(const portaudio_config_t *config, int *device_index);

bool open_portaudio_stream(const portaudio_config_t *config, PaStream **pa_stream, PaStreamCallback *callback, void *user_data)
{
    PaStreamParameters output_parameters;
    if (!get_output_parameters(config, &output_parameters))
    {
        return false;
    }
    const PaError err = Pa_OpenStream(
        pa_stream,
        NULL,
        &output_parameters,
        SAMPLE_RATE,
        0,
        paClipOff,
        callback,
        user_data);
    if (err != paNoError)
    {
        error(Pa_GetErrorText(err));
        return false;
    }
    const PaStreamInfo *stream_info = Pa_GetStreamInfo(*pa_stream);
    if (stream_info != NULL)
    {
        printf("Actual latency: %f\n", stream_info->outputLatency);
    }
    return true;
}

bool start_portaudio_stream(PaStream *pa_stream)
{
    const PaError err = Pa_StartStream(pa_stream);
    if (err != paNoError)
    {
        error(Pa_GetErrorText(err));
        return false;
    }
    return true;
}

void close_portaudio_stream(PaStream *pa_stream)
{
    if (pa_stream != NULL)
    {
        printf("Closing PortAudio stream...");
        const PaError err = Pa_CloseStream(pa_stream);
        if (err == paNoError) printf("closed successfully\n");
        else printf("error: %s\n", Pa_GetErrorText(err));
    }
}

bool start_portaudio(void)
{
    const PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        error(Pa_GetErrorText(err));
        return false;
    }
    return true;
}

bool stop_portaudio(void)
{
    const PaError err = Pa_Terminate();
    if (err != paNoError)
    {
        error(Pa_GetErrorText(err));
        return false;
    }
    return true;
}

int get_available_output_count(void)
{
    const PaDeviceIndex total_devices = Pa_GetDeviceCount();
    if (total_devices < 0)
    {
        // In this case total_devices is an error code instead.
        error(Pa_GetErrorText(total_devices));
    }
    int available_devices = 0;
    for (int device_index = 0; device_index < total_devices; device_index++)
    {
        const PaDeviceInfo* device_info = Pa_GetDeviceInfo(device_index);
        if (device_info != NULL && device_info->maxOutputChannels >= 2)
            available_devices++;
    }
    return available_devices;
}

bool get_available_outputs(audio_device_info_t* available_outputs, const int requested_outputs)
{
    const PaDeviceIndex total_devices = Pa_GetDeviceCount();
    if (total_devices < 0)
    {
        // In this case total_devices is an error code instead.
        error(Pa_GetErrorText(total_devices));
        return false;
    }
    if (total_devices == 0 || requested_outputs == 0)
    {
        return true;
    }
    for (int output_index = 0; output_index < requested_outputs; output_index++)
    {
        available_outputs[output_index] = (audio_device_info_t) {0};
    }
    int output_index = 0;
    for (int device_index = 0; device_index < total_devices && output_index < requested_outputs; device_index++)
    {
        const PaDeviceInfo* device_info = Pa_GetDeviceInfo(device_index);
        if (device_info == NULL || device_info->maxOutputChannels < 2) continue;
        const PaHostApiInfo* host_api_info = Pa_GetHostApiInfo(device_info->hostApi);
        if (host_api_info == NULL) continue;
        audio_device_info_t output = (audio_device_info_t) {
            .device_index = device_index,
        };
        snprintf(output.name, sizeof(output.name), "%s", device_info->name);
        snprintf(output.host_api_name, sizeof(output.host_api_name), "%s", host_api_info->name);
        available_outputs[output_index] = output;
        output_index++;
    }
    return true;
}

static bool get_output_parameters(const portaudio_config_t *config, PaStreamParameters *output_parameters)
{
    PaDeviceIndex device_index;
    if (!get_device_index(config, &device_index)) return false;
    const PaDeviceInfo *device_info = Pa_GetDeviceInfo(device_index);
    if (device_info == NULL)
    {
        error(FAILED_TO_READ_DEVICE_INFO);
        return false;
    }
    if (config->device_index != DEFAULT_PORTAUDIO_DEVICE_INDEX)
    {
        // Verify that the device actually matches what the user selected.
        const PaHostApiInfo* host_api_info = Pa_GetHostApiInfo(device_info->hostApi);
        if (host_api_info == NULL)
        {
            error(FAILED_TO_READ_HOST_API_INFO);
            return false;
        }
        if (strncmp(config->name, device_info->name, AUDIO_DEVICE_NAME_SIZE) != 0)
        {
            error(AUDIO_DEVICE_MISMATCH);
            return false;
        }
        if (strncmp(config->host_api_name, host_api_info->name, HOST_API_NAME_SIZE) != 0)
        {
            error(AUDIO_DEVICE_MISMATCH);
            return false;
        }
    }
    output_parameters->device = device_index;
    output_parameters->channelCount = 2;
    output_parameters->sampleFormat = paFloat32;
    output_parameters->suggestedLatency = device_info->defaultLowOutputLatency;
    output_parameters->hostApiSpecificStreamInfo = NULL;
    return true;
}

static bool get_device_index(const portaudio_config_t *config, int *device_index)
{
    if (config->device_index == DEFAULT_PORTAUDIO_DEVICE_INDEX)
    {
        const PaDeviceIndex result = Pa_GetDefaultOutputDevice();
        if (result == paNoDevice)
        {
            error(Pa_GetErrorText(result));
            return false;
        }
        *device_index = result;
        return true;
    }
    *device_index = config->device_index;
    return true;
}
