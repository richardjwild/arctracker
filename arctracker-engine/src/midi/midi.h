#ifndef ARCTRACKER_ENGINE_MIDI_H
#define ARCTRACKER_ENGINE_MIDI_H

#include <rtmidi_c.h>
#include <stdbool.h>

#define MAX_MIDI_DEVICE_NAME_LEN 128

typedef struct {
    RtMidiInPtr midi_in;
} midi_subsystem_t;

typedef struct {
    char name[MAX_MIDI_DEVICE_NAME_LEN];
} midi_device_info_t;

midi_subsystem_t midi_initialise(void);

bool midi_get_device_count(const midi_subsystem_t *midi, unsigned int *count);

bool midi_get_devices(const midi_subsystem_t *midi, midi_device_info_t *device_info, unsigned int requested_count);

void midi_destroy(const midi_subsystem_t *midi);

#endif //ARCTRACKER_ENGINE_MIDI_H
