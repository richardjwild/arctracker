#ifndef ARCTRACKER_UI_H
#define ARCTRACKER_UI_H

#include <stdbool.h>
#define TUNE_NAME_MAX_LEN 65
#define AUTHOR_NAME_MAX_LEN 65
#define MAX_LEN_SAMPLENAME 33
#define MAX_TRACKS 256

typedef struct ui_sample_info {
    int sample_index;
    int sample_length;
} ui_sample_info_t;

typedef struct ui_instrument_info {
    bool assigned;
    char name[MAX_LEN_SAMPLENAME];
    int default_volume;
    int transpose;
    bool repeats;
    int repeat_offset;
    int repeat_length;
    ui_sample_info_t sample_info;
} ui_instrument_info_t;

typedef struct ui_instrument_update {
    bool assigned;
    const char name[MAX_LEN_SAMPLENAME];
    int default_volume;
    int transpose;
    int sample_index;
    bool repeats;
    int repeat_offset;
    int repeat_length;
} ui_instrument_update_t;

typedef struct ui_track_state {
    bool muted;
    int panning;
    int effects_displayed;
} ui_track_state_t;

typedef enum ui_interpolation_type {
    ARCTRACKER = 0, ARCHIMEDES = 1,
} ui_interpolation_type_t;

typedef enum ui_volume_mapping_type {
    UI_VOLUME_ARCHIMEDES = 0, UI_VOLUME_AMIGA = 1,
} ui_volume_mapping_type_t;

typedef struct ui_module_info {
    char name[TUNE_NAME_MAX_LEN];
    char author[AUTHOR_NAME_MAX_LEN];
    int num_tracks;
    int tune_length;
    int num_patterns;
    float master_gain;
    int default_pattern_length;
    int lines_per_beat;
    int initial_bpm;
    ui_interpolation_type_t interpolation_type;
    ui_volume_mapping_type_t volume_mapping_type;
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
    bool playback_available;
    bool looping;
    int sequence_pos;
    int pattern_index;
    int pattern_no;
    int pattern_length;
    int current_bpm;
} ui_transport_state_t;

typedef struct ui_export_state {
    bool completed;
    int percent_complete;
} ui_export_state_t;

typedef struct ui_peak_level {
    float left;
    float right;
} ui_peak_level_t;

typedef struct ui_audio_device_info {
    int device_index;
    char name[256];
    char host_api_name[256];
} ui_audio_device_info_t;

typedef struct ui_midi_device_info {
    char name[128];
} ui_midi_device_info_t;

#endif //ARCTRACKER_UI_H
