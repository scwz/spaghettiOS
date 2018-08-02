#include <pqueue.h>

#define SIZE 10

struct pqueue *pqueue_init(void) {
    struct pqueue *pq = malloc(sizeof(struct pqueue));

    pq->time = 0;
    pq->cur_size = 0;
    pq->max_size = SIZE;
    pq->jobs = malloc(sizeof(struct job) * SIZE);

    return pq;
}

int pqueue_push(struct pqueue *pq, uint64_t delay, timer_callback_t callback, void *data) {
    return 0;
}

struct job pqueue_peek(struct pqueue *pq) {
    struct job job;
    return job;
}

int pqueue_pop(struct pqueue *pq) {
    return 0;
}

void pqueue_destroy(struct pqueue *pq) {

}
