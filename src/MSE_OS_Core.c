/*
 * MSE_OS_Core.c
 *
 *  Created on: 2020
 *      Author: mbrignone
 */

#include "MSE_OS_Core.h"


/*==================[definicion de hooks debiles]=================================*/

/*
 * El usuario del OS puede redefinir los hooks dentro de su codigo.
 */

/*************************************************************************************************
     *  @brief Hook de retorno de tareas
     *
     *  @details
     *   Esta funcion no deberia accederse bajo ningun concepto, porque ninguna tarea del OS
     *   debe retornar. Si lo hace, es un comportamiento anormal y debe ser tratado.
     *
     *  @param  none
     *
     *  @return none.
***************************************************************************************************/
void __attribute__((weak)) os_return_hook(void)  {
    while(1);
}



/*************************************************************************************************
     *  @brief Hook de tick de sistema
     *
     *  @details
     *   Se ejecuta cada vez que se produce un tick de sistema. Es llamada desde el handler de
     *   SysTick.
     *
     *  @param none
     *
     *  @return none.
     *
     *  @warning    Esta funcion debe ser lo mas corta posible porque se ejecuta dentro del handler
     *              mencionado, por lo que tiene prioridad sobre el cambio de contexto y otras IRQ.
     *
     *  @warning    Esta funcion no debe bajo ninguna circunstancia utilizar APIs del OS dado
     *              que podria dar lugar a un nuevo scheduling.
***************************************************************************************************/
void __attribute__((weak)) os_tick_hook(void)  {
    __asm volatile( "nop" );
}



/*************************************************************************************************
     *  @brief Hook de error de sistema
     *
     *  @details
     *   Esta funcion es llamada en caso de error del sistema, y puede ser utilizada a fin de hacer
     *   debug. El puntero de la funcion que llama a error_hook es pasado como parametro para tener
     *   informacion de quien la esta llamando, y dentro de ella puede verse el codigo de error
     *   en la estructura de control de sistema. Si ha de implementarse por el usuario para manejo
     *   de errores, es importante tener en cuenta que la estructura de control solo esta disponible
     *   dentro de este archivo.
     *
     *  @param caller       Puntero a la funcion donde fue llamado error_hook. Implementado solo a
     *                      fines de trazabilidad de errores
     *
     *  @return none.
***************************************************************************************************/
void __attribute__((weak)) error_hook(void *caller)  {
    // while(1);
}



/*==================[definicion de variables globales]=================================*/

static os_control os_controller;

/*************************************************************************************************
     *  @brief Inicializa las tareas que correran en el OS.
     *
     *  @details
     *   Inicializa una tarea para que pueda correr en el OS implementado.
     *   Es necesario llamar a esta funcion para cada tarea antes que inicie
     *   el OS.
***************************************************************************************************/
os_error os_init_task(void* entry_point, os_task* task, void* task_param)	{

    static uint8_t id = 0;

    if (os_controller.number_of_tasks < OS_MAX_TASK)	{

        task->stack[STACK_SIZE/4 - XPSR] 	= INIT_XPSR;                    //necesario para bit thumb
        task->stack[STACK_SIZE/4 - PC_REG] 	= (uint32_t)entry_point;        //direccion de la tarea (ENTRY_POINT)
        task->stack[STACK_SIZE/4 - R0] 		= (uint32_t)task_param;         //parametro de la tarea

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
        os_controller.last_error = OS_ERROR_MAX_TASK;       //excedimos la cantidad de tareas posibles
        error_hook(os_init_task);
        return OS_ERROR_MAX_TASK;
    }

}


/*************************************************************************************************
     *  @brief Inicializa el OS.
     *
     *  @details
     *   Inicializa el OS seteando la prioridad de PendSV como la mas baja posible. Es necesario
     *   llamar esta funcion antes de que inicie el sistema. Es mandatorio llamarla luego de
     *   inicializar las tareas
     *
     *  @param      None.
     *  @return     None.
***************************************************************************************************/
void os_init(void)  {

    NVIC_SetPriority(PendSV_IRQn, (1 << __NVIC_PRIO_BITS)-1);

    os_controller.state = OS_STATE_RESET;
    os_controller.current_task = NULL;
    os_controller.next_task = NULL;

    for (uint8_t i=0; i<OS_MAX_TASK; i++)	{
        if (i >= os_controller.number_of_tasks)	{
            os_controller.task_list[i] = NULL;
        }
    }
}


os_error os_get_last_error(void)    {
    return os_controller.last_error;
}



/*************************************************************************************************
     *  @brief Funcion que efectua las decisiones de scheduling.
     *
     *  @details
     *   Segun el criterio al momento de desarrollo, determina que tarea debe ejecutarse luego, y
     *   por lo tanto provee los punteros correspondientes para el cambio de contexto. Esta
     *   implementacion de scheduler es muy sencilla, del tipo Round-Robin
     *
     *  @param      None.
     *  @return     None.
***************************************************************************************************/
static void scheduler(void)  {
    uint8_t index;      //variable auxiliar para legibilidad

    if (os_controller.state == OS_STATE_RESET)	{
        os_controller.current_task = (os_task*) os_controller.task_list[0];
    }
    else	{
        index = (os_controller.current_task->id + 1) % os_controller.number_of_tasks;
        os_controller.next_task = (os_task*) os_controller.task_list[index];
    }

}



/*************************************************************************************************
     *  @brief SysTick Handler.
     *
     *  @details
     *   El handler del Systick no debe estar a la vista del usuario. En este handler se llama al
     *   scheduler y luego de determinarse cual es la tarea siguiente a ejecutar, se setea como
     *   pendiente la excepcion PendSV.
     *
     *  @param      None.
     *  @return     None.
***************************************************************************************************/
void SysTick_Handler(void)  {

    scheduler();

    /**
     * Se setea el bit correspondiente a la excepcion PendSV
     */
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;

    /**
     * Instruction Synchronization Barrier; flushes the pipeline and ensures that
     * all previous instructions are completed before executing new instructions
     */
    __ISB();

    /**
     * Data Synchronization Barrier; ensures that all memory accesses are
     * completed before next instruction is executed
     */
    __DSB();
}




/*************************************************************************************************
     *  @brief Funcion para determinar el proximo contexto.
     *
     *  @details
     *   Esta funcion obtiene el siguiente contexto a ser cargado. El cambio de contexto se
     *   ejecuta en el handler de PendSV, dentro del cual se llama a esta funcion.
     *
     *  @param      current_stack_pointer	Este valor es una copia del contenido de MSP al momento en
     *              que la funcion es invocada.
     *  @return     El valor a cargar en MSP para apuntar al contexto de la tarea siguiente.
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
        os_controller.current_task->state = OS_TASK_READY;

        next_stack_pointer = os_controller.next_task->stack_pointer;

        os_controller.current_task = os_controller.next_task;
        os_controller.current_task->state = OS_TASK_RUNNING;
    }

    return next_stack_pointer;
}
