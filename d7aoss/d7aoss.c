/*
 * d7aoss.c
 *
 *  Created on: 1-August-2013
 *      Author: Maarten Weyn
 */

#include "d7aoss.h"
#include "system_core.h"
#include "framework/timer.h"
#include "framework/queue.h"

#define RX_BUFFER_SIZE 255
#define TX_BUFFER_SIZE 255

static uint8_t rxtx_buffer[RX_BUFFER_SIZE+TX_BUFFER_SIZE];

queue rx_queue;
queue tx_queue;


void d7aoss_init(const filesystem_address_info *address_info)
{
	pres_init(address_info);

	queue_init(&rx_queue, rxtx_buffer, RX_BUFFER_SIZE);
	queue_init(&tx_queue, &rxtx_buffer[RX_BUFFER_SIZE], TX_BUFFER_SIZE);

	timer_init();
	system_core_init();

	phy_init();
	dll_init();
	nwl_init();
	trans_init();

	system_core_start();
}
