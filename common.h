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
 
#ifndef __COMMON_H__
#define __COMMON_H__
 
#ifdef __cplusplus  
extern "C" {  
#endif  

extern int stdioInit(void);

extern int blinkyInit(void);
extern void setBlinkyMode(int mode);
  
extern int ultrasonicRangingInit(void);
extern void setUltrasonicRangingSampleInterval(uint16_t sampleInterval);
extern uint32_t getUltrasonicRangingSample(uint16_t index);

extern int multiPointComInit(void);
extern void radioReset(void);
extern void radioDumpReg(void);
extern uint8_t radioReadReg(uint8_t reg);
extern void radioWriteReg(uint8_t reg, uint8_t value);

#ifdef __cplusplus  
}  
#endif

#endif /* __COMMON_H__ */
