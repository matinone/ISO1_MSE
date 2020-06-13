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
void os_delay(uint32_t ticks)   {

    os_task* current_task = os_get_current_task();

    if (current_task->state == OS_TASK_RUNNING && ticks > 0)    {
        current_task->state = OS_TASK_BLOCKED;
        current_task->remaining_blocked_ticks = ticks;
        // force scheduling to go out of the delayed task
        os_cpu_yield();
    }

}



/*************************************************************************************************
	 *  @brief Inicializa un semaforo biario
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
    }
}
