#pragma once

#include <clock/clock.h>


struct job { 
    uint32_t id;
    uint64_t delay;
    job_type_t type;
    timer_callback_t callback;
    void *data; 
    struct job * next_job;
};

struct pqueue {
    uint32_t size;
    uint64_t time;
    struct job *head;
};

struct pqueue *pqueue_init(void);

uint32_t pqueue_push(struct pqueue *pq, uint32_t id, uint64_t delay, job_type_t type, timer_callback_t callback, void *data);

struct job *pqueue_peek(struct pqueue *pq);

int pqueue_pop(struct pqueue *pq);

int pqueue_remove(struct pqueue *pq, uint32_t id);

void pqueue_destroy(struct pqueue *pq);
