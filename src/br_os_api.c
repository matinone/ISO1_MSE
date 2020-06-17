/*
 * br_os_api.c
 *
 *  Created on: 2020
 *      Author: mbrignone
 */


#include "br_os_api.h"


/*************************************************************************************************
     *  @brief Delay en unidades de tick del OS.
     *
***************************************************************************************************/
os_error os_delay(uint32_t ticks)   {

    os_task* current_task;

    // cannot call a delay from an ISR
    if (os_get_global_state() == OS_STATE_ISR)  {
        os_set_error(OS_ERROR_DELAY_FROM_ISR, os_delay);
        return OS_ERROR_DELAY_FROM_ISR;
    }

    if (ticks > 0)  {

        os_enter_critical_section();

        current_task = os_get_current_task();
        current_task->state = OS_TASK_BLOCKED;
        current_task->remaining_blocked_ticks = ticks;

        os_exit_critical_section();

        // force scheduling to go out of the delayed task
        os_cpu_yield();
    }

    return OS_OK;
}



/*************************************************************************************************
     *  @brief Inicializa un semaforo binario.
     *
***************************************************************************************************/
void os_semaphore_init(os_semaphore* semaphore) {
    semaphore->taken = true;            // initially taken
    semaphore->associated_task = NULL;  // no task initially associated
}


/*************************************************************************************************
     *  @brief Se toma un semaforo. La tarea queda bloqueada hasta que el semaforo este libre
     *  o hasta que transcurra el timeout (ticks_to_wait).
     *  ticks_to_wait = 0 significa que no hay timeout y se queda esperando por siempre
     *  hasta que el semaforo este libre.
     *
     * Retorna true si se pudo tomar el semaforo, false si expiro el timeout.
***************************************************************************************************/
bool os_semaphore_take(os_semaphore* semaphore, uint32_t ticks_to_wait)   {

    os_task* current_task = os_get_current_task();

    if (current_task->state == OS_TASK_RUNNING) {
        if (ticks_to_wait != NO_TIMEOUT)    {
            current_task->remaining_blocked_ticks = ticks_to_wait;
        }

        while (1)    {

            if (semaphore->taken == true)   {
                semaphore->associated_task = current_task;

                // the timeout expired, could not take the semaphore but must return anyway
                if (current_task->remaining_blocked_ticks == 0 && ticks_to_wait != NO_TIMEOUT) {
                    return false;   // this also breaks out of the while(1)
                }
                else    {
                    current_task->state = OS_TASK_BLOCKED;
                    os_cpu_yield();
                }

            }
            else    {
                semaphore->taken = true;
                current_task->remaining_blocked_ticks = 0;
                return true;    // this also breaks out of the while (1)
            }

        } // while (1)
    }
    else    {
        return false;
    }
}



/*************************************************************************************************
     *  @brief Se libera un semaforo.
     *
***************************************************************************************************/
void os_semaphore_give(os_semaphore* semaphore) {

    os_task* current_task = os_get_current_task();

    if (current_task->state == OS_TASK_RUNNING  &&
        semaphore->taken == true                &&
        semaphore->associated_task != NULL)
    {
        semaphore->taken = false;
        semaphore->associated_task->state = OS_TASK_READY;
        semaphore->associated_task->remaining_blocked_ticks = 0;

        // if called from an ISR, a new scheduling is required
        if (os_get_global_state() == OS_STATE_ISR)    {
            os_set_scheduler_from_isr(true);
        }
    }
}


/*************************************************************************************************
     *  @brief Inicializa una cola.
     * 
     * Retorna true si se pudo crear la cola, false si el tamaño del
     * elemento es mayor al maximo que puede almacenar la cola.
***************************************************************************************************/
bool os_queue_init(os_queue* queue, uint16_t element_size) {
    if (element_size > MAX_QUEUE_SIZE_BYTES)    {
        return false;
    }

    queue->element_size     = element_size;
    queue->associated_task  = NULL;
    queue->front            = 0;
    queue->back             = 0;
    queue->current_elements = 0;

    return true;
}


/*************************************************************************************************
     *  @brief Coloca un dato en una cola.
     *
***************************************************************************************************/
bool os_queue_send(os_queue* queue, void* data) {

    uint16_t total_elements = MAX_QUEUE_SIZE_BYTES / queue->element_size;
    os_task* current_task   = os_get_current_task();

    if (current_task->state == OS_TASK_RUNNING) {

        // the operation must be canceled if trying to send data
        // to a full queue from an ISR (cannot block inside an ISR)
        if (os_get_global_state() == OS_STATE_ISR && queue->current_elements == total_elements)  {
            return false;
        }

        // block until the queue has space
        while (queue->current_elements == total_elements) {
            os_enter_critical_section();
            current_task = os_get_current_task();
            current_task->state     = OS_TASK_BLOCKED;
            queue->associated_task  = current_task;
            os_exit_critical_section();
            // force scheduling
            os_cpu_yield();
        }

        // if there was a task blocked waiting to receive an element from an empty queue,
        // it must go to the READY state if the queue is not empty anymore
        if (queue->current_elements == 0 && queue->associated_task != NULL) {
            if (queue->associated_task->state == OS_TASK_BLOCKED)   {
                queue->associated_task->state = OS_TASK_READY;

                // if called from an ISR, a new scheduling is required
                if (os_get_global_state() == OS_STATE_ISR)    {
                    os_set_scheduler_from_isr(true);
                }
            }
        }

        // if the queue has enough space, copy the data to the
        // corresponding block of memory inside the queue data
        memcpy(queue->data + queue->front *  queue->element_size, data, queue->element_size);
        queue->front = (queue->front + 1) % total_elements;
        queue->associated_task = NULL;
        queue->current_elements++;
    }

    return true;
}


/*************************************************************************************************
     *  @brief Lee un dato de una cola.
     *
***************************************************************************************************/
bool os_queue_receive(os_queue* queue, void* data)  {

    uint16_t total_elements = MAX_QUEUE_SIZE_BYTES / queue->element_size;
    os_task* current_task   = os_get_current_task();

    if (current_task->state == OS_TASK_RUNNING) {

        // the operation must be canceled if trying to receive data
        // from an empty queue from an ISR (cannot block inside an ISR)
        if (os_get_global_state() == OS_STATE_ISR && queue->current_elements == 0)  {
            return false;
        }

        // block until the queue is not empty
        while (queue->current_elements == 0)    {
            os_enter_critical_section();
            current_task = os_get_current_task();
            current_task->state     = OS_TASK_BLOCKED;
            queue->associated_task  = current_task;
            os_exit_critical_section();
            // force scheduling
            os_cpu_yield();
        }

        // if there was a task blocked waiting to send an element to a full queue,
        // it must go to the READY state if the queue has space now
        if (queue->current_elements == total_elements && queue->associated_task != NULL) {
            if (queue->associated_task->state == OS_TASK_BLOCKED)   {
                queue->associated_task->state = OS_TASK_READY;

                // if called from an ISR, a new scheduling is required
                if (os_get_global_state() == OS_STATE_ISR)    {
                    os_set_scheduler_from_isr(true);
                }
            }
        }

        memcpy(data, queue->data + queue->back *  queue->element_size, queue->element_size);
        queue->back = (queue->back + 1) % total_elements;
        queue->associated_task = NULL;
        queue->current_elements--;
    }

    return true;
}
