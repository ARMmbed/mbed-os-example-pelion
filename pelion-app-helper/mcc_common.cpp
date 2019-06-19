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

///////////
// INCLUDES
///////////

// Note: this macro is needed on armcc to get the the PRI*32 macros
// from inttypes.h in a C++ code.
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "mbed.h"
#include "mcc_common.h"
#include "mbed_trace.h"
#include "mbed-trace-helper.h"
#include "kv_config.h"



#define TRACE_GROUP "plat"

#define SECONDS_TO_MS 1000  // to avoid using floats, wait() uses floats

#ifndef MCC_PLATFORM_WAIT_BEFORE_BD_INIT
#define MCC_PLATFORM_WAIT_BEFORE_BD_INIT 2
#endif

/* local help functions. */
const char* network_type(NetworkInterface *iface);

////////////////////////////////////////
// PLATFORM SPECIFIC DEFINES & FUNCTIONS
////////////////////////////////////////

static NetworkInterface* network_interface=NULL;

/* Callback function which informs about status of connected NetworkInterface.
 * */
static void network_status_callback(nsapi_event_t status, intptr_t param);

////////////////////////////////
// SETUP_COMMON.H IMPLEMENTATION
////////////////////////////////

int mcc_platform_init_connection(void) {
// Perform number of retries if network init fails.
#ifndef MCC_PLATFORM_CONNECTION_RETRY_COUNT
#define MCC_PLATFORM_CONNECTION_RETRY_COUNT 5
#endif
#ifndef MCC_PLATFORM_CONNECTION_RETRY_TIMEOUT
#define MCC_PLATFORM_CONNECTION_RETRY_TIMEOUT 1000
#endif
    printf("mcc_platform_init_connection()\n");
    network_interface = NetworkInterface::get_default_instance();
    if(network_interface == NULL) {
        printf("ERROR: No NetworkInterface found!\n");
        return -1;
    }

    network_interface->add_event_listener(mbed::callback(&network_status_callback));
    printf("Connecting with interface: %s\n", network_type(NetworkInterface::get_default_instance()));

    for (int i=1; i <= MCC_PLATFORM_CONNECTION_RETRY_COUNT; i++) {
        nsapi_error_t e;
        e = network_interface->connect();
        if (e == NSAPI_ERROR_OK || e == NSAPI_ERROR_IS_CONNECTED) {
            printf("IP: %s\n", network_interface->get_ip_address());
            return 0;
        }
        printf("Failed to connect! error=%d. Retry %d/%d\n", e, i, MCC_PLATFORM_CONNECTION_RETRY_COUNT);
        (void) network_interface->disconnect();
        wait_ms(MCC_PLATFORM_CONNECTION_RETRY_TIMEOUT * i);
    }
    return -1;
}

int mcc_platform_close_connection(void) {

    if (network_interface) {
        const nsapi_error_t err = network_interface->disconnect();
        if (err == NSAPI_ERROR_OK) {
            network_interface->remove_event_listener(mbed::callback(&network_status_callback));
            network_interface = NULL;
            return 0;
        }
    }
    return -1;
}

void* mcc_platform_get_network_interface(void) {
    return network_interface;
}

void network_status_callback(nsapi_event_t status, intptr_t param)
{
    if (status == NSAPI_EVENT_CONNECTION_STATUS_CHANGE) {
        switch(param) {
            case NSAPI_STATUS_GLOBAL_UP:

#if MBED_CONF_MBED_TRACE_ENABLE
                tr_info("NSAPI_STATUS_GLOBAL_UP");
#else
                printf("NSAPI_STATUS_GLOBAL_UP\n");
#endif
                break;
            case NSAPI_STATUS_LOCAL_UP:
#if MBED_CONF_MBED_TRACE_ENABLE
                tr_info("NSAPI_STATUS_LOCAL_UP");
#else
                printf("NSAPI_STATUS_LOCAL_UP\n");
#endif
                break;
            case NSAPI_STATUS_DISCONNECTED:
#if MBED_CONF_MBED_TRACE_ENABLE
                tr_info("NSAPI_STATUS_DISCONNECTED");
#else
                printf("NSAPI_STATUS_DISCONNECTED\n");
#endif
                break;
            case NSAPI_STATUS_CONNECTING:
#if MBED_CONF_MBED_TRACE_ENABLE
                tr_info("NSAPI_STATUS_CONNECTING");
#else
                printf("NSAPI_STATUS_CONNECTING\n");
#endif
                break;
            case NSAPI_STATUS_ERROR_UNSUPPORTED:

#if MBED_CONF_MBED_TRACE_ENABLE
                tr_info("NSAPI_STATUS_ERROR_UNSUPPORTED");
#else
                printf("NSAPI_STATUS_ERROR_UNSUPPORTED\n");
#endif
                break;
        }
    }
}

const char* network_type(NetworkInterface *iface)
{
    if (iface->ethInterface()) {
        return "Ethernet";
    } else if (iface->wifiInterface()) {
        return "WiFi";
    } else if (iface->meshInterface()) {
        return "Mesh";
    } else if (iface->cellularInterface()) {
        return "Cellular";
    } else if (iface->emacInterface()) {
        return "Emac";
    } else {
        return "Unknown";
    }
}

int mcc_platform_init(void)
{
    // On CortexM (3 and 4) the MCU has a write buffer, which helps in performance front,
    // but has a side effect of making data access faults imprecise.
    //
    // So, if one gets a Mbed OS crash dump with following content, a re-build with
    // "PLATFORM_DISABLE_WRITE_BUFFER=1" will help in getting the correct crash location.
    //
    // --8<---
    // Crash Info:
    // <..>
    //    Target and Fault Info:
    //          Forced exception, a fault with configurable priority has been escalated to HardFault
    //          Imprecise data access error has occurred
    // --8<---
    //
    // This can't be enabled by default as then we would test with different system setup than customer
    // and possible OS and driver issues might get pass the tests.
    //
#if defined(PLATFORM_DISABLE_WRITE_BUFFER) && (PLATFORM_DISABLE_WRITE_BUFFER==1)

#if defined(TARGET_CORTEX_M)

    SCnSCB->ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk;

    tr_info("mcc_platform_init: disabled CPU write buffer, expect reduced performance");
#else
    tr_info("mcc_platform_init: disabling CPU write buffer not possible or needed on this MCU");
#endif

#endif

    return 0;
}

void mcc_platform_sw_build_info(void) {
    printf("Application ready. Build at: " __DATE__ " " __TIME__ "\n");

    // The Mbed OS' master branch does not define the version numbers at all, so we need
    // some ifdeffery to keep compilations running.
#if defined(MBED_MAJOR_VERSION) && defined(MBED_MINOR_VERSION) && defined(MBED_PATCH_VERSION)
    printf("Mbed OS version %d.%d.%d\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
#else
    printf("Mbed OS version <UNKNOWN>\n");
#endif
}

// Note we only support config with MBED_CONF_MBED_CLOUD_CLIENT_EXTERNAL_SST_SUPPORT
// TODO: find out how to reset storage when a button is pressed
// as this is critical to ensure that the user can force new credentials
// to be installed
int mcc_platform_storage_init(void) {
    // This wait will allow the board more time to initialize
    wait_ms(MCC_PLATFORM_WAIT_BEFORE_BD_INIT * SECONDS_TO_MS);
    int status = kv_init_storage_config();
    if (status != MBED_SUCCESS) {
        printf("kv_init_storage_config() - failed, status %d\n", status);
    }
    return status;
}


#if PLATFORM_ENABLE_BUTTON
// Make button and led definitions optional
#ifndef MBED_CONF_APP_BUTTON_PINNAME
#define MBED_CONF_APP_BUTTON_PINNAME NC
#endif
#endif

#if PLATFORM_ENABLE_LED
// Define led on/off

#define LED_ON (true)
#define LED_OFF (!LED_ON)

#ifndef MBED_CONF_APP_LED_PINNAME
#define MBED_CONF_APP_LED_PINNAME NC
#endif

// Button and led
static DigitalOut led(MBED_CONF_APP_LED_PINNAME, LED_OFF);
#endif

#if PLATFORM_ENABLE_BUTTON
static InterruptIn button(MBED_CONF_APP_BUTTON_PINNAME);
static bool button_pressed = false;
static void button_press(void);

static void button_press(void)
{
    button_pressed = true;
}
#endif

uint8_t mcc_platform_button_clicked(void)
{
#if PLATFORM_ENABLE_BUTTON
    if (button_pressed) {
        button_pressed = false;
        return true;
    }
#endif
    return false;
}

uint8_t mcc_platform_init_button_and_led(void)
{
#if PLATFORM_ENABLE_BUTTON
   if(MBED_CONF_APP_BUTTON_PINNAME != NC) {
        button.fall(&button_press);
    }
#endif
    return 0;
}

void mcc_platform_toggle_led(void)
{
#if PLATFORM_ENABLE_LED
    if (MBED_CONF_APP_LED_PINNAME != NC) {
        led = !led;
    }
    else {
        printf("Virtual LED toggled\n");
    }
#endif
}

void mcc_platform_led_off(void)
{
#if PLATFORM_ENABLE_LED
    if (MBED_CONF_APP_LED_PINNAME != NC) {
        led = LED_OFF;
    }
    else {
        printf("Virtual LED off\n");
    }
#endif
}




#if MBED_CONF_APP_DEVELOPER_MODE == 1
#ifdef PAL_USER_DEFINED_CONFIGURATION
    #include PAL_USER_DEFINED_CONFIGURATION
#endif
#endif // #if MBED_CONF_APP_DEVELOPER_MODE == 1

// Include this only for Developer mode and device which doesn't have in-built TRNG support
#if MBED_CONF_APP_DEVELOPER_MODE == 1
#ifdef PAL_USER_DEFINED_CONFIGURATION
#define FCC_ROT_SIZE                       16
const uint8_t MBED_CLOUD_DEV_ROT[FCC_ROT_SIZE] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 };
#if !PAL_USE_HW_TRNG
#define FCC_ENTROPY_SIZE                   48
const uint8_t MBED_CLOUD_DEV_ENTROPY[FCC_ENTROPY_SIZE] = { 0xf6, 0xd6, 0xc0, 0x09, 0x9e, 0x6e, 0xf2, 0x37, 0xdc, 0x29, 0x88, 0xf1, 0x57, 0x32, 0x7d, 0xde, 0xac, 0xb3, 0x99, 0x8c, 0xb9, 0x11, 0x35, 0x18, 0xeb, 0x48, 0x29, 0x03, 0x6a, 0x94, 0x6d, 0xe8, 0x40, 0xc0, 0x28, 0xcc, 0xe4, 0x04, 0xc3, 0x1f, 0x4b, 0xc2, 0xe0, 0x68, 0xa0, 0x93, 0xe6, 0x3a };
#endif // PAL_USE_HW_TRNG = 0
#endif // PAL_USER_DEFINED_CONFIGURATION
#endif // #if MBED_CONF_APP_DEVELOPER_MODE == 1

int mcc_platform_reset_storage(void)
{
#if MBED_CONF_APP_DEVELOPER_MODE == 1
    printf("Resets storage to an empty state.\n");
    int status = fcc_storage_delete();
    if (status != FCC_STATUS_SUCCESS) {
        printf("Failed to delete storage - %d\n", status);
// Format call is not needed with SST implementation
#ifndef MBED_CONF_MBED_CLOUD_CLIENT_EXTERNAL_SST_SUPPORT
// Flagging here because of reformat contains only implementation for mbed-os.
#ifdef TARGET_LIKE_MBED
        status = mcc_platform_reformat_storage();
        if (status == 0) {
            printf("Storage reformatted, try reset storage again.\n");
            // Try to reset storage again after format.
            // It is required to run fcc_storage_delete() after format.
            status = fcc_storage_delete();
            if (status != FCC_STATUS_SUCCESS) {
                printf("Failed to delete storage - %d\n", status);
            }
        }
#endif
#endif
    }
    return status;
#else
    return FCC_STATUS_SUCCESS;
#endif
}

int mcc_platform_fcc_init(void)
{
#if MBED_CONF_APP_DEVELOPER_MODE == 1
    int status = fcc_init();
    // Ignore pre-existing RoT/Entropy in SOTP
    if (status != FCC_STATUS_SUCCESS && status != FCC_STATUS_ENTROPY_ERROR && status != FCC_STATUS_ROT_ERROR) {
        printf("fcc_init failed with status %d! - exit\n", status);
        return status;
    }
    status = mcc_platform_sotp_init();
    if (status != FCC_STATUS_SUCCESS) {
        printf("fcc_init failed with status %d! - exit\n", status);
        mcc_platform_fcc_finalize();
    } else {
        // We can return SUCCESS here as preexisting RoT/Entropy is expected flow.
        status = FCC_STATUS_SUCCESS;
    }
    return status;
#else
    return FCC_STATUS_SUCCESS;
#endif
}

int mcc_platform_sotp_init(void)
{
    int status = FCC_STATUS_SUCCESS;
// Include this only for Developer mode and a device which doesn't have in-built TRNG support.
#if MBED_CONF_APP_DEVELOPER_MODE == 1
#ifdef PAL_USER_DEFINED_CONFIGURATION
#if !PAL_USE_HW_TRNG
    status = fcc_entropy_set(MBED_CLOUD_DEV_ENTROPY, FCC_ENTROPY_SIZE);

    if (status != FCC_STATUS_SUCCESS && status != FCC_STATUS_ENTROPY_ERROR) {
        printf("fcc_entropy_set failed with status %d! - exit\n", status);
        mcc_platform_fcc_finalize();
        return status;
    }
#endif // PAL_USE_HW_TRNG = 0
/* Include this only for Developer mode. The application will use fixed RoT to simplify user-experience with the application.
 * With this change the application be reflashed/SOTP can be erased safely without invalidating the application credentials.
 */
    status = fcc_rot_set(MBED_CLOUD_DEV_ROT, FCC_ROT_SIZE);

    if (status != FCC_STATUS_SUCCESS && status != FCC_STATUS_ROT_ERROR) {
        printf("fcc_rot_set failed with status %d! - exit\n", status);
        mcc_platform_fcc_finalize();
    } else {
        // We can return SUCCESS here as preexisting RoT/Entropy is expected flow.
        printf("Using hardcoded Root of Trust, not suitable for production use.\n");
        status = FCC_STATUS_SUCCESS;
    }
#endif // PAL_USER_DEFINED_CONFIGURATION
#endif // #if MBED_CONF_APP_DEVELOPER_MODE == 1
    return status;
}

void mcc_platform_fcc_finalize(void)
{
#if MBED_CONF_APP_DEVELOPER_MODE == 1
    (void)fcc_finalize();
#endif
}

#if defined (MBED_HEAP_STATS_ENABLED) || (MBED_STACK_STATS_ENABLED)
#include "memory_tests.h"
#endif
#include "mcc_common.h"

void print_fcc_status(int fcc_status)
{
#ifndef MCC_MINIMAL
#ifndef DISABLE_ERROR_DESCRIPTION
    const char *error;
    switch(fcc_status) {
        case FCC_STATUS_SUCCESS:
            return;
        case FCC_STATUS_ERROR :
            error = "Operation ended with an unspecified error.";
            break;
        case FCC_STATUS_MEMORY_OUT:
            error = "An out-of-memory condition occurred.";
            break;
        case FCC_STATUS_INVALID_PARAMETER:
            error = "A parameter provided to the function was invalid.";
            break;
        case FCC_STATUS_STORE_ERROR:
            error = "Storage internal error.";
            break;
        case FCC_STATUS_INTERNAL_ITEM_ALREADY_EXIST:
            error = "Current item already exists in storage.";
            break;
        case FCC_STATUS_CA_ERROR:
            error = "CA Certificate already exist in storage (currently only bootstrap CA)";
            break;
        case FCC_STATUS_ROT_ERROR:
            error = "ROT already exist in storage";
            break;
        case FCC_STATUS_ENTROPY_ERROR:
            error = "Entropy already exist in storage";
            break;
        case FCC_STATUS_FACTORY_DISABLED_ERROR:
            error = "FCC flow was disabled - denial of service error.";
            break;
        case FCC_STATUS_INVALID_CERTIFICATE:
            error = "Invalid certificate found.";
            break;
        case FCC_STATUS_INVALID_CERT_ATTRIBUTE:
            error = "Operation failed to get an attribute.";
            break;
        case FCC_STATUS_INVALID_CA_CERT_SIGNATURE:
            error = "Invalid ca signature.";
            break;
        case FCC_STATUS_EXPIRED_CERTIFICATE:
            error = "LWM2M or Update certificate is expired.";
            break;
        case FCC_STATUS_INVALID_LWM2M_CN_ATTR:
            error = "Invalid CN field of certificate.";
            break;
        case FCC_STATUS_KCM_ERROR:
            error = "KCM basic functionality failed.";
            break;
        case FCC_STATUS_KCM_STORAGE_ERROR:
            error = "KCM failed to read, write or get size of item from/to storage.";
            break;
        case FCC_STATUS_KCM_FILE_EXIST_ERROR:
            error = "KCM tried to create existing storage item.";
            break;
        case FCC_STATUS_KCM_CRYPTO_ERROR:
            error = "KCM returned error upon cryptographic check of an certificate or key.";
            break;
        case FCC_STATUS_NOT_INITIALIZED:
            error = "FCC failed or did not initialized.";
            break;
        case FCC_STATUS_BUNDLE_ERROR:
            error = "Protocol layer general error.";
            break;
        case FCC_STATUS_BUNDLE_RESPONSE_ERROR:
            error = "Protocol layer failed to create response buffer.";
            break;
        case FCC_STATUS_BUNDLE_UNSUPPORTED_GROUP:
            error = "Protocol layer detected unsupported group was found in a message.";
            break;
        case FCC_STATUS_BUNDLE_INVALID_GROUP:
            error = "Protocol layer detected invalid group in a message.";
            break;
        case FCC_STATUS_BUNDLE_INVALID_SCHEME:
            error = "The scheme version of a message in the protocol layer is wrong.";
            break;
        case FCC_STATUS_ITEM_NOT_EXIST:
            error = "Current item wasn't found in the storage";
            break;
        case FCC_STATUS_EMPTY_ITEM:
            error = "Current item's size is 0";
            break;
        case FCC_STATUS_WRONG_ITEM_DATA_SIZE:
            error = "Current item's size is different then expected";
            break;
        case FCC_STATUS_URI_WRONG_FORMAT:
            error = "Current URI is different than expected.";
            break;
        case FCC_STATUS_FIRST_TO_CLAIM_NOT_ALLOWED:
            error = "Can't use first to claim without bootstrap or with account ID";
            break;
        case FCC_STATUS_BOOTSTRAP_MODE_ERROR:
            error = "Wrong value of bootstrapUse mode.";
            break;
        case FCC_STATUS_OUTPUT_INFO_ERROR:
            error = "The process failed in output info creation.";
            break;
        case FCC_STATUS_WARNING_CREATE_ERROR:
            error = "The process failed in output info creation.";
            break;
        case FCC_STATUS_UTC_OFFSET_WRONG_FORMAT:
            error = "Current UTC is wrong.";
            break;
        case FCC_STATUS_CERTIFICATE_PUBLIC_KEY_CORRELATION_ERROR:
            error = "Certificate's public key failed do not matches to corresponding private key";
            break;
        case FCC_STATUS_BUNDLE_INVALID_KEEP_ALIVE_SESSION_STATUS:
            error = "The message status is invalid.";
            break;
        default:
            error = "UNKNOWN";
    }
    printf("\nFactory Configurator Client [ERROR]: %s\r\n\n", error);
#endif
#endif
}


bool application_init_mbed_trace(void)
{
#ifndef MCC_MINIMAL
    // Create mutex for tracing to avoid broken lines in logs
    if(!mbed_trace_helper_create_mutex()) {
        printf("ERROR - Mutex creation for mbed_trace failed!\n");
        return 1;
    }
#endif

    // Initialize mbed trace
    (void) mbed_trace_init();
#ifndef MCC_MINIMAL
    mbed_trace_mutex_wait_function_set(mbed_trace_helper_mutex_wait);
    mbed_trace_mutex_release_function_set(mbed_trace_helper_mutex_release);
#endif

    return 0;
}

static bool application_init_verify_cloud_configuration()
{
    int status;
    bool result = 0;

#if MBED_CONF_APP_DEVELOPER_MODE == 1
    printf("Starting developer flow\n");
    status = fcc_developer_flow();
    if (status == FCC_STATUS_KCM_FILE_EXIST_ERROR) {
        printf("Developer credentials already exist, continuing..\n");
        result = 0;
    } else if (status != FCC_STATUS_SUCCESS) {
        printf("Failed to load developer credentials\n");
        result = 1;
    }
#endif
#ifndef MCC_MINIMAL
#if MBED_CONF_APP_DEVELOPER_MODE == 1
    status = fcc_verify_device_configured_4mbed_cloud();
    print_fcc_status(status);
    if (status != FCC_STATUS_SUCCESS && status != FCC_STATUS_EXPIRED_CERTIFICATE) {
        result = 1;
    }
#endif
#endif
    return result;
}

static bool application_init_fcc(void)
{
#ifdef MBED_STACK_STATS_ENABLED
    print_stack_statistics();
#endif
    int status;
    status = mcc_platform_fcc_init();
    if(status != FCC_STATUS_SUCCESS) {
        printf("application_init_fcc fcc_init failed with status %d! - exit\n", status);
        return 1;
    }
#if RESET_STORAGE
    status = mcc_platform_reset_storage();
    if(status != FCC_STATUS_SUCCESS) {
        printf("application_init_fcc reset_storage failed with status %d! - exit\n", status);
        return 1;
    }
    // Reinitialize SOTP
    status = mcc_platform_sotp_init();
    if (status != FCC_STATUS_SUCCESS) {
        printf("application_init_fcc sotp_init failed with status %d! - exit\n", status);
        return 1;
    }
#endif

    status = application_init_verify_cloud_configuration();
    if (status != 0) {
#ifndef MCC_MINIMAL
    // This is designed to simplify user-experience by auto-formatting the
    // primary storage if no valid certificates exist.
    // This should never be used for any kind of production devices.
#ifndef MBED_CONF_APP_MCC_NO_AUTO_FORMAT
#ifndef MBED_CONF_MBED_CLOUD_CLIENT_EXTERNAL_SST_SUPPORT
        printf("Certificate validation failed, trying autorecovery...\n");
        if (mcc_platform_reformat_storage() != 0) {
            return 1;
        }
#endif
        status = mcc_platform_reset_storage();
        if (status != FCC_STATUS_SUCCESS) {
            return 1;
        }
        status = mcc_platform_sotp_init();
        if (status != FCC_STATUS_SUCCESS) {
            return 1;
        }
        status = application_init_verify_cloud_configuration();
        if (status != 0) {
            return 1;
        }        
#else
        return 1;
#endif
#endif
    }
    return 0;
}

bool application_init(void)
{
    // The function always returns 0.
    (void) mcc_platform_init_button_and_led();

    // Print some statistics of current heap memory consumption, useful for finding
    // out where the memory goes.
#ifdef MBED_HEAP_STATS_ENABLED
    print_heap_stats();
#endif

#ifdef MBED_STACK_STATS_ENABLED
    print_stack_statistics();
#endif
    printf("Start Device Management Client\n");

    if (application_init_fcc() != 0) {
        printf("Failed initializing FCC\n" );
        return false;
    }

    return true;
}

