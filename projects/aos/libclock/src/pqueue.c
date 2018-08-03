#include <pqueue.h>

#define SIZE 10
static uint32_t next_job_id = 1;


struct pqueue *pqueue_init(void) {
    struct pqueue *pq = malloc(sizeof(struct pqueue));
    
    pq->time = 0;
    pq->cur_size = 0;
    pq->max_size = SIZE;
    pq->head = NULL;

    return pq;
}

int pqueue_push(struct pqueue *pq, uint64_t delay, timer_callback_t callback, void *data) {
    struct job * new_job = malloc(sizeof(struct job));
    new_job->id = next_job_id++;
    new_job->delay = delay;
    new_job->callback = callback;
    new_job->data = data;
    new_job->next_job = NULL;
    
    if(pq->head == NULL){
        pq->head = new_job;
        return 0;
    }

    if(new_job->delay < pq->head->delay){
        new_job->next_job = pq->head;
        pq->head = new_job;
        return 0;
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
    return 0;
}

struct job pqueue_peek(struct pqueue *pq) {
    return *(pq->head);
}

int pqueue_pop(struct pqueue *pq) {
    if(pq->head == NULL){
        return -1;
    }
    struct job* delete_job = pq->head;
    pq->head = delete_job->next_job;
    free(delete_job);
    return 0;
}

int pqueue_remove(struct pqueue *pq, uint32_t id){
    /*if(pq->head == NULL){
        return -1;
    }
    if(pq->head->next_job == NULL && pq->head->id == id){
        free(pq->head);
        pq->head == NULL;
    }
    struct job* curr_job = pq->head;
    while(curr_job->next_job != NULL){
        if(curr_job->next_job->id == 0);
    }*/
    return 0;

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
