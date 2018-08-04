#include <pqueue.h>

static uint32_t next_job_id = 1;

struct pqueue *pqueue_init(void) {
    struct pqueue *pq = malloc(sizeof(struct pqueue));
    
    pq->time = 0;
    pq->size = 0;
    pq->head = NULL;

    return pq;
}

uint32_t pqueue_push(struct pqueue *pq, uint64_t delay, timer_callback_t callback, void *data) {
    struct job * new_job = malloc(sizeof(struct job));
    new_job->id = next_job_id++;
    new_job->delay = delay;
    new_job->callback = callback;
    new_job->data = data;
    new_job->next_job = NULL;

    pq->size++;
    
    if(pq->head == NULL){
        pq->head = new_job;
        return new_job->id;
    }

    if(new_job->delay < pq->head->delay){
        new_job->next_job = pq->head;
        pq->head = new_job;
        return new_job->id;
    }

    struct job* curr_job = pq->head;
    while(curr_job != NULL){

        if(curr_job->next_job == NULL){
            if(curr_job->delay < new_job->delay){
                curr_job->next_job = new_job;
            }
        }
        else if(curr_job->next_job->delay > new_job->delay){
            new_job->next_job = curr_job->next_job;
            curr_job->next_job = new_job;
            break;
        }
        curr_job = curr_job->next_job;
    }
    return new_job->id;
}

struct job *pqueue_peek(struct pqueue *pq) {
    return pq->head;
}

int pqueue_pop(struct pqueue *pq) {
    if(pq->head == NULL){
        return CLOCK_R_FAIL;
    }
    struct job* delete_job = pq->head;
    pq->head = delete_job->next_job;
    free(delete_job);
    pq->size--;
    return CLOCK_R_OK;
}

int pqueue_remove(struct pqueue *pq, uint32_t id){
    if(pq->head == NULL){
        return CLOCK_R_FAIL;
    }
    if(pq->head->id == id){
        struct job* remove_job = pq->head;
        pq->head = pq->head->next_job;
        free(remove_job);
        pq->size--;
        return CLOCK_R_OK;
    }

    struct job* curr_job = pq->head;
    while(curr_job->next_job != NULL){
        if(curr_job->next_job->id == id){
            struct job* remove_job = curr_job->next_job;
            curr_job->next_job = remove_job->next_job;
            free(remove_job);
            pq->size--;
            return CLOCK_R_OK;
        }
        curr_job = curr_job->next_job;
    }
    return CLOCK_R_FAIL;
}

void pqueue_destroy(struct pqueue *pq) {
    struct job* curr_job = pq->head;
    while(curr_job != NULL){
        struct job* next_job = curr_job->next_job;
        free(curr_job);
        curr_job = next_job;
    }
    free(pq);
}
