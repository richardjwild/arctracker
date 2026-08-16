#ifndef ARCTRACKER_ENGINE_UI_EVENT_QUEUE_H
#define ARCTRACKER_ENGINE_UI_EVENT_QUEUE_H

#include <stdatomic.h>
#include <stdbool.h>
#include "player_event.h"

#define EVENT_QUEUE_SIZE 20

typedef struct {
    player_event_t buffer[EVENT_QUEUE_SIZE];
    atomic_uint head;
    atomic_uint tail;
} player_event_queue_t;

player_event_queue_t *event_queue_init(void);

bool event_queue_add(player_event_queue_t *queue, player_event_t event_in);

bool event_queue_read(player_event_queue_t *queue, player_event_t *event_out);

void event_queue_destroy(player_event_queue_t *queue);

#endif //ARCTRACKER_ENGINE_UI_EVENT_QUEUE_H
