#include <stdio.h>
#include <rtmidi_c.h>
#include "midi.h"
#include <string.h>
#include "messages.h"
#include "io/error.h"
#include "lib/libarctracker.h"
#include "memory/heap.h"

typedef struct {
    atomic_uchar selected_instrument;
    atomic_int selected_channel;
} note_input_state_t;

typedef struct midi_handle {
    RtMidiInPtr midi_in;
    arctracker_t *arctracker;
    note_input_state_t note_input;
} midi_subsystem_t;

const int PORT_NOT_FOUND = -1;
const unsigned char MIDI_CHAN_1_NOTE_ON = 0x90;
const unsigned char MIDI_CHAN_1_NOTE_OFF = 0x80;

static void handle_message(double, const unsigned char *, size_t, void *);

midi_subsystem_t *midi_initialise(arctracker_t *arctracker)
{
    // ReSharper disable once CppLocalVariableMayBeConst
    RtMidiInPtr midi_in = rtmidi_in_create_default();
    if (midi_in == NULL || !midi_in->ok)
    {
        if (midi_in)
        {
            error_with_detail(FAILED_TO_INITIALISE_MIDI, midi_in->msg);
            rtmidi_in_free(midi_in);
        }
        else
        {
            error(FAILED_TO_INITIALISE_MIDI);
        }
        return NULL;
    }
    midi_subsystem_t *midi = allocate_array(MAIN, sizeof(midi_subsystem_t), 1);
    midi->midi_in = midi_in;
    midi->arctracker = arctracker;
    return midi;
}

bool midi_get_device_count(const midi_subsystem_t *midi, unsigned int *count)
{
    if (midi->midi_in == NULL)
    {
        *count = 0;
        return true;
    }
    *count = rtmidi_get_port_count(midi->midi_in);
    if (!midi->midi_in->ok)
    {
        error(midi->midi_in->msg);
        return false;
    }
    return true;
}

bool midi_get_devices(const midi_subsystem_t *midi, midi_device_info_t *device_info, const unsigned int requested_count)
{
    if (requested_count == 0 || midi->midi_in == NULL) return true;
    const unsigned int device_count = rtmidi_get_port_count(midi->midi_in);
    if (!midi->midi_in->ok)
    {
        error(midi->midi_in->msg);
        return false;
    }
    for (unsigned int i = 0; i < device_count && i < requested_count; i++)
    {
        int name_length = MAX_MIDI_DEVICE_NAME_LEN;
        rtmidi_get_port_name(midi->midi_in, i, device_info[i].name, &name_length);
        if (!midi->midi_in->ok)
        {
            error(midi->midi_in->msg);
            return false;
        }
        device_info[i].name[MAX_MIDI_DEVICE_NAME_LEN - 1] = '\0';
    }
    return true;
}

bool midi_use_device(const midi_subsystem_t *midi, const char *requested_name)
{
    if (midi == NULL) return true;
    const unsigned int device_count = rtmidi_get_port_count(midi->midi_in);
    if (!midi->midi_in->ok)
    {
        error(midi->midi_in->msg);
        return false;
    }
    int requested_port = PORT_NOT_FOUND;
    for (unsigned int candidate_port = 0; requested_port == PORT_NOT_FOUND && candidate_port < device_count; candidate_port++)
    {
        char name[MAX_MIDI_DEVICE_NAME_LEN];
        int name_length = MAX_MIDI_DEVICE_NAME_LEN;
        rtmidi_get_port_name(midi->midi_in, candidate_port, name, &name_length);
        if (!midi->midi_in->ok)
        {
            error(midi->midi_in->msg);
            return false;
        }
        if (strncmp(requested_name, name, MAX_MIDI_DEVICE_NAME_LEN) == 0)
        {
            requested_port = (int) candidate_port;
        }
    }
    if (requested_port == PORT_NOT_FOUND)
    {
        error(MIDI_DEVICE_UNAVAILABLE);
        return false;
    }
    //
    // Now we know the requested port is available. Cancel any existing callbacks.
    //
    rtmidi_in_cancel_callback(midi->midi_in);
    rtmidi_close_port(midi->midi_in);
    //
    // Configure a new callback.
    //
    rtmidi_open_port(midi->midi_in, requested_port, "Arctracker Main Input");
    rtmidi_in_set_callback(midi->midi_in, handle_message, (void *) midi);
    return true;
}

static void handle_message(const double timestamp, const unsigned char *message, const size_t message_size, void *user_data)
{
    if (message_size < 3) return;
    (void) timestamp;
    const midi_subsystem_t *midi = (midi_subsystem_t *) user_data;
    const unsigned char status = message[0] & 0xf0;
    const unsigned char note = message[1];
    const unsigned char velocity = message[2];
    if (status == MIDI_CHAN_1_NOTE_ON && velocity > 0)
    {
        player_command_t command = (player_command_t) {
            .cmd_type = MIDI_NOTE_ON,
            .track = atomic_load(&midi->note_input.selected_channel),
            .instrument_no = atomic_load(&midi->note_input.selected_instrument),
            .note = note - 47,
            .velocity = velocity,
        };
        arctracker_player_cmd(midi->arctracker, &command);
    }
    else if (status == MIDI_CHAN_1_NOTE_OFF || (status == MIDI_CHAN_1_NOTE_ON && velocity == 0))
    {
        player_command_t command = (player_command_t) {
            .cmd_type = MIDI_NOTE_OFF,
            .track = atomic_load(&midi->note_input.selected_channel),
        };
        arctracker_player_cmd(midi->arctracker, &command);
    }
}

void midi_set_playback_channel(midi_subsystem_t *midi, const int channel)
{
    if (midi != NULL)
        atomic_store(&midi->note_input.selected_channel, channel);
}

void midi_set_playback_instrument(midi_subsystem_t *midi, const uint8_t instrument)
{
    if (midi != NULL)
        atomic_store(&midi->note_input.selected_instrument, instrument);
}

void midi_destroy(midi_subsystem_t *midi)
{
    if (midi == NULL) return;
    if (midi->midi_in)
    {
        rtmidi_in_cancel_callback(midi->midi_in);
        rtmidi_close_port(midi->midi_in);
        rtmidi_in_free(midi->midi_in);
    }
    deallocate(MAIN, midi);
}
