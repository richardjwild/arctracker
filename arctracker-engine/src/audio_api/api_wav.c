#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "api_wav.h"
#include "audio_file/wav.h"
#include "io/error.h"

#define FRAME_SIZE (NUM_CHANNELS * sizeof(int16_t))

static const uint16_t NUM_CHANNELS = 2;
static int16_t *audio_data = NULL;
static int frames_written = 0;
static int data_capacity = 0;

static bool init_wav(const audio_api_info_t *);
static bool collect_audio(const stereo_frame_t *, int);
static bool ensure_capacity(int);
static int16_t clamp_to_pcm16(float);
static void create_structure_and_write(const audio_api_info_t *info);

audio_api_t initialise_wav(const char *output_filename, const interpolation_type_t interpolation_type)
{
    audio_api_t audio_api = {0};
    if (strlen(output_filename) > MAX_FILE_PATH_LENGTH)
    {
        error("File path is too long");
        return audio_api;
    }
    frames_written = 0;
    data_capacity = 0;
    audio_api.info.buffer_size_frames = AUDIO_BUFFER_SIZE_FRAMES;
    audio_api.info.sample_rate = SAMPLE_RATE;
    audio_api.info.bouncing = true;
    audio_api.info.healthy = true;
    audio_api.info.interpolation_type = interpolation_type;
    snprintf(audio_api.info.config.wav.output_filename, MAX_FILE_PATH_LENGTH, "%s", output_filename);
    audio_api.init = init_wav;
    audio_api.write = collect_audio;
    audio_api.finish = create_structure_and_write;
    return audio_api;
}

static bool init_wav(const audio_api_info_t *info)
{
    return true;
}

static bool collect_audio(const stereo_frame_t *audio_buffer, int frames_in_buffer)
{
    if (!ensure_capacity(frames_written + frames_in_buffer))
    {
        error(get_error_message());
        return false;
    }
    int16_t *data_ptr = audio_data + frames_written * NUM_CHANNELS;
    while (frames_in_buffer > 0)
    {
        data_ptr[0] = clamp_to_pcm16(audio_buffer->l);
        data_ptr[1] = clamp_to_pcm16(audio_buffer->r);
        data_ptr += 2;
        audio_buffer++;
        frames_written++;
        frames_in_buffer--;
    }
    return true;
}

static bool ensure_capacity(const int required_frames)
{
    if (required_frames <= data_capacity) return true;
    int new_capacity = data_capacity == 0 ? AUDIO_BUFFER_SIZE_FRAMES : data_capacity;
    while (new_capacity < required_frames) new_capacity *= 2;
    const size_t required_size = new_capacity * FRAME_SIZE;
    int16_t *new_audio_data = realloc(audio_data, required_size);
    if (new_audio_data == NULL)
    {
        error("Failed to allocate memory");
        return false;
    }
    data_capacity = new_capacity;
    audio_data = new_audio_data;
    return true;
}

static int16_t clamp_to_pcm16(const float sample)
{
    if (sample <= -1.0f) return INT16_MIN;
    if (sample >= 1.0f) return INT16_MAX;
    return (int16_t)(sample * INT16_MAX);
}

static void create_structure_and_write(const audio_api_info_t *info)
{
    if (info->healthy)
    {
        const wav_pcm16_t wav = {
            .sample_rate = SAMPLE_RATE,
            .channels = NUM_CHANNELS,
            .bits_per_sample = 16,
            .frames = frames_written,
            .pcm16 = audio_data,
        };
        wav_write_pcm16(info->config.wav.output_filename, &wav);
    }
    free(audio_data);
    audio_data = NULL;
}