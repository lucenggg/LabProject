/*
 * bit_fiddling.h
 *
 *  Created on: Apr 22, 2026
 *      Author: Lucas England
 */

#ifndef LUCENG_MACROS_H_
#define LUCENG_MACROS_H_

#include <inc/tm4c123gh6pm.h>

#define cat(x,y) cat__(x,y)
#define cat__(x,y) x ## y

#define BIT_0 0x1
#define BIT_1 0x2
#define BIT_2 0x4
#define BIT_3 0x8
#define BIT_4 0x10
#define BIT_5 0x20
#define BIT_6 0x40
#define BIT_7 0x80
#define BIT_8 0x100
#define BIT_9 0x200
#define BIT_10 0x400
#define BIT_11 0x800
#define BIT_12 0x1000
#define BIT_13 0x2000
#define BIT_14 0x4000
#define BIT_15 0x8000
#define BIT_16 0x10000
#define BIT_17 0x20000
#define BIT_18 0x40000
#define BIT_19 0x80000
#define BIT_20 0x100000
#define BIT_21 0x200000
#define BIT_22 0x400000
#define BIT_23 0x800000
#define BIT_24 0x1000000
#define BIT_25 0x2000000
#define BIT_26 0x4000000
#define BIT_27 0x8000000
#define BIT_28 0x10000000
#define BIT_29 0x20000000
#define BIT_30 0x40000000
#define BIT_31 0x80000000

#endif /* LUCENG_MACROS_H_ */
