/*
 * Water Tower Monitor
 *
 * Copyright (c) 2015, longfeng.xiao <xlongfeng@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <cmsis_os.h>                                           // CMSIS RTOS header file

#include <stdio.h>
#include <stdlib.h>

#include "stm32f10x.h"

/*----------------------------------------------------------------------------
 *      Thread 1: blinky thread
 *---------------------------------------------------------------------------*/
 
static void blinky(void const *argument);                             // thread function
static osThreadId tidBlinky;                                          // thread id
static osThreadDef (blinky, osPriorityNormal, 1, 0);                   // thread object

static int blinkyMode = 0;

static void ledInit(void)
{
  GPIO_InitTypeDef gpioInitStructure;
  
  gpioInitStructure.GPIO_Pin = GPIO_Pin_0;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &gpioInitStructure);
}

static void ledOn(void)
{
  GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_SET);
}

static void ledOff(void)
{
  GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
}

static void iwdgInit(void)
{
  const uint32_t LsiFreq = 40000;
  /* IWDG timeout equal to 8s (the timeout may varies due to LSI frequency
     dispersion) */
  /* Enable write access to IWDG_PR and IWDG_RLR registers */
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

  /* IWDG counter clock: LSI/128 */
  IWDG_SetPrescaler(IWDG_Prescaler_128);

  /* Set counter reload value to obtain 4s IWDG TimeOut.
     Counter Reload Value = 8s/IWDG counter clock period
                          = 8s / (LSI/128)
                          = 8s / (LsiFreq/128)
                          = LsiFreq/(128 / 8)
                          = LsiFreq/16
   */
  IWDG_SetReload(LsiFreq/(128 / 8));

  /* Reload IWDG counter */
  IWDG_ReloadCounter();

  /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
  IWDG_Enable();
}

static void watchdog(void)
{
  /* Reload IWDG counter */
  IWDG_ReloadCounter();
}

int blinkyInit(void)
{
  tidBlinky = osThreadCreate (osThread(blinky), NULL);
  if(!tidBlinky) return(-1);
  
  return(0);
}

void setBlinkyMode(int mode)
{
  blinkyMode = mode;
}

void blinky(void const *argument)
{
  iwdgInit();
  ledInit();
  
  while (1) {
    watchdog();
    if (blinkyMode == 0) {
      ledOn();
      osDelay(500);
    } else if (blinkyMode == 1) {
      ledOn();
      osDelay(200);
      ledOff();
      osDelay(100);
      ledOn();
      osDelay(200);
    } else {
    }
    ledOff();
    osDelay(1500);
  }
}
