#ifndef ARCTRACKER_QUEUE_H
#define ARCTRACKER_QUEUE_H

#include <stdbool.h>
#include <sys/_pthread/_pthread_mutex_t.h>
#include "player_command.h"

#define CMD_QUEUE_SIZE 64

typedef struct {
    player_command_t buffer[CMD_QUEUE_SIZE];
    volatile int head;
    volatile int tail;
    pthread_mutex_t mutex;
} player_command_queue_t;

player_command_queue_t *command_queue_init(void);

bool command_queue_add(player_command_queue_t *queue, player_command_t command_in);

bool command_queue_read(player_command_queue_t *queue, player_command_t *command_out);

void command_queue_destroy(player_command_queue_t *queue);

#endif //ARCTRACKER_QUEUE_H
