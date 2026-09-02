#ifndef ARCTRACKER_MODULE_H
#define ARCTRACKER_MODULE_H

#include <stdbool.h>
#include <stdint.h>
#include "tempo.h"
#include "audio/interpolation_type.h"
#include "audio/volume_mapping_type.h"
#include "ui/ui.h"

#define MAX_LEN_TUNENAME 65
#define MAX_LEN_AUTHOR 65
#define MAX_LEN_SAMPLENAME 33
#define MAX_TRACKS 256
#define NUM_PATTERNS 256
#define MAX_EFFECTS 4
#define NUM_INSTRUMENT_SLOTS 256

static const int EVENT_SIZE_SINGLE_EFFECT = 4;
static const int EVENT_SIZE_MULTIPLE_EFFECT = 8;
static const int INTERNAL_GAIN_MAX = 255;

typedef enum
{
    NO_EFFECT = '\0',
    ARPEGGIO = '0',
    PITCH_SLIDE_UP = '1',
    PITCH_SLIDE_DOWN = '2',
    PORTAMENTO = '3',
    VIBRATO = '4',
    PORTAMENTO_PLUS_VOLUME_SIDE = '5',
    VIBRATO_PLUS_VOLUME_SLIDE = '6',
    // TODO: Implement tremolo (7).
    PATTERN_BREAK = 'B',
    CRESCENDO = 'C',
    DECRESCENDO = 'D',
    FINE_CRESCENDO = 'E',
    FINE_DECRESCENDO = 'F',
    FINE_PORTAMENTO = 'G',
    SEQUENCE_JUMP = 'J',
    DELAY_NEXT_EVENT = 'L',
    USE_SAMPLE_SLICE = 'O',
    SET_PANNING = 'P',
    SET_TEMPO = 'S',
    SET_TICKS_PER_SECOND = 'T',
    SET_VOLUME = 'V',
    SILENCE_SAMPLE_AFTER_DELAY = 'X',
    SET_LFO_WAVEFORM = 'Y',
} command_t;

typedef struct
{
    command_t command;
    uint8_t data;
} effect_t;

typedef struct
{
    int note;
    int instrument_no;
    effect_t effects[4];
} event_t;

typedef struct
{
    int num_lines;
    int line_capacity;
    event_t *events;
} pattern_t;

typedef struct {
    uint32_t offset;
    uint32_t length;
} sample_slice_t;

typedef struct {
    // TODO: Add instrument_type here when the time comes.
    bool assigned;
    char name[MAX_LEN_SAMPLENAME];
    uint8_t default_volume;
    int transpose;
    bool repeats;
    int repeat_offset;
    int repeat_length;
    int sample_index;
    sample_slice_t sample_slices[256];
} instrument_t;

typedef struct
{
    int sample_length;
    float sample_rate;
    uint8_t base_note;
    int8_t finetune;
    const float *sample_data;
} sample_t;

typedef struct {
    bool muted;
    int panning;
    int effects_displayed;
} track_t;

typedef struct
{
    const char *format;
    char name[MAX_LEN_TUNENAME];
    char author[MAX_LEN_AUTHOR];
    int sequence_length;
    int *sequence;
    int sequence_capacity;
    int num_tracks;
    track_t *tracks;
    uint32_t track_capacity;
    int num_patterns;
    pattern_t *patterns;
    int pattern_capacity;
    int sample_slots;
    sample_t *samples;
    int sample_capacity;
    instrument_t instruments[NUM_INSTRUMENT_SLOTS];
    int initial_ticks_per_event;
    int lines_per_beat;
    int initial_bpm;
    tempo_t *tempo_lookup;
    float master_gain;
    uint16_t default_pattern_length;
    interpolation_type_t interpolation_type;
    volume_mapping_type_t volume_mapping_type;
} module_t;

module_t *module_create(int num_tracks, int sequence_len, int num_patterns, int num_samples);

bool module_init(module_t *module, int default_pattern_length, int lines_per_beat, int beats_per_minute);

tempo_t module_get_initial_tempo(const module_t *module);

bool module_set_sequence(module_t *module, const int *new_sequence, int new_sequence_len);

bool module_create_pattern(module_t *module, int pattern_no, int num_lines);

void module_delete_pattern(module_t *module, int pattern_no);

bool module_set_pattern_length(const module_t *module, int pattern_no, int new_length);

void module_get_info(module_t *module, ui_module_info_t *module_info);

void module_get_instrument_info(const module_t *module, int instrument_index, ui_instrument_info_t *instrument_info);

void module_set_instrument(module_t *module, int instrument_index, instrument_t instrument_update);

bool module_link_sample(module_t *module, const float *sample_data, int sample_length, float sample_rate, int *sample_index);

bool module_set_sample(module_t *module, const float *sample_data, int sample_length, float sample_rate, uint8_t base_note, int8_t finetune, int sample_index);

void module_set_name(module_t *module, const char *name);

void module_set_author(module_t *module, const char *author);

void module_set_default_pattern_length(module_t *module, uint16_t default_pattern_length);

bool module_adjust_track_capacity(module_t *module, uint32_t new_track_capacity);

void module_set_num_tracks(module_t *module, uint32_t num_tracks);

void module_toggle_mute_state(const module_t *module, int track);

void module_set_effects_displayed(const module_t *module, int track, int effects_displayed);

void module_set_lines_per_beat(module_t *module, uint8_t lines_per_beat);

void module_set_initial_bpm(module_t *module, uint8_t beats_per_minute);

void module_destroy(module_t *module);

#endif //ARCTRACKER_MODULE_H
