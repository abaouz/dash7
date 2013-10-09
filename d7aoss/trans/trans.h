/*
 * (C) Copyright 2013 University of Antwerp (http://www.cosys-lab.be) and others.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * Contributors:
 *     maarten.weyn@uantwerpen.be
 *     Dragan Subotic
 *
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
