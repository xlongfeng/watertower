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
 
/*----------------------------------------------------------------------------
 * CMSIS-RTOS 'main' function template
 *---------------------------------------------------------------------------*/

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions

#include <stdio.h>
#include <stdlib.h>

#include "misc.h"

extern int stdioInit(void);
extern int blinkyInit(void);
extern int ultrasonicRangingInit(void);
extern uint32_t getUltrasonicRangingSample(uint16_t index);

static void rccInit(void)
{
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
}

static void nvicInit(void)
{
  NVIC_InitTypeDef nvicInitStruct;
  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  
  nvicInitStruct.NVIC_IRQChannel = SPI1_IRQn;
  nvicInitStruct.NVIC_IRQChannelPreemptionPriority = 2;
  nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
  nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvicInitStruct);
  
  nvicInitStruct.NVIC_IRQChannel = USART1_IRQn;
  nvicInitStruct.NVIC_IRQChannelPreemptionPriority = 2;
  nvicInitStruct.NVIC_IRQChannelSubPriority = 1;
  nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvicInitStruct);
  
  nvicInitStruct.NVIC_IRQChannel = USART2_IRQn;
  nvicInitStruct.NVIC_IRQChannelPreemptionPriority = 2;
  nvicInitStruct.NVIC_IRQChannelSubPriority = 2;
  nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
  
  nvicInitStruct.NVIC_IRQChannel = TIM5_IRQn;
  nvicInitStruct.NVIC_IRQChannelPreemptionPriority = 1;
  nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
  nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvicInitStruct);
}

/*
 * main: initialize and start the system
 */
int main (void)
{
  rccInit();
  nvicInit();
  
  stdioInit();
  
  printf("Water Tower Monitor\n");
  
  blinkyInit();
  ultrasonicRangingInit();
  
  while (1) {
    int ch = getchar();
    if (ch != -1) {
      putchar(ch);
      if (ch == 'p') {
        printf("Ultrasonic Ranging Sample: %d, %d\n", getUltrasonicRangingSample(0), getUltrasonicRangingSample(1));
      }
    } else {
      osDelay(100);
    }
  }
}
