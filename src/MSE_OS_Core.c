/*
 * MSE_OS_Core.c
 *
 *  Created on: 2020
 *      Author: mbrignone
 */

#include "MSE_OS_Core.h"


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

		task->stack[STACK_SIZE/4 - XPSR] 	= INIT_XPSR;					//necesario para bit thumb
		task->stack[STACK_SIZE/4 - PC_REG] 	= (uint32_t)entry_point;		//direccion de la tarea (ENTRY_POINT)
		task->stack[STACK_SIZE/4 - R0] 		= (uint32_t)task_param;			//parametro de la tarea

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

	else {
		os_controller.last_error = OS_ERROR_MAX_TASK;		//excedimos la cantidad de tareas posibles
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
	 *  @param 		None.
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



/*************************************************************************************************
	 *  @brief Funcion que efectua las decisiones de scheduling.
     *
     *  @details
     *   Segun el criterio al momento de desarrollo, determina que tarea debe ejecutarse luego, y
     *   por lo tanto provee los punteros correspondientes para el cambio de contexto. Esta
     *   implementacion de scheduler es muy sencilla, del tipo Round-Robin
     *
	 *  @param 		None.
	 *  @return     None.
***************************************************************************************************/
static void scheduler(void)  {
	uint8_t index;		//variable auxiliar para legibilidad

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
	 *  @param 		None.
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
	 *  @param 		current_stack_pointer	Este valor es una copia del contenido de MSP al momento en
	 *  			que la funcion es invocada.
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
