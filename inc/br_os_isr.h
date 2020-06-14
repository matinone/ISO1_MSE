/*
 * br_os_isr.h
 *
 *  Created on: 2020
 *      Author: mbrignone
 */

#ifndef __BR_OS_ISR_H__
#define __BR_OS_ISR_H__


#include "br_os_core.h"
#include "br_os_api.h"
#include "board.h"
#include "cmsis_43xx.h"

#define N_IRQ   53

bool os_register_isr(LPC43XX_IRQn_Type irq, void* user_isr);
bool os_remove_isr(LPC43XX_IRQn_Type irq);


#endif  // __BR_OS_ISR_H__
