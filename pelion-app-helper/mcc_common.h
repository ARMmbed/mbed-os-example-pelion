/*
 * Copyright (c) 2015-2018 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MCC_COMMON_SETUP_H
#define MCC_COMMON_SETUP_H

#include <stdint.h>

/* #define PLATFORM_ENABLE_BUTTON 1 for enabling button.*/
#ifndef PLATFORM_ENABLE_BUTTON
#define PLATFORM_ENABLE_BUTTON 0
#endif 

/* #define PLATFORM_ENABLE_LED 1 for enabling led.*/
#ifndef PLATFORM_ENABLE_LED
#define PLATFORM_ENABLE_LED 0 
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*main_t)(void);

// Initialize platform
// related platform specific initializations required.
//
// @returns
//   0 for success, anything else for error
int mcc_platform_init(void);

// Initialize network connection
int mcc_platform_init_connection(void);

// Close network connection
int mcc_platform_close_connection(void);

// Return network interface.
void *mcc_platform_get_network_interface(void);

// Format storage (DEPRECATED)
int mcc_platform_reformat_storage(void);

// initialize common details for storage for storing KCM data etc.
// creates default folders, reformat.
int mcc_platform_storage_init(void);

// Wait
void mcc_platform_do_wait(int timeout_ms);

// for printing sW build info
void mcc_platform_sw_build_info(void);

// Toggle led (if available)
void mcc_platform_toggle_led(void);

// Put led off (if available)
void mcc_platform_led_off(void);

// Check if button has been pressed (if available)
uint8_t mcc_platform_button_clicked(void);

uint8_t mcc_platform_init_button_and_led(void);

#ifdef __cplusplus
}
#endif

#endif // #ifndef MCC_COMMON_SETUP_H
