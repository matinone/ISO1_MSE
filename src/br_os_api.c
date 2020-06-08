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

        // make sure that the task stays here even if it is unblocked for some unknown reason
        while (current_task->remaining_blocked_ticks > 0)   {
            current_task->state = OS_TASK_BLOCKED;
            os_cpu_yield();
        }
    }

}
