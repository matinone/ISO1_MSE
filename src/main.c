/*==================[inclusions]=============================================*/

#include "main.h"

#include "board.h"

#include "MSE_OS_Core.h"


/*==================[macros and definitions]=================================*/

#define MILISEC		1000

/*==================[Global data declaration]==============================*/

uint32_t stack1[STACK_SIZE];		//espacio reservado para el stack de la tarea 1
uint32_t stack2[STACK_SIZE];		//espacio reservado para el stack de la tarea 2

uint32_t sp_tarea1;					//Stack Pointer para la tarea 1
uint32_t sp_tarea2;					//Stack Pointer para la tarea 2


/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/** @brief hardware initialization function
 *	@return none
 */
static void initHardware(void)  {
	Board_Init();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / MILISEC);		//systick 1ms
}


/*==================[Definicion de tareas para el OS]==========================*/
void tarea1(void)  {
	int i;
	while (1) {
		i++;
	}
}

void tarea2(void)  {
	int j;
	while (1) {
		j++;
	}
}

/*============================================================================*/

int main(void)  {

	initHardware();

	os_InitTarea(tarea1, &stack1, &sp_tarea1);
	os_InitTarea(tarea2, &stack2, &sp_tarea2);

	while (1) {
		__WFI();
	}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/
