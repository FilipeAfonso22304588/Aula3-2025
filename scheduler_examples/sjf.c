#include "sjf.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include <unistd.h>

/**
 * @brief Shortest-Job-First (SJF) scheduling algorithm.
 *
 * @param current_time_ms The current time in milliseconds.
 * @param listaDeProcessos Pointer to the ready queue containing tasks that are ready to run.
 * @param cpu_task Double pointer to the currently running task. This will be updated
 *                 to point to the next task to run.
 */

pcb_t *dequeue_shortest_job(queue_t *listaDeProcessos) {
    //verifica se a lista não esta vázia
    if (!listaDeProcessos || !listaDeProcessos->head) {
        return NULL;
    }

    queue_elem_t *elementoAnteriorAoMenor = NULL;
    queue_elem_t *menorElemento = listaDeProcessos->head;
    queue_elem_t *anteriorProcura = NULL;
    queue_elem_t *procuraElemento = listaDeProcessos->head;

    //Varre os processos em queue e procura o com menor tempo de consumo
    while (procuraElemento) {
        if (procuraElemento->pcb->time_ms < menorElemento->pcb->time_ms) {
            menorElemento = procuraElemento;
            elementoAnteriorAoMenor = anteriorProcura;
        }
        anteriorProcura = procuraElemento;
        procuraElemento = procuraElemento->next;
    }

    //Remove o elemento mínimo da fila já que vou o CPU vai correr ele e não preciso dele mais na lista
    if (elementoAnteriorAoMenor) {
        elementoAnteriorAoMenor->next = menorElemento->next;
    } else {
        listaDeProcessos->head = menorElemento->next;
    }

    if (listaDeProcessos->tail == menorElemento) {
        listaDeProcessos->tail = elementoAnteriorAoMenor;
    }

    //retorna o processo escolhido com menor tempo
    pcb_t *chosen = menorElemento->pcb;
    free(menorElemento);
    return chosen;
}


void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS; // Add to the running time of the application/task
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // Task finished
            // Send msg to application
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
    }


    if (*cpu_task == NULL) {
        // If CPU is idle
       *cpu_task = dequeue_shortest_job(rq); // Get next task from ready queue (dequeue from head)
    }
}
