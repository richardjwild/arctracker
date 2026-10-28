#ifndef ARCTRACKER_ENGINE_MIDI_CMD_QUEUE_H
#define ARCTRACKER_ENGINE_MIDI_CMD_QUEUE_H

#include <stdatomic.h>
#include <stdbool.h>
#include "concurrency/mutexes.h"
#include "midi_command.h"

#define MIDI_CMD_QUEUE_SIZE 64

typedef struct {
    midi_command_t buffer[MIDI_CMD_QUEUE_SIZE];
    atomic_int head;
    atomic_int tail;
    arc_mutex_t producer_mutex;
} midi_command_queue_t;

midi_command_queue_t *midi_queue_init(void);

bool midi_queue_add(midi_command_queue_t *queue, midi_command_t command_in);

bool midi_queue_read(midi_command_queue_t *queue, midi_command_t *command_out);

void midi_queue_destroy(midi_command_queue_t *queue);

#endif //ARCTRACKER_ENGINE_MIDI_CMD_QUEUE_H
