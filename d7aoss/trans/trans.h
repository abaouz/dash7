/*
 * trans.h
 *
 *  Created on: 21-feb.-2013
 *      Author: Dragan Subotic
 *      		Maarten Weyn
 */

#ifndef TRANS_H_
#define TRANS_H_

#include "../types.h"
#include "../nwl/nwl.h"

#define D7AQP_COMMAND_CODE_EXTENSION		1 << 7
#define D7AQP_COMMAND_TYPE_RESPONSE			0 << 4
#define D7AQP_COMMAND_TYPE_ERROR_RESPONSE	1 << 4
#define D7AQP_COMMAND_TYPE_NA2P_REQUEST		2 << 4
#define D7AQP_COMMAND_TYPE_A2P_INIT_REQUEST	4 << 4
#define D7AQP_COMMAND_TYPE_A2P_INTERMEDIATE_REQUEST	5 << 4
#define D7AQP_COMMAND_TYPE_A2P_FINAL_REQUEST	7 << 4
#define D7AQP_OPCODE_ANNOUNCEMENT_FILE				0
#define D7AQP_OPCODE_ANNOUNCEMENT_FILE_SERIES		1
#define D7AQP_OPCODE_INVENTORY_FILE					2
#define D7AQP_OPCODE_INVENTORY_FILE_SERIES			3
#define D7AQP_OPCODE_COLLECTION_FILE_FILE			4
#define D7AQP_OPCODE_COLLECTION_SERIES_FILE			5


typedef enum {
	TransPacketSent,
	TransPacketFail,
	TransTCAFail
} Trans_Tx_Result;


typedef void (*trans_tx_callback_t)(Trans_Tx_Result);
void trans_init();

void trans_set_tx_callback(trans_tx_callback_t);
void trans_set_initial_t_ca(uint16_t t_ca);


void trans_tx_foreground_frame(uint8_t* data, uint8_t length, uint8_t subnet, uint8_t spectrum_id, int8_t tx_eirp);
//void trans_tx_background_frame(uint8_t* data, uint8_t subnet, uint8_t spectrum_id, int8_t tx_eirp);


void trans_rigd_ccp(uint8_t spectrum_id, bool init_status, bool wait_for_t_ca_timeout);

#endif /* TRANS_H_ */
