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


typedef struct  {
    os_task*    associated_task;
    uint8_t     data[MAX_QUEUE_SIZE_BYTES];
    uint16_t    element_size;
    uint16_t    front;
    uint16_t    back;

} os_queue;


void os_delay(uint32_t ticks);

void os_semaphore_init(os_semaphore* semaphore);
bool os_semaphore_take(os_semaphore* semaphore, uint32_t ticks_to_wait);
void os_semaphore_give(os_semaphore* semaphore);

void os_queue_init(os_queue* queue, uint16_t element_size);
void os_queue_send(os_queue* queue, void* data);
void os_queue_receive(os_queue* queue, void* data);
// void os_queue_get_max_elements(os_queue* queue);


#endif  // __BR_OS_API_H__
