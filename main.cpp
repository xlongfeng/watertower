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
#include <string.h>

#include "misc.h"

#define SYS_CBSIZE 128
#define SYS_MAX_ARGS 4

static const char *sysPrompt = "STM32# ";
static char consoleBuffer[SYS_CBSIZE];

extern int stdioInit(void);
extern int blinkyInit(void);
extern int multiPointComInit(void);
extern int ultrasonicRangingInit(void);

static void rccInit(void)
{
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
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

struct CmdTableS
{
  const char *name;
  int maxargs;
  int (*cmd)(struct CmdTableS *, int, char *[]);
  const char *usage;
};

typedef struct CmdTableS CmdTableT;

int do_help(CmdTableT *cmdtp, int argc, char *argv[]);
int do_ur(CmdTableT *cmdtp, int argc, char *argv[]);
int do_ra(CmdTableT *cmdtp, int argc, char *argv[]);

static const CmdTableT cmdTable[] = {
  {"?", SYS_MAX_ARGS, do_help, "alias for 'help'"},
  {"help", SYS_MAX_ARGS, do_help, "print online help"},
  {"ur", SYS_MAX_ARGS, do_ur, "ultrasonic ranging"},
  {"ra", SYS_MAX_ARGS, do_ra, "radio controller"},
};

#define CMD_ITEMS (sizeof (cmdTable) / sizeof (cmdTable[0]))

static CmdTableT *findCmd(char *cmd)
{
  const char *p;
  int len;
  int i;
  
  len = ((p = strchr(cmd, '.')) ==NULL) ? strlen(cmd) : (p - cmd);
  
  for (i = 0; i < CMD_ITEMS; i++) {
    if (strncmp(cmd, cmdTable[i].name, len) == 0) {
      if (len == strlen(cmdTable[i].name))
        return (CmdTableT *)&cmdTable[i];
    }
  }
  
  return 0;
}

static void cmdUsage(CmdTableT *cmdtp)
{
  printf("%s - %s\n\n", cmdtp->name, cmdtp->usage);
}

int do_help(CmdTableT *cmdtp, int argc, char *argv[])
{
  int i;
  
  if (argc == 1) {
    for (i = 0; i < CMD_ITEMS; i++) {
      printf("%*s- %s\n", 8, cmdTable[i].name, cmdTable[i].usage);
    }
  } else {
    for (i = 1; i < argc; i++) {
      if ((cmdtp = findCmd(argv[i])) != NULL) {
        cmdUsage(cmdtp);
      } else {
        printf("Unknown comand '%s' - try help;"
               " without arguments for list of all"
               " known commands\n\n", argv[i]);
      }
    }
  }
  
  return 0;
}

extern uint32_t getUltrasonicRangingSample(uint16_t index);

int do_ur(CmdTableT *cmdtp, int argc, char *argv[])
{
  uint32_t sample0 = getUltrasonicRangingSample(0);
  uint32_t sample1 = getUltrasonicRangingSample(1);
  uint32_t distance0 = sample0 * 340 / 2 / 10000;
  uint32_t distance1 = sample1 * 340 / 2 / 10000;
  printf("Ultrasonic ranging sample[0]: %d cm <%d usec>, sample[1]: %d cm <%d usec>\n",
      distance0, sample0, distance1, sample1);
  return 0;
}

extern void radioReset(void);
extern void radioDumpReg(void);
extern uint8_t radioReadReg(uint8_t reg);
extern void radioWriteReg(uint8_t reg, uint8_t value);

int do_ra(CmdTableT *cmdtp, int argc, char *argv[])
{
  int len;
  
  switch (argc) {
  case 2:
    len = strlen(argv[1]);
    if (strncmp(argv[1], "reset", len) == 0) {
      printf("Reset radio\n");
      radioReset();
    }
    if (strncmp(argv[1], "dump", len) == 0) {
      radioDumpReg();
    }
    break;
  case 3:
    len = strlen(argv[1]);
    if (strncmp(argv[1], "read", len) == 0) {
      uint8_t reg = strtoul(argv[2], NULL, 0);
      printf("Read radio reg %x: %x\n", reg, radioReadReg(reg));
    }
    break;
  case 4:
    len = strlen(argv[1]);
    if (strncmp(argv[1], "read", len) == 0) {
      uint8_t reg = strtoul(argv[2], NULL, 0);
      uint8_t val = strtoul(argv[3], NULL, 0);
      radioWriteReg(reg, val);
      printf("Write radio reg %x: %x [%x]\n", reg, val, radioReadReg(reg));
    }
    break;
  default:
    cmdUsage(cmdtp);
    break;
  }
  return 0;
}

static int readline(void)
{
  int len, ch;
  char *p = consoleBuffer;
  
  printf(sysPrompt);
  
  len = 0;
  while (1) {
    ch = getchar();
    if (ch < 0) {
      osDelay(100);
      continue;
    }
    
    switch (ch) {
    case '\r':
    case '\n':
      *p = '\0';
      putchar('\n');
      return len;
    case '\0':
      continue;
    case 0x03: /* ^C - break */
      consoleBuffer[0] = '\0';
      return -1;
    default:
      if (len < SYS_CBSIZE - 2) {
        len++;
        *p++ = ch;
        putchar(ch);
      } else {
        putchar('\a');
      }
    }
  }
}

static int parseLine(char *line, char *argv[])
{
  int nargs = 0;
  
  while (nargs < SYS_MAX_ARGS) {
    while ((*line == ' ') || (*line == '\t')) {
      line++;
    }
    
    if (*line == '\0') {
      argv[nargs] = NULL;
      return nargs;
    }
    
    argv[nargs++] = line;
    
    while (*line && (*line != ' ') && (*line != '\t')) {
      ++line;
    }
    
    if (*line == '\0') {
      argv[nargs] = NULL;
      return nargs; 
    }
    
    *line++ = '\0';
  }
  
  printf(" ** Too many args (max. %d) **\n", SYS_MAX_ARGS);
  
  return nargs;
}

static int runCommand(void)
{
  char *cmdBuffer = consoleBuffer;
  int len = strlen(cmdBuffer);
  char *argv[SYS_MAX_ARGS];
  int argc;
  CmdTableT *cmdtp;
  
  if (len > SYS_CBSIZE) {
    puts("## Command too long!");
    return -1;
  }
  
  if ((argc = parseLine(cmdBuffer, argv)) == 0) {
    puts("## No command at all");
    return -1;
  }
  
  if ((cmdtp = findCmd(argv[0])) == NULL) {
    printf("Unknown command '%s' - try 'help'\n", argv[0]);
    return -1;
  }
  
  if (argc > cmdtp->maxargs) {
    cmdUsage(cmdtp);
    return -1;
  }
  
  return (cmdtp->cmd)(cmdtp, argc, argv);
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
  multiPointComInit();
  ultrasonicRangingInit();
  
  while (1) {
    int len = readline();
    if (len > 0) {
      runCommand();
    } else if (len < 0) {
      puts("<INTERRUPT>");
    }
  }
}
