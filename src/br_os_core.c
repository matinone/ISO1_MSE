/*
 * br_os_core.c
 *
 *  Created on: 2020
 *      Author: mbrignone
 */

#include "br_os_core.h"

#define IDLE_TASK_ID    0xFF


// partial initialization so all the rest of the fields are set to 0
static os_control os_controller = {.number_of_tasks = 0};

static os_task idle_task_instance;

/*************************************************************************************************
     *  @brief Inicializa idle task.
     *
***************************************************************************************************/
static void os_init_idle_task();


/*************************************************************************************************
     *  @brief Ordena las tareas de acuerdo a sus prioridades.
     *
***************************************************************************************************/
static void os_order_task_priority();


/*************************************************************************************************
     *  @brief Setea PendSV.
     *
***************************************************************************************************/
static void os_set_pendsv();


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



/*************************************************************************************************
     *  @brief Inicializa las tareas del OS.
     *
***************************************************************************************************/
os_error os_init_task(task_function entry_point, os_task* task, void* task_param, uint8_t priority) {

    static uint8_t id = 0;

    if (priority > OS_MIN_PRIORITY) {
        os_set_error(OS_ERROR_MAX_PRIORITY, os_init_task);
        return OS_ERROR_MAX_PRIORITY;
    }
    else if (os_controller.number_of_tasks >= OS_MAX_TASK)  {
        // reached the maximum number of tasks
        os_set_error(OS_ERROR_MAX_TASK, os_init_task);
        return OS_ERROR_MAX_TASK;
    }
    else    {

        task->stack[STACK_SIZE/4 - XPSR]    = INIT_XPSR;                    // required for bit thumb
        task->stack[STACK_SIZE/4 - PC_REG]  = (uint32_t)entry_point;        // pointer to the task (entry point)
        task->stack[STACK_SIZE/4 - LR]      = (uint32_t)os_return_hook;     // task return (should never happen)

        task->stack[STACK_SIZE/4 - R0]      = (uint32_t)task_param;         // task parameter

        task->stack[STACK_SIZE/4 - LR_PREV_VALUE] = EXEC_RETURN;
        task->stack_pointer = (uint32_t) (task->stack + STACK_SIZE/4 - FULL_STACKING_SIZE);

        task->entry_point = entry_point;
        task->id = id;
        task->state = OS_TASK_READY;
        task->priority = priority;

        os_controller.task_list[id] = task;
        os_controller.number_of_tasks++;
        os_controller.tasks_per_priority[priority]++;

        id++;

        return OS_OK;
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

    os_controller.state                     = OS_STATE_RESET;
    os_controller.current_task              = NULL;
    os_controller.next_task                 = NULL;
    os_controller.current_critical_sections = 0;
    os_controller.schedule_from_isr         = false;
    os_controller.system_time               = 0;

    for (uint8_t i=0; i<OS_MAX_TASK; i++)	{
        if (i >= os_controller.number_of_tasks)	{
            os_controller.task_list[i] = NULL;
        }
    }

    // order all the tasks inside the OS controller based on their priorities
    os_order_task_priority();
}


/*************************************************************************************************
     *  @brief Setea un error del OS y llama al error hook.
     *
***************************************************************************************************/
void os_set_error(os_error error, void* caller)   {
    os_controller.last_error = error;
    error_hook(caller);
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
     *  @brief Funcion del sistema operativo para obtener la tarea
     *  actualmente en ejecucion.
     *
***************************************************************************************************/
os_task* os_get_current_task(void)  {
    return os_controller.current_task;
}


/*************************************************************************************************
     *  @brief Devuelve el tiempo transcurrido (en ticks) desde que se inicio el OS.
     *
***************************************************************************************************/
uint32_t os_get_current_time(void)  {
    return os_controller.system_time;
}

/*************************************************************************************************
     *  @brief Devuelve el estado actual del OS.
     *
***************************************************************************************************/
os_state os_get_global_state(void)  {
    return os_controller.state;
}


/*************************************************************************************************
     *  @brief Setea el estado del OS.
     *
**************************************************************************************************/
void os_set_global_state(os_state state)    {
    os_controller.state = state;
}


/*************************************************************************************************
     *  @brief Setea la bandera del OS para indicar que se debe llamar al scheduler
     *  luego de haber atendido una interrupcion.
     *
**************************************************************************************************/
void os_set_scheduler_from_isr(bool value)  {
    os_controller.schedule_from_isr = value;
}


/*************************************************************************************************
     *  @brief Devuelve la bandera del OS que indica si se debe llamar al scheduler
     *  luego de haber atendido una interrupcion.
     *
**************************************************************************************************/
bool os_get_scheduler_from_isr(void)    {
    return os_controller.schedule_from_isr;
}


/*************************************************************************************************
     *  @brief Funcion que efectua las decisiones de scheduling.
     *
***************************************************************************************************/
static void scheduler(void)  {
    static uint8_t index_per_priority[OS_N_PRIORITY]; // must be static for Round-robin between tasks of the same priority

    uint8_t index_offset = 0;
    uint8_t real_index = 0;
    uint8_t total_iterated_tasks = 0;
    uint8_t priority_iterated_tasks = 0;
    uint8_t current_priority = OS_MAX_PRIORITY;

    if (os_controller.state == OS_STATE_RESET)  {
        // consider the possibility of having no tasks
        if (os_controller.number_of_tasks > 0)  {
            // task_list is ordered, so the first task will have the higher priority
            os_controller.current_task = os_controller.task_list[0];
        }
        else    {
            os_controller.current_task = &idle_task_instance;
        }
        // the index array must be set to zero
        for (uint8_t i=0; i<OS_N_PRIORITY; i++) {
            index_per_priority[i] = 0;
        }
    }
    else    {
        // iterate over all tasks until a READY/RUNNING task is found
        while (total_iterated_tasks < os_controller.number_of_tasks)  {

            priority_iterated_tasks = 0;
            // iterate over all the tasks of the current priority
            while (priority_iterated_tasks < os_controller.tasks_per_priority[current_priority])    {
                real_index = index_per_priority[current_priority] + index_offset;

                if(((os_controller.task_list[real_index]))->state != OS_TASK_BLOCKED)  {
                    // select next task and update the index, so the same task is not chosen again the next time
                    os_controller.next_task = os_controller.task_list[real_index];
                    index_per_priority[current_priority] = (index_per_priority[current_priority] + 1) % os_controller.tasks_per_priority[current_priority];
                    break;  // break inner while
                }

                // go to the next task of the current priority
                index_per_priority[current_priority] = (index_per_priority[current_priority] + 1) % os_controller.tasks_per_priority[current_priority];
                priority_iterated_tasks++;
            }

            // all the tasks of the current priority are blocked, so go to the next priority
            if (priority_iterated_tasks == os_controller.tasks_per_priority[current_priority])  {

                index_offset = index_offset + os_controller.tasks_per_priority[current_priority];
                total_iterated_tasks = total_iterated_tasks + priority_iterated_tasks;
                current_priority++;
            }
            // the next READY task was found, so break out of the outer while
            else    {
                break;  // break outer while
            }
        }

        // all tasks are blocked, so the idle task must be run
        if (total_iterated_tasks == os_controller.number_of_tasks)    {
            os_controller.next_task = &idle_task_instance;
        }


    }

    // set PendSV exception to do the context switch after scheduling
    os_set_pendsv();
}



/*************************************************************************************************
     *  @brief SysTick Handler.
     *
***************************************************************************************************/
void SysTick_Handler(void)  {

    // update system time
    os_controller.system_time++;

    // update the remaining blocked ticks for all the corresponding tasks
    for (uint8_t i; i<os_controller.number_of_tasks; i++)   {
        if (os_controller.task_list[i]->remaining_blocked_ticks > 0)    {
            os_controller.task_list[i]->remaining_blocked_ticks--;

            // update the task state if the remaining ticks are 0
            // it must be done here to avoid updating the state if it was
            // blocked for another reason
            if (os_controller.task_list[i]->state == OS_TASK_BLOCKED && os_controller.task_list[i]->remaining_blocked_ticks == 0)    {

                os_controller.task_list[i]->state = OS_TASK_READY;
            }
        }
    }

    scheduler();

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
static void os_init_idle_task()    {

    idle_task_instance.stack[STACK_SIZE/4 - XPSR]    = INIT_XPSR;                    // required for bit thumb
    idle_task_instance.stack[STACK_SIZE/4 - PC_REG]  = (uint32_t)idle_task;          // pointer to the task (entry point)
    idle_task_instance.stack[STACK_SIZE/4 - LR]      = (uint32_t)os_return_hook;     // task return (should never happen)

    idle_task_instance.stack[STACK_SIZE/4 - R0]      = (uint32_t)NULL;               // task parameter

    idle_task_instance.stack[STACK_SIZE/4 - LR_PREV_VALUE] = EXEC_RETURN;
    idle_task_instance.stack_pointer = (uint32_t)(idle_task_instance.stack + STACK_SIZE/4 - FULL_STACKING_SIZE);

    idle_task_instance.entry_point = idle_task;
    idle_task_instance.id = IDLE_TASK_ID;
    idle_task_instance.state = OS_TASK_READY;
    idle_task_instance.priority = OS_IDLE_PRIORITY;
}

/*************************************************************************************************
     *  @brief Ordena las tareas de acuerdo a sus prioridades.
     *
***************************************************************************************************/
static void os_order_task_priority()    {
    // really inefficient bubble sort ( O(N^2) )
    os_task* swap_variable;

    for (uint8_t i=0; i < os_controller.number_of_tasks - 1; i++)  {
        for (uint8_t j=0; j < os_controller.number_of_tasks - i - 1; j++)   {

            if (((os_controller.task_list[j]))->priority > ((os_controller.task_list[j+1]))->priority)  {
                swap_variable = os_controller.task_list[j];
                os_controller.task_list[j] = os_controller.task_list[j+1];
                os_controller.task_list[j+1] = swap_variable;
            }
        }
    }
}

/*************************************************************************************************
     *  @brief Se fuerza a una ejecución del scheduler.
     *
***************************************************************************************************/
void os_cpu_yield(void) {
    // just call the scheduler
    scheduler();
}


static void os_set_pendsv() {
    // set the corresponding bit for PendSV exception
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;

    // Instruction Synchronization Barrier: flushes the pipeline and ensures that
    // all previous instructions are completed before executing new instructions
    __ISB();

    // Data Synchronization Barrier: ensures that all memory accesses are
    // completed before next instruction is executed
    __DSB();
}


/*************************************************************************************************
     *  @brief Inidica el inicio de una seccion critica, en la que las interrupciones no
     *  estan habilitadas, para garantizar que las operaciones sean atomicas.
     *
***************************************************************************************************/
inline void os_enter_critical_section(void)    {
    __disable_irq();
    os_controller.current_critical_sections++;
}


/*************************************************************************************************
     *  @brief Inidica el fin de una seccion critica.
     *
***************************************************************************************************/
inline void os_exit_critical_section(void)  {
    os_controller.current_critical_sections--;
    if (os_controller.current_critical_sections <= 0)   {
        __enable_irq();
    }
}
