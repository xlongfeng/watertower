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
  GPIO_TypeDef* triggerGPIO;
  uint16_t trigger;
  GPIO_TypeDef* echoGPIO;
  uint16_t echo;
  TIM_TypeDef* tim;
  uint16_t channel;
  uint16_t irq;
  uint16_t capture;
  uint32_t delta_usec;
} UltrasonicRanging;

static UltrasonicRanging sensors[2] = {
  {
    0,
    GPIOA,
    GPIO_Pin_7,
    GPIOA,
    GPIO_Pin_2,
    TIM5,
    TIM_Channel_3,
    TIM_IT_CC3,
    0,
  },
  {
    1,
    GPIOA,
    GPIO_Pin_5,
    GPIOA,
    GPIO_Pin_3,
    TIM5,
    TIM_Channel_4,
    TIM_IT_CC4,
    0,
  },
};

#define NUM_OF_SENSOR (sizeof (sensors) / sizeof (sensors[0]))

#define TIM_CAPTURE_INVALID 0xffff
#define TIM_PERIOD 0x7fff
#define TIM_ACCURACY_USEC_SHIFT 3
#define TIM_ACCURACY_USEC (1 << TIM_ACCURACY_USEC_SHIFT)

static uint16_t ultrasonicRangingsampleInterval = 10;

/*----------------------------------------------------------------------------
 *      Thread 1: ultrasonic ranging thread
 *---------------------------------------------------------------------------*/
 
static void ultrasonicRanging(void const *argument);                             // thread function
static osThreadId tidultrasonicRanging;                                          // thread id
static osThreadDef (ultrasonicRanging, osPriorityNormal, 1, 0);                   // thread object

static uint16_t TIM_GetCapture(TIM_TypeDef* TIMx, uint16_t TIM_IT)
{
  uint16_t ccr;
  switch (TIM_IT) {
    case TIM_IT_CC1:
      ccr = TIM_GetCapture1(TIMx);
      break;
    case TIM_IT_CC2:
      ccr = TIM_GetCapture2(TIMx);
      break;
    case TIM_IT_CC3:
      ccr = TIM_GetCapture3(TIMx);
      break;
    case TIM_IT_CC4:
      ccr = TIM_GetCapture4(TIMx);
      break;
    default:
      ccr = 0;
      break;
  }
  return ccr;
}

static uint16_t captureAccuracy(uint16_t usec)
{
  uint32_t prescale;
  
  usec = usec < 900 ? usec : 900;
  
  prescale = SystemCoreClock / 1000000 * usec - 1;
  
  return prescale < 0xffff ? prescale : 0xffff;
}

static void timICInit(UltrasonicRanging *sensor, uint16_t polarity)
{
  TIM_ICInitTypeDef timICInitStruct;
  
  timICInitStruct.TIM_Channel = sensor->channel;
  timICInitStruct.TIM_ICPolarity = polarity;
  timICInitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
  timICInitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  timICInitStruct.TIM_ICFilter = 0x00;
  
  TIM_ICInit(sensor->tim, &timICInitStruct);
}

static void timICStart(UltrasonicRanging *sensor)
{
  sensor->capture = TIM_CAPTURE_INVALID;
  timICInit(sensor, TIM_ICPolarity_Rising);
  TIM_Cmd(sensor->tim, ENABLE);
}

static void timICStop(UltrasonicRanging *sensor)
{
  TIM_Cmd(sensor->tim, DISABLE);
}

void TIM5_IRQHandler(void)
{
  UltrasonicRanging *sensor;
  uint16_t capture;
  int i;
  
  for (i = 0; i < NUM_OF_SENSOR; i++) {
    sensor = &sensors[i];
    if (TIM_GetITStatus(sensor->tim, sensor->irq) == SET) {
      TIM_ClearITPendingBit(sensor->tim, sensor->irq);
      if (sensor->capture != TIM_CAPTURE_INVALID) {
        capture = TIM_GetCapture(sensor->tim, sensor->irq);
        if (capture > sensor->capture) {
          sensor->delta_usec = (capture - sensor->capture) << TIM_ACCURACY_USEC_SHIFT;
        } else {
          sensor->delta_usec = ((TIM_PERIOD - sensor->capture + 1) + capture) << TIM_ACCURACY_USEC_SHIFT;
        }
        timICStop(sensor);
      } else {
        sensor->capture = TIM_GetCapture(sensor->tim, sensor->irq);
        timICInit(sensor, TIM_ICPolarity_Falling);
      }
    }
  }
}

static void sensorInit(void)
{
  GPIO_InitTypeDef gpioInitStructure;
  TIM_TimeBaseInitTypeDef timTimeBaseInitStruct;
  UltrasonicRanging *sensor;
  int i;
  
  for (i = 0; i < NUM_OF_SENSOR; i++) {
    sensor = &sensors[i];
    sensor->capture = TIM_CAPTURE_INVALID;
    
    gpioInitStructure.GPIO_Pin = sensor->trigger;
    gpioInitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    gpioInitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(sensor->triggerGPIO, &gpioInitStructure);
    GPIO_WriteBit(sensor->triggerGPIO, sensor->trigger, Bit_RESET);
    
    gpioInitStructure.GPIO_Pin = sensor->echo;
    gpioInitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    gpioInitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(sensor->echoGPIO, &gpioInitStructure);
    
    timTimeBaseInitStruct.TIM_Prescaler = captureAccuracy(TIM_ACCURACY_USEC);
    timTimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    timTimeBaseInitStruct.TIM_Period = TIM_PERIOD;
    timTimeBaseInitStruct.TIM_ClockDivision = 0;
    timTimeBaseInitStruct.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(sensor->tim, &timTimeBaseInitStruct);
    
    TIM_ITConfig(sensor->tim, sensor->irq, ENABLE);
  }
}

int ultrasonicRangingInit(void)
{
  tidultrasonicRanging = osThreadCreate (osThread(ultrasonicRanging), NULL);
  if(!tidultrasonicRanging) return(-1);
  
  sensorInit();
  
  return(0);
}

void setUltrasonicRangingSampleInterval(uint16_t sampleInterval)
{
  ultrasonicRangingsampleInterval = sampleInterval;
}

uint32_t getUltrasonicRangingSample(uint16_t index)
{
  if (index > NUM_OF_SENSOR)
    return 0;
  return sensors[index].delta_usec;
}

void ultrasonicRanging(void const *argument)
{
  UltrasonicRanging *sensor;
  int i;
  
  while (1) {
    if (ultrasonicRangingsampleInterval > 0) {
      for (i = 0; i < NUM_OF_SENSOR; i++) {
        sensor = &sensors[i];
        timICStart(sensor);
        GPIO_WriteBit(sensor->triggerGPIO, sensor->trigger, Bit_SET);
        osDelay(2);
        GPIO_WriteBit(sensor->triggerGPIO, sensor->trigger, Bit_RESET);
      }
      osDelay(ultrasonicRangingsampleInterval * 1000);
    } else {
      osDelay(1000);
    }
  }
}
