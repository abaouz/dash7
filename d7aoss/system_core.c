/*
 * system_core.c
 *
 *  Created on: 8-aug.-2013
 *      Author: Maarten Weyn
 */


#include "system_core.h"
#include "dll/dll.h"
#include "framework/timer.h"




static task tasks[TASK_QUEUE_LENGTH];
static int8_t current_task;
static bool is_active;
static uint16_t current_time;
static timer_event task_timer;

static void sys_beacon_task();
static void sys_schedule_next_task();
static void sys_handle_timer_event();
static void sys_updated_task_times();

void system_core_init()
{
	current_task = -1;
	is_active = false;

	tasks[TASK_BEACON].active = false;
	tasks[TASK_BEACON].handle = &sys_beacon_task;

	task_timer.f = &sys_handle_timer_event;
}

void system_core_start()
{
	if (is_active) return;

	is_active = true;
	current_time = timer_get_counter_value();
	sys_schedule_next_task();
}

task* system_core_get_task(uint8_t task_id)
{
	if (task_id >= TASK_QUEUE_LENGTH) return NULL;

	return &tasks[task_id];
}

void system_core_set_task(uint8_t task_id, uint8_t task_progress, uint8_t task_size, uint8_t task_scheduled_at)
{
	if (task_id >= TASK_QUEUE_LENGTH) return;

	tasks[task_id].progress = task_progress;
	tasks[task_id].size = task_size;
	tasks[task_id].scheduled_at = task_scheduled_at;
	tasks[task_id].active = true;

	if (is_active)
	{
		uint16_t next_scheduled = tasks[current_task].scheduled_at - (timer_get_counter_value() - current_time);

		if (next_scheduled > task_scheduled_at)
		{
			current_task = task_id;
			task_timer.next_event = tasks[current_task].scheduled_at;
			timer_add_event(&task_timer);
		}
	}
}

void system_core_dissable_task(uint8_t task_id)
{
	if (task_id >= TASK_QUEUE_LENGTH) return;
	tasks[task_id].active = false;
}


static void sys_beacon_task()
{
	dll_create_beacon(&tasks[TASK_BEACON]);
}

static void sys_schedule_next_task()
{
	uint16_t first_scheduled = 0xFFFF;
	current_task = -1;
	uint8_t i = 0;

	sys_updated_task_times();

	for (; i < TASK_QUEUE_LENGTH; i++)
	{
		if (tasks[i].active == true)
		{
			if (tasks[i].scheduled_at < first_scheduled)
			{
				first_scheduled = tasks[i].scheduled_at;
				current_task = i;
			}
		}
	}

	if (current_task > -1)
	{
		task_timer.next_event = tasks[current_task].scheduled_at;
		timer_add_event(&task_timer);
	}
}

static void sys_handle_timer_event()
{
	sys_updated_task_times();

	if (tasks[current_task].scheduled_at == 0)
	{
		tasks[current_task].active = false;
		tasks[current_task].handle(NULL);
	}

	sys_schedule_next_task();
}

static void sys_updated_task_times()
{
	uint16_t time_sync_last_run = timer_get_counter_value() - current_time;
	current_time += time_sync_last_run;

	uint8_t i = 0;
	for (; i < TASK_QUEUE_LENGTH; i++)
	{
		if (tasks[i].active == true)
		{
			if (tasks[i].scheduled_at <= time_sync_last_run)
				tasks[i].scheduled_at = 0;
			else
				tasks[i].scheduled_at -= time_sync_last_run;
		}
	}
}
