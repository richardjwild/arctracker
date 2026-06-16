#include "wav.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "io/error.h"

#define RIFF_CHUNK_SIZE 36

static const char *CHUNK_ID = "RIFF";
static const char *FORMAT = "WAVE";
static const char *SUB_CHUNK_1_ID = "fmt ";
static const char *SUB_CHUNK_2_ID = "data";
static const char *WRITEONLY_BINARY = "wb";
static const int AUDIO_FORMAT_PCM = 1;
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

bool wav_write_pcm16(const char *filename, const wav_pcm16_t *wav)
{
    const size_t data_size = wav->frames * wav->channels * BITS_PER_SAMPLE_PCM16;
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
    return write_fourcc(fp, CHUNK_ID)
        && write_u32_le(fp, RIFF_CHUNK_SIZE + data_size)
        && write_fourcc(fp, FORMAT);
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
