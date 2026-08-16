#include <pthread.h>
#include <stdatomic.h>
#include "player_cmd_queue.h"
#include "memory/heap.h"

player_command_queue_t *command_queue_init(void)
{
    player_command_queue_t *queue = allocate_array(PLAYER, 1, sizeof(player_command_queue_t));
    if (queue == NULL) return NULL;
    atomic_init(&queue->head, 0);
    atomic_init(&queue->tail, 0);
    pthread_mutex_init(&queue->producer_mutex, NULL);
    return queue;
}

bool command_queue_add(player_command_queue_t *queue, const player_command_t command_in)
{
    pthread_mutex_lock(&queue->producer_mutex);
    const int current_head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    const int current_tail = atomic_load_explicit(&queue->tail, memory_order_acquire);
    const int next_slot = (current_head + 1) % CMD_QUEUE_SIZE;
    if (next_slot == current_tail)
    {
        pthread_mutex_unlock(&queue->producer_mutex);
        return false;
    }
    queue->buffer[current_head] = command_in;
    atomic_store_explicit(&queue->head, next_slot, memory_order_release);
    pthread_mutex_unlock(&queue->producer_mutex);
    return true;
}

bool command_queue_read(player_command_queue_t *queue, player_command_t *command_out)
{
    const int current_head = atomic_load_explicit(&queue->head, memory_order_acquire);
    const int current_tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    if (current_tail == current_head) return false;
    *command_out = queue->buffer[current_tail];
    atomic_store_explicit( &queue->tail, (current_tail + 1) % CMD_QUEUE_SIZE, memory_order_release);
    return true;
}

void command_queue_destroy(player_command_queue_t *queue)
{
    if (queue != NULL)
    {
        pthread_mutex_destroy(&queue->producer_mutex);
        deallocate(PLAYER, queue);
    }
}