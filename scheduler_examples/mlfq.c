#include "mlfq.h"
#include "msg.h"
#include "queue.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PRIORITY_LEVELS 8
#define TIME_SLICE_MS 500
#define PROMOTION_INTERVAL_MS 5000    // tempo S para reset das prioridades

static uint32_t last_promotion_time = 0;

// 8 filas estáticas (não precisamos criar dinamicamente)
static queue_t priority_queues[MAX_PRIORITY_LEVELS] = {0};

/**
 * @brief Multi-Level Feedback Queue (MLFQ) scheduler.
 *
 * Regras:
 * 1. Se Prioridade(A) > Prioridade(B), A executa-se (B não)
 * 2. Se Prioridade(A) = Prioridade(B), RR
 * 3. Quando um processo inicia, recebe prioridade máxima (0)
 * 4. Se usar o CPU durantetodo o time slice, baixa um nível
 * 5. Após tempo S, todos sobem para prioridade máxima
 *
 * @param current_time_ms tempo atual em milissegundos
 * @param rq fila geral de processos prontos (novos)
 * @param cpu_task processo atualmente a correr (NULL se CPU ociosa)
 */


void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {

    //Mover novos processos para a prioridade máxima como foram iniciados agora
    pcb_t *new_pcb;
    while ((new_pcb = dequeue_pcb(rq)) != NULL) {
        // prioridade máxima
        new_pcb->priority_level = 0;
        enqueue_pcb(&priority_queues[0], new_pcb);
    }

    //Após N tempo todos os processos sobrem para a priopridade máxima conforme é dito na regra 5
    if (current_time_ms - last_promotion_time >= PROMOTION_INTERVAL_MS) {
        for (int i = 1; i < MAX_PRIORITY_LEVELS; i++) {
            pcb_t *p;
            while ((p = dequeue_pcb(&priority_queues[i])) != NULL) {
                p->priority_level = 0;
                enqueue_pcb(&priority_queues[0], p);
            }
        }
        last_promotion_time = current_time_ms;
    }

    //Atualiza processo atualmente em execução
    if (*cpu_task) {

        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        // Caso 1: terminou
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            free(*cpu_task);
            *cpu_task = NULL;
        }

        // Caso 2: time slice terminou → baixa prioridade (Regra 4)
        else if (current_time_ms - (*cpu_task)->slice_start_ms >= TIME_SLICE_MS) {
            uint32_t lvl = (*cpu_task)->priority_level;
            if (lvl < MAX_PRIORITY_LEVELS - 1)
                (*cpu_task)->priority_level++;

            (*cpu_task)->slice_start_ms = current_time_ms;
            enqueue_pcb(&priority_queues[(*cpu_task)->priority_level], *cpu_task);
            *cpu_task = NULL;
        }
    }

    //Se CPU não estiver a correr nenhum processo ele escolhe o próximo na lisa de prioridade
    if (*cpu_task == NULL) {
        for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
            if (priority_queues[i].head != NULL) {
                *cpu_task = dequeue_pcb(&priority_queues[i]);
                (*cpu_task)->slice_start_ms = current_time_ms;
                break;
            }
        }
    }
}
