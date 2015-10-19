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
 
#include "Driver_UART.h"
 
//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------
 
// <h>STDIN UART Interface
 
//   <o>Connect to hardware via Driver_UART# <0-255>
//   <i>Select driver control block for UART interface
#define UART_DRV_NUM           1
 
//   <o>Baudrate
#define UART_BAUDRATE          115200
 
// </h>
 
 
#define _UART_Driver_(n)  Driver_UART##n
#define  UART_Driver_(n) _UART_Driver_(n)
 
extern ARM_DRIVER_UART  UART_Driver_(UART_DRV_NUM);
#define ptrUART       (&UART_Driver_(UART_DRV_NUM))

extern "C" int stdin_getchar (void);
extern "C" int stderr_putchar (int ch);
extern "C" int stdout_putchar (int ch);
 
 
/**
  Initialize stdin
 
  \return          0 on success, or -1 on error.
*/
int stdioInit (void) {
  int32_t status;
 
  status = ptrUART->Initialize(NULL, 0);
  if (status != ARM_UART_OK) return (-1);
 
  status = ptrUART->PowerControl(ARM_POWER_FULL);
  if (status != ARM_UART_OK) return (-1);
 
  status = ptrUART->Configure(UART_BAUDRATE, 8, ARM_UART_PARITY_NONE,
                      ARM_UART_STOP_BITS_1,
                      ARM_UART_FLOW_CONTROL_NONE);
  if (status != ARM_UART_OK) return (-1);
 
  return (0);
}
 
 
/**
  Get a character from stdin
 
  \return     The next character from the input, or -1 on read error.
*/
int stdin_getchar (void) {
  uint8_t buf[1];
 
  if (ptrUART->ReadData(buf, 1) == 0) {
    return (-1);
  }
  return (buf[0]);
}

/**
  Put a character to the stderr
 
  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/
int stderr_putchar (int ch) {
  uint8_t buf[1];
 
  buf[0] = ch;
  if (ptrUART->WriteData(buf, 1) != 1) {
    return (-1);
  }
  return (ch);
}

/**
  Put a character to the stdout
 
  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/
int stdout_putchar (int ch) {
  uint8_t buf[1];
 
  buf[0] = ch;
  if (ptrUART->WriteData(buf, 1) != 1) {
    return (-1);
  }
  return (ch);
}
