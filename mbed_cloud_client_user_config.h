// ----------------------------------------------------------------------------
// Copyright 2019 ARM Ltd.
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

#ifndef MBED_CLOUD_CLIENT_USER_CONFIG_H
#define MBED_CLOUD_CLIENT_USER_CONFIG_H

#define MBED_CLOUD_CLIENT_ENDPOINT_TYPE         "default"
#define MBED_CLOUD_CLIENT_LIFETIME              3600

// Application can override the default transport mode via `target.macros_add` definition in mbed_app.json.
#if !defined(MBED_CLOUD_CLIENT_TRANSPORT_MODE_UDP) && !defined(MBED_CLOUD_CLIENT_TRANSPORT_MODE_TCP) && !defined(MBED_CLOUD_CLIENT_TRANSPORT_MODE_UDP_QUEUE)
#define MBED_CLOUD_CLIENT_TRANSPORT_MODE_TCP
#endif

#ifdef MBED_CONF_MBED_CLIENT_SN_COAP_MAX_BLOCKWISE_PAYLOAD_SIZE
    #define SN_COAP_MAX_BLOCKWISE_PAYLOAD_SIZE    MBED_CONF_MBED_CLIENT_SN_COAP_MAX_BLOCKWISE_PAYLOAD_SIZE
#else
    #define SN_COAP_MAX_BLOCKWISE_PAYLOAD_SIZE      512
#endif

/* set flag to enable update support in mbed Cloud client */
#define MBED_CLOUD_CLIENT_SUPPORT_UPDATE

/* set download buffer size in bytes (min. 1024 bytes) */
#define MBED_CLOUD_CLIENT_UPDATE_BUFFER          1024

#define MBED_CLOUD_DEV_UPDATE_CERT
#define MBED_CLOUD_DEV_UPDATE_ID

#endif /* MBED_CLOUD_CLIENT_USER_CONFIG_H */
