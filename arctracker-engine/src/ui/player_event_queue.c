#include "player_event_queue.h"
#include "memory/heap.h"

player_event_queue_t *event_queue_init(void)
{
    player_event_queue_t *queue = allocate_array(MAIN, 1, sizeof(player_event_queue_t));
    if (queue == NULL)
        return NULL;
    atomic_init(&queue->head, 0);
    atomic_init(&queue->tail, 0);
    return queue;
}

bool event_queue_add(player_event_queue_t *queue, player_event_t event_in)
{
    const int current_tail = atomic_load_explicit(&queue->tail, memory_order_acquire);
    const int current_head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    const int next_slot = (current_head + 1) % EVENT_QUEUE_SIZE;
    if (next_slot == current_tail) return false;
    queue->buffer[current_head] = event_in;
    atomic_store_explicit(&queue->head, next_slot, memory_order_release);
    return true;
}

bool event_queue_read(player_event_queue_t *queue, player_event_t *event_out)
{
    const int current_head = atomic_load_explicit(&queue->head, memory_order_acquire);
    const int current_tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    if (current_tail == current_head) return false;
    *event_out = queue->buffer[current_tail];
    atomic_store_explicit(&queue->tail, (current_tail + 1) % EVENT_QUEUE_SIZE, memory_order_release);
    return true;
}

void event_queue_destroy(player_event_queue_t *queue)
{
    if (queue != NULL) deallocate(MAIN, queue);
}
