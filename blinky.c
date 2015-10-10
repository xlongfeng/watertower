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

#include "LED.h"

/*----------------------------------------------------------------------------
 *      Thread 1: blinky thread
 *---------------------------------------------------------------------------*/
 
static void blinky(void const *argument);                             // thread function
static osThreadId tidBlinky;                                          // thread id
static osThreadDef (blinky, osPriorityNormal, 1, 0);                   // thread object

int blinkyInit(void)
{
  tidBlinky = osThreadCreate (osThread(blinky), NULL);
  if(!tidBlinky) return(-1);
  
  LED_Initialize();
  
  return(0);
}

void blinky(void const *argument)
{
  while (1) {
    LED_On(0);
    osDelay(500);
    LED_Off(0);
    osDelay(1500);
  }
}
