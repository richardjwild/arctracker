#include "midi.h"
#include <stdio.h>
#include "io/error.h"

midi_subsystem_t midi_initialise(void)
{
    // ReSharper disable once CppLocalVariableMayBeConst
    RtMidiInPtr midi_in = rtmidi_in_create_default();
    if (midi_in == NULL || !midi_in->ok)
    {
        fprintf(stderr, "Failed to initialise RtMidi, MIDI input will be unavailable\n");
        if (midi_in)
        {
            fprintf(stderr, "%s\n", midi_in->msg);
            rtmidi_in_free(midi_in);
        }
        return (midi_subsystem_t) {0};
    }
    return (midi_subsystem_t) {
        .midi_in = midi_in
    };
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

void midi_destroy(const midi_subsystem_t *midi)
{
    // TODO: Stop listeners here.
    if (midi->midi_in)
        rtmidi_in_free(midi->midi_in);
}
