/*
 * trans.h
 *
 *  Created on: 26th of July 2013
 *      Author: Maarten.Weyn@uantwerpen.be
 */

#ifndef PRES_H_
#define PRES_H_


#define FILESYSTEM_NETWORK_CONFIGURATION 			0
#define FILESYSTEM_NETWORK_CONFIGURATION_SIZE 		10
#define FILESYSTEM_DEVICE_FEATURES 					FILESYSTEM_NETWORK_CONFIGURATION + FILESYSTEM_NETWORK_CONFIGURATION_SIZE
#define FILESYSTEM_DEVICE_FEATURES_SIZE 			48
#define FILESYSTEM_CHANNEL_CONFIGURATION			FILESYSTEM_DEVICE_FEATURES + FILESYSTEM_DEVICE_FEATURES_SIZE
#define FILESYSTEM_CHANNEL_CONFIGURATION_SIZE		64
#define FILESYSTEM_REAL_TIME_SCHEDULER				FILESYSTEM_CHANNEL_CONFIGURATION + FILESYSTEM_CHANNEL_CONFIGURATION_SIZE
#define FILESYSTEM_REAL_TIME_SCHEDULER_SIZE			0
#define FILESYSTEM_SLEEP_SCAN_SCHEDULER				FILESYSTEM_REAL_TIME_SCHEDULER + FILESYSTEM_REAL_TIME_SCHEDULER_SIZE
#define FILESYSTEM_SLEEP_SCAN_SCHEDULER_SIZE		32
#define FILESYSTEM_HOLD_SCAN_SCHEDULER				FILESYSTEM_SLEEP_SCAN_SCHEDULER + FILESYSTEM_SLEEP_SCAN_SCHEDULER_SIZE
#define FILESYSTEM_HOLD_SCAN_SCHEDULER_SIZE			32
#define FILESYSTEM_BEACON_TRANSMIT_SERIES			FILESYSTEM_HOLD_SCAN_SCHEDULER + FILESYSTEM_HOLD_SCAN_SCHEDULER_SIZE
#define FILESYSTEM_BEACON_TRANSMIT_SERIES_SIZE		24

#define SETTING_SELECTOR_SLEEP_SCHED 1 << 15
#define SETTING_SELECTOR_HOLD_SCHED 1 << 14
#define SETTING_SELECTOR_BEACON_SCHED 1 << 13
#define SETTING_SELECTOR_GATEWAY 1 << 11
#define SETTING_SELECTOR_SUBCONTR 1 << 10
#define SETTING_SELECTOR_ENDPOINT 1 << 9
#define SETTING_SELECTOR_BLINKER 1 << 8
#define SETTING_SELECTOR_345_WAY_TRANSFER 1 << 7
#define SETTING_SELECTOR_2_WAY_TRANSFER 1 << 6
#define SETTING_SELECTOR_FEC_TX 1 << 5
#define SETTING_SELECTOR_FEC_RX 1 << 4
#define SETTING_SELECTOR_BLINK_CHANNELS 1 << 3
#define SETTING_SELECTOR_HI_RATE_CHANNELS 1 << 2
#define SETTING_SELECTOR_NORMAL_CHANNELS 1 << 1

#include "../types.h"
#include "../hal/system.h"

// ADD .fs			: {} > FLASH to .cmd file to use filesystem

#pragma DATA_SECTION(filesystem, ".fs")

const uint8_t isfb_files[] = {				// ID
		FILESYSTEM_NETWORK_CONFIGURATION, 	// 0x01
		FILESYSTEM_DEVICE_FEATURES,			// 0x02
		FILESYSTEM_CHANNEL_CONFIGURATION,	// 0x0
		FILESYSTEM_REAL_TIME_SCHEDULER,		// 0x0
		FILESYSTEM_SLEEP_SCAN_SCHEDULER,	// 0x0
		FILESYSTEM_HOLD_SCAN_SCHEDULER,		// 0x0
		FILESYSTEM_BEACON_TRANSMIT_SERIES,	// 0x0
};

const uint8_t filesystem[] = {
		/* ID=0x00: network configuration - length = 10 - allocation = 10 */
		0x00,0x00,             						// Virtual ID
	    0xFF,               						// Device Subnet
	    0xFF,       								// Beacon Subnet
	    B00000010,B00000110,  						// Active Setting
	    0x00,               						// Default Device Flags
	    2,                  						// Beacon Redundancy (attempts)
	    SPLITUINT16(0x0002),             			// Hold Scan Sequence Cycles

	    /* ID=0x01: device features - length = 48 - allocation = 48 */
	    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// Device ID
	    B00000010,B00000110,  						// Supported Setting
	    127,										// Max Frame Length
	    1,											// Max Frames Per Packet
	    SPLITUINT16(0),								// DLLS Available Methods
	    SPLITUINT16(0),								// NLS Available Methods
	    SPLITUINT16(0x0600),						// Total ISFB Memory TODO: check this
	    SPLITUINT16(0),								// Available ISFB Memory TODO: check this
	    SPLITUINT16(0),								// Total ISFSB File Memory TODO: check this
	    SPLITUINT16(0),								// Available ISFSB File Memory TODO: check this
	    SPLITUINT16(0),								// Total GFB File Memory TODO: check this
	    SPLITUINT16(0),								// Available GFB File Memory TODO: check this
	    0,											// RFU
	    1,											// Session Stack Depth TODO: check this
	    SPLITUINT16(0x0701),						// Firmware & Version - 0x07: OSS-7 / 0x01: v. 01

	    /* ID=0x02: Channel Configuration - Length = 8 bytes per channel - Allocation = minimum 64 bytes */
	    // Channel 1
	    0x10,                                       // Channel Spectrum ID
	    0x00,                                       // Channel Parameters
	    (uint8_t)(( (-15) + 40 )*2),                // Channel TX Power Limit
	    100,                                        // Channel Link Quality Filter Level
	    (uint8_t)( (-85) + 140 ),                   // CS RSSI Threshold
	    (uint8_t)( (-92) + 140 ),                   // CCA RSSI Threshold
	    0x00,                                       // Regulatory Code
	    0x01,                                       // TX Duty Cycle

	    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Channel 2 - dummy data
	    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Channel 3 - dummy data
	    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Channel 4 - dummy data
	    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Channel 5 - dummy data
	    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Channel 6 - dummy data
	    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Channel 7 - dummy data
	    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Channel 8 - dummy data

	    /* ID=0x03: Real Time Scheduler (Optional) - Length = 12 bytes - Allocation = 12 bytes */
	    // Not used

	    /* ID=0x04:  Sleep Channel Scan Series - Length 4 bytes / channel scan datum - Allocation = min 32 bytes */
	    // Not used in this example
	    0xFF, 0xFF, 0xFF, 0xFF,                      // Channel ID, Scan Code, Next Scan ticks
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,

		/* ID=0x05:  Hold Channel Scan Series - Length 4 bytes / channel scan datum - Allocation = min 32 bytes */
		// Not used in this example
		0xFF, 0xFF, 0xFF, 0xFF,                      // Channel ID, Scan Code, Next Scan ticks
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,

		/* ID=0x06: Beacon Transmit Period List - Length = 8 bytes - Allocation = 24 bytes */
		// Period 1
		0x10, 										// Channel ID
		B00000010, 									// Beacon Command Params -> No Response
		0x20, 0x00, 0x00, 0x08, 					// D7AQP Call Template
		SPLITUINT16(0x0400),						// Next Beacon -> 1024 ticks = 1 s

		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Period 2 - Not Used
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Period 3 - Not Used


};

#endif /* PRES_H_ */
