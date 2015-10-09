
#include <cmsis_os.h>                                           // CMSIS RTOS header file

#include "LED.h"

/*----------------------------------------------------------------------------
 *      Thread 1: blinky thread
 *---------------------------------------------------------------------------*/
 
static void Thread (void const *argument);                             // thread function
static osThreadId tid_Thread;                                          // thread id
static osThreadDef (Thread, osPriorityNormal, 1, 0);                   // thread object

int Init_BlinkyThread (void) {

  tid_Thread = osThreadCreate (osThread(Thread), NULL);
  if(!tid_Thread) return(-1);
  
  return(0);
}

void Thread (void const *argument) {

  while (1) {
    LED_On(0);
    osDelay(500);
    LED_Off(0);
    osDelay(1500);
  }
}
