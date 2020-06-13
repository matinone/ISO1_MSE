/*==================[inclusions]=============================================*/

#include "main.h"
#include "board.h"
#include "sapi.h"

#include "br_os_core.h"
#include "br_os_api.h"


/*==================[macros and definitions]=================================*/

#define MILISEC     1000

/*==================[global data declaration]==============================*/

os_task instance_os_task_1, instance_os_task_2, instance_os_task_3, instance_os_task_4;
os_semaphore sem_task_1, sem_task_2, sem_task_3;


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
        os_semaphore_take(&sem_task_1, NO_TIMEOUT);
        gpioWrite(LED1, true);
        os_delay(task_delay);
        gpioWrite(LED1, false);
        // os_delay(task_delay);
    }
}

void task_2(void* task_param)  {

    uint32_t task_delay = (uint32_t)task_param;

    while (1) {
        os_semaphore_take(&sem_task_2, NO_TIMEOUT);
        gpioWrite(LED2, true);
        os_delay(task_delay);
        gpioWrite(LED2, false);
        // os_delay(task_delay);
    }
}

void task_3(void* task_param)  {

    uint32_t task_delay = (uint32_t)task_param;
    uint32_t real_delay;
    bool sem_taken;
    while (1) {
        sem_taken = os_semaphore_take(&sem_task_3, 2000);
        if (sem_taken == true) {
            real_delay = task_delay;
        }
        else {
            real_delay = 100;
        }
        gpioWrite(LED3, true);
        os_delay(real_delay);
        gpioWrite(LED3, false);
        // os_delay(task_delay);
    }
}



void task_4(void* task_param)  {

    // uint32_t task_delay = (uint32_t)task_param;

    while (1) {
        if (!gpioRead(TEC1))    {
            os_semaphore_give(&sem_task_1);
        }

        if (!gpioRead(TEC2))    {
            os_semaphore_give(&sem_task_2);
        }

        if (!gpioRead(TEC3))    {
            os_semaphore_give(&sem_task_3);
        }
    }
}

/*============================================================================*/

int main(void)  {

    initHardware();

    os_init_task(task_1, &instance_os_task_1, (void*)1000, 0);
    os_init_task(task_2, &instance_os_task_2, (void*)500,  0);
    os_init_task(task_3, &instance_os_task_3, (void*)250,  0);
    os_init_task(task_4, &instance_os_task_4, NULL,        0);

    os_semaphore_init(&sem_task_1);
    os_semaphore_init(&sem_task_2);
    os_semaphore_init(&sem_task_3);

    os_init();

    while (1) {
        __WFI();
    }
}


/*==================[end of file]============================================*/
