/*
 * session.c
 *
 *  Created on: 2-aug.-2013
 *      Author: Maarten Weyn
 */

#include "session.h"
#include "../types.h"
#include "../pres/pres.h"
#include <string.h>

static uint8_t session_stack_depth;
static int8_t session_stack_top = -1;
static session_data session_stack[4]; // should be based on session_stack_depth_currently fixed to 4
static uint8_t session_dialogid; // should be random

void session_init()
{
	// Get Configuration Data
	file_handler fh;
	uint8_t result = fs_open(&fh, file_system_type_isfb, ISFB_ID(DEVICE_FEATURES), file_system_user_root, file_system_access_type_read);
	if (result != 0 || fh.file_info->length == 0)
	{
		session_stack_depth = 1;
	} else {
		session_stack_depth = fs_read_byte(&fh, 31);
		// Currently maximum 4
		// TODO: stack depth should come from configuration, but malloc needed
		if (session_stack_depth > 4)
		{
			session_stack_depth = 4;
		}
	}

	session_dialogid = fs_read_byte(&fh, 7); // init dialog_id with latest byte of UID

	fs_close(&fh);
}

session_data* session_new(uint16_t counter, uint8_t channel)
{
	session_stack_top++;

	uint8_t pos = session_stack_top;	// this is only true for adhoc sessions
	// TODO: if counter != 0 find correct position for stack

	// stack is full
	if (session_stack_top >= session_stack_depth)
	{
		// Remove oldest session
		session_stack_top--;
		if (pos == 0)
		{
			// new session had lowest priority
			return NULL;
		}

		uint8_t i = 1;
		for (; i < session_stack_top; i--)
		{
			memcpy((void*) &session_stack[i-1], (void*) &session_stack[i], sizeof(session_data));
		}
	}

	session_stack[pos].counter_until_scheduled = counter;
	session_stack[pos].channel_id = channel;
	session_stack[pos].network_state = network_state_unassociated;
	session_stack[pos].dialog_id = session_dialogid++;

	return &session_stack[pos];
}


void session_pop()
{
	if (session_stack_top < 0) return;

	session_stack_top--;
}

session_data* session_top()
{
	if (session_stack_top < 0) return NULL;
	return &session_stack[session_stack_top];


}
