/*
 * mbed Microcontroller Library
 * Copyright (c) 2006-2018 ARM Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "FATFileSystem.h"
#include "LittleFileSystem.h"
#include "platform_setup.h"
#include "greentea-client/test_env.h"
#include "common_defines_test.h"

#ifndef MBED_CONF_APP_TESTS_FS_SIZE
  #define MBED_CONF_APP_TESTS_FS_SIZE (2*1024*1024)
#endif

#if !defined(MBED_CONF_APP_NO_LED)
DigitalOut led1(LED1);
DigitalOut led2(LED2);
void led_thread() {
    led1 = !led1;
    led2 = !led1;
}
#endif

RawSerial pc(USBTX, USBRX);

void wait_nb(uint16_t ms) {
    wait_ms(ms);
}

void logger(const char* message, const char* decor) {
    wait_nb(10);
    pc.printf(message, decor);
    wait_nb(10);
}
void logger(const char* message) {
    wait_nb(10);
    pc.printf(message);
    wait_nb(10);
}
void test_failed() {
    greentea_send_kv("test_failed", 1);
}
void test_case_start(const char *name, size_t index) {
    wait_nb(10);
    pc.printf("\r\n>>> Running case #%u: '%s'...\n", index, name);
    GREENTEA_TESTCASE_START(name);
}
void test_case_finish(const char *name, size_t passed, size_t failed) {
    GREENTEA_TESTCASE_FINISH(name, passed, failed);
    wait_nb(10);
    pc.printf(">>> '%s': %u passed, %u failed\r\n", name, passed, failed);
}

static const ConnectorClientEndpointInfo* endpointInfo;
void registered(const ConnectorClientEndpointInfo *endpoint) {
    logger("[INFO] Connected to Pelion Device Management. Device ID: %s\n",
            endpoint->internal_endpoint_name.c_str());
    endpointInfo = endpoint;
}

void post_test_callback(MbedCloudClientResource *resource, const uint8_t *buffer, uint16_t size) {
    logger("[INFO] POST test callback executed.\r\n");
    greentea_send_kv("verify_lwm2m_post_test_result", 0);
}

void spdmc_testsuite_connect(void) {
    int i = 0;
    int iteration = 0;
    char _key[20] = { };
    char _value[128] = { };

    greentea_send_kv("device_ready", true);
    while (1) {
        greentea_parse_kv(_key, _value, sizeof(_key), sizeof(_value));

        if (strcmp(_key, "iteration") == 0) {
            iteration = atoi(_value);
            break;
        }
    }

    // provide manifest to greentea so it can correct show skipped and failed tests
    if (iteration == 0) {
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_COUNT, 10);
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Connect to " TEST_NETWORK_TYPE);
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Initialize " TEST_BLOCK_DEVICE_TYPE "+" TEST_FILESYSTEM_TYPE);
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Format " TEST_FILESYSTEM_TYPE);
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Initialize Simple PDMC");
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Pelion Bootstrap & Reg.");
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Pelion Directory");
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Pelion Re-register");
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Post-reset Identity");
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Resource LwM2M GET");
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Resource LwM2M SET");
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Resource LwM2M PUT");
        greentea_send_kv(GREENTEA_TEST_ENV_TESTCASE_NAME, "Resource LwM2M POST");
    }

    // Start network connection test.
    test_case_start("Connect to " TEST_NETWORK_TYPE, 1);
    logger("[INFO] Attempting to connect to network.\r\n");

    // Connection definition.
    NetworkInterface *net = NetworkInterface::get_default_instance();
    nsapi_error_t net_status = -1;
    for (int tries = 0; tries < 3; tries++) {
        net_status = net->connect();
        if (net_status == NSAPI_ERROR_OK) {
            break;
        } else {
            logger("[WARN] Unable to connect to network. Retrying...");
        }
    }

    // Report status to console.
    if (net_status != 0) {
        logger("[ERROR] Device failed to connect to Network.\r\n");
        test_failed();
    } else {
        logger("[INFO] Connected to network successfully. IP address: %s\n", net->get_ip_address());
    }

    test_case_finish("Connect to " TEST_NETWORK_TYPE, iteration + (net_status == 0), (net_status != 0));

    test_case_start("Initialize " TEST_BLOCK_DEVICE_TYPE "+" TEST_FILESYSTEM_TYPE, 2);
    logger("[INFO] Attempting to initialize storage.\r\n");

    // Default storage definition.
    BlockDevice* bd = BlockDevice::get_default_instance();
    SlicingBlockDevice sd(bd, 0, MBED_CONF_APP_TESTS_FS_SIZE);
#if TEST_USE_FILESYSTEM == FS_FAT
    FATFileSystem fs("fs", &sd);
#else
	LittleFileSystem fs("fs", &sd);
#endif

    test_case_finish("Initialize " TEST_BLOCK_DEVICE_TYPE "+" TEST_FILESYSTEM_TYPE, iteration + 1, 0);

    if (iteration == 0) {
        test_case_start("Format " TEST_FILESYSTEM_TYPE, 3);
        logger("[INFO] Resetting storage to a clean state for test.\n");

        int storage_status = fs.reformat(&sd);
        if (storage_status != 0) {
            storage_status = sd.erase(0, sd.size());
            if (storage_status == 0) {
                storage_status = fs.format(&sd);
                if (storage_status != 0) {
                    logger("[ERROR] Filesystem init failed\n");
                }
            }
        }

        // Report status to console.
        if (storage_status == 0) {
            logger("[INFO] Storage format successful.\r\n");
        } else {
            logger("[ERROR] Storage format failed.\r\n");
            test_failed();
        }

        test_case_finish("Format " TEST_FILESYSTEM_TYPE, (storage_status == 0), (storage_status != 0));
    }

    // SimpleMbedCloudClient initialization must be successful.
    test_case_start("Initialize Simple PDMC", 4);

    SimpleMbedCloudClient client(net, &sd, &fs);
    int client_status = client.init();

    // Report status to console.
    if (client_status == 0) {
        logger("[INFO] Simple PDMC initialization successful.\r\n");
    } else {
        logger("[ERROR] Simple PDMC failed to initialize.\r\n");
        // End the test early, cannot continue without successful cloud client initialization.
        test_failed();
    }

    test_case_finish("Initialize Simple PDMC", iteration + (client_status == 0), (client_status != 0));

    //Create LwM2M resources
    MbedCloudClientResource *res_get_test;
    res_get_test = client.create_resource("5000/0/1", "get_resource");
    res_get_test->observable(true);
    res_get_test->methods(M2MMethod::GET);
    res_get_test->set_value("test0");

    MbedCloudClientResource *res_put_test;
    res_put_test = client.create_resource("5000/0/2", "put_resource");
    res_put_test->methods(M2MMethod::PUT | M2MMethod::GET);
    res_put_test->set_value(1);

    MbedCloudClientResource *res_post_test;
    res_post_test = client.create_resource("5000/0/3", "post_resource");
    res_post_test->methods(M2MMethod::POST);
    res_post_test->attach_post_callback(post_test_callback);

    // Register to Pelion Device Management.
    if (iteration == 0) {
        test_case_start("Pelion Bootstrap & Reg.", 5);
    } else {
        test_case_start("Pelion Re-register", 7);
    }
    // Set client callback to report endpoint name.
    client.on_registered(&registered);
    client.register_and_connect();

    i = 1200; // wait 120 seconds
    while (i-- > 0 && !client.is_client_registered()) {
        wait_ms(100);
    }

    // Get registration status.
    bool client_registered = client.is_client_registered();
    if (client_registered) {
        client_status = 0;
        wait_nb(100);
        logger("[INFO] Device successfully registered to Pelion DM.\r\n");
    } else {
        client_status = -1;
        logger("[ERROR] Device failed to register.\r\n");
        test_failed();
    }
    if (iteration == 0) {
        test_case_finish("Pelion Bootstrap & Reg.", (client_status == 0), (client_status != 0));
    } else {
        test_case_finish("Pelion Re-register", (client_status == 0), (client_status != 0));
    }

    if (iteration == 0) {
        //Start registration status test
        test_case_start("Pelion Directory", 6);
        int reg_status;

        logger("[INFO] Wait up to 10 seconds for Device Directory to update after initial registration.\r\n");
        i = 100;
        while (i-- > 0 and !endpointInfo) {
            wait(100);
        }

        // Start host tests with device id
        logger("[INFO] Starting Pelion verification using Python SDK...\r\n");
        greentea_send_kv("verify_registration", endpointInfo->internal_endpoint_name.c_str());
        while (1) {
            greentea_parse_kv(_key, _value, sizeof(_key), sizeof(_value));

            if (strcmp(_key, "registered") == 0) {
                if (atoi(_value)) {
                    reg_status = 0;
                    logger("[INFO] Device is registered in the Device Directory.\r\n");
                } else {
                    reg_status = -1;
                    logger("[ERROR] Device could not be verified as registered in Device Directory.\r\n");
                    test_failed();
                }
                break;
            }
        }

        test_case_finish("Pelion Directory", (reg_status == 0), (reg_status != 0));

        if (reg_status == 0) {
            logger("[INFO] Resetting device.\r\n");
            greentea_send_kv("test_advance", 0);
            while (1) {
                greentea_parse_kv(_key, _value, sizeof(_key), sizeof(_value));

                if (strcmp(_key, "reset") == 0) {
                    system_reset();
                    break;
                }
            }
        }
    } else {
        //Start consistent identity test.
        test_case_start("Post-reset Identity", 8);
        int identity_status;

        logger("[INFO] Wait up to 5 seconds for Device Directory to update after reboot.\r\n");
        i = 50;
        while (i-- > 0 and !endpointInfo) {
            wait(100);
        }

        // Wait for Host Test to verify consistent device ID (blocking here)
        logger("[INFO] Verifying consistent Device ID...\r\n");
        greentea_send_kv("verify_identity", endpointInfo->internal_endpoint_name.c_str());
        while (1) {
            greentea_parse_kv(_key, _value, sizeof(_key), sizeof(_value));

            if (strcmp(_key, "verified") == 0) {
                if (atoi(_value)) {
                    identity_status = 0;
                    logger("[INFO] Device ID consistent, SOTP and Secure Storage is preserved correctly.\r\n");
                } else {
                    identity_status = -1;
                    logger("[ERROR] Device ID is inconsistent. SOTP and Secure Storage was not preserved.\r\n");
                }
                break;
            }
        }

        test_case_finish("Post-reset Identity", (identity_status == 0), (identity_status != 0));

        // LwM2M tests
        logger("[INFO] Beginning LwM2M resource tests.\r\n");

        wait_nb(1000);

        // ---------------------------------------------
        // GET test
        test_case_start("Resource LwM2M GET", 9);
        int get_status;
        // Read original value of /5000/0/1 and wait for Host Test to verify it read the value and send it back.
        greentea_send_kv("verify_lwm2m_get_test", "/5000/0/1");
        while (1) {
            greentea_parse_kv(_key, _value, sizeof(_key), sizeof(_value));

            if (strcmp(_key, "get_value") == 0) {
                if (strcmp(_value, "test0") == 0) {
                    get_status = 0;
                    logger("[INFO] Original value of LwM2M resource /5000/0/1 is read correctly\r\n");
                } else {
                    get_status = -1;
                    logger("[ERROR] Wrong value reported in Pelion DM.\r\n");
                }
                break;
            } else if (strcmp(_key, "timeout") == 0) {
                get_status = -1;
                logger("[ERROR] Observation of LwM2M resource /5000/0/1 timed out.\r\n");
                break;
            }
        }
        test_case_finish("Resource LwM2M GET", (get_status == 0), (get_status != 0));

        wait_nb(500);

        // ---------------------------------------------
        // SET test
        test_case_start("Resource LwM2M SET", 10);
        int set_status;
        // Update resource /5000/0/1 from client and observe value
        res_get_test->set_value("test1");

        greentea_send_kv("verify_lwm2m_set_test", "/5000/0/1");
        while (1) {
            greentea_parse_kv(_key, _value, sizeof(_key), sizeof(_value));

            if (strcmp(_key, "set_value") == 0) {
                if (strcmp(_value, "test1") == 0) {
                    set_status = 0;
                    logger("[INFO] Changed value of LwM2M resource /5000/0/1 is observed correctly\r\n");
                } else {
                    set_status = -1;
                    logger("[ERROR] Wrong value observed in Pelion DM.\r\n");
                }
                break;
            } else if (strcmp(_key, "timeout") == 0) {
                set_status = -1;
                logger("[ERROR] Observation of LwM2M resource /5000/0/1 timed out.\r\n");
                break;
            }
        }
        test_case_finish("Resource LwM2M SET", (set_status == 0), (set_status != 0));

        wait_nb(500);

        // ---------------------------------------------
        // PUT Test
        test_case_start("Resource LwM2M PUT", 11);
        int put_status;
        int current_res_value;
        int updated_res_value;

        // Observe resource /5000/0/2 from cloud, add +5, and confirm value is correct on client
        greentea_send_kv("verify_lwm2m_put_test", "/5000/0/2");
        while (1) {
            greentea_parse_kv(_key, _value, sizeof(_key), sizeof(_value));

            if (strcmp(_key, "res_set") == 0) {
                // Get updated value from host test.
                updated_res_value = atoi(_value);
                // Get current value from resource.
                current_res_value = res_put_test->get_value_int();

                if (updated_res_value == current_res_value) {
                    put_status = 0;
                    logger("[INFO] Value of resource /5000/0/2 successfully changed from the cloud using PUT.\r\n");
                } else {
                    put_status = -1;
                    logger("[ERROR] Wrong value read from device after resource update.\r\n");
                }
                break;
            } else if (strcmp(_key, "timeout") == 0) {
                put_status = -1;
                logger("[ERROR] PUT of LwM2M resource /5000/0/2 timed out.\r\n");
                break;
            }
        }

        test_case_finish("Resource LwM2M PUT", (put_status == 0), (put_status != 0));

        wait_nb(500);

        // ---------------------------------------------
        // POST test
        test_case_start("Resource LwM2M POST", 12);
        int post_status;

        logger("[INFO] Executing POST on /5000/0/3 and waiting for callback function\r\n.");
        greentea_send_kv("verify_lwm2m_post_test", "/5000/0/3");
        while (1) {
            greentea_parse_kv(_key, _value, sizeof(_key), sizeof(_value));

            if (strcmp(_key, "post_test_executed") == 0) {
                int result = atoi(_value);
                if (result == 0) {
                    post_status = 0;
                    logger("[INFO] Callback on resource /5000/0/3 executed successfully.\r\n");
                } else {
                    post_status = -1;
                    logger("[ERROR] Callback on resource /5000/0/3 failed.\r\n");
                }
                break;
            } else if (strcmp(_key, "timeout") == 0) {
                post_status = -1;
                logger("[ERROR] POST of LwM2M resource /5000/0/3 timed out.\r\n");
                break;
            }
        }

        test_case_finish("Resource LwM2M POST", (post_status == 0), (post_status != 0));

        GREENTEA_TESTSUITE_RESULT((get_status == 0) && (set_status == 0) && (put_status == 0) && (post_status == 0));

        while (1) {
            wait(100);
        }
    }
}

int main(void) {
    //Create a thread to blink an LED and signal that the device is alive
#if !defined(MBED_CONF_APP_NO_LED)
    Ticker t;
    t.attach(led_thread, 0.5);
#endif

    greentea_send_kv("device_booted", 1);

    GREENTEA_SETUP(240, "sdk_host_tests");
    spdmc_testsuite_connect();

    return 0;
}
