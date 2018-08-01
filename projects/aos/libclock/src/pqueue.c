#include <pqueue.h>

struct pqueue *pqueue_init(int size) {
    return NULL;
}

int pqueue_push(struct pqueue *pq, uint64_t delay, timer_callback_t callback, void *data) {
    return 0;
}

struct event pqueue_pop(struct pqueue *pq) {
    struct event event;
    return event;
}

void pqueue_destroy(struct pqueue *pq) {

}
