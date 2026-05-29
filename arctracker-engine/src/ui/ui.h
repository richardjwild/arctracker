#ifndef ARCTRACKER_UI_H
#define ARCTRACKER_UI_H

#include <stdbool.h>

#define TUNE_NAME_MAX_LEN 65
#define AUTHOR_NAME_MAX_LEN 65
#define MAX_LEN_SAMPLENAME 33
#define MAX_SAMPLES 36

typedef struct ui_sample_info {
    char name[MAX_LEN_SAMPLENAME];
    int default_gain;
    int sample_length;
    bool repeats;
    int repeat_offset;
    int repeat_length;
    int transpose;
} ui_sample_info_t;

typedef struct ui_module_info {
    char name[TUNE_NAME_MAX_LEN];
    char author[AUTHOR_NAME_MAX_LEN];
    int num_channels;
    int tune_length;
    int num_samples;
} ui_module_info_t;

typedef struct ui_effect {
    char effect_code;
    int effect_data[2];
} ui_effect_t;

typedef struct ui_pattern_event {
    int note;
    int sample_no;
    ui_effect_t effects[4];
} ui_pattern_event_t;

typedef struct ui_transport_state {
    bool playing;
    bool looping;
    int sequence_pos;
    int pattern_index;
    int pattern_no;
    int pattern_length;
} ui_transport_state_t ;

typedef struct ui_export_state {
    bool completed;
    int percent_complete;
} ui_export_state_t;

#endif //ARCTRACKER_UI_H
