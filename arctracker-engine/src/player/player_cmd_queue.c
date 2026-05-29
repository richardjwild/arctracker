#include <pthread.h>
#include "player_cmd_queue.h"
#include "memory/heap.h"

player_command_queue_t *command_queue_init(void)
{
    player_command_queue_t *queue = allocate_array(PLAYER, 1, sizeof(player_command_queue_t));
    if (queue == NULL)
        return NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    return queue;
}

bool command_queue_add(player_command_queue_t *queue, const player_command_t command_in)
{
    pthread_mutex_lock(&queue->mutex);
    const int next_slot = (queue->head + 1) % CMD_QUEUE_SIZE;
    if (next_slot == queue->tail)
    {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    queue->buffer[queue->head] = command_in;
    queue->head = next_slot;
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

bool command_queue_read(player_command_queue_t *queue, player_command_t *command_out)
{
    pthread_mutex_lock(&queue->mutex);
    if (queue->tail == queue->head)
    {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    *command_out = queue->buffer[queue->tail];
    queue->tail = (queue->tail + 1) % CMD_QUEUE_SIZE;
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

void command_queue_destroy(player_command_queue_t *queue)
{
    if (queue != NULL)
    {
        pthread_mutex_destroy(&queue->mutex);
        deallocate(PLAYER, queue);
    }
}
