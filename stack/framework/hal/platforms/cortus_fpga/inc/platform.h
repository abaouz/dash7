/* * OSS-7 - An opensource implementation of the DASH7 Alliance Protocol for ultra
 * lowpower wireless sensor communication
 *
 * Copyright 2015 University of Antwerp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __PLATFORM_H_
#define __PLATFORM_H_

#include "platform_defs.h"

#ifndef PLATFORM_CORTUS_FPGA
    #error Mismatch between the configured platform and the actual platform. Expected PLATFORM_CORTUS_FPGA to be defined
#endif

//#define HW_USE_HFXO
//#define HW_USE_LFXO

#include "cortus_chip.h"


/********************
 * LED DEFINITIONS *
 *******************/

#define HW_NUM_LEDS 8


//INT_HANDLER

/********************
 *  USB SUPPORT      *
 ********************/

//#define USB_DEVICE

/********************
 * UART DEFINITIONS *
 *******************/

// console configuration
//#define CONSOLE_UART        PLATFORM_EFM32GG_STK3700_CONSOLE_UART
//#define CONSOLE_LOCATION    PLATFORM_EFM32GG_STK3700_CONSOLE_LOCATION
//#define CONSOLE_BAUDRATE    PLATFORM_EFM32GG_STK3700_CONSOLE_BAUDRATE
// dummy
#define CONSOLE_UART        0
#define CONSOLE_LOCATION    1
#define CONSOLE_BAUDRATE    115200

/*************************
 * DEBUG PIN DEFINITIONS *
 ************************/

//#define DEBUG_PIN_NUM 4
//#define DEBUG0	D4
//#define DEBUG1	D5
//#define DEBUG2	D6
//#define DEBUG3	D7

/**************************
 * USERBUTTON DEFINITIONS *
 *************************/

#define NUM_USERBUTTONS 	2
#define BUTTON0				A4
#define BUTTON1				A5

// CC1101 PIN definitions
#ifdef USE_CC1101
#define CC1101_SPI_USART    1  // not used
#define CC1101_SPI_BAUDRATE 8  // divider (efm32gg: 6000000)
#define CC1101_SPI_LOCATION 1  // not used
#define CC1101_SPI_PIN_CS   A2
#define CC1101_GDO0_PIN     A0
#define CC1101_GDO2_PIN     A1
#endif

//#define HAS_LCD

#endif
