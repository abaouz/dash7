/*
 * nwl.c
 *
 *  Created on: 27-feb.-2013
 *      Author: Maarten Weyn
 */

#include "nwl.h"
#include "../framework/log.h"
#include "../hal/system.h"
#include "../dll/dll.h"

static nwl_rx_callback_t nwl_rx_callback;
static nwl_tx_callback_t nwl_tx_callback;

static void dll_tx_callback(Dll_Tx_Result status)
{
	nwl_tx_callback(status);
}

static void dll_rx_callback(dll_rx_res_t* result)
{
	nwl_rx_res_t res;
	res.frame_type = result->frame_type;

	if (res.frame_type == FrameTypeBackgroundFrame)
	{
		nwl_background_frame_t bf;
		bf.tx_eirp = 0;
		bf.subnet = ((dll_background_frame_t*) result->frame)->subnet;
		bf.bpid = ((dll_background_frame_t*) result->frame)->payload[0];
		memcpy((void*) bf.protocol_data,(void*) &(((dll_background_frame_t*) result->frame)->payload[1]), 2);

		res.frame = &bf;
	}

	nwl_rx_callback(&res);
}

void nwl_init()
{
	dll_set_tx_callback(&dll_tx_callback);
	dll_set_rx_callback(&dll_rx_callback);
}

void nwl_set_tx_callback(nwl_tx_callback_t cb)
{
	nwl_tx_callback = cb;
}

void nwl_set_rx_callback(nwl_rx_callback_t cb)
{
	nwl_rx_callback = cb;
}

void nwl_build_background_frame(nwl_background_frame_t* data, uint8_t spectrum_id)
{
	dll_create_background_frame(&(data->bpid), data->subnet, spectrum_id, data->tx_eirp);
}

void nwl_build_advertising_protocol_data(uint8_t channel_id, uint16_t eta, int8_t tx_eirp, uint8_t subnet)
{
	nwl_background_frame_t frame;
	frame.tx_eirp = tx_eirp;
	frame.subnet = subnet;
	frame.bpid = BPID_AdvP;
	// change to MSB
	frame.protocol_data[0] = eta >> 8;
	frame.protocol_data[1] = eta & 0XFF;

	nwl_build_background_frame(&frame, channel_id);
}

void nwl_build_network_protocol_data(uint8_t* data, uint8_t length, nwl_security* security, nwl_routing_header* routing, uint8_t subnet, uint8_t spectrum_id, int8_t tx_eirp, uint8_t dialog_id)
{
	//TODO: merge to new implementation
	dll_ff_tx_cfg_t dll_params;
	dll_params.eirp = tx_eirp;
	dll_params.spectrum_id = spectrum_id;
	dll_params.subnet = subnet;

	//TODO: get from params
	dll_params.listen = false;
	dll_params.security = NULL;

	dll_foreground_frame_adressing adressing;
	adressing.dialog_id = dialog_id;
	adressing.addressing_option = ADDR_CTL_BROADCAST; //TODO: enable other
	adressing.virtual_id = false;
	adressing.source_id = device_id;
	dll_params.addressing = &adressing;


	if (security != NULL)
	{
		#ifdef LOG_NWL_ENABLED
			log_print_string("NWL: security not implemented");
		#endif

		//dll_params.nwl_security = true;
		dll_params.nwl_security = false;
	} else {
		dll_params.nwl_security = false;
	}

	if (routing != NULL)
	{
		#ifdef LOG_NWL_ENABLED
			log_print_string("NWL: routing not implemented");
		#endif
	}

	//memcpy(&dll_data[offset], data, length);
	queue_push_value(&tx_queue, data, length);

	//TODO: assert dll_data_length < 255-7
	//dll_add_header_footer(&tx_queue, &dll_params);
}

void nwl_build_network_protocol_header(session_data *session, uint8_t addressing, bool nack, uint8_t* destination)
{

	dll_build_foreground_frame_header(session, (d7a_frame_type) nack, addressing, destination);

	#ifdef LOG_NWL_ENABLED
	if (session->data_link_control_flags & ADDR_CTL_NLS)
	{
		log_print_string("NWL: security not implemented");
	}
	#endif

	// routing
	if ((addressing & ADDR_CTL_BROADCAST) == 0x00) // unicast or anycast: enable routing
	{
		#ifdef LOG_NWL_ENABLED
		log_print_string("NWL: multihop not implemented");
		#endif

		// currently multihop is dissabled
		queue_push_u8(&tx_queue, ROUTING_HOP_CONTROL_HOPS_REMAINING(0));
	}

//
//	if (routing != NULL)
//	{
//		#ifdef LOG_NWL_ENABLED
//			log_print_string("NWL: routing not implemented");
//		#endif
//	}

	//memcpy(&dll_data[offset], data, length);
	//queue_push_value(&tx_queue, data, length);

	//TODO: assert dll_data_length < 255-7
	//dll_add_header_footer_from_session(session, &tx_queue);
}

void nwl_build_network_protocol_footer(session_data *session)
{
	#ifdef LOG_NWL_ENABLED
	if (session->data_link_control_flags & ADDR_CTL_NLS)
	{
		log_print_string("NWL: security not implemented");
	}
	#endif

	dll_build_foreground_frame_footer(session);
}

