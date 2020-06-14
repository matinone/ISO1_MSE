/*
 * br_os_core.h
 *
 *  Created on: 2020
 *      Author: mbrignone
 */

#ifndef __BR_OS_CORE_H__
#define __BR_OS_CORE_H__

#include <stdint.h>
#include <string.h>
#include "board.h"


#define STACK_SIZE 256  // predefined task stack size (in bytes)

//----------------------------------------------------------------------------------

// positions inside the stack frame for each of the stack frame registers
#define XPSR            1
#define PC_REG          2
#define LR              3
#define R12             4
#define R3              5
#define R2              6
#define R1              7
#define R0              8
#define LR_PREV_VALUE   9
#define R4              10
#define R5              11
#define R6              12
#define R7              13
#define R8              14
#define R9              15
#define R10             16
#define R11             17

//----------------------------------------------------------------------------------

// initial values for stack frame registers
#define INIT_XPSR   1 << 24             // xPSR.T = 1
#define EXEC_RETURN	0xFFFFFFF9          // return to thread mode with MSP, FPU unused

//----------------------------------------------------------------------------------

#define STACK_FRAME_SIZE            8
#define FULL_STACKING_SIZE          17	// 16 core registers + LR previous value

#define OS_MAX_TASK                 8

#define OS_MAX_PRIORITY             0   // maximum priority for a task
#define OS_MIN_PRIORITY             3   // minimum priority for a task
#define OS_N_PRIORITY               (OS_MIN_PRIORITY - OS_MAX_PRIORITY + 1)
#define OS_IDLE_PRIORITY            (OS_MIN_PRIORITY + 1)     // idle task priority lower than the lowest priority

// #define MAX_QUEUE_SIZE_BYTES        64
#define MAX_QUEUE_SIZE_BYTES        16   // 4 uint32_t values

//----------------------------------------------------------------------------------

// function pointer with the prototype of a task function
typedef void (* task_function) (void *);

typedef enum    {
    OS_TASK_READY,
    OS_TASK_RUNNING,
    OS_TASK_BLOCKED,
} os_task_state;

typedef enum    {
    OS_STATE_NORMAL,
    OS_STATE_RESET,
} os_state;

typedef enum    {
    OS_OK                   = 0x00,
    OS_ERROR_MAX_TASK       = 0x01,
    OS_ERROR_MAX_PRIORITY   = 0x02,
    OS_ERROR_TIMEOUT        = 0x03,
} os_error;

typedef struct  {
    uint32_t        stack[STACK_SIZE/4];
    uint32_t        stack_pointer;
    task_function   entry_point;
    uint8_t         id;
    os_task_state   state;
    uint8_t         priority;
    uint32_t        remaining_blocked_ticks;
} os_task;

typedef struct  {
    os_task*    task_list[OS_MAX_TASK];
    uint8_t     number_of_tasks;
    uint8_t     tasks_per_priority[OS_N_PRIORITY];
    os_error    last_error;
    os_state    state;
    os_task*    current_task;
    os_task*    next_task;
} os_control;


//----------------------------------------------------------------------------------

os_error os_init_task(task_function entry_point, os_task* task, void* task_param, uint8_t priority);

void os_init(void);

os_error os_get_last_error(void);

os_task* os_get_current_task(void);

void os_cpu_yield(void);


#endif  // __BR_OS_CORE_H__
