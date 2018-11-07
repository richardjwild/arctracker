#include "format_soundtracker.h"
#include <memory/heap.h>
#include <memory/bits.h>
#include <audio/mix.h>

#define LEN_TUNENAME_ST 20
#define LEN_SAMPLENAME_ST 22
#define MAX_LEN_SEQUENCE 128
#define LEN_FORMAT_ID 4
#define NO_SAMPLES_SOUNDTRACKER_2_4 31 // Sample count was increased to 31 in Soundtracker 2.4

typedef struct
{
    char name[LEN_SAMPLENAME_ST];
    uint16_t length;
    uint8_t fine_tune;
    uint8_t volume;
    uint16_t repeat_offset;
    uint16_t repeat_length;
} st_sample_format_t;

typedef struct
{
    char title[LEN_TUNENAME_ST];
    st_sample_format_t sample_metadata[NO_SAMPLES_SOUNDTRACKER_2_4];
    uint8_t sequence_length;
    uint8_t beats_per_minute;
    uint8_t sequence[MAX_LEN_SEQUENCE];
    uint8_t format_id[LEN_FORMAT_ID];
} st_31_inst_file_format_t;

static const char *SOUNDTRACKER_FORMAT = "SOUNDTRACKER";
static const char *AUTHOR_UNKNOWN = "Not specified";
static const int MODULE_GAIN_MAX = 64;
static const int ST_NUM_CHANNELS = 4;
static const int ST_NUM_PATTERNS = 64;
static const int ST_PATTERN_LENGTH = 64;
static const int ST_INITIAL_SPEED = 6; // TODO: verify this assumption.

static int AMIGA_PANNING[] = {
        PAN_LEFT_FULL,
        PAN_RIGHT_FULL,
        PAN_RIGHT_FULL,
        PAN_LEFT_FULL
};

bool is_soundtracker_format(mapped_file_t);

module_t read_soundtracker_module(mapped_file_t);

static int *pattern_lengths();

static sample_t *get_samples(st_sample_format_t *file_samples);

static int *convert_sequence(const uint8_t *, int);

const format_t soundtracker_format()
{
    format_t format = {
            .is_this_format = is_soundtracker_format,
            .read_module = read_soundtracker_module
    };
    return format;
}

bool is_soundtracker_format(mapped_file_t file)
{
    return true;
}

module_t read_soundtracker_module(mapped_file_t file)
{
    st_31_inst_file_format_t *modfile = file.addr;
    module_t module;
    module.format = SOUNDTRACKER_FORMAT;
    module.num_samples = NO_SAMPLES_SOUNDTRACKER_2_4;
    module.initial_speed = ST_INITIAL_SPEED;
    memset(module.name, 0, MAX_LEN_TUNENAME);
    memcpy(module.name, modfile->title, LEN_TUNENAME_ST);
    strcpy(module.author, AUTHOR_UNKNOWN);
    module.gain_maximum = MODULE_GAIN_MAX;
    module.initial_panning = AMIGA_PANNING;
    module.num_channels = ST_NUM_CHANNELS;
    module.num_patterns = ST_NUM_PATTERNS;
    module.pattern_lengths = pattern_lengths();
    module.samples = get_samples(modfile->sample_metadata);
    module.tune_length = modfile->sequence_length;
    module.sequence = convert_sequence(modfile->sequence, module.tune_length);
    return module;
}

static int *pattern_lengths()
{
    int *array = allocate_array(ST_NUM_PATTERNS, sizeof(int));
    for (int i = 0; i < ST_NUM_PATTERNS; i++)
        array[i] = ST_PATTERN_LENGTH;
    return array;
}

static sample_t *get_samples(st_sample_format_t *file_samples)
{
    sample_t *samples = allocate_array(NO_SAMPLES_SOUNDTRACKER_2_4, sizeof(sample_t));
    for (int i = 0; i < NO_SAMPLES_SOUNDTRACKER_2_4; i++)
    {
        strncpy(samples[i].name, file_samples[i].name, LEN_SAMPLENAME_ST);
        samples[i].default_gain = file_samples[i].volume;
        samples[i].sample_length = FROM_BIG_ENDIAN(file_samples[i].length) * AMIGA_WORD_LENGTH;
        samples[i].repeat_offset = FROM_BIG_ENDIAN(file_samples[i].repeat_offset) * AMIGA_WORD_LENGTH;
        samples[i].repeat_length = FROM_BIG_ENDIAN(file_samples[i].repeat_length) * AMIGA_WORD_LENGTH;
        samples[i].transpose = 12;
        samples[i].repeats = (samples[i].repeat_length > 2);
    }
    return samples;
}

static int *convert_sequence(const uint8_t *sequence_in, int tune_length)
{
    int *sequence_out = allocate_array(tune_length, sizeof(int));
    for (int i = 0; i < tune_length; i++)
        sequence_out[i] = sequence_in[i];
    return sequence_out;
}
