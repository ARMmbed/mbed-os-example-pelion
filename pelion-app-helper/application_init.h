// ----------------------------------------------------------------------------
// Copyright 2016-2017 ARM Ltd.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------


#ifndef APPLICATION_INIT_H
#define APPLICATION_INIT_H

/*
 * Initializes tracing library.
 */

bool application_init_mbed_trace(void);

/*
 * application_init() runs the following initializations:
 *  1. platform initialization
 *  2. print memory statistics if MBED_HEAP_STATS_ENABLED is defined
 *  3. FCC initialization.
 */

bool application_init(void);

/*
 * Prints the FCC status and corresponding error description, if any.
 */
void print_fcc_status(int fcc_status);

#endif //APPLICATION_INIT_H

