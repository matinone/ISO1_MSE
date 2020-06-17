/*==================[inclusions]=============================================*/

#include "board.h"
#include "sapi.h"

#include "br_os_core.h"
#include "br_os_api.h"
#include "br_os_isr.h"


/*==================[macros and definitions]=================================*/

#define MILISEC     1000

#define TEC1_PORT_NUM   0
#define TEC1_BIT_VAL    4


/*==================[global data declaration]==============================*/

os_task instance_os_task_1, instance_os_task_2, instance_os_task_3, instance_os_task_4;
os_semaphore sem_task_1, sem_task_2, sem_task_3;
os_queue queue_task_4;

os_task on_led_task, off_led_task, uart_task;
os_semaphore sem_tec_falling, sem_tec_rising;
os_queue uart_queue;


/*==================[internal functions declaration]=========================*/
void tec1_down_isr(void);
void tec1_up_isr(void);


/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/


static void initHardware(void)  {
    Board_Init();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / MILISEC);      // systick 1ms

    // configure int 0 for TEC1 falling edge
    Chip_SCU_GPIOIntPinSel( 0, TEC1_PORT_NUM, TEC1_BIT_VAL );
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 0 ) );
    Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT, PININTCH( 0 ) );
    Chip_PININT_EnableIntLow( LPC_GPIO_PIN_INT, PININTCH( 0 ) );

    // configure int 1 for TEC1 rising edge
    Chip_SCU_GPIOIntPinSel(1, TEC1_PORT_NUM, TEC1_BIT_VAL );
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 1 ) );
    Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT, PININTCH( 1 ) );
    Chip_PININT_EnableIntHigh( LPC_GPIO_PIN_INT, PININTCH( 1 ) );

    // config UART with baud rate 115200
    uartConfig( UART_USB, 115200 );
}


/*============================================================================*/

void task_1(void* task_param)  {

    // uint32_t task_delay = (uint32_t)task_param;
    uint32_t data = 500;

    while (1) {
        //os_semaphore_take(&sem_task_1, NO_TIMEOUT);
        os_queue_receive(&queue_task_4, &data);
        gpioWrite(LED1, true);
        os_delay(data);
        gpioWrite(LED1, false);
        os_delay(data);
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
    uint32_t num = 500;


    while (1) {

        if (!gpioRead(TEC2))    {
            os_queue_send(&queue_task_4, &num);
            // os_semaphore_give(&sem_task_1);
            os_delay(250);

            num = (num + 500) % 3000;
        }

        if (!gpioRead(TEC3))    {
            os_semaphore_give(&sem_task_2);
        }

        if (!gpioRead(TEC4))    {
            os_semaphore_give(&sem_task_3);
        }

        os_delay(100);
    }
}

void turn_led(void* task_param) {
    char msg[25];
    uint8_t char_index;
    bool led_action = (bool)task_param;
    os_semaphore* sem_ptr;

    if (led_action == true) {
        strcpy(msg, "TEC1 pressed\n\r");
        sem_ptr = &sem_tec_falling;
    }
    else    {
        strcpy(msg, "TEC1 released\n\r");
        sem_ptr = &sem_tec_rising;
    }

    while(1)    {
        os_semaphore_take(sem_ptr, NO_TIMEOUT);
        gpioWrite(LEDR, led_action);

        char_index = 0;
        while (msg[char_index] != NULL && char_index < 25) {
            os_queue_send(&uart_queue, (msg + char_index));
            char_index++;
        }
    }
}


void send_uart(void* task_param)    {
    char aux;

    while(1)    {
        os_queue_receive(&uart_queue, &aux);
        uartWriteByte(UART_USB, aux);
    }
}

/*============================================================================*/

int main(void)  {

    initHardware();

    os_init_task(task_1, &instance_os_task_1, (void*)1000, 1);
    os_init_task(task_2, &instance_os_task_2, (void*)500,  1);
    os_init_task(task_3, &instance_os_task_3, (void*)250,  1);
    os_init_task(task_4, &instance_os_task_4, NULL,        1);

    os_semaphore_init(&sem_task_1);
    os_semaphore_init(&sem_task_2);
    os_semaphore_init(&sem_task_3);

    os_queue_init(&queue_task_4, sizeof(uint32_t));

    os_init_task(turn_led, &on_led_task, (void*)true, 0);
    os_init_task(turn_led, &off_led_task, (void*)false, 0);
    os_init_task(send_uart, &uart_task, NULL, 3);

    os_queue_init(&uart_queue, sizeof(char));
    os_semaphore_init(&sem_tec_falling);
    os_semaphore_init(&sem_tec_rising);

    os_register_isr(PIN_INT0_IRQn, tec1_down_isr);
    os_register_isr(PIN_INT1_IRQn, tec1_up_isr);

    os_init();

    while (1) {
        __WFI();
    }
}


void tec1_down_isr (void) {
    os_semaphore_give(&sem_tec_falling);
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 0 ) );
}


void tec1_up_isr(void)  {
    os_semaphore_give(&sem_tec_rising);
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 1 ) );
}


/*==================[end of file]============================================*/
