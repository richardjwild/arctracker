#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "wav.h"
#include "messages.h"
#include "io/error.h"
#include "memory/heap.h"

#define RIFF_CHUNK_SIZE 36
#define CHUNK_HEADER_SIZE 8
#define OFFSET(arr,offset) (uint8_t *) (arr + offset)
#define AUDIO_FORMAT_PCM 0x0001
#define AUDIO_FORMAT_FLOAT 0x0003
#define AUDIO_FORMAT_EXTENSIBLE  0xFFFE
#define BASE_FMT_SIZE 16
#define WAVEFORMATEX_SIZE 18
#define EXTENSIBLE_EXTRA_SIZE 22
#define EXTENSIBLE_FMT_SIZE 40

static const char *RIFF_CHUNK_ID = "RIFF";
static const char *FORMAT_WAVE = "WAVE";
static const char *FMT_SUBCHUNK = "fmt ";
static const char *DATA_SUBCHUNK = "data";
static const char *WRITEONLY_BINARY = "wb";
static const int SUB_CHUNK_1_SIZE = 16;
static const uint16_t BITS_PER_SAMPLE_PCM16 = 16;

static const uint8_t SUBFORMAT_PCM[16] = {
    0x01, 0x00, 0x00, 0x00,
    0x00, 0x00,
    0x10, 0x00,
    0x80, 0x00, 0x00, 0xAA,
    0x00, 0x38, 0x9B, 0x71,
};

static const uint8_t SUBFORMAT_FLOAT[16] = {
    0x03, 0x00, 0x00, 0x00,
    0x00, 0x00,
    0x10, 0x00,
    0x80, 0x00, 0x00, 0xAA,
    0x00, 0x38, 0x9B, 0x71,
};

typedef struct {
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint16_t valid_bits_per_sample;
    uint32_t channel_mask;
    uint32_t audio_data_size;
    const uint8_t *audio_data;
} wav_file_in_t;

static bool write_riff_chunk_descriptor(FILE *, size_t);
static bool write_format_subchunk(FILE *, int, int, uint32_t);
static bool write_data_subchunk(FILE *, size_t);
static bool write_audio_data(FILE *, const int16_t *, int, int);
static bool write_fourcc(FILE *fp, const char *ch);
static bool write_u16_le(FILE *fp, uint16_t);
static bool write_16_le(FILE *fp, int16_t);
static bool write_u32_le(FILE *fp, uint32_t);
static bool read_fmt_chunk(const uint8_t *, size_t, wav_file_in_t *);
static bool chunk_is(const char *, const uint8_t *);
static uint32_t read_u32_le(const uint8_t *data);
static int16_t read_u16_le(const uint8_t *data);
static float *copy_sample_data_8(const uint8_t *, int, int);
static float *copy_sample_data_16(const int16_t *, int, int);
static float *copy_sample_data_24(const uint8_t *, int, int);
static float *copy_sample_data_32(const int32_t *, int, int);
static float *copy_sample_data_float(const float *, int, int);

/******************************************************************************
 * Code for writing a WAV file.                                               *
 ******************************************************************************/

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

static bool write_riff_chunk_descriptor(FILE *fp, const size_t data_size)
{
    return write_fourcc(fp, RIFF_CHUNK_ID)
        && write_u32_le(fp, RIFF_CHUNK_SIZE + data_size)
        && write_fourcc(fp, FORMAT_WAVE);
}

static bool write_format_subchunk(FILE *fp, const int channels, const int bits_per_sample, const uint32_t sample_rate)
{
    return write_fourcc(fp, FMT_SUBCHUNK)
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
    return write_fourcc(fp, DATA_SUBCHUNK)
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

/******************************************************************************
 * Code for reading a WAV file.                                               *
 ******************************************************************************/

audio_t wav_read_audio(const uint8_t *wav_data, const size_t loaded_data_size)
{
    audio_t output = {0};
    if (!chunk_is(RIFF_CHUNK_ID, wav_data))
    {
        error(RIFF_HEADER_NOT_FOUND);
        return output;
    }
    const size_t riff_size = read_u32_le(wav_data + 4);
    const size_t riff_end_offset = CHUNK_HEADER_SIZE + riff_size;
    if (riff_end_offset > loaded_data_size)
    {
        fprintf(stderr, "RIFF end offset: %lu\n", riff_end_offset);
        error(SIZE_MISMATCH);
        return output;
    }
    if (memcmp(wav_data + 8, FORMAT_WAVE, 4) != 0)
    {
        error(NOT_WAVE_FORMAT);
        return output;
    }
    wav_file_in_t wav_file_in = {0};
    size_t chunk_offset = 12;
    while (chunk_offset + CHUNK_HEADER_SIZE <= riff_end_offset)
    {
        if (chunk_offset > loaded_data_size || loaded_data_size - chunk_offset < CHUNK_HEADER_SIZE)
        {
            error(CHUNK_HEADER_EXTENDS_BEYOND_EOF);
            return output;
        }
        const uint8_t *chunk_addr = wav_data + chunk_offset;
        const size_t data_size = read_u32_le(chunk_addr + 4);
        const size_t data_offset = chunk_offset + CHUNK_HEADER_SIZE;
        if (data_size > riff_end_offset - data_offset)
        {
            error(CHUNK_EXTENDS_BEYOND_EOF);
            return output;
        }
        const uint8_t *data = wav_data + data_offset;
        if (chunk_is(FMT_SUBCHUNK, chunk_addr))
        {
            if (!read_fmt_chunk(data, data_size, &wav_file_in))
                return output;
        }
        if (chunk_is(DATA_SUBCHUNK, chunk_addr))
        {
            wav_file_in.audio_data_size = read_u32_le(OFFSET(chunk_addr, 4));
            wav_file_in.audio_data = data;
        }
        const size_t padded_size = data_size + (data_size & 1);
        if (padded_size > riff_end_offset - data_offset)
        {
            error(CHUNK_EXTENDS_BEYOND_EOF);
            return output;
        }
        chunk_offset = data_offset + padded_size;
    }
    if (wav_file_in.audio_data == NULL)
    {
        error(DATA_SUBCHUNK_NOT_FOUND);
        return output;
    }
    if (wav_file_in.audio_format != AUDIO_FORMAT_PCM && wav_file_in.audio_format != AUDIO_FORMAT_FLOAT)
    {
        error(UNKNOWN_AUDIO_FORMAT);
        return output;
    }
    if (wav_file_in.audio_format == AUDIO_FORMAT_PCM && wav_file_in.valid_bits_per_sample != wav_file_in.bits_per_sample)
    {
        error(UNSUPPORTED_VALID_BIT_DEPTH);
        return output;
    }
    if (wav_file_in.audio_format == AUDIO_FORMAT_FLOAT && wav_file_in.bits_per_sample != 32)
    {
        error(UNSUPPORTED_FLOATING_POINT_SIZE);
        return output;
    }
    const uint32_t calculated_frame_size = wav_file_in.channels * (wav_file_in.bits_per_sample / 8);
    if (wav_file_in.block_align != calculated_frame_size)
    {
        error(INCONSISTENT_WAV_BLOCK_ALIGNMENT);
        return output;
    }
    output.sample_rate = wav_file_in.sample_rate;
    if (wav_file_in.bits_per_sample % 8 != 0 || wav_file_in.bits_per_sample > 32)
    {
        fprintf(stderr, "Invalid bits per sample: %d\n", wav_file_in.bits_per_sample);
        error(INVALID_BITS_PER_SAMPLE);
        return output;
    }
    const uint32_t frame_size = wav_file_in.bits_per_sample / 8 * wav_file_in.channels;
    if (wav_file_in.audio_data_size % frame_size != 0)
    {
        fprintf(stderr, "Audio data size: %d\n", wav_file_in.audio_data_size);
        error(INCONSISTENT_AUDIO_DATA_SIZE);
        return output;
    }
    output.frames = wav_file_in.audio_data_size / frame_size;
    const uint8_t *audio_data = wav_file_in.audio_data;
    if (wav_file_in.bits_per_sample == 8)
        output.audio_data = copy_sample_data_8(audio_data, wav_file_in.channels, output.frames);
    if (wav_file_in.bits_per_sample == 16)
        output.audio_data = copy_sample_data_16((int16_t *) audio_data, wav_file_in.channels, output.frames);
    if (wav_file_in.bits_per_sample == 24)
        output.audio_data = copy_sample_data_24(audio_data, wav_file_in.channels, output.frames);
    if (wav_file_in.bits_per_sample == 32 && wav_file_in.audio_format == AUDIO_FORMAT_PCM)
        output.audio_data = copy_sample_data_32((int32_t *) audio_data, wav_file_in.channels, output.frames);
    if (wav_file_in.bits_per_sample == 32 && wav_file_in.audio_format == AUDIO_FORMAT_FLOAT)
        output.audio_data = copy_sample_data_float((float *) audio_data, wav_file_in.channels, output.frames);
    if (output.audio_data == NULL)
    {
        error(MEMORY_ALLOCATION_FAILED);
        return output;
    }
    return output;
}

static bool read_fmt_chunk(const uint8_t *data, const size_t data_size, wav_file_in_t *wav)
{
    if (data_size < BASE_FMT_SIZE)
    {
        error(INVALID_FORMAT_CHUNK_SIZE);
        return false;
    }
    uint16_t format = read_u16_le(data);
    wav->channels = read_u16_le(data + 2);
    wav->sample_rate = read_u32_le(data + 4);
    wav->block_align = read_u16_le(data + 12);
    wav->bits_per_sample = read_u16_le(data + 14);
    wav->valid_bits_per_sample = wav->bits_per_sample;
    if (format == AUDIO_FORMAT_EXTENSIBLE)
    {
        if (data_size < EXTENSIBLE_FMT_SIZE)
        {
            error(INVALID_WAVEFORMATEXTENSIBLE_FORMAT_CHUNK);
            return false;
        }
        const uint16_t extension_size = read_u16_le(data + 16);
        if (extension_size < EXTENSIBLE_EXTRA_SIZE || WAVEFORMATEX_SIZE + extension_size > data_size)
        {
            error(INVALID_WAVEFORMATEXTENSIBLE_EXTENSION_SIZE);
            return false;
        }
        wav->valid_bits_per_sample = read_u16_le(data + 18);
        wav->channel_mask = read_u32_le(data + 20);
        const uint8_t *subformat = data + 24;
        if (memcmp(subformat, SUBFORMAT_PCM, sizeof SUBFORMAT_PCM) == 0)
            format = AUDIO_FORMAT_PCM;
        else if (memcmp(subformat, SUBFORMAT_FLOAT, sizeof SUBFORMAT_FLOAT) == 0)
            format = AUDIO_FORMAT_FLOAT;
        else
        {
            error(UNSUPPORTED_WAVEFORMATEXTENSIBLE_SUB_FORMAT);
            return false;
        }
    }
    wav->audio_format = format;
    return true;
}

static bool chunk_is(const char *chunk_id, const uint8_t *addr)
{
    return memcmp(chunk_id, addr, 4) == 0;
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
            const int16_t value = *sample_data;
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

