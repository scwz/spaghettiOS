#pragma once

#include <clock/clock.h>

struct event { 
    uint32_t id;
    uint64_t delay;
    timer_callback_t callback;
    void *data; 
};

struct pqueue {
    int len;
    int size;
    struct event *events;
};

struct pqueue *pqueue_init(int size);

int pqueue_push(struct pqueue *pq, uint64_t delay, timer_callback_t callback, void *data);

struct event pqueue_pop(struct pqueue *pq);

void pqueue_destroy(struct pqueue *pq);
