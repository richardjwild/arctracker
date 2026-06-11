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
static ui_module_info_t load_module(arctracker_t *arctracker_handle, char *filename);
static void start_player(arctracker_t *arctracker_handle);
static void start_export(arctracker_t *arctracker_handle, char *filename);
static void shutdown_player(arctracker_t *arctracker_handle);
static void destroy(arctracker_t *arctracker_handle);

int main(const int argc, char *argv[])
{
    const args_t config = read_configuration(argc, argv);
    arctracker_t *arctracker_handle = arctracker_create();
    if (arctracker_handle == NULL)
    {
        fprintf(stderr, "Failed to initialise arctracker\n");
        exit(EXIT_FAILURE);
    }
    ui_module_info_t module_info = load_module(arctracker_handle, config.mod_filename);
    printf("Module name: %s\n", module_info.name);
    for (int slot = 0; slot < 256; slot++)
    {
        ui_instrument_info_t instrument_info;
        arctracker_get_instrument_info(arctracker_handle, slot, &instrument_info);
        if (instrument_info.assigned)
            printf("Slot %d -> Sample %d -> %s\n", slot, instrument_info.sample_index, instrument_info.sample_info.name);
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
    destroy(arctracker_handle);
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
    const api_result_t result = arctracker_player_start(arctracker_handle);
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

static void destroy(arctracker_t *arctracker_handle)
{
    const api_result_t result = arctracker_destroy(arctracker_handle);
    if (!result.success)
    {
        fprintf(stderr, "Destroy failed: %s\n", result.error_message);
        exit(EXIT_FAILURE);
    }
}
