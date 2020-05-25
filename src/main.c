/*==================[inclusions]=============================================*/

#include "main.h"

#include "board.h"

#include "MSE_OS_Core.h"


/*==================[macros and definitions]=================================*/

#define MILISEC		1000

/*==================[Global data declaration]==============================*/

os_task instance_os_task_1, instance_os_task_2, instance_os_task_3;


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
void task_1(void* task_param)  {
	uint32_t i = 0;
	uint32_t j = 0;

	uint32_t increment = (uint32_t)task_param;
	while (1) {
		i = i + increment;
		j++;
	}
}

void task_2(void* task_param)  {
	uint32_t k = 0;
	uint32_t l = 0;
	while (1) {
		k++;
		l++;
	}
}

void task_3(void* task_param)  {
	uint32_t m = 0;
	uint32_t n = 0;
	float f = 0.0;
	while (1) {
		m++;
		n++;
		f = f + 1.5;
	}
}

/*============================================================================*/

int main(void)  {

	initHardware();

	os_init_task(task_1, &instance_os_task_1, (void*)5);
	os_init_task(task_2, &instance_os_task_2, NULL);
	os_init_task(task_3, &instance_os_task_3, NULL);

	os_init();

	while (1) {
		__WFI();
	}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/
