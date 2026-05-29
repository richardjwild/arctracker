#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include "console_ui.h"

static const char *NOTES[] = {
    "---",
    "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
    "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
    "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3"
};

static const char ALPHANUM[] = {
    '-',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
};

static bool state_changed(ui_transport_state_t);
static bool pattern_changed(ui_transport_state_t);
static void render_ui(ui_transport_state_t, ui_module_info_t);
static bool handle_input(arctracker_t *, ui_transport_state_t, char);
static void toggle_player_state(arctracker_t *);
static void toggle_loop_state(arctracker_t *);
static void seek(arctracker_t *, ui_transport_state_t, int, int);
static bool handle_playback_events(arctracker_t *);
static bool handle_export_events(arctracker_t *);

static ui_transport_state_t old_state = {
    .playing = false
};
static ui_pattern_event_t current_pattern[4096];

void ui_loop(arctracker_t *arctracker_handle, ui_module_info_t module_info)
{
    printf("Press space to start\n");
    ui_transport_state_t ui_state;
    arctracker_get_transport_state(arctracker_handle, &ui_state);
    arctracker_get_pattern(arctracker_handle, ui_state.pattern_no, current_pattern, ui_state.pattern_length, module_info.num_channels);
    bool player_healthy = true;
    bool exit_requested = false;
    while (!exit_requested && player_healthy)
    {
        int c;
        if ((c = getchar()) != EOF)
            exit_requested = handle_input(arctracker_handle, ui_state, c);
        if (state_changed(ui_state))
        {
            if (pattern_changed(ui_state))
                arctracker_get_pattern(arctracker_handle, ui_state.pattern_no, current_pattern, ui_state.pattern_length, module_info.num_channels);
            if (ui_state.playing)
                render_ui(ui_state, module_info);
            old_state = ui_state;
        }
        player_healthy = handle_playback_events(arctracker_handle);
        usleep(16000);
        if (player_healthy)
            arctracker_get_transport_state(arctracker_handle, &ui_state);
    }
    printf("Goodbye!\n");
}

bool monitor_export(arctracker_t *arctracker_handle)
{
    ui_export_state_t export_state;
    arctracker_get_export_state(arctracker_handle, &export_state);
    bool player_healthy = true;
    int pct_complete = 0;
    while (player_healthy && !export_state.completed)
    {
        if (export_state.percent_complete > pct_complete)
        {
            pct_complete = export_state.percent_complete;
            printf("%d%% done\n", pct_complete);
        }
        player_healthy = handle_export_events(arctracker_handle);
        usleep(16000);
        if (player_healthy)
            arctracker_get_export_state(arctracker_handle, &export_state);
    }
    return player_healthy;
}

static bool state_changed(const ui_transport_state_t new_state)
{
    if (new_state.playing != old_state.playing)
        return true;
    if (new_state.sequence_pos != old_state.sequence_pos)
        return true;
    if (new_state.pattern_index != old_state.pattern_index)
        return true;
    return false;
}

static bool pattern_changed(ui_transport_state_t new_state)
{
    return (new_state.playing != old_state.playing) || (new_state.pattern_no != old_state.pattern_no);
}

static void render_ui(ui_transport_state_t ui_state, ui_module_info_t module_info)
{
    printf("%3d %3d | ", ui_state.sequence_pos, ui_state.pattern_index);
    const ui_pattern_event_t *event_line = current_pattern + (ui_state.pattern_index * module_info.num_channels);
    for (int track = 0; track < module_info.num_channels; track++)
    {
        const ui_pattern_event_t event = event_line[track];
        printf(
            "%s %c%c%X%X | ",
            NOTES[event.note],
            ALPHANUM[event.sample_no],
            event.effects[0].effect_code == 0 ? '-' : event.effects[0].effect_code,
            event.effects[0].effect_data[0],
            event.effects[0].effect_data[1]);
    }
    printf("\n");
}

static bool handle_input(arctracker_t *arctracker_handle, ui_transport_state_t ui_state, char c)
{
    switch (tolower(c))
    {
        case ' ': toggle_player_state(arctracker_handle);
            break;
        case 'l': toggle_loop_state(arctracker_handle);
            break;
        case 'z': seek(arctracker_handle, ui_state, ui_state.sequence_pos - 1, 0);
            break;
        case 'x': seek(arctracker_handle, ui_state, ui_state.sequence_pos + 1, 0);
            break;
        case 'q': return true;
        default: { /* Nothing */ }
    }
    return false;
}

static void toggle_player_state(arctracker_t *arctracker_handle)
{
    player_command_t command = {
        .cmd_type = TOGGLE_PLAY,
    };
    arctracker_player_cmd(arctracker_handle, &command);
}

static void toggle_loop_state(arctracker_t *arctracker_handle)
{
    player_command_t command = {
        .cmd_type = TOGGLE_LOOP,
    };
    arctracker_player_cmd(arctracker_handle, &command);
}

static void seek(arctracker_t *arctracker_handle, ui_transport_state_t ui_state, int new_sequence_pos, int new_pattern_pos)
{
    player_command_t command = {
        .cmd_type = SEEK,
        .new_pattern_pos = new_pattern_pos,
        .new_sequence_pos = new_sequence_pos,
    };
    arctracker_player_cmd(arctracker_handle, &command);
    if (!ui_state.playing)
        printf("New sequence pos: %d\n", new_sequence_pos);
}

static bool handle_playback_events(arctracker_t *arctracker_handle)
{
    player_event_t event;
    while (arctracker_poll_playback_event(arctracker_handle, &event))
    {
        if (event.type == PLAYER_ERROR)
        {
            fprintf(stderr, "Player error occurred: %s\n", event.error_message);
            return false;
        }
    }
    return true;
}

static bool handle_export_events(arctracker_t *arctracker_handle)
{
    player_event_t event;
    while (arctracker_poll_export_event(arctracker_handle, &event))
    {
        if (event.type == PLAYER_ERROR)
        {
            fprintf(stderr, "Export error occurred: %s\n", event.error_message);
            return false;
        }
    }
    return true;
}
