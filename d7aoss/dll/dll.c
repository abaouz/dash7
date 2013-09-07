/*
 * The PHY layer API
 *  Created on: Nov 23, 2012
 *  Authors:
 * 		maarten.weyn@artesis.be
 *  	glenn.ergeerts@artesis.be
 */
#include "dll.h"
#include "../framework/timer.h"
#include "../hal/system.h"
#include "../hal/crc.h"
#include "../framework/log.h"
#include "../pres/pres.h"
#include "../nwl/nwl.h"
#include "../trans/trans.h"
#include "../system_core.h"
#include <string.h>

static dll_rx_callback_t dll_rx_callback;
static dll_tx_callback_t dll_tx_callback;
static dll_tx_callback_t dll_tx_callback_temp;
static dll_rx_res_t dll_res;

static dll_channel_scan_series_t* current_css;
static uint8_t current_scan_id = 0;

static isfb_network_configuration network_config;
static uint8_t* uid;

static phy_tx_cfg_t *current_phy_cfg;


uint8_t timeout_listen; // TL
uint8_t frame_data[255]; // TODO max frame size
uint16_t timestamp;
uint8_t timeout_ca; 	// T_ca

Dll_State_Enum dll_state;


//Scan parameters
int16_t scan_minimum_energy = -140; // E_sm
uint16_t background_scan_detection_timeout;
uint16_t foreground_scan_detection_timeout;
uint8_t spectrum_id = 0;


phy_tx_cfg_t foreground_frame_tx_cfg = {
            0x10, 	// spectrum ID
			1, 		// Sync word class
			0,		// Transmission power level in dBm ranged [-39, +10]
			0,		// Packet length
            frame_data	//Packet data
};

phy_tx_cfg_t background_frame_tx_cfg = {
		0x10, 	// spectrum ID
		0, 		// Sync word class
		0,		// Transmission power level in dBm ranged [-39, +10]
		6,
		frame_data	//Packet data
};

static bool check_subnet(uint8_t device_subnet, uint8_t frame_subnet);
static void scan_next(void* arg);
static void scan_timeout();
static void tx_callback();
static void rx_callback(phy_rx_data_t* res);
static void dll_check_for_automated_processes();

void dll_init()
{
	phy_set_rx_callback(rx_callback);
	phy_set_tx_callback(tx_callback);

	dll_state = DllStateNone;


	// Get Configuration Data
	file_handler fh;
	uint8_t result = fs_open(&fh, file_system_type_isfb, ISFB_ID(NETWORK_CONFIGURATION), file_system_user_root, file_system_access_type_read);

	network_config.vid[1] = fs_read_byte(&fh, 0);
	network_config.vid[0] = fs_read_byte(&fh, 1);
	network_config.device_subnet = fs_read_byte(&fh, 2);
	network_config.beacon_subnet = fs_read_byte(&fh, 3);
	network_config.active_settings = fs_read_short(&fh, 4);
	network_config.default_frame_config = fs_read_byte(&fh, 6);
	network_config.beacon_redundancy = fs_read_byte(&fh, 7);
	// TODO: byte order is device dependent, D7 is big endian, CC430 is little endian -> move to hal?
	network_config.hold_scan_series_cycles = SWITCH_BYTES(fs_read_short(&fh, 8));

	fs_close(&fh);

	result = fs_open(&fh, file_system_type_isfb, ISFB_ID(DEVICE_FEATURES), file_system_user_root, file_system_access_type_read);
	if (result != 0 || fh.file_info->length == 0)
	{
		// Should not happen!!!
		// TODO: how to resolve, this means there is no filesystem.
	} else {
		uid = fh.file; // uid pointer now points to uid in filesystem (first 8 bytes of device_featuers
	}

	fs_close(&fh);

	dll_check_for_automated_processes();
}

void dll_set_tx_callback(dll_tx_callback_t cb)
{
	dll_tx_callback = cb;
}
void dll_set_rx_callback(dll_rx_callback_t cb)
{
	dll_rx_callback = cb;
}

void dll_set_scan_minimum_energy(int16_t e_sm)
{
	scan_minimum_energy = e_sm; // E_sm
}

void dll_set_background_scan_detection_timeout(uint16_t t_bsd)
{
	background_scan_detection_timeout = t_bsd;
}

void dll_set_foreground_scan_detection_timeout(uint16_t t_fsd)
{
	foreground_scan_detection_timeout = t_fsd;
}

void dll_set_scan_spectrum_id(uint8_t spect_id)
{
	spectrum_id = spect_id;
}

void dll_stop_channel_scan()
{
	// TODO remove scan_timeout events from queue?
	current_css = NULL;
	dll_state = DllStateNone;
	phy_idle();
}

void dll_background_scan()
{
	#ifdef LOG_DLL_ENABLED
		log_print_string("DLL Starting background scan");
	#endif

	dll_state = DllStateScanForegroundFrame;

	//check for signal detected above E_sm
	// TODO: is this the best method?
	if (phy_get_rssi(spectrum_id, 0) <= scan_minimum_energy)
	{
		#ifdef LOG_DLL_ENABLED
			log_print_string("DLL No signal deteced");
		#endif
		return;
	}

	phy_rx_cfg_t rx_cfg;
	rx_cfg.length = 0;
	rx_cfg.timeout = background_scan_detection_timeout; // timeout
	rx_cfg.spectrum_id = spectrum_id; // spectrum ID
	rx_cfg.scan_minimum_energy = scan_minimum_energy;
	rx_cfg.sync_word_class = 0;

	current_css = NULL;

	#ifdef LOG_DLL_ENABLED
	bool phy_rx_result = phy_rx(&rx_cfg);
	if (!phy_rx_result)
	{
		log_print_string("DLL Starting channel scan FAILED");
	}
	#else
	phy_rx(&rx_cfg);
	#endif
}
void dll_foreground_scan()
{
	#ifdef LOG_DLL_ENABLED
		log_print_string("Starting foreground scan");
	#endif

	dll_state = DllStateScanForegroundFrame;

	phy_rx_cfg_t rx_cfg;
	rx_cfg.length = 0;
	rx_cfg.timeout = foreground_scan_detection_timeout; // timeout
	rx_cfg.spectrum_id = spectrum_id; // spectrum ID
	rx_cfg.scan_minimum_energy = scan_minimum_energy;
	rx_cfg.sync_word_class = 1;

	current_css = NULL;

	#ifdef LOG_DLL_ENABLED
	bool phy_rx_result = phy_rx(&rx_cfg);
	if (!phy_rx_result)
	{
		log_print_string("DLL Starting channel scan FAILED");
	}
	#else
	phy_rx(&rx_cfg);
	#endif
}

void dll_channel_scan_series(dll_channel_scan_series_t* css)
{
	#ifdef LOG_DLL_ENABLED
		log_print_string("DLL Starting channel scan series");
	#endif

	phy_rx_cfg_t rx_cfg;
	rx_cfg.length = 0;
	rx_cfg.timeout = css->values[current_scan_id].timeout_scan_detect; // timeout
	//rx_cfg.multiple = 0; // multiple TODO
	rx_cfg.spectrum_id = css->values[current_scan_id].spectrum_id; // spectrum ID TODO
	//rx_cfg.coding_scheme = 0; // coding scheme TODO
	rx_cfg.scan_minimum_energy = scan_minimum_energy;
	if (css->values[current_scan_id].scan_type == FrameTypeForegroundFrame)
	{
		rx_cfg.sync_word_class = 1;
		dll_state = DllStateScanForegroundFrame;
	} else {
		rx_cfg.sync_word_class = 0;
		dll_state = DllStateScanBackgroundFrame;
	}

	current_css = css;


	#ifdef LOG_DLL_ENABLED
	bool phy_rx_result = phy_rx(&rx_cfg);
	if (!phy_rx_result)
	{
		log_print_string("DLL Starting channel scan FAILED");
	}
	#else
	phy_rx(&rx_cfg);
	#endif
}

static void dll_cca2(void* arg)
{
	bool cca2 = phy_cca(current_phy_cfg->spectrum_id, current_phy_cfg->sync_word_class);;
	if (!cca2)
	{
		dll_tx_callback(DLLTxResultCCA2Fail);
		return;
	}

	dll_tx_callback(DLLTxResultCCAOK);
}

void dll_tx_frame()
{
	if (!phy_tx(current_phy_cfg))
	{
		dll_tx_callback(DLLTxResultFail);
	}
}

void dll_csma(bool enabled)
{
	if (!enabled)
	{
		dll_tx_callback(DLLTxResultCCAOK);
		return;
	}

	bool cca1 = phy_cca(current_phy_cfg->spectrum_id, current_phy_cfg->sync_word_class);

	if (!cca1)
	{
		dll_tx_callback(DLLTxResultCCA1Fail);
		return;
	}

	timer_event event;
	event.next_event = 5; // TODO: get T_G from config
	event.f = &dll_cca2;

	if (!timer_add_event(&event))
	{
		dll_tx_callback(DLLTxResultFail);
		return;
	}
}


void dll_ca_callback(Dll_Tx_Result result)
{
	dll_tx_callback = dll_tx_callback_temp;

	if (result == DLLTxResultCCAOK)
	{
		dll_tx_callback(DLLTxResultCCAOK);
	} else
	{

        uint16_t new_time = timer_get_counter_value();
		uint16_t diff = (new_time - timestamp) >> 6;
		if (diff < (timeout_ca - 5))
		{
			timeout_ca-= diff;
			dll_ca(timeout_ca);
		} else {
			dll_tx_callback(DLLTxResultCAFail);
		}
	}
}

void dll_ca(uint8_t t_ca)
{
	dll_tx_callback_temp = dll_tx_callback;
	dll_tx_callback = dll_ca_callback;

	timeout_ca = t_ca;
    timestamp = timer_get_counter_value();
	dll_csma(true);
}



void dll_create_foreground_frame(uint8_t* data, uint8_t length, dll_ff_tx_cfg_t* params)
{
	//TODO: check if in idle state
	foreground_frame_tx_cfg.spectrum_id = params->spectrum_id; // TODO check valid (should be done in the upper layer of stack)
	foreground_frame_tx_cfg.eirp = params->eirp;
	foreground_frame_tx_cfg.sync_word_class = 1;

	dll_foreground_frame_t* frame = (dll_foreground_frame_t*) frame_data;
	frame->frame_header.tx_eirp = (params->eirp + 40) * 2; // (-40 + 0.5n) dBm
	frame->frame_header.subnet = params->subnet;
	frame->frame_header.frame_ctl = 0;

	if (params->listen) frame->frame_header.frame_ctl |= FRAME_CTL_LISTEN;

	if (params->security != NULL)
	{
		#ifdef LOG_DLL_ENABLED
			log_print_string("DLL: security not implemented");
		#endif
		//frame->frame_header.frame_ctl |= FRAME_CTL_DLLS;
	}

	uint8_t* pointer = frame_data + 1 + sizeof(dll_foreground_frame_header_t);

	if (params->addressing != NULL)
	{
		frame->frame_header.frame_ctl |= FRAME_CTL_EN_ADDR;

		dll_foreground_frame_address_ctl_t address_ctl;
		address_ctl.dialogId = params->addressing->dialog_id;
		address_ctl.flags = params->addressing->addressing_option;
		if (params->addressing->virtual_id) address_ctl.flags |= ADDR_CTL_VID;
		if (params->nwl_security) address_ctl.flags |= ADDR_CTL_NLS;

		memcpy(pointer, &address_ctl, 2);
		pointer += 2;

		uint8_t address_length = params->addressing->virtual_id ? 2 : 8;
		memcpy(pointer, params->addressing->source_id, address_length);
		pointer += address_length;

		if (params->addressing->addressing_option == ADDR_CTL_UNICAST && !params->nwl_security)
		{
			memcpy(pointer, params->addressing->target_id, address_length);
			pointer += address_length;
		}
	}

	if (params->frame_continuity) frame->frame_header.frame_ctl |= FRAME_CTL_FR_CONT;

	// CRC32 not implemented
	// frame->frame_header.frame_ctl |= FRAME_CTL_CRC32;

	frame->frame_header.frame_ctl |= params->frame_type;

	*pointer++ = 0; //dunno
	*pointer++ = 0; //isfid
	*pointer++ = 0; //isfoffset
	*pointer++ = length; // payload length;


	memcpy(pointer, data, length); // TODO fixed size for now
	pointer += length;

	frame->length = (pointer - frame_data) + 2;  // length includes CRC

	uint16_t crc16 = crc_calculate(frame_data, frame->length - 2);
	memcpy(pointer, &crc16, 2);

	foreground_frame_tx_cfg.length = frame->length;

	current_phy_cfg = &foreground_frame_tx_cfg;
}

void dll_create_background_frame(uint8_t* data, uint8_t subnet, uint8_t spectrum_id, int8_t tx_eirp)
{
	background_frame_tx_cfg.spectrum_id = spectrum_id;
	background_frame_tx_cfg.eirp = tx_eirp;
	background_frame_tx_cfg.length = 6;

	dll_background_frame_t* frame = (dll_background_frame_t*) frame_data;
	frame->subnet = subnet;
	memcpy(frame->payload, data, 3);

	uint8_t* pointer = frame_data + 4;

	uint16_t crc16 = crc_calculate(frame_data, 4);
	memcpy(pointer, &crc16, 2);

	current_phy_cfg = &background_frame_tx_cfg;
}

void dll_create_beacon(task *beacon_task)
{
	if (network_config.beacon_redundancy == 0) return;

	// Get Configuration Data
	file_handler fh;
	uint8_t result = fs_open(&fh, file_system_type_isfb, ISFB_ID(BEACON_TRANSMIT_SERIES), file_system_user_root, file_system_access_type_read);
	if (result != 0 || fh.file_info->length == 0) return;



	uint8_t channel_id = fs_read_byte(&fh, beacon_task->progress);
	session_data* session = session_new(0, channel_id);
	uint8_t beacon_command_params = fs_read_byte(&fh, beacon_task->progress + 1);
	session->data_link_control_flags = beacon_command_params & B00110000;
	session->data_link_control_flags |= network_config.default_frame_config & B11000000;
	session->data_link_control_flags |= SESSION_DATA_LINK_CONTROL_FLAGS_FF;
	session->subnet = network_config.beacon_subnet;
	session->tx_eirp = phy_get_max_tx_eirp(channel_id);

	if (session->tx_eirp == 0xFF) // channel is not configured in file system
	{
		session_pop();
		return;
	}

	uint8_t isf_call[4];
	fs_read_data(&fh, isf_call, beacon_task->progress + 2, 4);

	uint16_t next_scan = SWITCH_BYTES(fs_read_short(&fh, beacon_task->progress + 6)); //TODO switch_bytes should only be done in a little endian system such as cc430

	fs_close(&fh);

	beacon_task->scheduled_at = timer_get_counter_value() + next_scan;

	beacon_task->progress += 8;

	if (beacon_task->progress >= beacon_task->size) beacon_task->progress = 0;


	// Build packet
	nwl_build_network_protocol_header(session, ADDR_CTL_BROADCAST, false, NULL);


	// Query
	uint8_t da7qp_cmd_code = D7AQP_COMMAND_TYPE_NA2P_REQUEST;
	if (beacon_command_params & B00000001) da7qp_cmd_code |= D7AQP_OPCODE_ANNOUNCEMENT_FILE_SERIES;
	if (beacon_command_params & B00000110 != 0) da7qp_cmd_code |= D7AQP_COMMAND_CODE_EXTENSION;

	queue_push_u8(&tx_queue, da7qp_cmd_code); // Write command code

	if (da7qp_cmd_code & D7AQP_COMMAND_CODE_EXTENSION)
	{
		queue_push_u8(&tx_queue, beacon_command_params & B00000110); // Write extension
	}

	// Query - Dialog Template
	uint16_t response_timeout = 10; // TODO: set to default based on channel
	if (beacon_command_params & B00000010) // no response
		response_timeout = 0;

	queue_push_u16(&tx_queue, SWITCH_BYTES(response_timeout)); //TODO: make switch hardware dependent
	queue_push_u8(&tx_queue, 0); // Currently default response channel is same as transmit channel

	// Query - Command Data
	// ISF Call -> single file  return template
	uint8_t max_bytes = isf_call[0];
	uint8_t isf_id = isf_call[1];

	queue_push_u8(&tx_queue, isf_id); // return file id

	// Currently only files, no series
	result = fs_open(&fh, file_system_type_isfb, isf_id, file_system_user_guest, file_system_access_type_read);
	if (result != 0 || fh.file_info->length == 0)
	{
		session_pop();
		return;
	}


	uint16_t offset = MERGEUINT16(isf_call[3], isf_call[2]);
	queue_push_u8(&tx_queue, (uint8_t) offset);  // file offset
	queue_push_u8(&tx_queue, fh.file_info->length);

	uint8_t data_offset = offset;
	uint8_t data_length = fh.file_info->length - data_offset;
	if (data_length > max_bytes) data_length = max_bytes;
	for (; data_offset < offset + data_length; data_offset++)
	{
		queue_push_u8(&tx_queue, fs_read_byte(&fh, data_offset));
	}

	nwl_build_network_protocol_footer(session);



}

void dll_build_foreground_frame_header(session_data *session, d7a_frame_type frame_type, uint8_t addressing, uint8_t* destination)
{
	// Length
	queue_set(&tx_queue, 1); // first byte will be used for the length

	// TX_EIRP - (-40 + 0.5n) dBm
	queue_push_u8(&tx_queue, (0 + 40) * 2); //where to get the eirp from?

	// Subnet
	queue_push_u8(&tx_queue, session->subnet);

	// Frame Control
	uint8_t frame_ctrl = session->data_link_control_flags & B10000000; // should be B11000000; but DLLS is currently not supported
	frame_ctrl |= frame_type; // Frame Type
	frame_ctrl |= FRAME_CTL_EN_ADDR; // Enable addressing, when not to enable this?
	// Frame continuity is currently not implemented

	queue_push_u8(&tx_queue, frame_ctrl);

	// DLLS is currently not supported!
	#ifdef LOG_DLL_ENABLED
	if (frame_ctrl & FRAME_CTL_DLLS)
	{
		log_print_string("DLL: security not implemented");
	}
	#endif

	// Address Control - Dialog ID
	queue_push_u8(&tx_queue, session->dialog_id);

	// Address Control - Flags
	uint8_t address_ctrl = addressing;
	address_ctrl |= session->data_link_control_flags & B00100000; // should be B00110000 but NLS is currently not supported
	queue_push_u8(&tx_queue, address_ctrl);

	uint8_t address_length = 2;

	if (address_ctrl & ADDR_CTL_VID)
	{
		// VID
		queue_push_value(&tx_queue, network_config.vid, 2);
	} else {
		// UID
		queue_push_value(&tx_queue, uid, 8);
		address_length = 8;
	}

	// if unicast and no network layer security (if NLS -> destination address is encoded in network layer)
	if (!(address_ctrl & ADDR_CTL_NLS) && (address_ctrl & ADDR_CTL_UNICAST))
	{
		queue_push_value(&tx_queue, destination, address_length);
	}
}

void dll_build_foreground_frame_footer(session_data *session)
{
	// DLLS is currently not supported!
//	if (session->data_link_control_flags & FRAME_CTL_DLLS)
//	{
//
//	}

}

void dll_tx_session()
{
	session_data* session = session_top();

	// do csma first

	if (!phy_tx_session(session))
	{
		dll_tx_callback(DLLTxResultFail);
	}

}


static bool check_subnet(uint8_t device_subnet, uint8_t frame_subnet)
{
	// FFS = 0xF?
	if (frame_subnet & 0xF0 != 0xF0)
	{
		// No -> FSS = DSS?
		if (frame_subnet & 0xF0 != device_subnet & 0xF0)
			return 0;
	}

	uint8_t fsm = frame_subnet & 0x0F;
	uint8_t dsm = device_subnet & 0x0F;

	// FSM & DSM = DSM?
	if ((fsm & dsm) == dsm)
			return 1;

	return 0;
}

static void scan_next(void* arg)
{
	dll_channel_scan_series(current_css);
}

static void scan_timeout()
{
	if (dll_state == DllStateNone)
		return;

	#ifdef LOG_DLL_ENABLED
		log_print_string("DLL scan time-out");
	#endif
	phy_idle();

	if (current_css == NULL)
	{
		return;
	}

	//Channel scan series
	timer_event event;
	event.next_event = current_css->values[current_scan_id].time_next_scan;
	event.f = &scan_next;

	current_scan_id = current_scan_id < current_css->length - 1 ? current_scan_id + 1 : 0;

	timer_add_event(&event);
}

static void tx_callback()
{
	#ifdef LOG_DLL_ENABLED
		log_print_string("DLL TX OK");
	#endif
	dll_tx_callback(DLLTxResultOK);
}

static void rx_callback(phy_rx_data_t* res)
{
	//log_packet(res->data);
	if (res == NULL)
	{
		scan_timeout();
		return;
	}

	// Data Link Filtering
	// Subnet Matching do not parse it yet
	if (dll_state == DllStateScanBackgroundFrame)
	{
		uint16_t crc = crc_calculate(res->data, 4);
		if (memcmp((uint8_t*) &(res->data[4]), (uint8_t*) &crc, 2) != 0)
		{
			#ifdef LOG_DLL_ENABLED
				log_print_string("DLL CRC ERROR");
			#endif
			scan_next(NULL); // how to reïnitiate scan on CRC Error, PHY should stay in RX
			return;
		}

		if (!check_subnet(0xFF, res->data[0])) // TODO: get device_subnet from datastore
		{
			#ifdef LOG_DLL_ENABLED
				log_print_string("DLL Subnet mismatch");
			#endif
			scan_next(NULL); // how to reïnitiate scan on subnet mismatch, PHY should stay in RX
			return;
		}
	} else if (dll_state == DllStateScanForegroundFrame)
	{
		uint16_t crc = crc_calculate(res->data, res->length - 2);
		if (memcmp((uint8_t*) &(res->data[res->length - 2]), (uint8_t*) &crc, 2) != 0)
		{
			#ifdef LOG_DLL_ENABLED
				log_print_string("DLL CRC ERROR");
			#endif
			scan_next(NULL); // how to reïnitiate scan on CRC Error, PHY should stay in RX
			return;
		}
		if (!check_subnet(0xFF, res->data[2])) // TODO: get device_subnet from datastore
		{
			#ifdef LOG_DLL_ENABLED
				log_print_string("DLL Subnet mismatch");
			#endif
				scan_next(NULL); // how to reïnitiate scan on subnet mismatch, PHY should stay in RX

			return;
		}
	} else
	{
		#ifdef LOG_DLL_ENABLED
			log_print_string("DLL You fool, you can't be here");
		#endif
	}

	// Optional Link Quality Assessment

	// parse packet
	dll_res.rssi = res->rssi;
	dll_res.lqi = res->lqi;
	dll_res.spectrum_id = current_css->values[current_scan_id].spectrum_id;

	if (dll_state == DllStateScanBackgroundFrame)
	{
		dll_background_frame_t* frame = (dll_background_frame_t*)frame_data;
		frame->subnet = res->data[0];
		memcpy(frame->payload, res->data+1, 4);

		dll_res.frame_type = FrameTypeBackgroundFrame;
		dll_res.frame = frame;
	}
	else
	{
		dll_foreground_frame_t* frame = (dll_foreground_frame_t*)frame_data;
		frame->length = res->data[0];

		frame->frame_header.tx_eirp = res->data[1] * 0.5 - 40;
		frame->frame_header.subnet = res->data[2];
		frame->frame_header.frame_ctl = res->data[3];

		uint8_t* data_pointer = res->data + 4;

		if (frame->frame_header.frame_ctl & FRAME_CTL_LISTEN) // Listen
			timeout_listen = 10;
		else
			timeout_listen = 0;

		if (frame->frame_header.frame_ctl & FRAME_CTL_DLLS) // DLLS present
		{
			// TODO parse DLLS Header
			frame->dlls_header = NULL;
		} else {
			frame->dlls_header = NULL;
		}

		if (frame->frame_header.frame_ctl & 0x20) // Enable Addressing
		{
			// Address Control Header
			dll_foreground_frame_address_ctl_t address_ctl;// = (dll_foreground_frame_address_ctl_t*) data_pointer;
			frame->address_ctl = &address_ctl;
			frame->address_ctl->dialogId = *data_pointer;
			data_pointer++;
			frame->address_ctl->flags = *data_pointer;
			data_pointer++;
			//data_pointer += sizeof(uint8_t*);

			uint8_t addressing = (frame->address_ctl->flags & 0xC0) >> 6;
			uint8_t vid = (frame->address_ctl->flags & 0x20) >> 5;
			uint8_t nls = (frame->address_ctl->flags & 0x10) >> 4;
			// TODO parse Source ID Header

			frame->address_ctl->source_id = data_pointer;
			if (vid)
			{
				data_pointer += 2;
			}
			else
			{
				data_pointer += 8;
			}

			if (addressing == 0 && nls == 0)
			{
				uint8_t id_target[8];
				if (vid)
				{
					memcpy(data_pointer, &id_target, 2);
					data_pointer += 2;
				}
				else
				{
					memcpy(data_pointer, &id_target, 8);
					data_pointer += 8;
				}
				frame->address_ctl->target_id = (uint8_t*) &id_target;
			} else {
				frame->address_ctl->target_id = NULL;
			}
		} else {
			frame->address_ctl = NULL;
			frame->address_ctl->source_id = NULL;
		}

		if (frame->frame_header.frame_ctl & 0x10) // Frame continuity
		{
			// TODO handle more than 1 frame
		}

		if (frame->frame_header.frame_ctl & 0x08) // CRC 32
		{
			// TODO support CRC32
		}

		if (frame->frame_header.frame_ctl & 0x04) // Note Mode 2
		{
			// Not supported
		}

		// Frame Type
		//u8 ffType = frame_header.frame_ctl & 0x03;

		data_pointer++; // TODO what is this?
		data_pointer++; //isfid
		data_pointer++; //isfoffset

		frame->payload_length = (*data_pointer);
		data_pointer++;
		frame->payload = data_pointer;

		dll_res.frame_type = FrameTypeForegroundFrame;
		dll_res.frame = frame;
	}


	#ifdef LOG_DLL_ENABLED
		log_dll_rx_res(&dll_res);
	#endif
	dll_rx_callback(&dll_res);


	if (current_css == NULL)
	{
		log_print_string(("DLL no series so stop listening"));
		return;
	}

	// in current spec reset channel scan
	#ifdef LOG_DLL_ENABLED
		log_print_string(("DLL restart channel scan series"));
	#endif

	current_scan_id = 0;
	scan_next(NULL);
}


static void dll_check_for_automated_processes()
{
	if (network_config.active_settings & SETTING_SELECTOR_SLEEP_SCHED)
	{

	}

	if (network_config.active_settings & SETTING_SELECTOR_HOLD_SCHED)
	{

	}

	if ((network_config.active_settings & SETTING_SELECTOR_BEACON_SCHED) && network_config.beacon_redundancy > 0)
	{
		// Get Configuration Data
		file_handler fh;
		uint8_t result = fs_open(&fh, file_system_type_isfb, ISFB_ID(BEACON_TRANSMIT_SERIES), file_system_user_root, file_system_access_type_read);

		if (result == 0 && fh.file_info->length != 0)
		{
			system_core_set_task(TASK_BEACON,0, fh.file_info->length, 0);
		}
	}
}

