/*
 * br_os_isr.c
 *
 *  Created on: 2020
 *      Author: mbrignone
 */


#include "br_os_isr.h"


static void* user_isr_vector[N_IRQ];


/*************************************************************************************************
     *  @brief Registra una interrupcion.
     *
***************************************************************************************************/
bool os_register_isr(LPC43XX_IRQn_Type irq, void* user_isr) {

    if (user_isr_vector[irq] == NULL)   {
        user_isr_vector[irq] = user_isr;
        NVIC_ClearPendingIRQ(irq);
        NVIC_EnableIRQ(irq);
        return true;
    }

    return false;
}


/*************************************************************************************************
     *  @brief Elimina una interrupcion.
     *
***************************************************************************************************/
bool os_RemoverIRQ(LPC43XX_IRQn_Type irq)   {

    if (user_isr_vector[irq] != NULL)   {
        user_isr_vector[irq] = NULL;
        NVIC_ClearPendingIRQ(irq);
        NVIC_DisableIRQ(irq);
        return true;
    }

    return false;
}




/*************************************************************************************************
     *  @brief Las interrupciones llaman este handler. De esta forma se tiene
     *  control desde el OS sobre la forma en que se ejecutan las interrupciones
     *
***************************************************************************************************/
static void os_isr_handler(LPC43XX_IRQn_Type IRQn)  {
    void (*user_isr)(void); // function pointer to the ISR

    // get the OS state, to restore after calling the ISR
    os_state previous_os_state = os_get_global_state();

    // set the OS state to indicate that it is inside an ISR
    os_set_global_state(OS_STATE_ISR);

    // call the user defined ISR
    user_isr = user_isr_vector[IRQn];
    user_isr();

    // restore the OS state
    os_set_global_state(previous_os_state);

    // clear the corresponding interrupt flag
    NVIC_ClearPendingIRQ(IRQn);

    // immediatelly call the scheduler if needed (because the
    // interrupt released a resource/event)
    if (os_get_scheduler_from_isr() == true)    {

        os_set_scheduler_from_isr(false);
        os_cpu_yield();
    }
}

/*==================[interrupt service routines]=============================*/

void DAC_IRQHandler(void)           { os_isr_handler( DAC_IRQn         ); }
void M0APP_IRQHandler(void)         { os_isr_handler( M0APP_IRQn       ); }
void DMA_IRQHandler(void)           { os_isr_handler( DMA_IRQn         ); }
void FLASH_EEPROM_IRQHandler(void)  { os_isr_handler( RESERVED1_IRQn   ); }
void ETH_IRQHandler(void)           { os_isr_handler( ETHERNET_IRQn    ); }
void SDIO_IRQHandler(void)          { os_isr_handler( SDIO_IRQn        ); }
void LCD_IRQHandler(void)           { os_isr_handler( LCD_IRQn         ); }
void USB0_IRQHandler(void)          { os_isr_handler( USB0_IRQn        ); }
void USB1_IRQHandler(void)          { os_isr_handler( USB1_IRQn        ); }
void SCT_IRQHandler(void)           { os_isr_handler( SCT_IRQn         ); }
void RIT_IRQHandler(void)           { os_isr_handler( RITIMER_IRQn     ); }
void TIMER0_IRQHandler(void)        { os_isr_handler( TIMER0_IRQn      ); }
void TIMER1_IRQHandler(void)        { os_isr_handler( TIMER1_IRQn      ); }
void TIMER2_IRQHandler(void)        { os_isr_handler( TIMER2_IRQn      ); }
void TIMER3_IRQHandler(void)        { os_isr_handler( TIMER3_IRQn      ); }
void MCPWM_IRQHandler(void)         { os_isr_handler( MCPWM_IRQn       ); }
void ADC0_IRQHandler(void)          { os_isr_handler( ADC0_IRQn        ); }
void I2C0_IRQHandler(void)          { os_isr_handler( I2C0_IRQn        ); }
void SPI_IRQHandler(void)           { os_isr_handler( I2C1_IRQn        ); }
void I2C1_IRQHandler(void)          { os_isr_handler( SPI_INT_IRQn     ); }
void ADC1_IRQHandler(void)          { os_isr_handler( ADC1_IRQn        ); }
void SSP0_IRQHandler(void)          { os_isr_handler( SSP0_IRQn        ); }
void SSP1_IRQHandler(void)          { os_isr_handler( SSP1_IRQn        ); }
void UART0_IRQHandler(void)         { os_isr_handler( USART0_IRQn      ); }
void UART1_IRQHandler(void)         { os_isr_handler( UART1_IRQn       ); }
void UART2_IRQHandler(void)         { os_isr_handler( USART2_IRQn      ); }
void UART3_IRQHandler(void)         { os_isr_handler( USART3_IRQn      ); }
void I2S0_IRQHandler(void)          { os_isr_handler( I2S0_IRQn        ); }
void I2S1_IRQHandler(void)          { os_isr_handler( I2S1_IRQn        ); }
void SPIFI_IRQHandler(void)         { os_isr_handler( RESERVED4_IRQn   ); }
void SGPIO_IRQHandler(void)         { os_isr_handler( SGPIO_INT_IRQn   ); }
void GPIO0_IRQHandler(void)         { os_isr_handler( PIN_INT0_IRQn    ); }
void GPIO1_IRQHandler(void)         { os_isr_handler( PIN_INT1_IRQn    ); }
void GPIO2_IRQHandler(void)         { os_isr_handler( PIN_INT2_IRQn    ); }
void GPIO3_IRQHandler(void)         { os_isr_handler( PIN_INT3_IRQn    ); }
void GPIO4_IRQHandler(void)         { os_isr_handler( PIN_INT4_IRQn    ); }
void GPIO5_IRQHandler(void)         { os_isr_handler( PIN_INT5_IRQn    ); }
void GPIO6_IRQHandler(void)         { os_isr_handler( PIN_INT6_IRQn    ); }
void GPIO7_IRQHandler(void)         { os_isr_handler( PIN_INT7_IRQn    ); }
void GINT0_IRQHandler(void)         { os_isr_handler( GINT0_IRQn       ); }
void GINT1_IRQHandler(void)         { os_isr_handler( GINT1_IRQn       ); }
void EVRT_IRQHandler(void)          { os_isr_handler( EVENTROUTER_IRQn ); }
void CAN1_IRQHandler(void)          { os_isr_handler( C_CAN1_IRQn      ); }
void ADCHS_IRQHandler(void)         { os_isr_handler( ADCHS_IRQn       ); }
void ATIMER_IRQHandler(void)        { os_isr_handler( ATIMER_IRQn      ); }
void RTC_IRQHandler(void)           { os_isr_handler( RTC_IRQn         ); }
void WDT_IRQHandler(void)           { os_isr_handler( WWDT_IRQn        ); }
void M0SUB_IRQHandler(void)         { os_isr_handler( M0SUB_IRQn       ); }
void CAN0_IRQHandler(void)          { os_isr_handler( C_CAN0_IRQn      ); }
void QEI_IRQHandler(void)           { os_isr_handler( QEI_IRQn         ); }
