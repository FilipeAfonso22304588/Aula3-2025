#ifndef MLFQ_H
#define MLFQ_H

#include <stdint.h>
#include "queue.h"

// Prototipo da função do escalonador MLFQ
void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task);

#endif // MLFQ_H
