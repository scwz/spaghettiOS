#pragma once

#include <clock/clock.h>

struct job { 
    uint32_t id;
    uint64_t delay;
    timer_callback_t callback;
    void *data; 
};

struct pqueue {
    int cur_size;
    int max_size;
    uint64_t time;
    struct job *jobs;
};

struct pqueue *pqueue_init(void);

int pqueue_push(struct pqueue *pq, uint64_t delay, timer_callback_t callback, void *data);

struct job pqueue_peek(struct pqueue *pq);

int pqueue_pop(struct pqueue *pq);

void pqueue_destroy(struct pqueue *pq);
