#ifndef ARCTRACKER_MODULE_H
#define ARCTRACKER_MODULE_H

#include <stdbool.h>
#include <stdint.h>
#include "ui/ui.h"

#define MAX_LEN_TUNENAME 65
#define MAX_LEN_AUTHOR 65
#define MAX_LEN_SAMPLENAME 33
#define MAX_CHANNELS 16
#define NUM_PATTERNS 256
#define MAX_EFFECTS 4

static const int EVENT_SIZE_SINGLE_EFFECT = 4;
static const int EVENT_SIZE_MULTIPLE_EFFECT = 8;
static const int INTERNAL_GAIN_MAX = 255;

typedef enum
{
    NO_EFFECT = '\0',
    ARPEGGIO = '0',
    PORTAMENTO_UP = '1',
    PORTAMENTO_DOWN = '2',
    TONE_PORTAMENTO = '3',
    VOLUME_SLIDE = 'A',
    BREAK_PATTERN = 'B',
    PORTAMENTO_FINE = 'C',
    SET_TEMPO_FINE = 'D',
    SET_TRACK_STEREO = 'E',
    VOLUME_SLIDE_FINE = 'F',
    VOLUME_SLIDE_UP = 'G',
    VOLUME_SLIDE_DOWN = 'H',
    JUMP_TO_POSITION = 'J',
    SET_TEMPO = 'S',
    SET_VOLUME = 'V',
} command_t;

typedef struct
{
    command_t command;
    uint8_t data;
} effect_t;

typedef struct
{
    int note;
    int sample_no;
    effect_t effects[4];
} event_t;

typedef struct
{
    int num_lines;
    event_t *events;
} pattern_t;

typedef struct
{
    char name[MAX_LEN_SAMPLENAME];
    int default_gain;
    int sample_length;
    bool repeats;
    int repeat_offset;
    int repeat_length;
    int transpose;
    float *sample_data;
} sample_t;

typedef struct
{
    const char *format;
    char name[MAX_LEN_TUNENAME];
    char author[MAX_LEN_AUTHOR];
    int tune_length;
    int *sequence;
    int num_channels;
    int *initial_panning;
    int num_patterns;
    pattern_t **patterns;
    int num_samples;
    sample_t *samples;
    int initial_speed;
    float volume_cmd_gain_factor;
    float master_gain;
    float *gain_curve;
} module_t;

module_t *module_create(int num_channels, int sequence_len, int num_patterns, int num_samples);

bool module_init(module_t *module);

bool module_create_pattern(module_t *module, int pattern_index, int num_lines);

void module_destroy(module_t *module);

void module_get_info(module_t *module, ui_module_info_t *module_info);

void module_get_sample_info(module_t *module, int sample_no, ui_sample_info_t *sample_info);

#endif //ARCTRACKER_MODULE_H
