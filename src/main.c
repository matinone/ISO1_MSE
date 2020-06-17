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

#define TEC2_PORT_NUM   0
#define TEC2_BIT_VAL    8

#define LED_AMARILLO    LED1
#define LED_ROJO        LED2
#define LED_VERDE       LED3
#define LED_AZUL        LEDB


/*==================[internal data definition]===============================*/
typedef struct  {
    uint32_t    tecla;
    uint32_t    tiempo_flanco_asc;
    uint32_t    tiempo_flanco_desc;
} tecla_info;


/*==================[global data declaration]==============================*/
// tareas para detectar los flancos de teclas 1 y 2
os_task tec1_task, tec2_task;
// tarea para el manejo de LEDs en base a los tiempos de las teclas
os_task led_task;
// tarea para manejar el envio de datos por UART
os_task uart_task;

// semaforos para indicar desde las correspondientes rutinas de interrupcion,
// que un determinado flanco ha ocurrido
os_semaphore sem_tec1_falling, sem_tec1_rising;
os_semaphore sem_tec2_falling, sem_tec2_rising;
// semaforos para indicar que las tareas de tecla 1 y 2 pueden comenzar nuevamente
// (debido a que ya han llegado ambos flancos de ambas teclas)
os_semaphore sem_reset_tec1, sem_reset_tec2;

// colas para enviar la informacion de las teclas a la tarea que manejo los LEDs
os_queue tec1_time, tec2_time;
// cola para enviar bytes por UART
os_queue uart_queue;

tecla_info info_tec1;
tecla_info info_tec2;


/*==================[internal functions declaration]=========================*/
void tec1_down_isr(void);
void tec1_up_isr(void);
void tec2_down_isr(void);
void tec2_up_isr(void);

void init_tecla_info(tecla_info* tec_info, uint32_t tecla);

char* itoa(int i, char b[]);
void send_uart_msg(char msg[], uint8_t max_len);


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

     // configure int 2 for TEC2 falling edge
    Chip_SCU_GPIOIntPinSel(2, TEC2_PORT_NUM, TEC2_BIT_VAL );
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 2 ) );
    Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT, PININTCH( 2 ) );
    Chip_PININT_EnableIntLow( LPC_GPIO_PIN_INT, PININTCH( 2 ) );

    // configure int 3 for TEC2 rising edge
    Chip_SCU_GPIOIntPinSel(3, TEC2_PORT_NUM, TEC2_BIT_VAL );
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 3 ) );
    Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT, PININTCH( 3 ) );
    Chip_PININT_EnableIntHigh( LPC_GPIO_PIN_INT, PININTCH( 3 ) );

    // config UART with baud rate 115200
    uartConfig( UART_USB, 115200 );
}


/*=================================[TASKS]====================================*/


/*************************************************************************************************
     *  @brief Tarea para registar los tiempos en los que se presiona y libera la tecla 1 (TEC1).
     * 
     * Notar que la tarea se queda esperando por el semaforo sem_reset_tec1, el cual es liberado
     * al finalizar la tarea encargada de encender los LEDs (led_task).
     * Por lo tanto, para un correcto funcionamieto se debe esperar a que el LED se apague para 
     * volver a presionar/soltar los botones (ya que sino se produce un desfasaje entre que la
     * interrupcion libera el semaforo y la tarea registra el tiempo). Caso contrario, la tarea
     * led_task indicara que se realizo una secuencia invalida.
     *
***************************************************************************************************/
void tec1_method(void* task_param) {

    while(1)    {

        // esperar el flanco de bajada (tecla pulsada)
        os_semaphore_take(&sem_tec1_falling, NO_TIMEOUT);
        info_tec1.tiempo_flanco_desc = os_get_current_time();

        // esperar el flanco de subida (tecla pulsada)
        os_semaphore_take(&sem_tec1_rising, NO_TIMEOUT);
        info_tec1.tiempo_flanco_asc = os_get_current_time();

        // enviar a la cola la informacion de la tecla
        os_queue_send(&tec1_time, &info_tec1);

        // esperar hasta que se hayan procesado los tiempos de ambas tareas
        os_semaphore_take(&sem_reset_tec1, NO_TIMEOUT);
        // resetear tiempos de la tecla
        init_tecla_info(&info_tec1, 1);

    }
}


/*************************************************************************************************
     *  @brief Tarea para registar los tiempos en los que se presiona y libera la tecla 2 (TEC2).
     * 
     * Ver tarea tec1_method.
     *
***************************************************************************************************/
void tec2_method(void* task_param) {

    while(1)    {

        // esperar el flanco de bajada (tecla pulsada)
        os_semaphore_take(&sem_tec2_falling, NO_TIMEOUT);
        info_tec2.tiempo_flanco_desc = os_get_current_time();

        // esperar el flanco de subida (tecla pulsada)
        os_semaphore_take(&sem_tec2_rising, NO_TIMEOUT);
        info_tec2.tiempo_flanco_asc = os_get_current_time();

        // enviar a la cola la informacion de la tecla
        os_queue_send(&tec2_time, &info_tec2);

        // esperar hasta que se hayan procesado los tiempos de ambas tareas
        os_semaphore_take(&sem_reset_tec2, NO_TIMEOUT);
        // resetear tiempos de la tecla
        init_tecla_info(&info_tec2, 2);

    }
}


/*************************************************************************************************
     *  @brief Tarea encarga de encender el LED correspondiente por el tiempo correspondiente,
     * en base a los tiempos entre flancos de las teclas 1 y 2. Tambien se encarga de enviar por
     * UART los mensajes correspondientes.
     *
***************************************************************************************************/
void turn_led(void* task_param) {
    char msg[50];
    gpioMap_t led;
    uint32_t tiempo_encendido, tiempo_asc, tiempo_desc;
    bool invalid_sequence;

    tecla_info tecla_1, tecla_2;

    while(1)    {
        memset(msg, 0, sizeof(char) * 50);
        invalid_sequence = false;

        os_queue_receive(&tec1_time, &tecla_1);
        os_queue_receive(&tec2_time, &tecla_2);

        if (tecla_1.tiempo_flanco_desc < tecla_2.tiempo_flanco_desc &&
            tecla_1.tiempo_flanco_asc < tecla_2.tiempo_flanco_asc)  {
                led = LED_VERDE;
                strcpy(msg, "Led Verde encendido: \n\r");
                tiempo_desc = tecla_2.tiempo_flanco_desc - tecla_1.tiempo_flanco_desc;
                tiempo_asc = tecla_2.tiempo_flanco_asc - tecla_1.tiempo_flanco_asc;
        }
        else if (tecla_1.tiempo_flanco_desc < tecla_2.tiempo_flanco_desc &&
            tecla_1.tiempo_flanco_asc > tecla_2.tiempo_flanco_asc)  {
                led = LED_ROJO;
                strcpy(msg, "Led Rojo encendido: \n\r");
                tiempo_desc = tecla_2.tiempo_flanco_desc - tecla_1.tiempo_flanco_desc;
                tiempo_asc = tecla_1.tiempo_flanco_asc - tecla_2.tiempo_flanco_asc;
        }
        else if (tecla_1.tiempo_flanco_desc > tecla_2.tiempo_flanco_desc &&
            tecla_1.tiempo_flanco_asc < tecla_2.tiempo_flanco_asc)  {
                led = LED_AMARILLO;
                strcpy(msg, "Led Amarillo encendido: \n\r");
                tiempo_desc = tecla_1.tiempo_flanco_desc - tecla_2.tiempo_flanco_desc;
                tiempo_asc = tecla_2.tiempo_flanco_asc - tecla_1.tiempo_flanco_asc;
        }
        else if (tecla_1.tiempo_flanco_desc > tecla_2.tiempo_flanco_desc &&
            tecla_1.tiempo_flanco_asc > tecla_2.tiempo_flanco_asc)  {
                led = LED_AZUL;
                strcpy(msg, "Led Azul encendido: \n\r");
                tiempo_desc = tecla_1.tiempo_flanco_desc - tecla_2.tiempo_flanco_desc;
                tiempo_asc = tecla_1.tiempo_flanco_asc - tecla_2.tiempo_flanco_asc;
        }
        else    {
            invalid_sequence = true;
            strcpy(msg, "Secuencia invalida.\n\r");
            send_uart_msg(msg, 50);
        }

        if (!invalid_sequence)  {

            tiempo_encendido = tiempo_desc + tiempo_asc;

            send_uart_msg(msg, 50);

            // envio de tiempo encendido
            strcpy(msg, "\tTiempo encendido: ");
            send_uart_msg(msg, 50);
            itoa(tiempo_encendido, msg);
            send_uart_msg(msg, 50);
            strcpy(msg, " ms\n\r");
            send_uart_msg(msg, 50);

            // envio de tiempo entre flancos descendentes
            strcpy(msg, "\tTiempo entre flancos descendentes: ");
            send_uart_msg(msg, 50);
            itoa(tiempo_desc, msg);
            send_uart_msg(msg, 50);
            strcpy(msg, " ms\n\r");
            send_uart_msg(msg, 50);

            // envio de tiempo entre flancos ascendentes
            strcpy(msg, "\tTiempo entre flancos ascendentes: ");
            send_uart_msg(msg, 50);
            itoa(tiempo_asc, msg);
            send_uart_msg(msg, 50);
            strcpy(msg, " ms\n\r");
            send_uart_msg(msg, 50);


            gpioWrite(led, true);
            os_delay(tiempo_encendido);
            gpioWrite(led, false);
        }

        os_semaphore_give(&sem_reset_tec1);
        os_semaphore_give(&sem_reset_tec2);
    }
}


/*************************************************************************************************
     *  @brief Tarea utilizada para administrar el envio de datos por UART.
     *
***************************************************************************************************/
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

    os_init_task(tec1_method, &tec1_task, NULL, 0);
    os_init_task(tec2_method, &tec2_task, NULL, 0);
    os_init_task(turn_led, &led_task, NULL, 0);
    os_init_task(send_uart, &uart_task, NULL, 3);

    os_semaphore_init(&sem_tec1_falling);
    os_semaphore_init(&sem_tec1_rising);
    os_semaphore_init(&sem_tec2_falling);
    os_semaphore_init(&sem_tec2_rising);
    os_semaphore_init(&sem_reset_tec1);
    os_semaphore_init(&sem_reset_tec2);

    os_queue_init(&tec1_time, sizeof(tecla_info));
    os_queue_init(&tec2_time, sizeof(tecla_info));
    os_queue_init(&uart_queue, sizeof(char));

    init_tecla_info(&info_tec1, 1);
    init_tecla_info(&info_tec2, 2);

    os_register_isr(PIN_INT0_IRQn, tec1_down_isr);
    os_register_isr(PIN_INT1_IRQn, tec1_up_isr);
    os_register_isr(PIN_INT2_IRQn, tec2_down_isr);
    os_register_isr(PIN_INT3_IRQn, tec2_up_isr);

    os_init();

    while (1) {
        __WFI();
    }
}


void tec1_down_isr (void) {
    os_semaphore_give(&sem_tec1_falling);
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 0 ) );
}


void tec1_up_isr(void)  {
    os_semaphore_give(&sem_tec1_rising);
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 1 ) );
}


void tec2_down_isr (void) {
    os_semaphore_give(&sem_tec2_falling);
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 2 ) );
}


void tec2_up_isr(void)  {
    os_semaphore_give(&sem_tec2_rising);
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 3 ) );
}


void init_tecla_info(tecla_info* tec_info, uint32_t tecla)    {
    tec_info->tecla                 = tecla;
    tec_info->tiempo_flanco_asc     = 0;
    tec_info->tiempo_flanco_desc    = 0;
}


char* itoa(int i, char b[]) {

    char const digit[] = "0123456789";
    char* p = b;
    int shifter;

    if (i < 0)  {
        *p++ = '-';
        i *= -1;
    }

    shifter = i;

    do {
        ++p;
        shifter = shifter/10;
    } while(shifter);
    *p = '\0';

    do {
        *--p = digit[i%10];
        i = i/10;
    } while(i);

    return b;
}

void send_uart_msg(char msg[], uint8_t max_len)    {
    uint8_t char_index = 0;
    while (msg[char_index] != NULL && char_index < max_len) {
        os_queue_send(&uart_queue, (msg + char_index));
        char_index++;
    }
}

/*==================[end of file]============================================*/
