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

static const uint8_t WaterTowerMaximumQuantity = 16;
static const uint8_t WaterTowerIdentityBase = 0x10;
static uint8_t waterTowerIdentity = WaterTowerIdentityBase;
  
static Si4432 *radio;
static bool radioInitialize = false;

static uint8_t sampleInterval = 2;
static uint32_t connectedTick;

static byte response[64];
static byte request[64];
 
void multiPointCom (void const *argument);                            // thread function
osThreadId tidMultiPointCom;                                          // thread id
osThreadDef (multiPointCom, osPriorityNormal, 1, 0);                  // thread object

static void sendResponse(int id)
{
  uint32_t sample = getUltrasonicRangingSample(id & 0x01);
  
  sample = id * 100;
  
  response[0] = id;
  response[1] = 0;
  response[2] = (sample >> 0) & 0xFF;
  response[3] = (sample >> 8) & 0xFF;
  response[4] = (sample >> 16) & 0xFF;
  response[5] = (sample >> 24) & 0xFF;
  
  osDelay(10);
  radio->sendPacket(6, response);
}

void setWaterTowerIdentity(int id)
{
  if (id > WaterTowerMaximumQuantity)
    printf("Set watertower identity %d error!\n", id);
  
  waterTowerIdentity = WaterTowerIdentityBase + id & ~0x01;
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
  
  radio->init();
  radio->setBaudRate(30);
  radio->setFrequency(433);
  radioInitialize = true;
  // radio->readAll();
  
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
  radio->startListening();
  while (1) {
    if (!radioInitialize) {
      printf("Reinitialize radio\n");
      radioInitialize = true;
      radio->hardReset();
      radio->setBaudRate(30);
      radio->setFrequency(433);
      radio->startListening();
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
      
      if ((id & ~0x01) != waterTowerIdentity) {
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
      if (osKernelSysTick() - connectedTick > osKernelSysTickMicroSec((sampleInterval * 4 * 1000) * 1000)) {
        printf("Multi point communication disconnected more than %d second\n", sampleInterval * 4);
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
