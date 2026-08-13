#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "arguments.h"
#include "lib/libarctracker.h"
#include "console_ui.h"

static struct termios original_termios;

static void enable_raw_mode(void);
static void disable_raw_mode(void);
static void print_output_devices(arctracker_t *arctracker_handle);
static void print_midi_input_devices(midi_subsystem_t *midi_handle);
static ui_module_info_t load_module(arctracker_t *arctracker_handle, char *filename);
static void start_player(arctracker_t *arctracker_handle);
static void start_export(arctracker_t *arctracker_handle, char *filename);
static void shutdown_player(arctracker_t *arctracker_handle);
static void destroy(arctracker_t *arctracker_handle, midi_subsystem_t *midi);

int main(const int argc, char *argv[])
{
    const args_t config = read_configuration(argc, argv);
    const arctracker_init_result_t init_result = arctracker_init();
    if (init_result.arctracker_handle == NULL)
    {
        fprintf(stderr, "Failed to initialise arctracker\n");
        exit(EXIT_FAILURE);
    }
    arctracker_t *arctracker_handle = init_result.arctracker_handle;
    midi_subsystem_t *midi_handle = init_result.midi_handle;
    print_output_devices(arctracker_handle);
    print_midi_input_devices(midi_handle);
    ui_module_info_t module_info = load_module(arctracker_handle, config.mod_filename);
    printf("Module name: %s\n", module_info.name);
    for (int slot = 0; slot < 256; slot++)
    {
        ui_instrument_info_t instrument_info;
        arctracker_get_instrument_info(arctracker_handle, slot, &instrument_info);
        if (instrument_info.assigned)
            printf("Slot %d -> Sample %d -> %s\n", slot, instrument_info.sample_info.sample_index, instrument_info.name);
    }
    if (config.bounce)
    {
        start_export(arctracker_handle, config.output_filename);
        if (monitor_export(arctracker_handle))
            printf("Export complete\n");
        arctracker_export_cleanup(arctracker_handle);
    }
    else
    {
        start_player(arctracker_handle);
        enable_raw_mode();
        ui_loop(arctracker_handle, module_info);
        disable_raw_mode();
        shutdown_player(arctracker_handle);
    }
    destroy(arctracker_handle, midi_handle);
    exit(EXIT_SUCCESS);
}

static void enable_raw_mode(void)
{
    tcgetattr(STDIN_FILENO, &original_termios);
    struct termios raw = original_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

static void disable_raw_mode(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

static void print_output_devices(arctracker_t *arctracker_handle)
{
    const int output_count = arctracker_get_available_output_count(arctracker_handle);
    if (output_count == 0)
    {
        fprintf(stderr, "No available audio outputs\n");
        exit(EXIT_FAILURE);
    }
    ui_audio_device_info_t *audio_devices = malloc(output_count * sizeof(ui_audio_device_info_t));
    if (audio_devices == NULL)
    {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    const api_result_t result = arctracker_get_available_outputs(arctracker_handle, audio_devices, output_count);
    if (!result.success)
    {
        fprintf(stderr, "Failed to read available audio devices: %s\n", result.error_message);
        exit(EXIT_FAILURE);
    }
    printf("Available audio outputs:\n");
    for (int i = 0; i < output_count; i++)
        printf("%d: %s (%s)\n", audio_devices[i].device_index, audio_devices[i].name, audio_devices[i].host_api_name);
    free(audio_devices);
}

static void print_midi_input_devices(midi_subsystem_t *midi_handle)
{
    const int input_count = arctracker_get_available_midi_count(midi_handle);
    if (input_count == 0)
    {
        printf("No available MIDI input devices\n");
        return;
    }
    ui_midi_device_info_t *input_devices = malloc(input_count * sizeof(ui_midi_device_info_t));
    if (input_devices == NULL)
    {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    const api_result_t result = arctracker_get_available_midi_devices(midi_handle, input_devices, input_count);
    if (!result.success)
    {
        fprintf(stderr, "Failed to read available MIDI input devices: %s\n", result.error_message);
        exit(EXIT_FAILURE);
    }
    printf("Available MIDI input devices:\n");
    for (int i = 0; i < input_count; i++)
        printf("%s\n", input_devices[i].name);
    free(input_devices);
}

static ui_module_info_t load_module(arctracker_t *arctracker_handle, char *filename)
{
    ui_module_info_t module_info;
    const api_result_t result = arctracker_module_load(arctracker_handle, filename, &module_info);
    if (!result.success)
    {
        fprintf(stderr, "Module failed to load: %s\n", result.error_message);
        exit(EXIT_FAILURE);
    }
    return module_info;
}

static void start_player(arctracker_t *arctracker_handle)
{
    const api_result_t result = arctracker_use_default_output(arctracker_handle);
    if (!result.success)
    {
        fprintf(stderr, "Player failed to start: %s\n", result.error_message);
        exit(EXIT_FAILURE);
    }
}

static void start_export(arctracker_t *arctracker_handle, char *filename)
{
    const api_result_t result = arctracker_export_audio(arctracker_handle, filename);
    if (!result.success)
    {
        fprintf(stderr, "Export failed: %s\n", result.error_message);
        exit(EXIT_FAILURE);
    }
}

static void shutdown_player(arctracker_t *arctracker_handle)
{
    const api_result_t result = arctracker_player_shutdown(arctracker_handle);
    if (!result.success)
    {
        fprintf(stderr, "Shutdown failed: %s\n", result.error_message);
        exit(EXIT_FAILURE);
    }
}

static void destroy(arctracker_t *arctracker_handle, midi_subsystem_t *midi)
{
    arctracker_midi_destroy(midi);
    const api_result_t result = arctracker_destroy(arctracker_handle);
    if (!result.success)
    {
        fprintf(stderr, "Destroy failed: %s\n", result.error_message);
        exit(EXIT_FAILURE);
    }
}
