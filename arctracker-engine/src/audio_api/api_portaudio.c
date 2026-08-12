#include "api_portaudio.h"
#include <portaudio.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messages.h"
#include "io/error.h"
#include "memory/heap.h"

#define DEFAULT_DEVICE_INDEX -1

static PaStream *stream;
static int16_t *output_buffer = NULL;

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

static bool get_device_index(const portaudio_config_t *config, int *device_index)
{
    if (config->device_index == DEFAULT_DEVICE_INDEX)
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
    if (config->device_index != DEFAULT_DEVICE_INDEX)
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
    output_parameters->sampleFormat = paInt16;
    output_parameters->suggestedLatency = device_info->defaultLowOutputLatency;
    output_parameters->hostApiSpecificStreamInfo = NULL;
    return true;
}

static bool open_stream(const portaudio_config_t *config)
{
    PaStreamParameters output_parameters;
    if (!get_output_parameters(config, &output_parameters))
    {
        return false;
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
    if (err != paNoError)
    {
        error(Pa_GetErrorText(err));
        return false;
    }
    const PaStreamInfo *stream_info = Pa_GetStreamInfo(stream);
    printf("Actual latency: %f\n", stream_info->outputLatency);
    return true;
}

static void destroy_output_buffer(void)
{
    deallocate(AUDIO, output_buffer);
    output_buffer = NULL;
}

static bool start_stream(void)
{
    const PaError err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        destroy_output_buffer();
        error(Pa_GetErrorText(err));
        return false;
    }
    return true;
}

static bool open_audio_stream(const audio_api_info_t *info)
{
    output_buffer = allocate_array(AUDIO, AUDIO_BUFFER_SIZE_FRAMES * 2, sizeof(int16_t));
    if (output_buffer == NULL)
    {
        error(MEMORY_ALLOCATION_FAILED);
        return false;
    }
    if (!open_stream(&info->config.portaudio))
    {
        destroy_output_buffer();
        return false;
    }
    return start_stream();
}

static int16_t clamp_to_pcm16(const float sample)
{
    if (sample <= -1.0f) return INT16_MIN;
    if (sample >= 1.0f) return INT16_MAX;
    return (int16_t)(sample * INT16_MAX);
}

static void copy_frames(const stereo_frame_t *input_buffer, int frames_to_copy)
{
    int index = 0;
    while (frames_to_copy > 0)
    {
        output_buffer[index++] = clamp_to_pcm16(input_buffer->l);
        output_buffer[index++] = clamp_to_pcm16(input_buffer->r);
        input_buffer++;
        frames_to_copy--;
    }
}

static bool write_audio(const stereo_frame_t *audio_buffer, int frames_in_buffer)
{
    copy_frames(audio_buffer, frames_in_buffer);
    const PaError err = Pa_WriteStream(stream, output_buffer, (unsigned) frames_in_buffer);
    if (err != paNoError)
    {
        error(Pa_GetErrorText(err));
        return false;
    }
    return true;
}

static void close_audio_stream(const audio_api_info_t *info)
{
    if (stream != NULL)
    {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }
    destroy_output_buffer();
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

audio_api_t get_output(const int device_index, const char *name, const char *host_api_name)
{
    audio_api_t audio_api_out = {
        .info = (audio_api_info_t) {
            .buffer_size_frames = AUDIO_BUFFER_SIZE_FRAMES,
            .sample_rate = SAMPLE_RATE,
            .bouncing = false,
            .healthy = true,
            .config = (audio_backend_config_t) {
                .portaudio = (portaudio_config_t) {
                    .device_index = device_index,
                },
            },
        },
        .init = open_audio_stream,
        .write = write_audio,
        .finish = close_audio_stream,
    };
    snprintf(audio_api_out.info.config.portaudio.name, AUDIO_DEVICE_NAME_SIZE, "%s", name);
    snprintf(audio_api_out.info.config.portaudio.host_api_name, HOST_API_NAME_SIZE, "%s", host_api_name);
    return audio_api_out;
}

audio_api_t get_default_output(void)
{
    return (audio_api_t) {
        .info = (audio_api_info_t) {
            .buffer_size_frames = AUDIO_BUFFER_SIZE_FRAMES,
            .sample_rate = SAMPLE_RATE,
            .bouncing = false,
            .healthy = true,
            .config = (audio_backend_config_t) {
                .portaudio = (portaudio_config_t) {
                    .device_index = DEFAULT_DEVICE_INDEX,
                },
            },
        },
        .init = open_audio_stream,
        .write = write_audio,
        .finish = close_audio_stream,
    };
}
