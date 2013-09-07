/*
 * session.h
 *
 *  Created on: 2-aug.-2013
 *      Author: Maarten Weyn
 */

#ifndef SESSION_H_
#define SESSION_H_

#include "../types.h"

#define SESSION_DATA_LINK_CONTROL_FLAGS_LISTEN	1 << 7
#define SESSION_DATA_LINK_CONTROL_FLAGS_VID		1 << 6
#define SESSION_DATA_LINK_CONTROL_FLAGS_DLLS	1 << 5
#define SESSION_DATA_LINK_CONTROL_FLAGS_NLS		1 << 4
#define SESSION_DATA_LINK_CONTROL_FLAGS_FF		1 << 3 // not in spec
#define SESSION_DATA_LINK_CONTROL_FLAGS_BF		0 << 3 // not in spec

typedef enum
{
	network_state_unassociated,
	network_state_scheduled,
	network_state_promiscuous,
	network_state_associated
} network_state_type;

typedef struct
{
	uint16_t counter_until_scheduled;
	network_state_type network_state; //unassociated, scheduled, promiscuous, associated
	uint8_t channel_id;
	uint8_t protocol_control_data;
	int8_t tx_eirp; // not according to current spec
	uint8_t subnet;
	uint8_t dialog_id;
	uint8_t data_link_control_flags; // Listen, DLLS, VID, NLS
} session_data;



void session_init();
session_data* session_new(uint16_t counter, uint8_t channel);
void session_pop();
session_data* session_top();

#endif /* SESSION_H_ */
