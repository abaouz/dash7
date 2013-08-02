/*
 * d7aoss.c
 *
 *  Created on: 1-August-2013
 *      Author: Maarten Weyn
 */

#include "d7aoss.h"

#include "framework/timer.h"


void d7aoss_init(const filesystem_address_info *address_info)
{
	pres_init(address_info);

	timer_init();

	phy_init();
	dll_init();
	nwl_init();
	trans_init();
}
