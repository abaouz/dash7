/*
 *  Created on: July 26th, 2013
 *  Authors:
 * 		maarten.weyn@uantwerpen.be
 *
 * 	Test of presentation layer implementation
 *
 * 	add the link to d7aoss library in de lnk_*.cmd file, e.g. -l "../../../d7aoss/Debug/d7aoss.lib"
 * 	Make sure to select the correct platform in d7aoss/hal/cc430/platforms.platform.h
 * 	If your platform is not present, you can add a header file in platforms and commit it to the repository.
 * 	Exclude the stub directories in d7aoss from the build when building for a device.
 *
 * 	ADD .fs			: {} > FLASH to .cmd file to use filesystem
 */


#include <string.h>
#include <d7aoss.h>
#include <hal/system.h>
#include <hal/button.h>
#include <hal/leds.h>
#include <hal/rtc.h>
#include <framework/log.h>
#include <framework/timer.h>
#include <msp430.h>

#include "filesystem.c"


#define SEND_INTERVAL_MS 2000
#define SEND_CHANNEL 0x12
#define TX_EIRP 10

// Macro which can be removed in production environment
#define USE_LEDS


int main(void) {

	// Initialize the OSS-7 Stack
	system_init();

	d7aoss_init(&fs_info);

	log_print_string("presentation layer test started");

	// Log the device id
	log_print_data(device_id, 8);

	file_handler fh;

	uint8_t i = 0;
	uint8_t result;

	for (; i < FILESYSTEM_NR_OF_ISFB_FILES + 1; i++)
	{
		log_print_string("Getting file %x", i);

		result = fs_open(&fh, file_system_type_isfb, i, file_system_user_user, file_system_access_type_read);
		log_print_string("Result: %d", result);

		if (result == 0)
		{
			log_print_data(fh.file, fh.file_info->length);
		}
	}

	while(1)
	{
		system_lowpower_mode(4,1);
	}
}


#pragma vector=ADC12_VECTOR,RTC_VECTOR,AES_VECTOR,COMP_B_VECTOR,DMA_VECTOR,PORT1_VECTOR,PORT2_VECTOR,SYSNMI_VECTOR,UNMI_VECTOR,USCI_A0_VECTOR,USCI_B0_VECTOR,WDT_VECTOR,TIMER0_A0_VECTOR,TIMER1_A1_VECTOR
__interrupt void ISR_trap(void)
{
  /* For debugging purposes, you can trap the CPU & code execution here with an
     infinite loop */
  //while (1);
	__no_operation();

  /* If a reset is preferred, in scenarios where you want to reset the entire system and
     restart the application from the beginning, use one of the following lines depending
     on your MSP430 device family, and make sure to comment out the while (1) line above */

  /* If you are using MSP430F5xx or MSP430F6xx devices, use the following line
     to trigger a software BOR.   */
  PMMCTL0 = PMMPW | PMMSWBOR;          // Apply PMM password and trigger SW BOR
}



