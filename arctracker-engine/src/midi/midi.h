#ifndef ARCTRACKER_ENGINE_MIDI_H
#define ARCTRACKER_ENGINE_MIDI_H

#include <stdbool.h>
#include "arctracker.h"

#define MAX_MIDI_DEVICE_NAME_LEN 128

typedef struct midi_handle midi_subsystem_t;

typedef struct {
    char name[MAX_MIDI_DEVICE_NAME_LEN];
} midi_device_info_t;

midi_subsystem_t *midi_initialise(arctracker_t *arctracker);

bool midi_get_device_count(const midi_subsystem_t *midi, unsigned int *count);

bool midi_get_devices(const midi_subsystem_t *midi, midi_device_info_t *device_info, unsigned int requested_count);

bool midi_use_device(const midi_subsystem_t *midi, const char *requested_name);

void midi_set_playback_channel(midi_subsystem_t *midi, int channel);

void midi_set_playback_instrument(midi_subsystem_t *midi, uint8_t instrument);

void midi_destroy(midi_subsystem_t *midi);

#endif //ARCTRACKER_ENGINE_MIDI_H
