#include "rr.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include <unistd.h>

#define TIME_SLICE_MS 5

void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {

    if (*cpu_task) {
        // Add to the running time of the application/task
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
        //incrementa o contador do time slice do processo
        (*cpu_task)->slice_start_ms += TICKS_MS;

        // Caso o processo tenha terminado
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            // Application finished and can be removed (this is FIFO after all)
            free((*cpu_task));
            (*cpu_task) = NULL;
        }
        //confirma se o time slice do processo já ultrapassou o seu limite
        else if (((*cpu_task)->slice_start_ms >= TIME_SLICE_MS)){
            // reinicia o contador do slice
            (*cpu_task)->slice_start_ms = 0;
            // coloca o processo de volta no final da fila
            enqueue_pcb(rq, *cpu_task);
            // CPU livre para o próximo
            *cpu_task = NULL;
        }
    }
    if (*cpu_task == NULL) {            // If CPU is idle
        *cpu_task = dequeue_pcb(rq);   // Get next task from ready queue (dequeue from head)
    }if (*cpu_task) {
        // reinicia o contador de slice ao pegar da fila
        (*cpu_task)->slice_start_ms = 0;
    }
}