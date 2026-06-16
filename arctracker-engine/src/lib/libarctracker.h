#ifndef ARCTRACKER_LIBARCTRACKER_H
#define ARCTRACKER_LIBARCTRACKER_H

#include <stdint.h>
#include "ui/ui.h"
#include "player/player_command.h"
#include "ui/player_event.h"

#define ERROR_MESSAGE_BUFFER_SIZE 256

typedef struct arctracker_handle arctracker_t;
typedef struct {
    bool success;
    char error_message[ERROR_MESSAGE_BUFFER_SIZE];
} api_result_t;

arctracker_t *arctracker_create(void);

api_result_t arctracker_get_current_module(arctracker_t *handle, ui_module_info_t *module_info);

api_result_t arctracker_module_load(arctracker_t *handle, char *mod_filename, ui_module_info_t *module_info);

api_result_t arctracker_get_instrument_info(arctracker_t *handle, uint8_t slot, ui_instrument_info_t *instrument_info);

api_result_t arctracker_get_pattern_lengths(arctracker_t *handle, int *pattern_lengths, int num_patterns);

api_result_t arctracker_module_create(arctracker_t *handle, int num_channels, ui_module_info_t *module_info);

api_result_t arctracker_player_start(arctracker_t *handle);

bool arctracker_player_cmd(arctracker_t *handle, player_command_t *command);

bool arctracker_poll_playback_event(arctracker_t *handle, player_event_t *event);

bool arctracker_poll_export_event(arctracker_t *handle, player_event_t *event);

void arctracker_get_transport_state(arctracker_t *handle, ui_transport_state_t *transport_state);

void arctracker_get_export_state(arctracker_t *handle, ui_export_state_t *export_state);

void arctracker_get_pattern(arctracker_t *handle, int pattern_no, ui_pattern_event_t *pattern_buffer, int requested_lines, int requested_channels);

api_result_t arctracker_export_audio(arctracker_t *handle, char *output_filename);

api_result_t arctracker_player_shutdown(arctracker_t *handle);

api_result_t arctracker_export_cleanup(arctracker_t *handle);

api_result_t arctracker_edit_get_event(arctracker_t *handle, int pattern_no, int pattern_index, int channel_no, ui_pattern_event_t *event);

api_result_t arctracker_edit_set_event(arctracker_t *handle, int pattern_no, int pattern_index, int channel_no, ui_pattern_event_t *event);

api_result_t arctracker_edit_get_sequence(arctracker_t *handle, int *sequence, int expected_sequence_len);

api_result_t arctracker_edit_set_sequence(arctracker_t *handle, const int *new_sequence, int new_sequence_len);

api_result_t arctracker_edit_create_pattern(arctracker_t *handle, int pattern_length, int *new_pattern_no);

api_result_t arctracker_edit_delete_pattern(arctracker_t *arctracker, int pattern_no);

api_result_t arctracker_edit_set_instrument(arctracker_t *arctracker, uint8_t slot, ui_instrument_update_t instrument_update);

api_result_t arctracker_edit_load_sample(arctracker_t *arctracker, const char *filename, ui_sample_info_t *sample_info);

api_result_t arctracker_destroy(arctracker_t *handle);

#endif //ARCTRACKER_LIBARCTRACKER_H
