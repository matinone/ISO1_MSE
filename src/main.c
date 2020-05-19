/*==================[inclusions]=============================================*/

#include "main.h"

#include "board.h"

#include "MSE_OS_Core.h"


/*==================[macros and definitions]=================================*/

#define MILISEC		1000

/*==================[Global data declaration]==============================*/

uint32_t stack1[STACK_SIZE / 4];		//espacio reservado para el stack de la tarea 1
uint32_t stack2[STACK_SIZE / 4];		//espacio reservado para el stack de la tarea 2
uint32_t stack3[STACK_SIZE / 4];		//espacio reservado para el stack de la tarea 3

uint32_t sp_tarea1;					//Stack Pointer para la tarea 1
uint32_t sp_tarea2;					//Stack Pointer para la tarea 2
uint32_t sp_tarea3;					//Stack Pointer para la tarea 3


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
	int i = 0;
	while (1) {
		i++;
	}
}

void tarea2(void)  {
	int j = 0;
	while (1) {
		j++;
	}
}

void tarea3(void)  {
	int k = 0;
	while (1) {
		k++;
	}
}

/*============================================================================*/

int main(void)  {

	initHardware();

	os_Init();

	os_InitTarea(tarea1, stack1, &sp_tarea1);
	os_InitTarea(tarea2, stack2, &sp_tarea2);
	os_InitTarea(tarea3, stack3, &sp_tarea3);

	while (1) {
		__WFI();
	}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/
