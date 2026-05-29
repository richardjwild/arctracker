#include <pthread.h>
#include "player_event_queue.h"
#include "memory/heap.h"

player_event_queue_t *event_queue_init(void)
{
    player_event_queue_t *queue = allocate_array(MAIN, 1, sizeof(player_event_queue_t));
    if (queue == NULL)
        return NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    return queue;
}

bool event_queue_add(player_event_queue_t *queue, player_event_t event_in)
{
    pthread_mutex_lock(&queue->mutex);
    const int next_slot = (queue->head + 1) % EVENT_QUEUE_SIZE;
    if (next_slot == queue->tail)
    {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    queue->buffer[queue->head] = event_in;
    queue->head = next_slot;
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

bool event_queue_read(player_event_queue_t *queue, player_event_t *event_out)
{
    pthread_mutex_lock(&queue->mutex);
    if (queue->tail == queue->head)
    {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    *event_out = queue->buffer[queue->tail];
    queue->tail = (queue->tail + 1) % EVENT_QUEUE_SIZE;
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

void event_queue_destroy(player_event_queue_t *queue)
{
    if (queue == NULL) return;
    pthread_mutex_destroy(&queue->mutex);
    deallocate(MAIN, queue);
}
