/*
 * br_os_api.h
 *
 *  Created on: 2020
 *      Author: mbrignone
 */

#ifndef __BR_OS_API_H__
#define __BR_OS_API_H__

#include "br_os_core.h"

#define NO_TIMEOUT  0   // used to indicate that the semaphore does not have a timeout

typedef struct  {
    os_task*    associated_task;
    bool        taken;
} os_semaphore;


void os_delay(uint32_t ticks);

void os_semaphore_init(os_semaphore* semaphore);
// returns true if the semaphore was obtained, false if the timeout expired
bool os_semaphore_take(os_semaphore* semaphore, uint32_t ticks_to_wait);
void os_semaphore_give(os_semaphore* semaphore);



#endif  // __BR_OS_API_H__
