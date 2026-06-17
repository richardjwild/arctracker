#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "wav.h"
#include "messages.h"
#include "io/error.h"
#include "memory/heap.h"

#define RIFF_CHUNK_SIZE 36
#define OFFSET(arr,offset) (uint8_t *) (arr + offset)

static const int RIFF_HEADER_SIZE = 8;
static const char *RIFF_CHUNK_ID = "RIFF";
static const char *FORMAT_WAVE = "WAVE";
static const char *SUB_CHUNK_1_ID = "fmt ";
static const char *SUB_CHUNK_2_ID = "data";
static const char *WRITEONLY_BINARY = "wb";
static const int AUDIO_FORMAT_PCM = 1;
static const int AUDIO_FORMAT_FLOAT = 3;
static const int SUB_CHUNK_1_SIZE = 16;
static const uint16_t BITS_PER_SAMPLE_PCM16 = 16;

static bool write_riff_chunk_descriptor(FILE *, size_t);
static bool write_format_subchunk(FILE *, int, int, uint32_t);
static bool write_data_subchunk(FILE *, size_t);
static bool write_audio_data(FILE *, const int16_t *, int, int);
static bool write_fourcc(FILE *fp, const char *ch);
static bool write_u16_le(FILE *fp, uint16_t);
static bool write_16_le(FILE *fp, int16_t);
static bool write_u32_le(FILE *fp, uint32_t);
static uint32_t read_u32_le(const uint8_t *data);
static int16_t read_u16_le(const uint8_t *data);
static float *copy_sample_data_8(const uint8_t *, int, int);
static float *copy_sample_data_16(const int16_t *, int, int);
static float *copy_sample_data_24(const uint8_t *, int, int);
static float *copy_sample_data_32(const int32_t *, int, int);
static float *copy_sample_data_float(const float *, int, int);

bool wav_write_pcm16(const char *filename, const wav_pcm16_t *wav)
{
    const size_t data_size = wav->frames * wav->channels * (BITS_PER_SAMPLE_PCM16 / 8);
    FILE *fp = fopen(filename, WRITEONLY_BINARY);
    bool ok = write_riff_chunk_descriptor(fp, data_size)
        && write_format_subchunk(fp, wav->channels, BITS_PER_SAMPLE_PCM16, wav->sample_rate)
        && write_data_subchunk(fp, data_size)
        && write_audio_data(fp, wav->pcm16, wav->frames, wav->channels);
    if (ferror(fp))
    {
        ok = false;
        system_error(errno);
    }
    fclose(fp);
    return ok;
}

audio_t wav_read_audio(const uint8_t *wav_data, const size_t loaded_data_size)
{
    audio_t output = {0};
    if (memcmp(wav_data, RIFF_CHUNK_ID, 4) != 0)
    {
        error(RIFF_HEADER_NOT_FOUND);
        return output;
    }
    const uint32_t reported_size = read_u32_le(OFFSET(wav_data, 4));
    if (loaded_data_size - reported_size != RIFF_HEADER_SIZE)
    {
        fprintf(stderr, "Reported size: %d\n", reported_size);
        error(SIZE_MISMATCH);
        return output;
    }
    if (memcmp(wav_data + 8, FORMAT_WAVE, 4) != 0)
    {
        error(NOT_WAVE_FORMAT);
        return output;
    }
    const uint8_t *format_subchunk = OFFSET(wav_data, 12);
    if (memcmp(format_subchunk, SUB_CHUNK_1_ID, 4) != 0)
    {
        error(FORMAT_SUBCHUNK_NOT_FOUND);
        return output;
    }
    const uint32_t format_subchunk_size = read_u32_le(OFFSET(format_subchunk, 4));
    const uint16_t audio_format = read_u16_le(OFFSET(format_subchunk, 8));
    if (audio_format != AUDIO_FORMAT_PCM && audio_format != AUDIO_FORMAT_FLOAT)
    {
        error(UNKNOWN_AUDIO_FORMAT);
        return output;
    }
    const uint16_t channels = read_u16_le(OFFSET(format_subchunk, 10));
    const uint32_t sample_rate = read_u32_le(OFFSET(format_subchunk, 12));
    output.sample_rate = sample_rate;
    const uint16_t bits_per_sample = read_u16_le(OFFSET(format_subchunk, 22));
    if (bits_per_sample % 8 != 0 || bits_per_sample > 32)
    {
        fprintf(stderr, "Invalid bits per sample: %d\n", bits_per_sample);
        error(INVALID_BITS_PER_SAMPLE);
        return output;
    }
    const uint8_t *data_subchunk = OFFSET(format_subchunk, (format_subchunk_size + 8));
    if (memcmp(data_subchunk, SUB_CHUNK_2_ID, 4) != 0)
    {
        error(DATA_SUBCHUNK_NOT_FOUND);
        return output;
    }
    const uint32_t audio_data_size = read_u32_le(OFFSET(data_subchunk, 4));
    const uint32_t frame_size = bits_per_sample / 8 * channels;
    if (audio_data_size % frame_size != 0)
    {
        fprintf(stderr, "Audio data size: %d\n", audio_data_size);
        error(INCONSISTENT_AUDIO_DATA_SIZE);
        return output;
    }
    output.frames = audio_data_size / frame_size;
    const uint8_t *audio_data = OFFSET(data_subchunk, 8);
    if (bits_per_sample == 8)
        output.audio_data = copy_sample_data_8(audio_data, channels, output.frames);
    if (bits_per_sample == 16)
        output.audio_data = copy_sample_data_16((int16_t *) audio_data, channels, output.frames);
    if (bits_per_sample == 24)
        output.audio_data = copy_sample_data_24(audio_data, channels, output.frames);
    if (bits_per_sample == 32 && audio_format == AUDIO_FORMAT_PCM)
        output.audio_data = copy_sample_data_32((int32_t *) audio_data, channels, output.frames);
    if (bits_per_sample == 32 && audio_format == AUDIO_FORMAT_FLOAT)
        output.audio_data = copy_sample_data_float((float *) audio_data, channels, output.frames);
    if (output.audio_data == NULL)
    {
        error(MEMORY_ALLOCATION_FAILED);
        return output;
    }
    return output;
}

static bool write_riff_chunk_descriptor(FILE *fp, const size_t data_size)
{
    return write_fourcc(fp, RIFF_CHUNK_ID)
        && write_u32_le(fp, RIFF_CHUNK_SIZE + data_size)
        && write_fourcc(fp, FORMAT_WAVE);
}

static bool write_format_subchunk(FILE *fp, const int channels, const int bits_per_sample, const uint32_t sample_rate)
{
    return write_fourcc(fp, SUB_CHUNK_1_ID)
        && write_u32_le(fp, SUB_CHUNK_1_SIZE)
        && write_u16_le(fp, AUDIO_FORMAT_PCM)
        && write_u16_le(fp, channels)
        && write_u32_le(fp, sample_rate)
        && write_u32_le(fp, sample_rate * channels * bits_per_sample / 8)
        && write_u16_le(fp, channels * bits_per_sample / 8)
        && write_u16_le(fp, bits_per_sample);
}

static bool write_data_subchunk(FILE *fp, const size_t data_size)
{
    return write_fourcc(fp, SUB_CHUNK_2_ID)
        && write_u32_le(fp, (uint32_t) data_size);
}

static bool write_audio_data(FILE *fp, const int16_t *pcm16, const int frames, const int channels)
{
    const int samples = frames * channels;
    for (int i = 0; i < samples; i++)
    {
        if (!write_16_le(fp, pcm16[i]))
            return false;
    }
    return true;
}

static bool write_fourcc(FILE *fp, const char *ch)
{
    return fwrite(ch, sizeof(char), 4, fp) == 4;
}

static bool write_u16_le(FILE *fp, const uint16_t value)
{
    return fwrite(&value, sizeof value, 1, fp) == 1;
}

static bool write_16_le(FILE *fp, const int16_t value)
{
    return fwrite(&value, sizeof value, 1, fp) == 1;
}

static bool write_u32_le(FILE *fp, const uint32_t value)
{
    return fwrite(&value, sizeof value, 1, fp) == 1;
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
}

static int16_t read_u16_le(const uint8_t *data)
{
    return data[0] + (data[1] << 8);
}

static float *copy_sample_data_8(const uint8_t *sample_data, const int channels, const int frames)
{
    float *output = allocate_array(MODULE, frames + 2, sizeof(float));
    if (output == NULL)
        return output;
    for (int f = 0; f < frames; f++)
    {
        float frame = 0.0f;
        for (int c = 0; c < channels; c++)
        {
            frame += ((float) *sample_data - 128.0f) / 128.0f;
            sample_data++;
        }
        output[f] = frame / channels;
    }
    return output;
}

static float *copy_sample_data_16(const int16_t *sample_data, const int channels, const int frames)
{
    float *output = allocate_array(MODULE, frames + 2, sizeof(float));
    if (output == NULL) return output;
    for (int f = 0; f < frames; f++)
    {
        float frame = 0.0f;
        for (int c = 0; c < channels; c++)
        {
            const int16_t value = *sample_data++;
            frame += value < 0 ? (float) value / 32768.0f : (float) value / 32767.0f;
            sample_data++;
        }
        output[f] = frame / channels;
    }
    return output;
}

static float *copy_sample_data_24(const uint8_t *sample_data, const int channels, const int frames)
{
    float *output = allocate_array(MODULE, frames + 2, sizeof(float));
    if (output == NULL) return output;
    for (int f = 0; f < frames; f++)
    {
        float frame = 0.0f;
        for (int c = 0; c < channels; c++)
        {
            int32_t value = (int32_t) sample_data[0] | (int32_t) sample_data[1] << 8 | (int32_t) sample_data[2] << 16;
            if (value & 0x800000) value |= ~0xFFFFFF;
            frame += value < 0 ? (float) value / 8388608.0f : (float) value / 8388607.0f;
            sample_data += 3;
        }
        output[f] = frame / channels;
    }
    return output;
}

static float *copy_sample_data_32(const int32_t *sample_data, const int channels, const int frames)
{
    float *output = allocate_array(MODULE, frames + 2, sizeof(float));
    if (output == NULL) return output;
    for (int f = 0; f < frames; f++)
    {
        float frame = 0.0f;
        for (int c = 0; c < channels; c++)
        {
            const int32_t value = *sample_data;
            frame += value < 0 ? (float) value / 2147483648.0f : (float) value / 2147483647.0f;
            sample_data++;
        }
        output[f] = frame / channels;
    }
    return output;
}

static float *copy_sample_data_float(const float *sample_data, const int channels, const int frames)
{
    float *output = allocate_array(MODULE, frames + 2, sizeof(float));
    if (output == NULL) return output;
    for (int f = 0; f < frames; f++)
    {
        float frame = 0.0f;
        for (int c = 0; c < channels; c++)
        {
            frame += *sample_data;
            sample_data++;
        }
        output[f] = frame / channels;
    }
    return output;
}

