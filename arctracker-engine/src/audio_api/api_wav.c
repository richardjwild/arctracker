#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "api_wav.h"

#include <errno.h>

#include "io/error.h"

static const char *CHUNK_ID = "RIFF";
static const char *FORMAT = "WAVE";
static const char *SUB_CHUNK_1_ID = "fmt ";
static const char *SUB_CHUNK_2_ID = "data";
static const char *WRITEONLY = "w";
static const int AUDIO_FORMAT_PCM = 1;
static const int SUB_CHUNK_1_SIZE = 16;
static const uint16_t BITS_PER_SAMPLE = 16;
static const uint16_t NUM_CHANNELS = 2;

#define FRAME_SIZE (NUM_CHANNELS * sizeof(int16_t))

typedef struct {
    uint32_t ChunkID;
    uint32_t ChunkSize;
    uint32_t Format;
} riff_header_t;

typedef struct {
    uint32_t Subchunk1ID;
    uint32_t Subchunk1Size;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
} fmt_subchunk_t;

typedef struct {
    uint32_t Subchunk2ID;
    uint32_t Subchunk2Size;
} data_subchunk_t;

typedef struct {
    riff_header_t riff_chunk_descriptor;
    fmt_subchunk_t fmt;
    data_subchunk_t data;
    int16_t Data[];
} wav_file_t;

static wav_file_t *wav_file = NULL;
static int frames_written = 0;
static int data_capacity = 0;
static char *file_name;

static audio_api_result_t init_wav(void)
{
    return AUDIO_API_SUCCESS;
}

static void ensure_capacity(int required_frames)
{
    if (required_frames <= data_capacity)
        return;
    int new_capacity = data_capacity == 0
        ? AUDIO_BUFFER_SIZE_FRAMES
        : data_capacity;
    while (new_capacity < required_frames)
        new_capacity *= 2;
    size_t data_size = new_capacity * FRAME_SIZE;
    wav_file_t *new_file = realloc(wav_file, sizeof(wav_file_t) + data_size);
    if (new_file == NULL)
    {
        fprintf(stderr, "Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    wav_file = new_file;
    data_capacity = new_capacity;
}

static int16_t clamp_to_pcm16(const float sample)
{
    if (sample <= -1.0f) return INT16_MIN;
    if (sample >= 1.0f) return INT16_MAX;
    return (int16_t)(sample * INT16_MAX);
}

static audio_api_result_t collect_audio(stereo_frame_t *audio_buffer, int frames_in_buffer)
{
    ensure_capacity(frames_written + frames_in_buffer);
    int16_t *data_ptr = wav_file->Data + (frames_written * NUM_CHANNELS);
    while (frames_in_buffer > 0)
    {
        data_ptr[0] = clamp_to_pcm16(audio_buffer->l);
        data_ptr[1] = clamp_to_pcm16(audio_buffer->r);
        data_ptr += 2;
        audio_buffer++;
        frames_written++;
        frames_in_buffer--;
    }
    return AUDIO_API_SUCCESS;
}

riff_header_t riff_header(size_t data_size)
{
    riff_header_t riff_header;
    memcpy(&riff_header.ChunkID, CHUNK_ID, sizeof(uint32_t));
    riff_header.ChunkSize = sizeof(uint32_t) + sizeof(fmt_subchunk_t) + sizeof(data_subchunk_t) + data_size;
    memcpy(&riff_header.Format, FORMAT, sizeof(uint32_t));
    return riff_header;
}

fmt_subchunk_t fmt_subchunk(void)
{
    fmt_subchunk_t fmt_subchunk;
    memcpy(&fmt_subchunk.Subchunk1ID, SUB_CHUNK_1_ID, sizeof(uint32_t));
    fmt_subchunk.Subchunk1Size = SUB_CHUNK_1_SIZE;
    fmt_subchunk.AudioFormat = AUDIO_FORMAT_PCM;
    fmt_subchunk.NumChannels = NUM_CHANNELS;
    fmt_subchunk.SampleRate = SAMPLE_RATE;
    fmt_subchunk.ByteRate = SAMPLE_RATE * NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    fmt_subchunk.BlockAlign = NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    fmt_subchunk.BitsPerSample = BITS_PER_SAMPLE;
    return fmt_subchunk;
}

data_subchunk_t data_subchunk(size_t data_size)
{
    data_subchunk_t data_subchunk;
    memcpy(&data_subchunk.Subchunk2ID, SUB_CHUNK_2_ID, sizeof(uint32_t));
    data_subchunk.Subchunk2Size = (uint32_t) data_size;
    return data_subchunk;
}

void write_file(size_t data_size)
{
    size_t file_size = sizeof(wav_file_t) + data_size;
    FILE *file_pointer = fopen(file_name, WRITEONLY);
    fwrite(wav_file, file_size, 1, file_pointer);
    if (ferror(file_pointer))
    {
        fclose(file_pointer);
        system_error(errno);
    }
    fclose(file_pointer);
}

static void create_structure_and_write(bool healthy)
{
    if (healthy)
    {
        size_t data_size = (frames_written * FRAME_SIZE);
        wav_file->riff_chunk_descriptor = riff_header(data_size);
        wav_file->fmt = fmt_subchunk();
        wav_file->data = data_subchunk(data_size);
        write_file(data_size);
        free(wav_file);
        wav_file = NULL;
        free(file_name);
        file_name = NULL;
    }
}

audio_api_t initialise_wav(char *file_name_in)
{
    file_name = malloc((strlen(file_name_in) + 1) * sizeof(char));
    strcpy(file_name, file_name_in);
    frames_written = 0;
    data_capacity = 0;
    audio_api_t audio_api = {
            .buffer_size_frames = AUDIO_BUFFER_SIZE_FRAMES,
            .sample_rate = SAMPLE_RATE,
            .bouncing = true,
            .init = init_wav,
            .write = collect_audio,
            .finish = create_structure_and_write
    };
    return audio_api;
}
