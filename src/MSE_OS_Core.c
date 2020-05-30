/*
 * MSE_OS_Core.c
 *
 *  Created on: 2020
 *      Author: mbrignone
 */

#include "MSE_OS_Core.h"

#define IDLE_TASK_ID    0xFF

static os_task idle_task_instance;


/*************************************************************************************************
     *  @brief Inicializa idle task.
     *
***************************************************************************************************/
void os_init_idle_task();


/*************************************************************************************************
     *  @brief Idle task.
     *
***************************************************************************************************/
void __attribute__((weak)) idle_task(void* task_param)  {
    while(1)    {
        __WFI();
    }
}



/*************************************************************************************************
     *  @brief Hook de retorno de tareas
     *
***************************************************************************************************/
void __attribute__((weak)) os_return_hook(void)  {
    while(1);
}



/*************************************************************************************************
     *  @brief Hook de tick de sistema
     *
***************************************************************************************************/
void __attribute__((weak)) os_tick_hook(void)  {
    __asm volatile( "nop" );
}



/*************************************************************************************************
     *  @brief Hook de error de sistema
     *
***************************************************************************************************/
void __attribute__((weak)) error_hook(void *caller)  {
    // while(1);
}



/*==================[definicion de variables globales]=================================*/

static os_control os_controller;

/*************************************************************************************************
     *  @brief Inicializa las tareas del OS.
     *
***************************************************************************************************/
os_error os_init_task(void* entry_point, os_task* task, void* task_param)	{

    static uint8_t id = 0;

    if (os_controller.number_of_tasks < OS_MAX_TASK)	{

        task->stack[STACK_SIZE/4 - XPSR]    = INIT_XPSR;                    // required for bit thumb
        task->stack[STACK_SIZE/4 - PC_REG]  = (uint32_t)entry_point;        // pointer to the task (entry point)
        task->stack[STACK_SIZE/4 - LR]      = (uint32_t)os_return_hook;     // task return (should never happen)

        task->stack[STACK_SIZE/4 - R0]      = (uint32_t)task_param;         // task parameter

        task->stack[STACK_SIZE/4 - LR_PREV_VALUE] = EXEC_RETURN;
        task->stack_pointer = (uint32_t) (task->stack + STACK_SIZE/4 - FULL_STACKING_SIZE);

        task->entry_point = entry_point;
        task->id = id;
        task->state = OS_TASK_READY;

        os_controller.task_list[id] = task;
        os_controller.number_of_tasks++;

        id++;

        return OS_OK;
    }

    else    {
        os_controller.last_error = OS_ERROR_MAX_TASK;   // reached the maximum number of tasks
        error_hook(os_init_task);
        return OS_ERROR_MAX_TASK;
    }

}


/*************************************************************************************************
     *  @brief Inicializa el OS.
     *
***************************************************************************************************/
void os_init(void)  {

    NVIC_SetPriority(PendSV_IRQn, (1 << __NVIC_PRIO_BITS)-1);

    // idle task must be automatically initialized
    os_init_idle_task();

    os_controller.state = OS_STATE_RESET;
    os_controller.current_task = NULL;
    os_controller.next_task = NULL;

    for (uint8_t i=0; i<OS_MAX_TASK; i++)	{
        if (i >= os_controller.number_of_tasks)	{
            os_controller.task_list[i] = NULL;
        }
    }
}


/*************************************************************************************************
     *  @brief Funcion del sistema operativo para obtener el último
     *  error ocurrido.
     *
***************************************************************************************************/
os_error os_get_last_error(void)    {
    return os_controller.last_error;
}



/*************************************************************************************************
     *  @brief Funcion que efectua las decisiones de scheduling.
     *
***************************************************************************************************/
static void scheduler(void)  {
    uint8_t index;
    uint8_t iterated_tasks = 0;

    if (os_controller.state == OS_STATE_RESET)	{
        // consider the possibility of having no tasks
        if (os_controller.number_of_tasks > 0)  {
            os_controller.current_task = (os_task*) os_controller.task_list[0];
        }
        else    {
            os_controller.current_task = &idle_task_instance;
        }
    }
    else	{
        index = os_controller.current_task->id;

        while (iterated_tasks < os_controller.number_of_tasks)  {
            // iterate over all the valid task IDs until a READY/RUNNING task is found
            index = (index + 1) % os_controller.number_of_tasks;

            if(((os_task*)(os_controller.task_list[index]))->state != OS_TASK_BLOCKED)  {
                os_controller.next_task = (os_task*) os_controller.task_list[index];
                break;
            }

            iterated_tasks++;
        }

        // all tasks are blocked, so the idle task must be run
        if (iterated_tasks == os_controller.number_of_tasks)    {
            os_controller.next_task = &idle_task_instance;
        }


    }
}



/*************************************************************************************************
     *  @brief SysTick Handler.
     *
***************************************************************************************************/
void SysTick_Handler(void)  {

    scheduler();

    // set the corresponding bit for PendSV exception
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;

    // Instruction Synchronization Barrier: flushes the pipeline and ensures that
    // all previous instructions are completed before executing new instructions
    __ISB();

    // Data Synchronization Barrier: ensures that all memory accesses are
    // completed before next instruction is executed
    __DSB();

    os_tick_hook();
}




/*************************************************************************************************
     *  @brief Funcion para determinar el proximo contexto.
     *
***************************************************************************************************/
uint32_t get_next_context(uint32_t current_stack_pointer)  {
    uint32_t next_stack_pointer;

    if (os_controller.state == OS_STATE_RESET)	{
        next_stack_pointer = os_controller.current_task->stack_pointer;
        os_controller.current_task->state = OS_TASK_RUNNING;
        os_controller.state = OS_STATE_NORMAL;
    }
    else {
        os_controller.current_task->stack_pointer = current_stack_pointer;
    
        // only go to READY if the task was RUNNING
        // if it was BLOCKED, it should stay BLOCKED
        if (os_controller.current_task->state == OS_TASK_RUNNING)   {
            os_controller.current_task->state = OS_TASK_READY;
        }

        next_stack_pointer = os_controller.next_task->stack_pointer;

        os_controller.current_task = os_controller.next_task;
        os_controller.current_task->state = OS_TASK_RUNNING;
    }

    return next_stack_pointer;
}


/*************************************************************************************************
     *  @brief Inicializa Idle Task.
     *
***************************************************************************************************/
void os_init_idle_task()    {

    idle_task_instance.stack[STACK_SIZE/4 - XPSR]    = INIT_XPSR;                    // required for bit thumb
    idle_task_instance.stack[STACK_SIZE/4 - PC_REG]  = (uint32_t)idle_task;          // pointer to the task (entry point)
    idle_task_instance.stack[STACK_SIZE/4 - LR]      = (uint32_t)os_return_hook;     // task return (should never happen)

    idle_task_instance.stack[STACK_SIZE/4 - R0]      = (uint32_t)NULL;               // task parameter

    idle_task_instance.stack[STACK_SIZE/4 - LR_PREV_VALUE] = EXEC_RETURN;
    idle_task_instance.stack_pointer = (uint32_t)(idle_task_instance.stack + STACK_SIZE/4 - FULL_STACKING_SIZE);

    idle_task_instance.entry_point = idle_task;
    idle_task_instance.id = IDLE_TASK_ID;
    idle_task_instance.state = OS_TASK_READY;
}
