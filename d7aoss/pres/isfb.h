/*
 * isfb.h
 *
 *  Created on: 1-aug.-2013
 *      Author: Maarten Weyn
 */

#ifndef ISFB_H_
#define ISFB_H_

#define ISFB_ID(VAL)                            ISFB_ID_##VAL
#define ISFB_ID_NETWORK_CONFIGURATION			0x00
#define ISFB_ID_DEVICE_FEATURES					0x01
#define ISFB_ID_CHANNEL_CONFIGURATION			0x02
#define ISFB_ID_REAL_TIME_SCHEDULER				0x03
#define ISFB_ID_SLEEP_SCAN_SCHEDULER			0x04
#define ISFB_ID_HOLD_SCAN_SCHEDULER				0x05
#define ISFB_ID_BEACON_TRANSMIT_SERIES			0x06

#define SETTING_SELECTOR_SLEEP_SCHED 		(uint16_t) (1 << 15)
#define SETTING_SELECTOR_HOLD_SCHED 		(uint16_t) (1 << 14)
#define SETTING_SELECTOR_BEACON_SCHED 		(uint16_t) (1 << 13)
#define SETTING_SELECTOR_GATEWAY 			(uint16_t) (1 << 11)
#define SETTING_SELECTOR_SUBCONTR 			(uint16_t) (1 << 10)
#define SETTING_SELECTOR_ENDPOINT 			(uint16_t) (1 << 9)
#define SETTING_SELECTOR_BLINKER 			(uint16_t) (1 << 8)
#define SETTING_SELECTOR_345_WAY_TRANSFER 	(uint16_t) (1 << 7)
#define SETTING_SELECTOR_2_WAY_TRANSFER 	(uint16_t) (1 << 6)
#define SETTING_SELECTOR_FEC_TX 			(uint16_t) (1 << 5)
#define SETTING_SELECTOR_FEC_RX 			(uint16_t) (1 << 4)
#define SETTING_SELECTOR_BLINK_CHANNELS 	(uint16_t) (1 << 3)
#define SETTING_SELECTOR_HI_RATE_CHANNELS 	(uint16_t) (1 << 2)
#define SETTING_SELECTOR_NORMAL_CHANNELS 	(uint16_t) (1 << 1)

typedef struct
{
	uint8_t vid[2];
	uint8_t device_subnet;
	uint8_t beacon_subnet;
	uint16_t active_settings;
	uint8_t default_frame_config;
	uint8_t beacon_redundancy;
	uint16_t hold_scan_series_cycles;
} isfb_network_configuration;

typedef struct
{
	uint8_t uid[8];
	uint16_t supported_settings;
	uint8_t max_frame_length;
	uint8_t max_frames_per_packet;
	uint16_t dlls_available_methods;
	uint16_t nls_available_methods;
	uint16_t total_isfb_memory;
	uint16_t available_isfb_memory;
	uint16_t total_isfsb_memory;
	uint16_t available_isfsb_memory;
	uint16_t total_gfb_memory;
	uint16_t available_gfb_memory;
	uint16_t gfb_file_size;
	uint8_t  rfu;
	uint8_t  session_stack_depth;
	uint8_t  firmware_version[2];
} isfb_device_parameters;

typedef struct
{
	uint8_t channel_spectrum_id;
	uint8_t channel_parameters;
	uint8_t channel_tx_power_limit;
	uint8_t link_quality_filter_level;
	uint8_t cs_rssi_threashold;
	uint8_t cca_rssi_threshold;
	uint8_t regulartory_code;
	uint8_t tx_duty_cycle;
} isfb_channel_configuration;


typedef struct
{
	uint8_t channel_id;
	uint8_t scan_code;
	uint16_t next_scan;
} isfb_sleep_hold_channel_scan_period;

typedef struct
{
	uint8_t channel_id;
	uint8_t command_params;
	uint8_t d7aqp_call_template[4];
	uint16_t next_beacon;
} isfb_beacon_transmit_period;



#endif /* ISFB_H_ */
