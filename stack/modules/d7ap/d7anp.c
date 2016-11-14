/*! \file d7anp.c
 *

 *  \copyright (C) Copyright 2015 University of Antwerp and others (http://oss-7.cosys.be)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  \author glenn.ergeerts@uantwerpen.be
 *
 */

#include "debug.h"
#include "d7anp.h"
#include "packet.h"
#include "fs.h"
#include "ng.h"
#include "log.h"
#include "math.h"
#include "hwdebug.h"
#include "aes.h"

#if defined(FRAMEWORK_LOG_ENABLED) && defined(MODULE_D7AP_NP_LOG_ENABLED)
#define DPRINT(...) log_print_stack_string(LOG_STACK_NWL, __VA_ARGS__)
#define DPRINT_DATA(...) log_print_data(__VA_ARGS__)
#else
#define DPRINT(...)
#define DPRINT_DATA(...)
#endif


typedef enum {
    D7ANP_STATE_IDLE,
    D7ANP_STATE_TRANSMIT,
    D7ANP_STATE_FOREGROUND_SCAN,
} state_t;

static state_t NGDEF(_d7anp_state);
#define d7anp_state NG(_d7anp_state)

static state_t NGDEF(_d7anp_prev_state);
#define d7anp_prev_state NG(_d7anp_prev_state)

static dae_access_profile_t NGDEF(_own_access_profile);
#define own_access_profile NG(_own_access_profile)

static timer_tick_t NGDEF(_fg_scan_timeout_ticks);
#define fg_scan_timeout_ticks NG(_fg_scan_timeout_ticks)

static d7anp_node_t NGDEF(_trusted_node_table)[MODULE_D7AP_TRUSTED_NODE_TABLE_SIZE];
#define trusted_node_table NG(_trusted_node_table)

static d7anp_security_t NGDEF(_security_state);
#define security_state NG(_security_state)

static inline uint8_t get_auth_len(uint8_t nls_method)
{
    switch(nls_method)
    {
    case AES_CTR:
        return 0;
    case AES_CBC_MAC_128:
    case AES_CCM_128:
        return 16;
    case AES_CBC_MAC_64:
    case AES_CCM_64:
        return 8;
    case AES_CBC_MAC_32:
    case AES_CCM_32:
        return 4;
    default:
        assert(false);
    }
}

static void switch_state(state_t next_state)
{
    switch(next_state)
    {
        case D7ANP_STATE_TRANSMIT:
          assert(d7anp_state == D7ANP_STATE_IDLE ||
                 d7anp_state == D7ANP_STATE_FOREGROUND_SCAN);
          d7anp_state = next_state;
          DPRINT("Switched to D7ANP_STATE_TRANSMIT");
          break;
        case D7ANP_STATE_IDLE:
          assert(d7anp_state == D7ANP_STATE_TRANSMIT ||
                 d7anp_state == D7ANP_STATE_FOREGROUND_SCAN);
          d7anp_state = next_state;
          DPRINT("Switched to D7ANP_STATE_IDLE");
          break;
        case D7ANP_STATE_FOREGROUND_SCAN:
          assert(d7anp_state == D7ANP_STATE_TRANSMIT ||
                 d7anp_state == D7ANP_STATE_IDLE);

          d7anp_state = next_state;
          DPRINT("Switched to D7ANP_STATE_FOREGROUND_SCAN");
          break;
    }

    // output state on debug pins
    d7anp_state == D7ANP_STATE_FOREGROUND_SCAN? DEBUG_PIN_SET(3) : DEBUG_PIN_CLR(3);
}

static void foreground_scan_expired()
{
    // the FG scan expiration may also happen while Tx is busy (d7anp_state = D7ANP_STATE_TRANSMIT) // TODO validate
    assert(d7anp_state == D7ANP_STATE_FOREGROUND_SCAN || d7anp_state == D7ANP_STATE_TRANSMIT);
    DPRINT("Foreground scan expired");

    if(d7anp_state == D7ANP_STATE_FOREGROUND_SCAN) // when in D7ANP_STATE_TRANSMIT d7anp_signal_packet_transmitted() will switch state
      switch_state(D7ANP_STATE_IDLE);

    /* switch to automation scan */
    dll_stop_foreground_scan(true);
    d7atp_signal_foreground_scan_expired();
}

static void schedule_foreground_scan_expired_timer()
{
    // TODO in case of responder timeout_ticks counts from reception time , so subtract time passed between now and reception time
    // in case of requester timeout_ticks counts from transmission time, so subtract time passed between now and transmission time
    // since this FG scan is started directly from the ISR (transmitted callback), I don't expect a significative delta between now and the transmission time

    DPRINT("starting foreground scan expiration timer (%i ticks)", fg_scan_timeout_ticks);
    assert(timer_post_task_delay(&foreground_scan_expired, fg_scan_timeout_ticks) == SUCCESS);
}

void d7anp_start_foreground_scan()
{
    if(fg_scan_timeout_ticks >= 0)
    {
        // if the FG scan timer is already set, update only the tl timeout value
        schedule_foreground_scan_expired_timer();

        if (d7anp_state != D7ANP_STATE_FOREGROUND_SCAN)
        {
            switch_state(D7ANP_STATE_FOREGROUND_SCAN);
            dll_start_foreground_scan();
        }
    }
}

void d7anp_set_foreground_scan_timeout(timer_tick_t timeout)
{
    DPRINT("Set FG scan timeout = %i", timeout);
    assert(d7anp_state == D7ANP_STATE_IDLE || d7anp_state == D7ANP_STATE_FOREGROUND_SCAN);
    fg_scan_timeout_ticks = timeout;
}

static void cancel_foreground_scan_task()
{
    // task can be scheduled now or in the future, try to cancel both // TODO refactor scheduler API
    timer_cancel_task(&foreground_scan_expired);
    sched_cancel_task(&foreground_scan_expired);
}

void d7anp_stop_foreground_scan(bool auto_scan)
{
    if (d7anp_state == D7ANP_STATE_FOREGROUND_SCAN)
    {
        cancel_foreground_scan_task();
        switch_state(D7ANP_STATE_IDLE);
    }

    /* start the automation scan or set the radio to idle */
    dll_stop_foreground_scan(auto_scan);
}

void d7anp_init()
{
    uint8_t own_access_class = fs_read_dll_conf_active_access_class();
     uint8_t key[AES_BLOCK_SIZE];

    // set early our own acces profile since this information may be needed when receiving a frame
    fs_read_access_class(own_access_class, &own_access_profile);

    d7anp_state = D7ANP_STATE_IDLE;
    fg_scan_timeout_ticks = 0;

    sched_register_task(&foreground_scan_expired);

#if defined(MODULE_D7AP_NLS_ENABLED)
    /*
     * Init Security
     * Read the 128 bits key from the "NWL Security Key" file
     */
    assert (fs_read_nwl_security_key(key) == ALP_STATUS_OK); // TODO permission
    DPRINT("KEY");
    DPRINT_DATA(key, AES_BLOCK_SIZE);
    AES128_init(key);

    /* Read the NWL security parameters */
    fs_read_nwl_security(&security_state);
    DPRINT("Initial Key counter %d", security_state.key_counter);
    DPRINT("Initial Frame counter %ld", security_state.frame_counter);
    for(uint8_t i = 0; i < MODULE_D7AP_TRUSTED_NODE_TABLE_SIZE; i++)
        trusted_node_table[i].used = false;
#endif
}

void d7anp_tx_foreground_frame(packet_t* packet, bool should_include_origin_template, dae_access_profile_t* access_profile, uint8_t slave_listen_timeout_ct)
{
    assert(d7anp_state == D7ANP_STATE_IDLE || d7anp_state == D7ANP_STATE_FOREGROUND_SCAN);

    packet->d7anp_ctrl.hop_enabled = false;

    // we need to switch back to the current state after the transmission procedure
    d7anp_prev_state = d7anp_state;

    if(!should_include_origin_template)
        packet->d7anp_ctrl.origin_addressee_ctrl_id_type = ID_TYPE_BCAST;
    else
    {
        uint8_t vid[2];
        fs_read_vid(vid);
        if(memcmp(vid, (uint8_t[2]){ 0xFF, 0xFF }, 2) == 0)
            packet->d7anp_ctrl.origin_addressee_ctrl_id_type = ID_TYPE_UID;
        else
            packet->d7anp_ctrl.origin_addressee_ctrl_id_type = ID_TYPE_VID;
    }

    packet->d7anp_ctrl.origin_addressee_ctrl_access_class = packet->d7anp_addressee->ctrl.access_class; // TODO validate
    packet->d7anp_listen_timeout = slave_listen_timeout_ct;

#if defined(MODULE_D7AP_NLS_ENABLED)
    packet->d7anp_ctrl.extension = true;
    // For now, hard code the security method
    packet->d7anp_ext.raw = SET_NLS_METHOD(AES_CCM_128);
    packet->d7anp_security.frame_counter = security_state.frame_counter++;
    packet->d7anp_security.key_counter = security_state.key_counter;
    DPRINT("Frame counter %ld", packet->d7anp_security.frame_counter);

    // Update the frame counter in the D7A file
    fs_write_nwl_security(&security_state);
#endif

    switch_state(D7ANP_STATE_TRANSMIT);
    dll_tx_frame(packet, access_profile);
}

void start_foreground_scan_after_D7AAdvP()
{
    switch_state(D7ANP_STATE_FOREGROUND_SCAN);
    dll_start_foreground_scan();
}

static void schedule_foreground_scan_after_D7AAdvP(timer_tick_t eta)
{
    DPRINT("Perform a dll foreground scan at the end of the delay period (%i ticks)", eta);
    assert(timer_post_task_delay(&start_foreground_scan_after_D7AAdvP, eta) == SUCCESS);
}

static inline void write_be32(uint8_t *buf, uint32_t val)
{
    buf[0] = (val >> 24) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[2] = (val >> 8) & 0xff;
    buf[3] = val & 0xff;
}

static inline uint32_t read_be32(const uint8_t *buf)
{
    return ((uint32_t) buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static void build_header(packet_t *packet, uint8_t payload_len, uint8_t *header)
{
    memset(header, 0, AES_BLOCK_SIZE); // for zero padding

    /* the CBC-MAC header is defined according Table 7.6.4.1 */
    header[0] = packet->d7anp_ext.raw;
    header[1] = packet->d7anp_listen_timeout;
    /* When Origin ID is not provided in the NWL frame, it is provided by upper layer.*/
    memcpy( header + 6, packet->origin_access_id, packet->d7anp_ctrl.origin_addressee_ctrl_id_type == ID_TYPE_VID? 2 : 8);
    header[14] = packet->d7anp_ctrl.raw;
    header[15] = payload_len;

    DPRINT("Header for CBC-MAC");
    DPRINT_DATA(header, AES_BLOCK_SIZE);
}

static void build_iv(packet_t *packet, uint8_t payload_len, uint8_t *iv)
{
    memset(iv, 0, AES_BLOCK_SIZE);

    /* AES-CTR/AES-CCM Initialization Vector (IV)*/
    iv[0] = SET_NLS_METHOD(packet->d7anp_ext.nls_method);
    iv[1] = packet->d7anp_security.key_counter;
    write_be32(&iv[2], packet->d7anp_security.frame_counter);
    /* When Origin ID is not provided in the NWL frame, it is provided by upper layer.*/
    memcpy( iv + 6, packet->origin_access_id, packet->d7anp_ctrl.origin_addressee_ctrl_id_type == ID_TYPE_VID? 2 : 8);
    iv[14] = packet->d7anp_ctrl.raw;
    iv[15] = payload_len;

    DPRINT("iv for CTR/CCM");
    DPRINT_DATA(iv, AES_BLOCK_SIZE);
}

uint8_t d7anp_secure_payload(packet_t *packet, uint8_t *payload, uint8_t payload_len)
{
    uint8_t nls_method;
    uint8_t ctr_blk[AES_BLOCK_SIZE];
    uint8_t header[AES_BLOCK_SIZE];
    uint8_t auth[AES_BLOCK_SIZE];
    uint8_t auth_len;

    nls_method = packet->d7anp_ext.nls_method;
    auth_len = get_auth_len(nls_method);

    switch (nls_method)
    {
    case AES_CTR:
        // Build the initial counter block
        build_iv(packet, payload_len, ctr_blk);

        // the encrypted payload replaces the plaintext
        AES128_CTR_encrypt(payload, payload, payload_len, ctr_blk);
        break;
    case AES_CBC_MAC_128:
    case AES_CBC_MAC_64:
    case AES_CBC_MAC_32:
        /* Build the header block to prepend to the payload */
        build_header(packet, payload_len, header);

        /* Compute the CBC-MAC */
        AES128_CBC_MAC(auth, payload, payload_len, header, NULL, 0, auth_len);

        /* Insert the authentication Tag */
        memcpy(payload + payload_len, auth, auth_len);
        break;
    case AES_CCM_128:
    case AES_CCM_64:
    case AES_CCM_32:
        /* For CCM, the same IV is used for the header block and the counter block */
        build_iv(packet, payload_len, header);
        memcpy(ctr_blk, header, AES_BLOCK_SIZE);

        // TODO check that the payload length does not exceed the maximum size
        AES128_CCM_encrypt(payload, payload_len, header, NULL, 0, ctr_blk, auth_len);
        break;
    }

    return auth_len;
}

bool d7anp_unsecure_payload(packet_t *packet, uint8_t index)
{
    uint8_t nls_method;
    uint8_t ctr_blk[AES_BLOCK_SIZE];
    uint8_t header[AES_BLOCK_SIZE];
    uint8_t auth[AES_BLOCK_SIZE];
    uint8_t auth_len;
    uint32_t payload_len;
    uint8_t *tag;

    nls_method = packet->d7anp_ext.nls_method;

    payload_len = packet->hw_radio_packet.length + 1 - index - 2; // exclude the headers CRC bytes // TODO exclude footers
    auth_len = get_auth_len(nls_method); // the authentication length is given in bytes

    /* remove the authentication tag from the payload length if relevant */
    payload_len -= auth_len;

    if (auth_len)
    {
        tag = packet->hw_radio_packet.data + index + payload_len;
        DPRINT("Tag  <%d>", auth_len);
        DPRINT_DATA(tag, auth_len);
    }

    switch (nls_method)
    {
    case AES_CTR:
        /* Build the initial counter block */
        build_iv(packet, payload_len, ctr_blk);

        // the decrypted payload replaces the encrypted data
        AES128_CTR_encrypt(packet->hw_radio_packet.data + index,
                           packet->hw_radio_packet.data + index,
                           payload_len, ctr_blk);
        break;
    case AES_CBC_MAC_128:
    case AES_CBC_MAC_64:
    case AES_CBC_MAC_32:
        /* Build the header block to prepend to the payload */
        build_header(packet, payload_len, header);

        /* Compute the CBC-MAC and check the authentication Tag */
        AES128_CBC_MAC(auth, packet->hw_radio_packet.data + index,
                       payload_len, header, NULL, 0, auth_len);

        if (memcmp(auth, tag, auth_len) != 0)
        {
            DPRINT("CBC-MAC: Auth mismatch");
            return false;
        }
        /* remove the authentication Tag */
        packet->hw_radio_packet.length -= auth_len;

        break;
    case AES_CCM_128:
    case AES_CCM_64:
    case AES_CCM_32:
        /* For CCM, the same IV is used for the header block and the counter block */
        build_iv(packet, payload_len, header);
        memcpy(ctr_blk, header, AES_BLOCK_SIZE);

        if (AES128_CCM_decrypt(packet->hw_radio_packet.data + index,
                               payload_len, header, NULL, 0, ctr_blk,
                               tag, auth_len) != 0)
            return false;

        /* remove the authentication Tag */
        packet->hw_radio_packet.length -= auth_len;
    }

    return true;
}


uint8_t d7anp_assemble_packet_header(packet_t *packet, uint8_t *data_ptr)
{
    assert(!packet->d7anp_ctrl.hop_enabled); // TODO hopping not yet supported

    uint8_t* d7anp_header_start = data_ptr;
    (*data_ptr) = packet->d7anp_listen_timeout; data_ptr++;
    (*data_ptr) = packet->d7anp_ctrl.raw; data_ptr++;

    if(packet->d7anp_ctrl.origin_addressee_ctrl_id_type != ID_TYPE_BCAST)
    {
        if(packet->d7anp_ctrl.origin_addressee_ctrl_id_type == ID_TYPE_UID)
        {
            fs_read_uid(data_ptr); data_ptr += 8;
        }
        else if(packet->d7anp_ctrl.origin_addressee_ctrl_id_type == ID_TYPE_VID)
        {
            fs_read_vid(data_ptr); data_ptr += 2;
        }
        else
        {
            assert(false);
        }
    }

    if (packet->d7anp_ctrl.extension && packet->d7anp_ext.nls_method)
    {
        uint8_t nls_method = packet->d7anp_ext.nls_method;

        /* set the extension byte */
        (*data_ptr) = packet->d7anp_ext.raw; data_ptr++;

        /* set the security header */
        if (nls_method == AES_CTR || nls_method == AES_CCM_32 ||
            nls_method == AES_CCM_64 || nls_method == AES_CCM_128)
        {
            (*data_ptr) = packet->d7anp_security.key_counter; data_ptr++;
            write_be32(data_ptr, packet->d7anp_security.frame_counter);
            data_ptr += sizeof(uint32_t);
        }
    }

    // TODO hopping ctrl

    return data_ptr - d7anp_header_start;
}

d7anp_node_t * get_trusted_node(uint8_t *address)
{
    for(uint8_t i = 0; i < MODULE_D7AP_TRUSTED_NODE_TABLE_SIZE; i++)
    {
        if((trusted_node_table[i].used == true) &&
           (memcmp(trusted_node_table[i].addr, address, 8) == 0))
            return &(trusted_node_table[i]);
    }

    return NULL;
}

d7anp_node_t* add_trusted_node(uint8_t *address, uint32_t frame_counter, uint8_t key_counter)
{
    for(uint8_t i = 0; i < MODULE_D7AP_TRUSTED_NODE_TABLE_SIZE; i++)
    {
        if(trusted_node_table[i].used == false)
        {
            memcpy(trusted_node_table[i].addr, address, 8);
            trusted_node_table[i].frame_counter = frame_counter;
            trusted_node_table[i].key_counter = key_counter;
            trusted_node_table[i].used = true;
            DPRINT("Add node %p! ", &(trusted_node_table[i]));
            return &(trusted_node_table[i]);
        }
    }

    assert(false); // should not happen, possible to small NODE_TABLE_SIZE
}

bool d7anp_disassemble_packet_header(packet_t* packet, uint8_t *data_idx)
{
    bool security_enabled = false;

    packet->d7anp_listen_timeout = packet->hw_radio_packet.data[(*data_idx)]; (*data_idx)++;
    packet->d7anp_ctrl.raw = packet->hw_radio_packet.data[(*data_idx)]; (*data_idx)++;

    if(packet->d7anp_ctrl.origin_addressee_ctrl_id_type != ID_TYPE_BCAST)
    {
        uint8_t origin_access_id_size = packet->d7anp_ctrl.origin_addressee_ctrl_id_type == ID_TYPE_VID? 2 : 8;
        memcpy(packet->origin_access_id, packet->hw_radio_packet.data + (*data_idx), origin_access_id_size); (*data_idx) += origin_access_id_size;
    }

    if (packet->d7anp_ctrl.extension)
    {
        packet->d7anp_ext.raw = packet->hw_radio_packet.data[(*data_idx)]; (*data_idx)++;
        if (packet->d7anp_ext.nls_method)
            security_enabled = true;
    }

    if (security_enabled)
    {
        d7anp_node_t *node;
        uint8_t nls_method = packet->d7anp_ext.nls_method;
        bool create_node = false;

        DPRINT("Received nls method %d", nls_method);

        if (nls_method == AES_CTR || nls_method == AES_CCM_32 ||
            nls_method == AES_CCM_64 || nls_method == AES_CCM_128)
        {
            // extract the key counter and the frame counter
            packet->d7anp_security.key_counter = packet->hw_radio_packet.data[(*data_idx)]; (*data_idx)++;
            packet->d7anp_security.frame_counter = read_be32(packet->hw_radio_packet.data + (*data_idx));
            (*data_idx) += sizeof(uint32_t);

            DPRINT("Received key counter <%d>, frame counter <%ld>", packet->d7anp_security.key_counter, packet->d7anp_security.frame_counter);

            /*
             * Check Frame counter
             * First look up the sender's address in the trusted node table
             * If no entry is found, create one
             */
            if(packet->d7anp_ctrl.origin_addressee_ctrl_id_type == ID_TYPE_BCAST)
            {
                DPRINT("Origin addressee is unknown, security can't be applied!");
                return false;
            }

            node = get_trusted_node(packet->origin_access_id);
            if (node && (node->frame_counter > packet->d7anp_security.frame_counter ||
                node->frame_counter == (uint32_t)~0))
            {
                DPRINT("Replay attack detected cnt %ld->%ld shift back", node->frame_counter, packet->d7anp_security.frame_counter);
                return false;
            }

            if (node == NULL)
                create_node = true;
            else{
            	DPRINT("Found node %p! frame counter %ld", node, node->frame_counter);
            	node->frame_counter = packet->d7anp_security.frame_counter;
            	DPRINT("node frame counter updated to %ld", node->frame_counter);
            }
        }

        if (!d7anp_unsecure_payload(packet, *data_idx))
            return false;

        if (create_node)
            add_trusted_node(packet->origin_access_id, packet->d7anp_security.frame_counter,
                             packet->d7anp_security.key_counter);
    }

    assert(!packet->d7anp_ctrl.hop_enabled); // TODO hopping not yet supported

    return true;
}

void d7anp_signal_transmission_failure()
{
    assert(d7anp_state == D7ANP_STATE_TRANSMIT);

    DPRINT("CSMA-CA insertion failed");

    // switch back to the previous state before the transmission
    switch_state(d7anp_prev_state);

    d7atp_signal_transmission_failure();
}

void d7anp_signal_packet_transmitted(packet_t* packet)
{
    assert(d7anp_state == D7ANP_STATE_TRANSMIT);

    /* switch back to the same state as before the transmission */
    switch_state(d7anp_prev_state);
    d7atp_signal_packet_transmitted(packet);

}

void d7anp_process_received_packet(packet_t* packet)
{
    // TODO handle case where we are intermediate node while hopping (ie start FG scan, after auth if needed, and return)

    if(d7anp_state == D7ANP_STATE_FOREGROUND_SCAN)
    {
        DPRINT("Received packet while in D7ANP_STATE_FOREGROUND_SCAN");
    }
    else if(d7anp_state == D7ANP_STATE_IDLE)
    {
        DPRINT("Received packet while in D7ANP_STATE_IDLE (scan automation)");

        // check if DLL was performing a background scan
        if(!own_access_profile.control_scan_type_is_foreground) {
            timer_tick_t eta;

            DPRINT("Received a background frame)");
            //TODO decode the D7A Background Network Protocols Frame in order to trigger the foreground scan after the advertising period
            schedule_foreground_scan_after_D7AAdvP(eta);
            return;
        }
    }
    else
        assert(false);

    d7atp_process_received_packet(packet);
}

uint8_t d7anp_addressee_id_length(id_type_t id_type)
{
    switch(id_type)
    {
        case ID_TYPE_BCAST:
          return ID_TYPE_BCAST_ID_LENGTH;
        case ID_TYPE_UID:
          return ID_TYPE_UID_ID_LENGTH;
        case ID_TYPE_VID:
          return ID_TYPE_VID_LENGTH;
        default:
          assert(false);
    }
}
