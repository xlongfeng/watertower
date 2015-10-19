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

#include "stm32f10x_gpio.h"

/*----------------------------------------------------------------------------
 *      Thread 1: blinky thread
 *---------------------------------------------------------------------------*/
 
static void blinky(void const *argument);                             // thread function
static osThreadId tidBlinky;                                          // thread id
static osThreadDef (blinky, osPriorityNormal, 1, 0);                   // thread object

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

int blinkyInit(void)
{
  tidBlinky = osThreadCreate (osThread(blinky), NULL);
  if(!tidBlinky) return(-1);
  
  return(0);
}

void blinky(void const *argument)
{
  ledInit();
  
  while (1) {
    ledOn();
    osDelay(500);
    ledOff();
    osDelay(1500);
  }
}
