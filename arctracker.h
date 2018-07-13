#ifndef ARCTRACKER_H
#define ARCTRACKER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_LEN_TUNENAME 65
#define MAX_LEN_AUTHOR 65
#define MAX_LEN_SAMPLENAME 33
#define MAX_CHANNELS 16
#define NUM_PATTERNS 256
#define MAX_TUNELENGTH 128

#define EVENT_SIZE_SINGLE_EFFECT 4;
#define EVENT_SIZE_MULTIPLE_EFFECT 8;

typedef enum
{
    NO_EFFECT,
    ARPEGGIO,
    PORTAMENTO_UP,
    PORTAMENTO_DOWN,
    BREAK_PATTERN,
    SET_TRACK_STEREO,
    VOLUME_SLIDE_UP,
    VOLUME_SLIDE_DOWN,
    JUMP_TO_POSITION,
    SET_TEMPO,
    SET_VOLUME_TRACKER,
    TONE_PORTAMENTO,
    VOLUME_SLIDE,
    SET_VOLUME_DESKTOP_TRACKER,
    PORTAMENTO_FINE,
    SET_TEMPO_FINE,
    VOLUME_SLIDE_FINE
} command_t;

typedef struct
{
    char name[MAX_LEN_SAMPLENAME];
    __uint32_t default_gain;
    __uint32_t sample_length;
    bool repeats;
    __uint32_t repeat_offset;
    __uint32_t repeat_length;
    int transpose;
    void *sample_data;
} sample_t;

typedef struct
{
    __uint8_t code;
    command_t command;
    __uint8_t data;
} effect_t;

typedef struct
{
    __uint8_t note;
    __uint8_t sample;
    effect_t effects[4];
} channel_event_t;

typedef struct
{
    char *format;
    char name[MAX_LEN_TUNENAME];
    char author[MAX_LEN_AUTHOR];
    long num_channels;
    unsigned char default_channel_stereo[MAX_CHANNELS];
    int initial_speed;
    int tune_length;
    int num_patterns;
    int num_samples;
    sample_t *samples;
    __uint8_t sequence[MAX_TUNELENGTH];
    void *patterns[NUM_PATTERNS];
    __uint8_t pattern_lengths[NUM_PATTERNS];
    size_t (*decode_event)(const __uint32_t *raw, channel_event_t *decoded);
} module_t;

#endif // ARCTRACKER_H