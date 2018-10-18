#include <pqueue.h>

static uint32_t next_job_id = 1;

struct pqueue *
pqueue_init(void) 
{
    struct pqueue *pq = malloc(sizeof(struct pqueue));
    
    pq->time = 0;
    pq->size = 0;
    pq->head = NULL;

    return pq;
}

uint32_t 
pqueue_push(struct pqueue *pq, uint32_t id, uint64_t delay, job_type_t type, timer_callback_t callback, void *data) 
{
    struct job *new_job = malloc(sizeof(struct job));
    new_job->id = id ? id : next_job_id++;
    new_job->delay = delay;
    new_job->tick = pq->time + delay;
    new_job->type = type;
    new_job->callback = callback;
    new_job->data = data;
    new_job->next_job = NULL;

    pq->size++;
    
    if (pq->head == NULL) {
        pq->head = new_job;
        return new_job->id;
    }

    if (new_job->tick <= pq->head->tick) {
        new_job->next_job = pq->head;
        pq->head = new_job;
        return new_job->id;
    }

    struct job *curr_job = pq->head;
    while (curr_job != NULL) {
        if (curr_job->next_job == NULL) {
            if (curr_job->tick < new_job->tick) {
                curr_job->next_job = new_job;
            }
        }
        else if (curr_job->next_job->tick > new_job->tick) {
            new_job->next_job = curr_job->next_job;
            curr_job->next_job = new_job;
            break;
        }
        curr_job = curr_job->next_job;
    }
    return new_job->id;
}

struct job *
pqueue_peek(struct pqueue *pq) 
{
    return pq->head;
}

int
pqueue_pop(struct pqueue *pq) 
{
    if(pq->head == NULL){
        return CLOCK_R_FAIL;
    }

    struct job *delete_job = pq->head;
    pq->head = delete_job->next_job;
    pq->size--;

    if (delete_job->type == PERIODIC) {
        pqueue_push(pq, 
                    delete_job->id, 
                    delete_job->delay, 
                    delete_job->type, 
                    delete_job->callback,
                    delete_job->data);
    }
    free(delete_job);

    return CLOCK_R_OK;
}

int 
pqueue_remove(struct pqueue *pq, uint32_t id)
{
    if (pq->head == NULL) {
        return CLOCK_R_FAIL;
    }
    if (pq->head->id == id) {
        struct job *remove_job = pq->head;
        pq->head = pq->head->next_job;
        free(remove_job);
        pq->size--;
        return CLOCK_R_OK;
    }

    struct job *curr_job = pq->head;
    while (curr_job->next_job != NULL) {
        if (curr_job->next_job->id == id) {
            struct job *remove_job = curr_job->next_job;
            curr_job->next_job = remove_job->next_job;
            free(remove_job);
            pq->size--;
            return CLOCK_R_OK;
        }
        curr_job = curr_job->next_job;
    }
    return CLOCK_R_FAIL;
}

void 
pqueue_destroy(struct pqueue *pq) 
{
    struct job *curr_job = pq->head;
    while (curr_job != NULL) {
        struct job *next_job = curr_job->next_job;
        free(curr_job);
        curr_job = next_job;
    }
    free(pq);
}
