#ifndef ARCTRACKER_LIBARCTRACKER_H
#define ARCTRACKER_LIBARCTRACKER_H

#include <stdint.h>
#include "midi/midi.h"
#include "ui/ui.h"
#include "player/player_command.h"
#include "ui/player_event.h"

#define ERROR_MESSAGE_BUFFER_SIZE 256

typedef struct arctracker_handle arctracker_t;

typedef struct {
    bool success;
    char error_message[ERROR_MESSAGE_BUFFER_SIZE];
} api_result_t;

typedef struct {
    arctracker_t *arctracker_handle;
    midi_subsystem_t *midi_handle;
} arctracker_init_result_t;

typedef struct {
    int num_tracks;
    int default_pattern_length;
    int lines_per_beat;
    int beats_per_minute;
    char *author;
} new_module_params_t;

static const int FORMAT_ARCTRACKER = 0;

arctracker_init_result_t arctracker_init(void);

arctracker_t *arctracker_create(void);

int arctracker_get_available_output_count(arctracker_t *handle);

api_result_t arctracker_get_available_outputs(arctracker_t *handle, ui_audio_device_info_t *output_devices, int requested_outputs);

api_result_t arctracker_use_output(arctracker_t *handle, int device_index, const char *name, const char *host_api_name);

api_result_t arctracker_use_default_output(arctracker_t *handle);

int arctracker_get_available_midi_count(midi_subsystem_t *handle);

api_result_t arctracker_get_available_midi_devices(midi_subsystem_t *handle, ui_midi_device_info_t *devices, int requested_count);

api_result_t arctracker_use_midi_device(midi_subsystem_t *handle, const char *name);

void arctracker_midi_set_playback_channel(midi_subsystem_t *midi, int channel);

void arctracker_midi_set_playback_instrument(midi_subsystem_t *midi, uint8_t instrument);

api_result_t arctracker_get_current_module(arctracker_t *handle, ui_module_info_t *module_info);

api_result_t arctracker_module_load(arctracker_t *handle, char *mod_filename, ui_module_info_t *module_info);

api_result_t arctracker_module_save(const arctracker_t *handle, const char *mod_filename, int format);

api_result_t arctracker_get_instrument_info(arctracker_t *handle, uint8_t slot, ui_instrument_info_t *instrument_info);

api_result_t arctracker_get_pattern_lengths(arctracker_t *handle, int *pattern_lengths, int num_patterns);

api_result_t arctracker_module_create(arctracker_t *handle, new_module_params_t params, ui_module_info_t *module_info);

api_result_t arctracker_player_start(arctracker_t *handle);

bool arctracker_player_cmd(arctracker_t *handle, player_command_t *command);

bool arctracker_poll_playback_event(arctracker_t *handle, player_event_t *event);

bool arctracker_poll_export_event(arctracker_t *handle, player_event_t *event);

void arctracker_get_transport_state(arctracker_t *handle, ui_transport_state_t *transport_state);

void arctracker_get_track_state(arctracker_t *handle, ui_track_state_t *track_state, int track);

void arctracker_toggle_mute_state(arctracker_t *handle, int track);

void arctracker_set_effects_displayed(arctracker_t *handle, int track, int effects_displayed);

void arctracker_get_and_reset_peak_levels(arctracker_t *handle, ui_peak_level_t *peak_levels);

void arctracker_get_export_state(arctracker_t *handle, ui_export_state_t *export_state);

void arctracker_get_pattern(arctracker_t *handle, int pattern_no, ui_pattern_event_t *pattern_buffer, int requested_lines, int requested_tracks);

api_result_t arctracker_export_audio(arctracker_t *handle, char *output_filename);

api_result_t arctracker_player_shutdown(arctracker_t *handle);

api_result_t arctracker_export_cleanup(arctracker_t *handle);

api_result_t arctracker_edit_get_event(arctracker_t *handle, int pattern_no, int pattern_index, int track, ui_pattern_event_t *event);

api_result_t arctracker_edit_set_event(arctracker_t *handle, int pattern_no, int pattern_index, int track, ui_pattern_event_t *event);

api_result_t arctracker_edit_get_sequence(arctracker_t *handle, int *sequence, int expected_sequence_len);

api_result_t arctracker_edit_set_sequence(arctracker_t *handle, const int *new_sequence, int new_sequence_len);

api_result_t arctracker_edit_create_pattern(arctracker_t *handle, int pattern_length, int *new_pattern_no);

api_result_t arctracker_edit_delete_pattern(arctracker_t *handle, int pattern_no);

api_result_t arctracker_edit_set_pattern_length(arctracker_t *handle, int pattern_no, int new_length);

api_result_t arctracker_edit_set_instrument(arctracker_t *handle, uint8_t slot, ui_instrument_update_t instrument_update);

api_result_t arctracker_edit_load_sample(arctracker_t *handle, const char *filename, ui_sample_info_t *sample_info);

api_result_t arctracker_edit_set_module_title(const arctracker_t *handle, const char *name, const char *author, int default_pattern_length);

api_result_t arctracker_edit_set_num_tracks(arctracker_t *handle, int num_tracks);

api_result_t arctracker_edit_set_tempo(arctracker_t *handle, uint8_t lines_per_beat, uint8_t beats_per_minute);

api_result_t arctracker_destroy(arctracker_t *handle);

void arctracker_midi_destroy(midi_subsystem_t *midi);

#endif //ARCTRACKER_LIBARCTRACKER_H
