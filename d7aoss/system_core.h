/*
 * system_core.h
 *
 *  Created on: 1-aug.-2013
 *      Author: Maarten Weyn
 */

#ifndef SYSTEM_CORE_H_
#define SYSTEM_CORE_H_

#include "framework/queue.h"

#define TASK_QUEUE_LENGTH 1
#define TASK_BEACON 0

typedef struct
{
	void (*handle) (void*);
	uint16_t scheduled_at;
	uint8_t size;
	uint8_t progress;
	bool active;

} task;

extern queue rx_queue;
extern queue tx_queue;

void system_core_init();
void system_core_start();
task* system_core_get_task(uint8_t task_id);
void system_core_set_task(uint8_t task_id, uint8_t task_progress, uint8_t task_size, uint8_t task_scheduled_at);
void system_core_dissable_task(uint8_t task_id);


#endif /* SYSTEM_CORE_H_ */
