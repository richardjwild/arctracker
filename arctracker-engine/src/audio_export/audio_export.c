#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "audio_export.h"
#include "audio_file/wav.h"
#include "audio_out/audio_out.h"
#include "io/error.h"
#include "memory/heap.h"

#define EXPORT_BUFFER_SIZE_FRAMES 1024
#define NUM_CHANNELS 2

static bool ensure_capacity(wav_export_t *wav_export, unsigned long required_frames);
static int16_t clamp_to_pcm16(float);

bool export_init(const char *output_filename, wav_export_t *wav_export)
{
    if (strlen(output_filename) > MAX_FILE_PATH_LENGTH)
    {
        error("File path is too long");
        return false;
    }
    wav_export->audio_buffer = allocate_array(AUDIO, EXPORT_BUFFER_SIZE_FRAMES * NUM_CHANNELS, sizeof(float));
    wav_export->audio_buffer_size = EXPORT_BUFFER_SIZE_FRAMES;
    if (wav_export->audio_buffer == NULL)
    {
        return false;
    }
    wav_export->frames_written = 0;
    wav_export->data_capacity = 0;
    wav_export->audio_data = NULL;
    snprintf(wav_export->output_filename, sizeof(wav_export->output_filename), "%s", output_filename);
    return true;
}

bool export_collect_audio(wav_export_t *wav_export, unsigned long frames_in_buffer)
{
    if (!ensure_capacity(wav_export, frames_in_buffer))
    {
        error(get_error_message());
        return false;
    }
    int16_t *data_ptr = wav_export->audio_data + wav_export->frames_written * NUM_CHANNELS;
    const float *audio_buffer = wav_export->audio_buffer;
    while (frames_in_buffer > 0)
    {
        data_ptr[0] = clamp_to_pcm16(audio_buffer[0]);
        data_ptr[1] = clamp_to_pcm16(audio_buffer[1]);
        data_ptr += NUM_CHANNELS;
        audio_buffer += NUM_CHANNELS;
        wav_export->frames_written++;
        frames_in_buffer--;
    }
    return true;
}

static bool ensure_capacity(wav_export_t *wav_export, const unsigned long required_frames)
{
    const unsigned long required_capacity = wav_export->frames_written + required_frames;
    if (required_capacity <= wav_export->data_capacity) return true;
    unsigned long new_capacity = wav_export->data_capacity == 0 ? EXPORT_BUFFER_SIZE_FRAMES : wav_export->data_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2;
    }
    const unsigned long required_size = new_capacity * NUM_CHANNELS;
    int16_t *new_audio_data = reallocate_array(AUDIO, wav_export->audio_data, (int) required_size, sizeof(int16_t));
    if (new_audio_data == NULL)
    {
        wav_export->audio_data = NULL;
        return false;
    }
    wav_export->data_capacity = new_capacity;
    wav_export->audio_data = new_audio_data;
    return true;
}

static int16_t clamp_to_pcm16(const float sample)
{
    if (sample <= -1.0f) return INT16_MIN;
    if (sample >= 1.0f) return INT16_MAX;
    return (int16_t)(sample * INT16_MAX);
}

bool export_finish(wav_export_t *wav_export)
{
    const wav_pcm16_t wav = {
        .sample_rate = SAMPLE_RATE,
        .channels = NUM_CHANNELS,
        .bits_per_sample = 16,
        .frames = (int) wav_export->frames_written,
        .pcm16 = wav_export->audio_data,
    };
    const bool success = wav_write_pcm16(wav_export->output_filename, &wav);
    deallocate(AUDIO, wav_export->audio_data);
    deallocate(AUDIO, wav_export->audio_buffer);
    wav_export->audio_data = NULL;
    wav_export->audio_buffer = NULL;
    return success;
}