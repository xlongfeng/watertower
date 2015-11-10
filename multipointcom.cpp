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

#include "stm32f10x_gpio.h"

#include "si4432.h"

// #define HOST_DEBUG
// #define DEBUG

/* Protocol data frame format
 * | Addr[1] | Protocol Field[1] | Data Field[n] | CRC[1] |
 * Len: data frame length, must be less than or equal 64 bytes.
 */

extern void setBlinkyMode(int mode);
extern void setUltrasonicRangingSampleInterval(uint16_t sampleInterval);
extern uint32_t getUltrasonicRangingSample(uint16_t index);

static const uint8_t MultiPointComMaximumQuantity = 16;
static const uint8_t MultiPointComIdentityBase = 0x10;
static uint8_t multiPointComIdentity = MultiPointComIdentityBase;
  
static Si4432 *radio;
static bool radioInitialize = false;

static uint8_t sampleInterval = 10;
static uint32_t connectedTick;
static uint32_t disconnectCount = 1;

static byte response[64];
static byte request[64];
 
void multiPointCom (void const *argument);                            // thread function
osThreadId tidMultiPointCom;                                          // thread id
osThreadDef (multiPointCom, osPriorityNormal, 1, 0);                  // thread object

static void sendResponse(int id)
{
  uint32_t sample;
  if (id < (MultiPointComIdentityBase + 2)) {
    sample = getUltrasonicRangingSample(id & 0x01);
  } else {
    sample = getUltrasonicRangingSample(0);
  }
  
  response[0] = id;
  response[1] = 0;
  response[2] = (sample >> 0) & 0xFF;
  response[3] = (sample >> 8) & 0xFF;
  response[4] = (sample >> 16) & 0xFF;
  response[5] = (sample >> 24) & 0xFF;
  
  osDelay(10);
  radio->sendPacket(6, response);
}

void setMultiPointComIdentity(uint8_t id)
{
  /*  chip 0 occupy 2 addresses: 0 and 1 */
  if (id > 0)
    id += 1;
  if (id > MultiPointComMaximumQuantity)
    printf("Set watertower identity %d error!\n", id);
  
  multiPointComIdentity = MultiPointComIdentityBase + id;
  printf("Multi point com identity 0x%02x\n", multiPointComIdentity);
}

uint8_t readChipAddress(void)
{
  GPIO_InitTypeDef gpioInitStructure;
  
  gpioInitStructure.GPIO_Pin = GPIO_Pin_0;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOC, &gpioInitStructure);
  
  gpioInitStructure.GPIO_Pin = GPIO_Pin_1;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOC, &gpioInitStructure);
  
  gpioInitStructure.GPIO_Pin = GPIO_Pin_2;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOC, &gpioInitStructure);
  
  gpioInitStructure.GPIO_Pin = GPIO_Pin_3;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOC, &gpioInitStructure);
  
  return GPIO_ReadInputData(GPIOC) & 0x0f;
}

int multiPointComInit(void)
{
  tidMultiPointCom = osThreadCreate (osThread(multiPointCom), NULL);
  if(!tidMultiPointCom) return(-1);
  
  radio = new Si4432();
  
  return(0);
}

void multiPointCom (void const *argument)
{
  bool pkg;
  
  setUltrasonicRangingSampleInterval(sampleInterval);
  setMultiPointComIdentity(readChipAddress());
  
  radio->init();
  
#ifdef HOST_DEBUG
  while (1) {
    byte resLen = 0;
    byte answer[64] = { 0 };
    pkg = radio->sendPacket(32, response, true, 70, &resLen, answer);
    if (pkg) {
      printf("Response packet received <%d>: ", resLen);
      for (i = 0; i < resLen; i++) {
        printf("%x ", answer[i]);
      }
      printf("\n");
    }
    osDelay(1000);
  }
#else

  while (1) {
    if (!radioInitialize) {
      printf("Radio initializing ...\n");
      radioInitialize = true;
      radio->hardReset();
      radio->setBaudRate(30);
      radio->setFrequency(433);
      radio->startListening();
      connectedTick = osKernelSysTick();
      setBlinkyMode(0);
    }
    
    pkg = radio->isPacketReceived();
    if (pkg) {
      byte len = 0;
      int id;
      uint8_t protocol;
      radio->getPacketReceived(&len, request);
      
#ifdef DEBUG
      {
        int i;
        printf("Packet received <%d>: ", len);
        for (i = 0; i < len; i++) {
          printf("%x ", request[i]);
        }
        printf("\n");
      }
#endif
      
      id = request[0];
      
      if (multiPointComIdentity < (MultiPointComIdentityBase + 2)) {
        /* chip 0 */
        if ((id & ~0x01) != multiPointComIdentity) {
          radio->startListening();
          continue;
        }
      } else if (id == multiPointComIdentity) {
        /* chip address form 2 */
      } else {
        radio->startListening();
        continue;
      }
      
      switch (len) {
      case 3:
        protocol = request[1]; /* alway zero */
        sampleInterval = request[2];
        setUltrasonicRangingSampleInterval(sampleInterval);
        break;
      default:
        continue;
      }
      
      sendResponse(id);
      radio->startListening();
      connectedTick = osKernelSysTick();
      setBlinkyMode(1);
    } else {
      if (osKernelSysTick() - connectedTick > osKernelSysTickMicroSec((sampleInterval * 5 * 1000) * 1000)) {
        printf("Radio %d disconnected more than %d second\n", disconnectCount++, sampleInterval * 5);
        sampleInterval = 10;
        connectedTick = osKernelSysTick();
        setBlinkyMode(0);
        radioInitialize = false;
      }
      osThreadYield();
    }
  }
#endif
}

void radioReset(void)
{
  radioInitialize = false;
}

void radioDumpReg(void)
{
  radio->readAll();
}

uint8_t radioReadReg(uint8_t reg)
{
  return radio->readReg(reg);
}

void radioWriteReg(uint8_t reg, uint8_t value)
{
  radio->writeReg(reg, value);
}
