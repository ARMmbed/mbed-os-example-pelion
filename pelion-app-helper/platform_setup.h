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

#ifndef PLATFORM_SETUP_H
#define PLATFORM_SETUP_H

#include <stdint.h>
#include "platform_setup.h"
#include "factory_configurator_client.h"

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

// Initializes tracing library.
bool application_init_mbed_trace(void);

// FCC init
bool application_init_fcc(void);

// Prints the FCC status and corresponding error description, if any.
void print_fcc_status(int fcc_status);

// Initialize platform
// related platform specific initializations required.
//
// @returns
//   0 for success, anything else for error
int platform_init(void);

// Initialize network connection
int platform_init_connection(void);

// Close network connection
int platform_close_connection(void);

// Return network interface.
void *platform_get_network_interface(void);

// Format storage (DEPRECATED)
int platform_reformat_storage(void);

// initialize common details for storage for storing KCM data etc.
// creates default folders, reformat.
int platform_storage_init(void);

// Wait
void platform_do_wait(int timeout_ms);

// for printing platform information in the console
void platform_info(void);

// Toggle led (if available)
void platform_toggle_led(void);

// Put led off (if available)
void platform_led_off(void);

// Check if button has been pressed (if available)
uint8_t platform_button_clicked(void);

uint8_t platform_init_button_and_led(void);

// Erases client credentials and SOTP storage, will also reformat
// the external storage for Mbed OS if initial erase fail.
int platform_reset_storage(void);

// Initialize common details for fcc.
int platform_fcc_init(void);

// For developer-mode only, (re)initializes the RoT and for non-TRNG boards
// also the entropy.
int platform_sotp_init(void);

// Reverse the resource allocations done by platform_fcc_init().
void platform_fcc_finalize(void);

#ifdef __cplusplus
}
#endif

#endif // #ifndef PLATFORM_SETUP_H
