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

#ifndef PLATFORM_EZR32LG_OCTA
    #error Mismatch between the configured platform and the actual platform. Expected PLATFORM_EZR32LG_USB01 to be defined
#endif


#include <ezr32lg_chip.h>

#define HW_USE_HFXO
#define HW_USE_LFXO

/********************
 * LED DEFINITIONS *
 *******************/

#define HW_NUM_LEDS 3
#define LED0	B4
#define	LED1	B5
#define LED2	B6
#define LED_GREEN 2
#define LED_ORANGE 1
#define LED_RED 0


/********************
 *  USB SUPPORT      *
 ********************/

#define USB_DEVICE

/********************
 * UART DEFINITIONS *
 *******************/

// console configuration
#define CONSOLE_UART        PLATFORM_EZR32LG_OCTA_CONSOLE_UART
#define CONSOLE_LOCATION    PLATFORM_EZR32LG_OCTA_CONSOLE_LOCATION
#define CONSOLE_BAUDRATE    PLATFORM_EZR32LG_OCTA_CONSOLE_BAUDRATE

/*******************
 *   VCOM          *
 ******************/



/********************
 * SPI RF DEFINITIONS *
 *******************/

#define si4455_GDO0_PIN A15
#define si4455_GDO1_PIN E14
#define si4455_SDN_PIN  E8

/*************************
 * DEBUG PIN DEFINITIONS *
 ************************/

#define DEBUG_PIN_NUM 0
#define DEBUG0	E0
#define DEBUG1	E1

/**************************
 * USERBUTTON DEFINITIONS *
 *************************/

#define NUM_USERBUTTONS 	2
#define BUTTON0				F5
#define BUTTON1				B3


#endif
