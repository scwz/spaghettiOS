#pragma once

#include <clock/clock.h>

struct job { 
    uint32_t id;
    uint64_t delay;
    timer_callback_t callback;
    void *data; 
    struct job * next_job;
};

struct pqueue {
    int cur_size;
    int max_size;
    uint64_t time;
    struct job *head;
};

struct pqueue *pqueue_init(void);

int pqueue_push(struct pqueue *pq, uint64_t delay, timer_callback_t callback, void *data);

struct job *pqueue_peek(struct pqueue *pq);

int pqueue_pop(struct pqueue *pq);

int pqueue_remove(struct pqueue *pq, uint32_t id);

void pqueue_destroy(struct pqueue *pq);
