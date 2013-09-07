/*
 * The PHY layer API
 *  Created on: Nov 22, 2012
 *  Authors:
 * 		maarten.weyn@artesis.be
 *  	glenn.ergeerts@artesis.be
 *  	alexanderhoet@gmail.com
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "phy.h"
#include "../pres/pres.h"

phy_tx_callback_t phy_tx_callback = NULL;
phy_rx_callback_t phy_rx_callback = NULL;

static isfb_channel_configuration current_channel_conf;

// check get config from file system
static bool phy_channel_lookup(uint8_t channel_id);


// set current channel id to non existent
void phy_init(void)
{
	current_channel_conf.channel_spectrum_id = 0xFF;

	phy_ral_init();
}

void phy_set_tx_callback(phy_tx_callback_t cb)
{
	phy_tx_callback = cb;
}
void phy_set_rx_callback(phy_rx_callback_t cb)
{
	phy_rx_callback = cb;
}

bool phy_cca(uint8_t spectrum_id, uint8_t sync_word_class)
{
    return (bool)(phy_get_rssi(spectrum_id, sync_word_class) < CCA_RSSI_THRESHOLD);
}

bool phy_translate_settings(uint8_t spectrum_id, uint8_t sync_word_class, bool* fec, uint8_t* channel_center_freq_index, uint8_t* channel_bandwidth_index, uint8_t* preamble_size, uint16_t* sync_word)
{
	*fec = (bool)spectrum_id >> 7;
	*channel_center_freq_index = spectrum_id & 0x0F;
	*channel_bandwidth_index = (spectrum_id >> 4) & 0x07;

	//Assert valid spectrum id and set preamble size;
	if(*channel_bandwidth_index == 0) {
		if(*channel_center_freq_index == 0 || *channel_center_freq_index == 1)
			*preamble_size = 4;
		else
			return false;
	} else if(*channel_bandwidth_index == 1) {
		if(~*channel_center_freq_index & 0x01)
			*preamble_size = 4;
		else
			return false;
	} else if (*channel_bandwidth_index == 2) {
		if(*channel_center_freq_index & 0x01)
			*preamble_size = 6;
		else
			return false;
	} else if (*channel_bandwidth_index == 3) {
		if(*channel_center_freq_index == 2 || *channel_center_freq_index == 12)
			*preamble_size = 6;
		else
			return false;
	} else {
		return false;
	}

	//Assert valid sync word class and set sync word
	if(sync_word_class == 0) {
		if(*fec == 0)
			*sync_word = SYNC_CLASS0_NON_FEC;
		else
			*sync_word = SYNC_CLASS0_FEC;
	} else if(sync_word_class == 1) {
		if(*fec == 0)
			*sync_word = SYNC_CLASS1_NON_FEC;
		else
			*sync_word = SYNC_CLASS1_FEC;
	} else {
	   return false;
	}

	return true;
}

bool phy_tx_session(session_data* session)
{
	if (!phy_init_tx())
	{
		return false;
	}

	if (!phy_channel_lookup(session->channel_id))
	{
		return false;
	}

	//General configuration
	phy_translate_and_set_settings(current_channel_conf.channel_spectrum_id, session->data_link_control_flags & SESSION_DATA_LINK_CONTROL_FLAGS_FF);
	set_eirp(session->tx_eirp);

	//TODO Return error if fec not enabled but requested
	//TODO is hw dependent? so should move to RAL?
	#ifdef D7_PHY_USE_FEC
	if (fec) {
		//Disable hardware data whitening
		set_data_whitening(false);
	} else {
	#endif
		//Enable hardware data whitening
		set_data_whitening(true);
	#ifdef D7_PHY_USE_FEC
	}
	#endif
}

// check get config from file system
bool phy_channel_lookup(uint8_t channel_id)
{
	uint8_t spectrum_id = channel_id & ~0x80;

	if (spectrum_id == current_channel_conf.channel_spectrum_id) return true;

	file_handler fh;
	uint8_t result = fs_open(&fh, file_system_type_isfb, ISFB_ID(CHANNEL_CONFIGURATION), file_system_user_root, file_system_access_type_read);

	if (result != 0) return false;

	uint8_t pointer = 0;

	isfb_channel_configuration config;
	for (; pointer < fh.file_info->length; pointer += 8)
	{
		result = fs_read_data(&fh, (uint8_t*) &config, pointer, 8);
		if (result != 0) return false;

		if (config.channel_spectrum_id == spectrum_id)
		{
			memcpy((void*) &current_channel_conf, (void*) &config, 8);
			return true;
		}
	}

	return false;
}


int8_t phy_get_max_tx_eirp(uint8_t channel_id)
{
	if (!phy_channel_lookup(channel_id)) return 0xFF;

	return ((int8_t) (current_channel_conf.channel_tx_power_limit * 0x7F)) / 2 - 40;
}
