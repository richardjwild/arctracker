#ifndef ARCTRACKER_QUEUE_H
#define ARCTRACKER_QUEUE_H

#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>
#include "player_command.h"

#define CMD_QUEUE_SIZE 64

typedef struct {
    player_command_t buffer[CMD_QUEUE_SIZE];
    atomic_int head;
    atomic_int tail;
    pthread_mutex_t producer_mutex;
} player_command_queue_t;

player_command_queue_t *command_queue_init(void);

bool command_queue_add(player_command_queue_t *queue, player_command_t command_in);

bool command_queue_read(player_command_queue_t *queue, player_command_t *command_out);

void command_queue_destroy(player_command_queue_t *queue);

#endif //ARCTRACKER_QUEUE_H
