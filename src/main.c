/*==================[inclusions]=============================================*/

#include "main.h"
#include "board.h"
#include "sapi.h"

#include "br_os_core.h"
#include "br_os_api.h"


/*==================[macros and definitions]=================================*/

#define MILISEC     1000

/*==================[global data declaration]==============================*/

os_task instance_os_task_1, instance_os_task_2, instance_os_task_3;


/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/


static void initHardware(void)  {
    Board_Init();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / MILISEC);      // systick 1ms
}


/*============================================================================*/

void task_1(void* task_param)  {

    uint32_t task_delay = (uint32_t)task_param;

    while (1) {
        gpioWrite(LED1, true);
        os_delay(task_delay);
        gpioWrite(LED1, false);
        os_delay(task_delay);
    }
}

void task_2(void* task_param)  {

    uint32_t task_delay = (uint32_t)task_param;

    while (1) {
        gpioWrite(LED2, true);
        os_delay(task_delay);
        gpioWrite(LED2, false);
        os_delay(task_delay);
    }
}

void task_3(void* task_param)  {

    uint32_t task_delay = (uint32_t)task_param;

    while (1) {
        gpioWrite(LED3, true);
        os_delay(task_delay);
        gpioWrite(LED3, false);
        os_delay(task_delay);
    }
}

/*============================================================================*/

int main(void)  {

    initHardware();

    os_init_task(task_1, &instance_os_task_1, (void*)1000, 2);
    os_init_task(task_2, &instance_os_task_2, (void*)500, 1);
    os_init_task(task_3, &instance_os_task_3, (void*)250, 0);

    os_init();

    while (1) {
        __WFI();
    }
}


/*==================[end of file]============================================*/
