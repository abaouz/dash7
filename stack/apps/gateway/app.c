/* * OSS-7 - An opensource implementation of the DASH7 Alliance Protocol for ultra
 * lowpower wireless sensor communication
 *
 * Copyright 2015 University of Antwerp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * \author	maarten.weyn@uantwerpen.be
 */

#include "hwuart.h"
#include "hwleds.h"
#include "hwsystem.h"
#include "scheduler.h"
#include "timer.h"
#include "log.h"
#include "debug.h"
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>

#include "hwlcd.h"
#include "d7ap_stack.h"
#include "dll.h"
#include "hwuart.h"
#include "fifo.h"
#include "alp_cmd_handler.h"
#include "version.h"

#if HW_NUM_LEDS > 0
#include "hwleds.h"

void led_blink_off()
{
	led_off(0);
}

void led_blink()
{
	led_on(0);

	timer_post_task_delay(&led_blink_off, TIMER_TICKS_PER_SEC * 0.2);
}

#endif

static d7asp_init_args_t d7asp_init_args;

static void on_unsollicited_response_received(d7asp_result_t d7asp_result, uint8_t *alp_command, uint8_t alp_command_size)
{
    alp_cmd_handler_output_d7asp_response(d7asp_result, alp_command, alp_command_size);

#if HW_NUM_LEDS > 0
	led_blink();
#endif
}

void bootstrap()
{
    dae_access_profile_t access_classes[] = {
        {
            .control_scan_type_is_foreground = true,
            .control_csma_ca_mode = CSMA_CA_MODE_UNC,
            .control_number_of_subbands = 1,
            .subnet = 0x00,
            .scan_automation_period = 0,
            .subbands[0] = (subband_t){
                .channel_header = {
                    .ch_coding = PHY_CODING_PN9,
                    .ch_class = PHY_CLASS_NORMAL_RATE,
                    .ch_freq_band = PHY_BAND_868
                },
                .channel_index_start = 0,
                .channel_index_end = 0,
                .eirp = 0,
                .ccao = 0
            }
        }
    };

    fs_init_args_t fs_init_args = (fs_init_args_t){
        .fs_user_files_init_cb = NULL,
        .access_profiles_count = 1,
        .access_profiles = access_classes
    };

    d7asp_init_args.d7asp_received_unsollicited_data_cb = &on_unsollicited_response_received;

    d7ap_stack_init(&fs_init_args, &d7asp_init_args, true, NULL);

    fs_write_dll_conf_active_access_class(0); // use access class 0 for scan automation
#ifdef HAS_LCD
    lcd_write_string("GW %s", _GIT_SHA1);
#endif

#if HW_NUM_LEDS > 0
    sched_register_task(&led_blink_off);
#endif
}

