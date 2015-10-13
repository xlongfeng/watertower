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

#include "stm32f10x_exti.h"

typedef struct
{
  uint8_t id;
  GPIO_TypeDef* triggerGPIOx;
  uint16_t trigger;
  GPIO_TypeDef* outputGPIOx;
  uint16_t output;
  uint8_t portSource;
  uint8_t pinSource;
  uint32_t irq;
  uint32_t microSec;
  uint32_t sysTickStart;
  uint32_t sysTickEnd;
} UltrasonicRanging;

static UltrasonicRanging sensors[2] = {
  {
    1 << 0,
    GPIOB,
    GPIO_Pin_11,
    GPIOB,
    GPIO_Pin_13,
    GPIO_PortSourceGPIOB,
    GPIO_PinSource11,
    EXTI_Line11,
  },
  {
    1 << 1,
    GPIOB,
    GPIO_Pin_14,
    GPIOB,
    GPIO_Pin_15,
    GPIO_PortSourceGPIOB,
    GPIO_PinSource14,
    EXTI_Line14,
  },
};

#define NUM_OF_SENSOR (sizeof (sensors) / sizeof (sensors[0]))

/*----------------------------------------------------------------------------
 *      Thread 1: ultrasonic ranging thread
 *---------------------------------------------------------------------------*/
 
static void ultrasonicRanging(void const *argument);                             // thread function
static osThreadId tidultrasonicRanging;                                          // thread id
static osThreadDef (ultrasonicRanging, osPriorityNormal, 1, 0);                   // thread object

uint32_t sysTickToMicroSec(uint32_t sysTick)
{
  uint32_t microsec_i, microsec_f;
  
  microsec_i = sysTick / os_tickus_i;
  microsec_f = (((uint64_t)sysTick) << 16) / os_tickus_f;
  
  return (microsec_i + microsec_f);
}

static uint32_t count;

void EXTI15_10_IRQHandler(void)
{
  UltrasonicRanging *sensor;
  int i;
  
  for (i = 0; i < NUM_OF_SENSOR; i++) {
    sensor = &sensors[i];
    count= osKernelSysTick();
    if (EXTI_GetITStatus(sensor->irq) == SET) {
      EXTI_ClearITPendingBit(sensor->irq);
      if (GPIO_ReadInputDataBit(sensor->triggerGPIOx, sensor->trigger) == Bit_SET) {
        sensor->sysTickStart = osKernelSysTick();
      } else {
        sensor->sysTickEnd = osKernelSysTick();
      }
    }
  }
}

static void sensorInit(void)
{
  GPIO_InitTypeDef gpioInitStructure;
  EXTI_InitTypeDef exitInitStructure;
  UltrasonicRanging *sensor;
  int i;
  
  for (i = 0; i < NUM_OF_SENSOR; i++) {
    sensor = &sensors[i];
    
    gpioInitStructure.GPIO_Pin = sensor->trigger;
    gpioInitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    gpioInitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(sensor->triggerGPIOx, &gpioInitStructure);
    
    gpioInitStructure.GPIO_Pin = sensor->output;
    gpioInitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    gpioInitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(sensor->outputGPIOx, &gpioInitStructure);
    
    EXTI_ClearITPendingBit(sensor->irq);
    GPIO_EXTILineConfig(sensor->portSource, sensor->pinSource);
    
    exitInitStructure.EXTI_Line = sensor->irq;
    exitInitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    exitInitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    exitInitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exitInitStructure);
  }
}

int ultrasonicRangingInit(void)
{
  tidultrasonicRanging = osThreadCreate (osThread(ultrasonicRanging), NULL);
  if(!tidultrasonicRanging) return(-1);
  
  sensorInit();
  
  return(0);
}

void ultrasonicRanging(void const *argument)
{
  UltrasonicRanging *sensor;

  NVIC_EnableIRQ(EXTI15_10_IRQn);
  
  while (1) {
    sensor = &sensors[0];
    if(sensor) {
      printf("%d: microSec %d, start %d, end %d, %d\n",
              sensor->id, sysTickToMicroSec(sensor->sysTickStart - sensor->sysTickEnd),
              sensor->sysTickStart, sensor->sysTickEnd, count);
    }
    osDelay(1000);
  }
}
