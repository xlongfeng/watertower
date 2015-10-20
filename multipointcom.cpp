
#include <cmsis_os.h>                                           // CMSIS RTOS header file
#include <stdio.h>

#include "si4432.h"

// #define HOST_DEBUG

Si4432 radio;
 
void multiPointCom (void const *argument);                             // thread function
osThreadId tidMultiPointCom;                                          // thread id
osThreadDef (multiPointCom, osPriorityNormal, 1, 0);                   // thread object

int multiPointComInit(void)
{
  tidMultiPointCom = osThreadCreate (osThread(multiPointCom), NULL);
  if(!tidMultiPointCom) return(-1);
  
  return(0);
}

void multiPointCom (void const *argument)
{
  byte dummy[70] = { 0x01, 0x3, 0x11, 0x13 };
  bool pkg;
  int i;
  
  radio.init();
  radio.setBaudRate(70);
  radio.setFrequency(433);
  // radio.readAll();
  
#ifndef HOST_DEBUG
  radio.startListening();
#endif
  
  while (1) {
#ifdef HOST_DEBUG
    byte resLen = 0;
    byte answer[64] = { 0 };
    pkg = radio.sendPacket(32, dummy, true, 70, &resLen, answer);
    if (pkg) {
      printf("Response packet received <%d>: ", resLen);
      for (i = 0; i < resLen; i++) {
        printf("%x ", answer[i]);
      }
      printf("\n");
    }
    osDelay(1000);
#else
    pkg = radio.isPacketReceived();
    if (pkg) {
      byte payLoad[64] = {0};
      byte len = 0;
      radio.getPacketReceived(&len, payLoad);
      printf("Packet received <%d>: ", len);
      for (i = 0; i < len; i++) {
        printf("%x ", payLoad[i]);
      }
      printf("\n");
      
      printf("Send response\n");
      radio.sendPacket(50, dummy);
      radio.startListening();
    } else {
      osThreadYield();
    }
#endif
  }
}


